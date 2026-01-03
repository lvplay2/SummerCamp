// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "IHttpRequest.h"
#include "OnlineSessionInterface.h"

#include "ILLBackendKeySession.h"
#include "ILLBackendPlayerIdentifier.h"
#include "ILLOnlineSessionSearch.h"
#include "ILLProjectSettings.h"
#include "ILLSessionBeaconClient.h"
#include "ILLLobbyBeaconClient.generated.h"

class AILLLobbyBeaconClient;
class AILLLobbyBeaconHostObject;
class AILLLobbyBeaconState;
class AILLPartyBeaconState;
class AILLPartyBeaconMemberState;
class AILLPlayerController;
class UILLLocalPlayer;

/**
 * @enum EILLLobbyBeaconSessionResult
 */
UENUM()
enum class EILLLobbyBeaconSessionResult : uint8
{
	Success,
	FailedToAuthorize, // Failed to authorize
	FailedToBind, // Failed to create a lobby beacon client
	FailedToLogin, // Failed to login to the platform to get an auth code
	FailedToRequestKeys, // Failed to request a key set from the key session service
	InvalidGameSession, // Failed to find/create a game session for the player
	InvalidPlayerSession, // PlayerSessionId sent to the server was not accepted
	FailedToFindSession, // FindSessions failed to start or find any results
	FailedToFindLocalPlayer, // Failed to find the local player's player session in the results
	FailedToJoin, // Found the session but failed to join it
	NotSignedIn, // Local player is not signed into the backend
};

/** @return String name for Response. */
FORCEINLINE const TCHAR* ToString(const EILLLobbyBeaconSessionResult Response)
{
	switch (Response)
	{
	case EILLLobbyBeaconSessionResult::Success: return TEXT("Success");
	case EILLLobbyBeaconSessionResult::FailedToAuthorize: return TEXT("FailedToAuthorize");
	case EILLLobbyBeaconSessionResult::FailedToBind: return TEXT("FailedToBind");
	case EILLLobbyBeaconSessionResult::FailedToLogin: return TEXT("FailedToLogin");
	case EILLLobbyBeaconSessionResult::FailedToRequestKeys: return TEXT("FailedToRequestKeys");
	case EILLLobbyBeaconSessionResult::InvalidGameSession: return TEXT("InvalidGameSession");
	case EILLLobbyBeaconSessionResult::InvalidPlayerSession: return TEXT("InvalidPlayerSession");
	case EILLLobbyBeaconSessionResult::FailedToFindSession: return TEXT("FailedToFindSession");
	case EILLLobbyBeaconSessionResult::FailedToFindLocalPlayer: return TEXT("FailedToFindLocalPlayer");
	case EILLLobbyBeaconSessionResult::FailedToJoin: return TEXT("FailedToJoin");
	case EILLLobbyBeaconSessionResult::NotSignedIn: return TEXT("NotSignedIn");
	}

	check(0);
	return TEXT("UNKNOWN");
}

/**
 * @enum EILLLobbyBeaconClientAuthorizationResult
 */
enum class EILLLobbyBeaconClientAuthorizationResult : uint8
{
	Pending,
	Success,
	Failed,
	TimedOut,
};

FORCEINLINE const TCHAR* ToString(const EILLLobbyBeaconClientAuthorizationResult Result)
{
	switch (Result)
	{
	case EILLLobbyBeaconClientAuthorizationResult::Pending: return TEXT("Pending");
	case EILLLobbyBeaconClientAuthorizationResult::Success: return TEXT("Success");
	case EILLLobbyBeaconClientAuthorizationResult::Failed: return TEXT("Failed");
	case EILLLobbyBeaconClientAuthorizationResult::TimedOut: return TEXT("TimeOut");
	}

	check(0);
	return TEXT("INVALID");
}

/**
 * Delegate fired when the local player login process completes.
 *
 * @param bWasSuccessful Did we do it dad?
 * @param Error Error string when bWasSuccessful is false.
 */
DECLARE_DELEGATE_TwoParams(FOnILLLobbyBeaconLoginComplete, bool /*bWasSuccessful*/, const FString& /*Error*/);

/**
 * Delegate fired when authorization completes.
 *
 * @param LobbyClient Lobby client that completed authorization.
 * @param Result Authorization result.
 */
DECLARE_DELEGATE_TwoParams(FOnILLLobbyBeaconAuthorizationComplete, AILLLobbyBeaconClient* /*LobbyClient*/, EILLLobbyBeaconClientAuthorizationResult /*Result*/);

#if USE_GAMELIFT_SERVER_SDK
/**
 * Delegate fired when matchmaking bypass completes.
 *
 * @param LobbyClient Lobby client that completed the matchmaking bypass.
 * @param bWasSuccessful Did we do it dad?
 */
DECLARE_DELEGATE_TwoParams(FOnILLLobbyBeaconMatchmakingBypassComplete, AILLLobbyBeaconClient* /*LobbyClient*/, bool /*bWasSuccessful*/);
#endif

/**
 * @struct FILLMatchmadePlayerSession
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLMatchmadePlayerSession
{
	GENERATED_USTRUCT_BODY()

	// Server connection string
	UPROPERTY()
	FString ConnectionString;

	// Player backend account identifier
	UPROPERTY()
	FILLBackendPlayerIdentifier AccountId;

	// Player session identifier
	UPROPERTY()
	FString PlayerSessionId;
};

/**
 * Delegate fired as soon as we receive our lobby information before attempting to find and join it.
 *
 * @param SessionId Session identifier.
 */
DECLARE_DELEGATE_OneParam(FILLOnLobbyBeaconSessionReceived, const FString& /*SessionId*/);

/**
 * Delegate fired for lobby session request completion.
 *
 * @param Response Response status.
 */
DECLARE_DELEGATE_OneParam(FILLOnLobbyBeaconSessionRequestComplete, EILLLobbyBeaconSessionResult /*Response*/);

/**
 * @class AILLLobbyBeaconClient
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLLobbyBeaconClient
: public AILLSessionBeaconClient
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void OnNetCleanup(UNetConnection* Connection) override;
	virtual bool ShouldReceiveVoiceFrom(const FUniqueNetIdRepl& Sender) const override;
	virtual bool ShouldSendVoice() const override;
	// End AActor interface

	// Begin AOnlineBeacon interface
	virtual void HandleNetworkFailure(UWorld* World, UNetDriver* InNetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) override;
	// End AOnlineBeacon interface

	// Begin AOnlineBeaconClient interface
	virtual void OnConnected() override;
	// End AOnlineBeaconClient interface

	// Begin AILLSessionBeaconClient interface
	virtual bool InitClientBeacon(const FString& ConnectionString, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId) override;
	virtual bool IsPendingAuthorization() const override;
	// End AILLSessionBeaconClient interface

	/** @return Player controller from the game world that is associated with this lobby client. */
	virtual AILLPlayerController* FindGamePlayerController() const;
	
	//////////////////////////////////////////////////
	// Network encryption
public:
	/** @return Key set associated with this client. */
	FORCEINLINE const FILLBackendKeySession& GetSessionKeySet() const { return SessionKeySet; }

protected:
	/** Callback for when RequestLobbySession or RequestLobbyReservation finish the BeginLoginForAuthorization process. */
	virtual void OnLoginToConnectComplete(bool bWasSuccessful, const FString& Error, UILLLocalPlayer* FirstLocalPlayer);

	/** Requests a key set from the backend then calls to InitClientBeacon, when encryption is required. Otherwise it skips straight to InitClientBeacon. */
	virtual void ConditionalRequestKeysAndConnect();

	/** Response handler for ConditionalRequestKeysAndConnect. */
	virtual void KeySessionResponse(int32 ResponseCode, const FString& ResponseContent);

	// Key session response data
	FILLBackendKeySession SessionKeySet;

	// ConnectionString for the next InitClientBeacon request
	FString PendingConnectionString;

#if PLATFORM_XBOXONE
	// Name of the SDA for the Lobby connection
	FString LobbyTemplateName;

	// SDA for the Lobby connection
	Windows::Xbox::Networking::SecureDeviceAssociationTemplate^ LobbyPeerTemplate;
	Windows::Foundation::EventRegistrationToken LobbyAssociationIncoming;

	// Name of the SDA for the Game connection
	FString GameTemplateName;

	// SDA for the Game connection
	Windows::Xbox::Networking::SecureDeviceAssociationTemplate^ GamePeerTemplate;
	Windows::Foundation::EventRegistrationToken GameAssociationIncoming;
#endif

	//////////////////////////////////////////////////
	// Session negotiation
public:
	/** Performs some final cleanup before calling ClientMigrateToReplacement. */
	virtual void MigrateToReplacement(const TArray<FILLMatchmadePlayerSession>& PlayerSessions);

protected:
	/** Tells this lobby client to migrate their party to PlayerSessions. */
	UFUNCTION(Client, Reliable)
	virtual void ClientMigrateToReplacement(const TArray<FILLMatchmadePlayerSession>& PlayerSessions);

public:
	/**
	 * Establish a connection with ConnectionString and request the lobby session.
	 *
	 * @param ConnectionString IpAddress:Port to establish a connection with.
	 * @param FirstLocalPlayer First local player that is making the request.
	 * @param PlayerSessionId Player session identifier for FirstLocalPlayer.
	 */
	virtual void RequestLobbySession(const FString& ConnectionString, UILLLocalPlayer* FirstLocalPlayer, const FString& PlayerSessionId);

	/** @return Player session identifier for this client. */
	FORCEINLINE const FString& GetClientPlayerSessionId() const { return ClientPlayerSessionId; }

	/** Delegate fired when RequestLobbySession completes. */
	FORCEINLINE FILLOnLobbyBeaconSessionReceived& OnLobbyBeaconSessionReceived() { return LobbyBeaconSessionReceived; }
	FORCEINLINE FILLOnLobbyBeaconSessionRequestComplete& OnLobbyBeaconSessionRequestComplete() { return LobbyBeaconSessionRequestComplete; }

protected:
	friend AILLLobbyBeaconHostObject;

	/** Callback for when client authorization is completed for session negotiation. */
	virtual void OnLobbySessionClientAuthorizationComplete(AILLLobbyBeaconClient* LobbyClient, EILLLobbyBeaconClientAuthorizationResult Result);

	/** Reply for ServerRequestLobbySession. */
	UFUNCTION(Client, Reliable)
	virtual void ClientLobbySessionAuthorizationFailure();

	/** Reply for ServerRequestLobbySession. */
	UFUNCTION(Client, Reliable)
	virtual void ClientLobbySessionGameSessionInvalid();

	/** Reply for ServerRequestLobbySession. */
	UFUNCTION(Client, Reliable)
	virtual void ClientLobbySessionPlayerSessionInvalid();

	/** Reply for ServerRequestLobbySession. */
	UFUNCTION(Client, Reliable)
	virtual void ClientLobbySessionReceived(const FString& SessionId, const FString& SessionLookupIdentifier);

#if PLATFORM_DESKTOP
	/** Callback for when lobby session lookup completes. */
	virtual void OnLobbySearchCompleted(bool bSuccess);
#else
	/** Callback for when lobby session lookup completes. */
	virtual void OnLobbySearchCompleted(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult);
#endif

	/** Callback for when lobby session joining completes. */
	virtual void OnLobbyJoinCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);

	/** Sends a session request to the server. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRequestLobbySession(const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& AuthParams, const FString& PlayerSessionId);

	// Player session identifier
	FString ClientPlayerSessionId;

#if PLATFORM_DESKTOP
	// Search delegate handle for lobby session lookup
	FOnFindSessionsCompleteDelegate LobbySearchDelegate;
	FDelegateHandle LobbySearchHandle;

	// Search object for lobby session lookup
	TSharedPtr<FILLOnlineSessionSearch> LobbySearchObject;
#else
	// Search delegate handle for lobby session lookup
	FOnSingleSessionResultCompleteDelegate LobbySearchDelegate;
#endif

	// Join delegate handle for lobby session joining
	FOnJoinSessionCompleteDelegate LobbyJoinDelegate;

	// Lobby session request delegates
	FILLOnLobbyBeaconSessionReceived LobbyBeaconSessionReceived;
	FILLOnLobbyBeaconSessionRequestComplete LobbyBeaconSessionRequestComplete;

	// Are we initializing for a lobby connection (as opposed to a session reservation)
	bool bPendingLobbySession;

	//////////////////////////////////////////////////
	// Reservations
public:
	/**
	 * Establish a connection with DesiredHost and request a lobby reservation.
	 *
	 * @param DesiredHost Host to connect to.
	 * @param FirstLocalPlayer First local player that is making the request.
	 * @param HostingPartyState Party session state if hosting a party.
	 * @param PartyLeaderAccountId Party leader backend account identifier when a party member.
	 * @param bForMatchmaking Is this a request from matchmaking?
	 */
	virtual void RequestLobbyReservation(const FOnlineSessionSearchResult& DesiredHost, UILLLocalPlayer* FirstLocalPlayer, AILLPartyBeaconState* HostingPartyState, const FILLBackendPlayerIdentifier& PartyLeaderAccountId, const bool bForMatchmaking);

	/** @return true if this was a matchmaking reservation. */
	FORCEINLINE bool HasMatchmakingReservation() const { return bMatchmakingReservation; }

protected:
	/** Sends a reservation request to the server. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRequestLobbyReservation(const FUniqueNetIdRepl& OwnerId, const FILLSessionBeaconReservation& Reservation, const FILLBackendPlayerIdentifier& AccountId, const FUniqueNetIdRepl& UniqueId, const FString& AuthParams, const bool bForMatchmaking);

	// Was this a matchmaking reservation?
	bool bMatchmakingReservation;

	//////////////////////////////////////////////////
	// Party handling
public:
	/** Make a request for the addition of NewPartyMember with the party. */
	virtual void RequestPartyAddition(AILLPartyBeaconMemberState* NewPartyMember);

	/** Notifies the lobby beacon host of party member removals. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerNotifyLeftParty(const FILLBackendPlayerIdentifier& LeaderAccountId);

	/** Notifies the lobby beacon host of party member removals. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerNotifyPartyMemberRemoved(const FILLBackendPlayerIdentifier& MemberAccountId);

protected:
	/** Server response for ServerRequestPartyAddition. */
	UFUNCTION(Client, Reliable)
	virtual void ClientPartyAdditionAccepted(const FILLBackendPlayerIdentifier& MemberAccountId);

	/** Server response for ServerRequestPartyAddition. */
	UFUNCTION(Client, Reliable)
	virtual void ClientPartyAdditionDenied(const FILLBackendPlayerIdentifier& MemberAccountId);

	/** Requests the addition of a new party member. Server will respond with Accepted/Denied. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRequestPartyAddition(const FILLBackendPlayerIdentifier& MemberAccountId, FUniqueNetIdRepl MemberUniqueId, const FString& DisplayName);

	//////////////////////////////////////////////////
	// Authorization
public:
	/**
	 * Starts the authorization process with a local or remote platform, depending on the AuthorizationParams.
	 *
	 * @param CompletionDelegate Delegate to fire upon completion.
	 * @return true if the authorization process has started.
	 */
	bool BeginAuthorization(FOnILLLobbyBeaconAuthorizationComplete CompletionDelegate);

	/**
	 * Ends a previously established authorization session.
	 */
	void EndAuthorization();

	/** @return Authorization type for this player. */
	FORCEINLINE const FString& GetAuthorizationType() const { return AuthorizationType; }

	/** @return true if we have successfully authorized. */
	FORCEINLINE bool HasAuthorized() const { return bAuthorized; }

protected:
	/**
	 * Calls to Login on the identity interface so that we can build an AuthorizationParams on the local client, to then be sent up to the server.
	 *
	 * @param FirstLocalPlayer Local player to build the AuthorizationParams from.
	 * @param CompletionDelegate Delegate to fire when completed.
	 * @return true if the login process and authorization token request process has began. Note that CompletionDelegate will still fire if this returns false.
	 */
	virtual bool BeginLoginForAuthorization(UILLLocalPlayer* FirstLocalPlayer, FOnILLLobbyBeaconLoginComplete CompletionDelegate);

	/** Fires when BeginLoginForAuthorization finishes the Login process from the online identity interface. */
	void OnLoginForAuthorizationComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error, const EOnlineEnvironment::Type OnlineEnvironment, FOnILLLobbyBeaconLoginComplete CompletionDelegate);

	/**
	 * Notification for when authorization completes.
	 * Called from OnAuthSessionStartComplete or OnPSNClientAuthorzationComplete.
	 */
	virtual void AuthorizationComplete(const EILLLobbyBeaconClientAuthorizationResult Result);

	/** Fired when we have finished authorizing an validating a local platform client. */
	void OnAuthSessionStartComplete(bool bWasSuccessful, const FUniqueNetId& PlayerId);

	/** Fired when we have finished authorizing and validating a PSN client. */
	void OnPSNClientAuthorzationComplete(bool bWasSuccessful);

	/** Called on a timer AuthorizationTimeout seconds after BeginAuthorization is called. Cleared if AuthorizationComplete beats it. */
	virtual void OnAuthorizationTimeout();

	// Authorization parameters
	FString AuthorizationParams;

	// Authorization type extracted from the AuthorizationParams
	FString AuthorizationType;

	// Sandbox received for XBL users
	FString AuthSandbox;

	// App ID to perform authorization for
	FString AppId;

	// Authorization token extracted from the AuthorizationParams
	FString AuthorizationToken;

	// Delegate handle for Login
	FDelegateHandle LoginCompleteDelegate;

	// Delegate to fire when authorization completes
	FOnILLLobbyBeaconAuthorizationComplete AuthorizationDelegate;
	
	// Delegate handle for AuthSessionStart
	FDelegateHandle AuthSessionStartCompleteDelegateHandle;

	// Time until we automatically timeout authorization
	UPROPERTY(Config)
	float AuthorizationTimeout;

	// Timer handle for AuthorizationTimeout
	FTimerHandle TimerHandle_AuthTimeout;

	// Have we already successfully authorized?
	bool bAuthorized;

#if USE_GAMELIFT_SERVER_SDK
	//////////////////////////////////////////////////
	// HTTP matchmaking bypass
public:
	/**
	 * Starts an HTTP matching request to bypass the queue and join on an invite, receiving a ClientPlayerSessionId in return.
	 *
	 * @param LobbyState Lobby state instance (client does not have one assigned until they are accepted).
	 * @param CompletionDelegate Delegate to fire when matchmaking bypass completes.
	 * @return true if the matchmaking bypass process started.
	 */
	virtual bool PerformMatchmakingBypass(AILLLobbyBeaconState* LobbyState, FOnILLLobbyBeaconMatchmakingBypassComplete CompletionDelegate);

protected:
	/** Response handler for PerformMatchmakingBypass. */
	virtual void MatchmakingBypassResponse(int32 ResponseCode, const FString& ResponseContent);

	// Delegate to fire when matchmaking bypass completes
	FOnILLLobbyBeaconMatchmakingBypassComplete MatchmakingBypassDelegate;
#endif // USE_GAMELIFT_SERVER_SDK
};
