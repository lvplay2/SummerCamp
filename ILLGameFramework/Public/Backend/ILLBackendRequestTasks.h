// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSessionSettings.h"

#include "ILLBackendEndpoint.h"
#include "ILLBackendPlayerIdentifier.h"
#include "ILLHTTPRequestTask.h"
#include "ILLBackendRequestTasks.generated.h"

class UILLBackendUser;
class UILLLocalPlayer;

/**
 * @class UILLBackendHTTPRequestTask
 */
UCLASS(Abstract, Within=ILLBackendRequestManager)
class ILLGAMEFRAMEWORK_API UILLBackendHTTPRequestTask
: public UILLHTTPRequestTask
{
	GENERATED_BODY()

public:
	UILLBackendHTTPRequestTask()
	{
		TimeoutDuration = 30.f;
	}

protected:
	// Begin UILLHTTPRequestTask interface
	virtual bool PrepareRequest() override;
	virtual void AsyncPrepareRequest() override;
	// End UILLHTTPRequestTask interface

	/** @return Generated URL parameter string to be appended to the URL. */
	virtual FString AsyncGetURLParameters() const { return FString(); }

public:
	// Endpoint to contact
	FILLBackendEndpointHandle Endpoint;

	// Associated backend user sending the request
	UPROPERTY()
	UILLBackendUser* User;
};

// Delegate fired for very basic backend requests
DECLARE_DELEGATE_TwoParams(FOnILLBackendSimpleRequestDelegate, int32 /*ResponseCode*/, const FString& /*ResponseContents*/);

/**
 * @class UILLBackendSimpleHTTPRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLBackendSimpleHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendHTTPRequestTask interface
	virtual FString AsyncGetURLParameters() const override { return URLParameters; }
	// End UILLBackendHTTPRequestTask interface

public:
	/** Wraps to IHttpRequest::SetContent. */
	FORCEINLINE void SetContent(const TArray<uint8>& ContentPayload)
	{
		check(!HasBeenQueued());
		HttpRequest->SetContent(ContentPayload);
	}

	/** Wraps to IHttpRequest::SetContentAsString. */
	FORCEINLINE void SetContentAsString(const FString& ContentString)
	{
		check(!HasBeenQueued());
		HttpRequest->SetContentAsString(ContentString);
	}

	/** Wraps to IHttpRequest::AppendToHeader. */
	FORCEINLINE void AppendToHeader(const FString& HeaderName, const FString& AdditionalHeaderValue)
	{
		check(!HasBeenQueued());
		HttpRequest->SetHeader(HeaderName, AdditionalHeaderValue);
	}

	/** Wraps to IHttpRequest::SetHeader. */
	FORCEINLINE void SetHeader(const FString& HeaderName, const FString& HeaderValue)
	{
		check(!HasBeenQueued());
		HttpRequest->SetHeader(HeaderName, HeaderValue);
	}

	// Retry attempt limit
	uint32 RetryCount;

	// Request parameters
	FString URLParameters;

	// Callback delegate for completion
	FOnILLBackendSimpleRequestDelegate CompletionDelegate;
};

/**
 * @enum EILLBackendAuthResult
 * @brief Authentication request completion result.
 */
enum class EILLBackendAuthResult : uint8
{
	// Authentication was successful
	Success,

	// Unknown internal error
	UnknownError,

	// Auth service did not respond
	NoAuthResponse,

	// Auth service responded with a 401
	AccountBanned,

	// Auth service response was not in the expected format
	InvalidAuthResponse,

	// Auth service response was missing information
	IncompleteAuthResponse,

	// Arbiter service did not respond
	NoArbiterResponse,

	// No platform login presence established
	NoPlatformLogin,

	// Arbiter service response was not in the expected format
	InvalidArbiterResponseCode,
};

// Delegate fired once an Auth request has completed
DECLARE_DELEGATE_OneParam(FOnILLBackendAuthCompleteDelegate, EILLBackendAuthResult /*Result*/);

/**
 * @class UILLBackendAuthHTTPRequestTask
 */
UCLASS()
class UILLBackendAuthHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

public:
	// Begin UILLHTTPRequestTask interface
	virtual void QueueRequest() override;
protected:
	virtual void AsyncPrepareRequest() override;
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendHTTPRequestTask interface
	virtual FString AsyncGetURLParameters() const override;
	// End UILLBackendHTTPRequestTask interface

public:
	// Local player we are authenticating
	UPROPERTY()
	UILLLocalPlayer* LocalPlayer;

	// Name of the local player
	FString Nickname;

	// Platform identifier of the local player
	FString PlatformId;

	// Error code generated in AsyncPrepareResponse
	EILLBackendAuthResult ErrorCode;

	// Backend identifier for the local player parsed in AsyncPrepareResponse
	FILLBackendPlayerIdentifier BackendId;

	// Session token parsed in AsyncPrepareResponse
	FString SessionToken;

	// Callback delegate for completion
	FOnILLBackendAuthCompleteDelegate CompletionDelegate;
};

/**
 * @class UILLBackendArbiterHTTPRequestTask
 */
UCLASS(Abstract)
class UILLBackendArbiterHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	// End UILLHTTPRequestTask interface

	/**
	 * Called from AsyncPrepareRequest to allow sub-classes to adjust the request body.
	 *
	 * @param JsonWriter JSON writer object.
	 */
	virtual void WriteRequestBody(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const {}

public:
	// Local player we are arbitrating
	UPROPERTY()
	UILLLocalPlayer* LocalPlayer;
};

/**
 * @class UILLBackendArbiterValidateHTTPRequestTask
 */
UCLASS()
class UILLBackendArbiterValidateHTTPRequestTask
: public UILLBackendArbiterHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

public:
	// Callback delegate for completion
	FOnILLBackendAuthCompleteDelegate CompletionDelegate;
};

/**
 * @enum EILLBackendArbiterHeartbeatResult
 */
enum class EILLBackendArbiterHeartbeatResult : uint8
{
	// Arbiter accepted the heartbeat
	Success,

	// Unknown internal error
	UnknownError,

	// Arbiter service did not respond
	NoArbiterResponse,

	// Arbiter service response was not in the expected format
	InvalidArbiterResponseCode,

	// Rejected heartbeat response code (400)
	HeartbeatDenied,
};

// Delegate fired once an Arbiter heartbeat request has completed
DECLARE_DELEGATE_OneParam(FOnILLBackendArbiterHeartbeatCompleteDelegate, EILLBackendArbiterHeartbeatResult /*Result*/);

/**
 * @class UILLBackendArbiterHeartbeatHTTPRequestTask
 */
UCLASS()
class UILLBackendArbiterHeartbeatHTTPRequestTask
: public UILLBackendArbiterHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendArbiterHTTPRequestTask interface
	virtual void WriteRequestBody(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const override;
	// End UILLBackendArbiterHTTPRequestTask interface

public:
	// Name of the map we are on to report
	FString MapName;

	// Callback delegate for completion
	FOnILLBackendArbiterHeartbeatCompleteDelegate CompletionDelegate;
};

/**
 * @enum EILLBackendLogoutResult
 */
enum class EILLBackendLogoutResult : uint8
{
	// Logout accepted
	Success,

	// Unknown internal error
	UnknownError,

	// No user was provided to logout
	NoUserProvided,

	// User was previously logged out
	UserAlreadyLoggedOut,
};

// Delegate fired for very basic backend requests
DECLARE_DELEGATE_ThreeParams(FOnILLBackendKeySessionDelegate, int /*ResponseCode*/, const FString& /*ResponseContent*/, const FOnEncryptionKeyResponse& /*EncryptionDelegate*/);

/**
 * @class UILLBackendKeySessionRetrieveHTTPRequestTask
 */
UCLASS()
class UILLBackendKeySessionRetrieveHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendHTTPRequestTask interface
	virtual FString AsyncGetURLParameters() const override { return FString::Printf(TEXT("?KeySessionID=%s"), *KeySessionId); }
	// End UILLBackendHTTPRequestTask interface

public:
	// Key session identifier
	FString KeySessionId;

	// Encryption response delegate to pass back to CompletionDelegate
	FOnEncryptionKeyResponse EncryptionDelegate;

	// Callback delegate for completion
	FOnILLBackendKeySessionDelegate CompletionDelegate;
};

/**
 * @enum EILLBackendMatchmakingStatusType
 */
enum class EILLBackendMatchmakingStatusType : uint8
{
	Unknown,
	Error,
	Ok,
	LowPriority,
	Banned
};

/** @return Status in string literal form. */
FORCEINLINE const TCHAR* const ToString(const EILLBackendMatchmakingStatusType Status)
{
	switch (Status)
	{
	case EILLBackendMatchmakingStatusType::Error: return TEXT("Error");
	case EILLBackendMatchmakingStatusType::Ok: return TEXT("Ok");
	case EILLBackendMatchmakingStatusType::LowPriority: return TEXT("LowPriority");
	case EILLBackendMatchmakingStatusType::Banned: return TEXT("Banned");
	}

	check(Status == EILLBackendMatchmakingStatusType::Unknown);
	return TEXT("Unknown");
}

/**
 * @struct FILLBackendMatchmakingStatus
 */
USTRUCT()
struct FILLBackendMatchmakingStatus
{
	GENERATED_USTRUCT_BODY()

	// Account identifier for the user
	FILLBackendPlayerIdentifier AccountId;

	// "OK", "LowPriority" or "Banned"
	EILLBackendMatchmakingStatusType MatchmakingStatus;

	// Number of seconds the user has left on their ban in seconds
	float MatchmakingBanSeconds;
};

// Delegate fired for matchmaking status requests, failed if empty
DECLARE_DELEGATE_OneParam(FOnILLBackendMatchmakingStatusDelegate, const TArray<FILLBackendMatchmakingStatus>& /*MatchmakingStatuses*/);

/**
 * @class UILLBackendMatchmakingStatusHTTPRequestTask
 */
UCLASS()
class UILLBackendMatchmakingStatusHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendHTTPRequestTask interface
	virtual FString AsyncGetURLParameters() const override;
	// End UILLBackendHTTPRequestTask interface

public:
	// Backend account identifier list of everybody involved in our party
	TArray<FILLBackendPlayerIdentifier> AccountIdList;

	// Matchmaking status parsed in AsyncPrepareResponse
	TArray<FILLBackendMatchmakingStatus> Statuses;

	// Callback delegate for completion
	FOnILLBackendMatchmakingStatusDelegate CompletionDelegate;
};

/**
 * Delegate fired once an XBL session creation/update has completed.
 *
 * @param SessionName Session name / identifier.
 * @param SessionUri Session URI, non-empty indicates success.
 */
DECLARE_DELEGATE_TwoParams(FOnILLPutXBLSessionCompleteDelegate, const FString& /*SessionId*/, const FString& /*SessionUri*/);

/**
 * @class UILLBackendPutXBLSessionHTTPRequestTask
 */
UCLASS()
class UILLBackendPutXBLSessionHTTPRequestTask
: public UILLBackendHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual bool PrepareRequest() override;
	virtual void AsyncPrepareRequest() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

	// Begin UILLBackendHTTPRequestTask interface
	virtual FString AsyncGetURLParameters() const override;
	// End UILLBackendHTTPRequestTask interface

public:
	// Completion delegate
	FOnILLPutXBLSessionCompleteDelegate CompletionDelegate;

	// XUID of the person creating the game session
	FString InitiatorXUID;

	// Sandbox this will be done in
	FString SandboxId;

	// Session name/identifier for us
	FString SessionName;

	// Session settings grabbed when the request started
	FOnlineSessionSettings SessionSettings;
};
