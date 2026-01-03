// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLHTTPRequestManager.h"
#include "ILLBackendPlayerIdentifier.h"
#include "ILLBackendEndpoint.h"
#include "ILLBackendRequestTasks.h"
#include "ILLBackendRequestManager.generated.h"

class UILLBackendUser;
class UILLBackendServer;
class UILLBackendPlayer;
class UILLGameInstance;
class UILLLocalPlayer;

ILLGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogILLBackend, Log, All);

/**
 * @class UILLBackendRequestManager
 * @brief Singleton frontend to the IllFonic backend.
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLBackendRequestManager
: public UILLHTTPRequestManager
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin UILLHTTPRequestManager interface
	virtual void Init() override;
	// End UILLHTTPRequestManager interface
	
	/** @return true if we are a backend dedicated server instance, or if we have any logged in players. */
	virtual bool CanSendAnyRequests() const;

	/** @return true if we have any logged in players. */
	virtual bool HasAnyLoggedInPlayers() const;

	//////////////////////////////////////////////////
	// Request task management
public:
	/**
	 * Backend request creation wrapper.
	 * Spawns a managed request to be filled out then queued.
	 * @see UILLHTTPRequestManager::CreateRequest.
	 *
	 * @param RequestClass Class of the request task to create.
	 * @param Endpoint Destination backend endpoint.
	 * @param User Optional backend user sending the request.
	 */
	virtual UILLBackendHTTPRequestTask* CreateBackendRequest(UClass* RequestClass, const FILLBackendEndpointHandle& Endpoint, UILLBackendUser* User = nullptr);

	/** Templated override of CreateBackendRequest. */
	template<class T>
	FORCEINLINE T* CreateBackendRequest(const FILLBackendEndpointHandle& Endpoint, UILLBackendUser* User = nullptr) { return CastChecked<T>(CreateBackendRequest(T::StaticClass(), Endpoint, User), ECastCheckedType::NullAllowed); }

	//////////////////////////////////////////////////
	// Simple requests
public:
	/**
	 * Wraps to CreateRequest, building an HTTP request task.
	 * Assign any additional headers and/or body text, then call QueueRequest.
	 *
	 * @param Endpoint Target endpoint.
	 * @param Callback Response delegate.
	 * @param Params Optional additional parameters to send. Should be in "?Param1=Value&Param2=Value" format.
	 * @param User Optional backend user handle for the sender, for appending user-specific headers. Include this whenever available.
	 * @param RetryCount Optional retry count.
	 * @return HTTP request task. Call QueueRequest after filling out any additional details on the request.
	 */
	virtual UILLBackendSimpleHTTPRequestTask* CreateSimpleRequest(const FILLBackendEndpointHandle& Endpoint, FOnILLBackendSimpleRequestDelegate Callback, const FString& Params = FString(), UILLBackendUser* User = nullptr, const uint32 RetryCount = 0);

	/**
	 * Helper function to CreateSimpleRequest and immediately QueueRequest.
	 * Useful when no HTTP Request object manipulation (adding headers/content) is needed.
	 * @see CreateSimpleRequest
	 *
	 * @return true if the request task was created and queued.
	 */
	FORCEINLINE bool QueueSimpleRequest(const FILLBackendEndpointHandle& Endpoint, FOnILLBackendSimpleRequestDelegate Callback, const FString& Params = FString(), UILLBackendUser* User = nullptr, const uint32 RetryCount = 0)
	{
		if (UILLBackendSimpleHTTPRequestTask* RequestTask = CreateSimpleRequest(Endpoint, Callback, Params, User, RetryCount))
		{
			RequestTask->QueueRequest();
			return true;
		}

		return false;
	}

	//////////////////////////////////////////////////
	// Player authentication requests
public:
	/**
	 * Sends an authentication and arbitration request to the backend.
	 *
	 * @param LocalPlayer Player to authenticate and login to the backend.
	 * @param Callback Delegate to fire upon completion.
	 * @return true if request processing has began.
	 */
	virtual bool StartAuthRequestSequence(UILLLocalPlayer* LocalPlayer, FOnILLBackendAuthCompleteDelegate Callback);

	/**
	 * Sends a heartbeat to the arbiter service for a particular player.
	 *
	 * @param Player Backend player to heartbeat.
	 * @param Callback Completion delegate;
	 */
	virtual void SendArbiterHeartbeatRequest(UILLBackendPlayer* Player, FOnILLBackendArbiterHeartbeatCompleteDelegate Callback);

	/** Sends a logout request to the backend. */
	EILLBackendLogoutResult LogoutPlayer(UILLBackendPlayer* Player);

	// Class to use for authenticated players
	UPROPERTY()
	TSubclassOf<UILLBackendPlayer> PlayerClass;

	// List of registered backend players
	UPROPERTY(Transient)
	TArray<UILLBackendPlayer*> PlayerList;

	//////////////////////////////////////////////////
	// Matchmaking
public:
	/** Requests matchmaking statuses for all players in the AccountIdList. */
	virtual bool RequestMatchmakingStatus(UILLBackendUser* User, const TArray<FILLBackendPlayerIdentifier> AccountIdList, FOnILLBackendMatchmakingStatusDelegate Callback);

	//////////////////////////////////////////////////
	// Server requests
public:
	/** @return Backend server object instance. */
	FORCEINLINE UILLBackendServer* GetBackendServer() const { return BackendServer; }

	/** Sends a key session retrieve request. */
	virtual bool SendKeySessionRetrieveRequest(const FString& KeySessionId, FOnILLBackendKeySessionDelegate& Callback, const FOnEncryptionKeyResponse& Delegate);

	/** Send game server report to instance authority. */
	virtual bool SendServerReport(const FString& ReportBuffer, FOnILLBackendSimpleRequestDelegate Callback);

	/** Send game server heartbeat to instance authority. */
	virtual bool SendServerHeartbeat(const FString& HeartbeatBuffer, FOnILLBackendSimpleRequestDelegate Callback);

protected:
	// Class to use for dedicated servers
	UPROPERTY()
	TSubclassOf<UILLBackendServer> ServerClass;

	// Backend server instance, for dedicated servers
	UPROPERTY(Transient)
	UILLBackendServer* BackendServer;
	
	//////////////////////////////////////////////////
	// Xbox server sessions
public:
	/** Clears ActiveXBLSessionUri. Called when all XBL clients leave. */
	void ClearActiveXBLSessionUri();

	/** @return Active XBL server session name/identifier. */
	FORCEINLINE const FString& GetActiveXBLServerSessionUri() const { return ActiveXBLSessionUri; }

	/** @return Active XBL session name / identifier. */
	FORCEINLINE const FString& GetActiveXBLSessionName() const { return ActiveXBLSessionName; }

	/** @return true if we are waiting on an existing XBL server session request. */
	FORCEINLINE bool IsCreatingXBLServerSession() const { return !PendingXBLSessionName.IsEmpty(); }

	/**
	 * Sends a request, using the backend as a proxy to XBL services, to PUT a game session for use by a dedicated server.
	 *
	 * @param InitiatorXUID XUID of the initial member.
	 * @param SandboxId Sandbox of the initial member.
	 * @param CompletionDelegate Completion delegate.
	 * @return true if the request was created and queued.
	 */
	virtual bool SendXBLServerSessionRequest(const FString& InitiatorXUID, const FString& SandboxId, FOnILLPutXBLSessionCompleteDelegate CompletionDelegate);

protected:
	friend UILLBackendPutXBLSessionHTTPRequestTask;

	// Pending XBL session name
	FString PendingXBLSessionName;

	// Active XBL session name
	FString ActiveXBLSessionName;

	// Active XBL session URI
	FString ActiveXBLSessionUri;

	//////////////////////////////////////////////////
	// Endpoint configuration
public:
	// Registered endpoint handles
	FILLBackendEndpointHandle AuthService_Login;
	FILLBackendEndpointHandle ArbiterService_Validate;
	FILLBackendEndpointHandle ArbiterService_Heartbeat;
	FILLBackendEndpointHandle ArbiterService_XboxServerSession;
	FILLBackendEndpointHandle ProfileAuthority_MatchmakingStatus;
	FILLBackendEndpointHandle KeyDistributionService_Create;
	FILLBackendEndpointHandle KeyDistributionService_Retrieve;
	FILLBackendEndpointHandle InstanceAuthority_Report;
	FILLBackendEndpointHandle InstanceAuthority_Heartbeat;
	FILLBackendEndpointHandle MatchmakingService_Bypass;
	FILLBackendEndpointHandle MatchmakingService_Cancel;
	FILLBackendEndpointHandle MatchmakingService_Describe;
	FILLBackendEndpointHandle MatchmakingService_Regions;
	FILLBackendEndpointHandle MatchmakingService_Queue;

protected:
	/**
	 * @struct FEndpointService
	 * @brief Temporary configuration structure used for RegisterEndpoint.
	 */
	struct ILLGAMEFRAMEWORK_API FEndpointService
	{
		// Host address in "https://domain.com" format
		const TCHAR* HostURL;

		// Port for this service, converted into a string to be appended onto HostURL in RegisterEndpoint
		const FString Port;

		// Client-Key header to use for this endpoint
		const FGuid ClientKey;

		FEndpointService(const TCHAR* InHostURL, const uint32 InPort, const FGuid& InClientKey)
		: HostURL(InHostURL), Port(FString::FromInt(InPort)), ClientKey(InClientKey)
		{
			check(InHostURL);
			checkf(FCString::Strncmp(HostURL+FCString::Strlen(HostURL)-1, TEXT("/"), 1) != 0, TEXT("HostURL should not end with a '/' character: %s"), HostURL);
			check(InPort > 0);
			check(!Port.IsEmpty());
		}
	};

	/**
	 * Register an endpoint listing.
	 *
	 * @param Service Service declaration.
	 * @param Path Relative path to the service in "/v0/name" format.
	 * @param Verb HTTP verb to use.
	 * @return Handle to the endpoint for making requests later.
	 */
	FILLBackendEndpointHandle RegisterEndpoint(const FEndpointService& Service, const TCHAR* Path, const EILLHTTPVerb Verb, const bool bRequireUser = true)
	{
		check(Path);
		checkf(FCString::Strncmp(Path, TEXT("/"), 1) == 0, TEXT("Path should start with a '/' character: %s"), Path);
		checkf(FCString::Strncmp(Path+FCString::Strlen(Path)-1, TEXT("/"), 1) != 0, TEXT("Path should not end with a '/' character: %s"), Path);

		// Build the full resource path
		FString FullPath = Service.HostURL;
		FullPath += TEXT(":");
		FullPath += Service.Port;
		FullPath += Path;

		// Determine the client key header index
		const int32 ClientKeyIndex = (Service.ClientKey.IsValid()) ? ClientKeyList.AddUnique(Service.ClientKey) : INDEX_NONE;

		// Build a handle
		FILLBackendEndpointHandle Result;
		Result.EndpointIndex = EndpointList.Emplace(FILLBackendEndpointResource(FullPath, Verb, ClientKeyIndex, bRequireUser));
		return Result;
	}
	FORCEINLINE FILLBackendEndpointHandle RegisterDeleteEndpoint(const FEndpointService& Service, const TCHAR* Path, const bool bRequireUser = true) { return RegisterEndpoint(Service, Path, EILLHTTPVerb::Delete, bRequireUser); }
	FORCEINLINE FILLBackendEndpointHandle RegisterGetEndpoint(const FEndpointService& Service, const TCHAR* Path, const bool bRequireUser = true) { return RegisterEndpoint(Service, Path, EILLHTTPVerb::Get, bRequireUser); }
	FORCEINLINE FILLBackendEndpointHandle RegisterHeadEndpoint(const FEndpointService& Service, const TCHAR* Path, const bool bRequireUser = true) { return RegisterEndpoint(Service, Path, EILLHTTPVerb::Head, bRequireUser); }
	FORCEINLINE FILLBackendEndpointHandle RegisterPostEndpoint(const FEndpointService& Service, const TCHAR* Path, const bool bRequireUser = true) { return RegisterEndpoint(Service, Path, EILLHTTPVerb::Post, bRequireUser); }
	FORCEINLINE FILLBackendEndpointHandle RegisterPutEndpoint(const FEndpointService& Service, const TCHAR* Path, const bool bRequireUser = true) { return RegisterEndpoint(Service, Path, EILLHTTPVerb::Put, bRequireUser); }

private:
	friend UILLBackendHTTPRequestTask;

	// List of registered endpoints
	TArray<FILLBackendEndpointResource> EndpointList;

	// List of registered client keys
	TArray<FGuid> ClientKeyList;
};
