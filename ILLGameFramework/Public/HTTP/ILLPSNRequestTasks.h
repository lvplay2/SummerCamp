// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineSubsystemTypes.h"

#include "ILLHTTPRequestTask.h"
#include "ILLPSNRequestTasks.generated.h"

/**
 * @class UILLPSNHTTPRequestTask
 */
UCLASS(Abstract, Within=ILLPSNRequestManager)
class ILLGAMEFRAMEWORK_API UILLPSNHTTPRequestTask
: public UILLHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	// End UILLHTTPRequestTask interface

public:
	UILLPSNHTTPRequestTask()
	{
		TimeoutDuration = 30.f;
	}

	// Online environment this is for
	EOnlineEnvironment::Type OnlineEnvironment;
};

/**
 * @class UILLPSNAuthorizationRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNAuthorizationRequestTask
: public UILLPSNHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	// End UILLHTTPRequestTask interface

	/** @return Base URL for OAuth requests. */
	FString GenerateOAuthURL() const;

public:
	// Access token
	FString AccessToken;

	// Client app ID
	FString ClientAppId;

	// Time in seconds until AccessToken expires
	int32 ExpiresInSeconds;
};

/**
 * Delegate fired once a client authorization and validation sequence has completed.
 *
 * @param bWasSuccessful Did we succeed?
 */
DECLARE_DELEGATE_OneParam(FOnILLPSNClientAuthorzationCompleteDelegate, bool /*bWasSuccessful*/);

/**
 * @class UILLPSNGetAccessTokenRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNGetAccessTokenRequestTask
: public UILLPSNAuthorizationRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

public:
	// Client authorization code to authorize
	FString ClientAuthCode;

	// Delegate fired when completed
	FOnILLPSNClientAuthorzationCompleteDelegate CompletionDelegate;
};

/**
 * @class UILLPSNValidateAccessTokenRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNValidateAccessTokenRequestTask
: public UILLPSNAuthorizationRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

public:
	// Country code fetched for the user
	FString CountryCode;

	// Delegate fired when completed
	FOnILLPSNClientAuthorzationCompleteDelegate CompletionDelegate;
};

class UILLPSNBaseUrlRequestTask;

/**
 * Delegate fired once a base URL request has completed.
 *
 * @param RequestTask Request that completed.
 */
DECLARE_DELEGATE_OneParam(FOnILLPSNBaseUrlRequestCompleteDelegate, UILLPSNBaseUrlRequestTask* /*RequestTask*/);

/**
 * @class UILLPSNBaseUrlRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNBaseUrlRequestTask
: public UILLPSNHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

public:
	// Access token for authorization
	FString AccessToken;

	// ApiGroup to request the BaseUrl for
	FString ApiGroup;

	// Country code the AccessToken belongs to
	FString CountryCode;

	// Delegate fired when completed
	FOnILLPSNBaseUrlRequestCompleteDelegate CompletionDelegate;

	// Parsed URL
	FString ReceivedUrl;

	// Expiration time in seconds
	int32 ExpiresInSeconds;
};

/**
 * @class UILLPSNDeleteSessionRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNDeleteSessionRequestTask
: public UILLPSNHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	virtual bool IsReadyToProcess() const { return !bCanceled; } // HACK: Always return true when not canceled so this isn't held back from being sent during shutdown
	virtual void AsyncPrepareResponse() override;
	// End UILLHTTPRequestTask interface

public:
	// Access token for authorization
	FString AccessToken;

	// Base URL for sessionInvitation
	FString BaseURL;

	// Session identifier to be destroyed
	FString SessionId;
};

/**
 * Delegate fired once PSN session creation completes.
 *
 * @param SessionId Session identifier, which will be non-empty on success.
 */
DECLARE_DELEGATE_OneParam(FOnILLPSNCreateSessionCompleteDelegate, const FString& /*SessionId*/);

/**
 * @class UILLPSNPostSessionRequestTask
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNPostSessionRequestTask
: public UILLPSNHTTPRequestTask
{
	GENERATED_BODY()

protected:
	// Begin UILLHTTPRequestTask interface
	virtual void AsyncPrepareRequest() override;
	virtual void AsyncPrepareResponse() override;
	virtual void HandleResponse() override;
	// End UILLHTTPRequestTask interface

public:
	// Access token for authorization
	FString AccessToken;

	// Base URL for sessionInvitation
	FString BaseURL;

	// Client app ID
	FString ClientAppId;

	// Session identifier parsed
	FString SessionId;

	// Delegate fired when completed
	FOnILLPSNCreateSessionCompleteDelegate CompletionDelegate;
};
