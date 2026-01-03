// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLHTTPRequestManager.h"
#include "ILLPSNRequestTasks.h"
#include "ILLPSNRequestManager.generated.h"

/**
 * @class UILLPSNRequestManager
 * @brief Singleton frontend to the PlayStation Network services, used by dedicated servers.
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPSNRequestManager
: public UILLHTTPRequestManager
{
	GENERATED_UCLASS_BODY()

	// Begin UILLHTTPRequestManager interface
	virtual void Shutdown() override;
	// End UILLHTTPRequestManager interface

	//////////////////////////////////////////////////
	// Access credentials, configured in sub-class
public:
	// Client identifier
	FGuid ClientId;

	// Client secret, in binary form
	TArray<uint8> ClientSecret;

	// Proxy address to use for development and certification environments
	FString DevelopmentProxyAddress;

	// Proxy credentials
	FString DevelopmentProxyUserPass;

	//////////////////////////////////////////////////
	// Request task management
public:
	/**
	 * PSN request creation wrapper.
	 * Spawns a managed request to be filled out then queued.
	 * @see UILLHTTPRequestManager::CreateRequest.
	 *
	 * @param RequestClass Class of the request task to create.
	 */
	virtual UILLPSNHTTPRequestTask* CreatePSNRequest(UClass* RequestClass);

	/** Templated override of CreatePSNRequest. */
	template<class T>
	FORCEINLINE T* CreatePSNRequest() { return CastChecked<T>(CreatePSNRequest(T::StaticClass()), ECastCheckedType::NullAllowed); }

	//////////////////////////////////////////////////
	// Authorization
public:
	/**
	 * Begins an authorization sequence for a client.
	 *
	 * @param ClientAppId Application ID for the client.
	 * @param ClientAuthCode Authorization code.
	 * @param OnlineEnvironment Online environment to authorize with.
	 * @param CompletionDelegate Delegate to fire when done.
	 * @return true if request processing has began.
	 */
	virtual bool StartAuthorizationSequence(const FString& ClientAppId, const FString& ClientAuthCode, const EOnlineEnvironment::Type OnlineEnvironment, FOnILLPSNClientAuthorzationCompleteDelegate CompletionDelegate);

	//////////////////////////////////////////////////
	// AccessToken cache
public:
	/**
	 * Registers an AccessToken to be tracked internally and automatically expired some time before ExpiresInSeconds.
	 *
	 * @param ClientAppId Application ID for the client.
	 * @param AccessToken Access token to store.
	 * @param CountryCode Country for this token.
	 * @param OnlineEnvironment Online environment for this token.
	 * @param ExpiresInSeconds Seconds until this expires.
	 */
	virtual void RegisterAccessToken(const FString& ClientAppId, const FString& AccessToken, const FString& CountryCode, const EOnlineEnvironment::Type OnlineEnvironment, const int32 ExpiresInSeconds);

protected:
	/**
	 * Unregisters a previously registered access token.
	 * Called on a timer set from RegisterAccessToken.
	 *
	 * @param Counter Access token counter to unregister.
	 */
	virtual void ExpireAccessToken(uint32 Counter);

	/**
	 * @struct FAccessToken
	 */
	struct FAccessToken
	{
		uint32 Counter;
		FString AccessToken;
		FString ClientAppId;
		FString CountryCode;
		EOnlineEnvironment::Type OnlineEnvironment;
		FTimerHandle TimerHandle_Expiration;
	};

	// Access token cache
	TArray<FAccessToken> CachedAccessTokens;

	// Multiplier against access token expiration seconds before we expire them locally
	UPROPERTY(Config)
	float AccessTokenExpirationMultiplier;

	// Counter used for identifying access tokens
	uint32 AccessTokenCounter;

	//////////////////////////////////////////////////
	// BaseUrl cache
public:
	/** @return BaseURL cached for ApiGroup. */
	virtual FString FindBaseURL(const FString& ApiGroup);

protected:
	/**
	 * Makes request(s) to collect the base URLs for the ApiGroup.
	 *
	 * @param ApiGroup API group we need the base URL(s) for.
	 * @return true if this process was needed and started, false if the current cache does not need to be updated.
	 */
	virtual bool LookupBaseUrls(const FString& ApiGroup);

	/** Delegate fired once base URL lookup completes. */
	virtual void OnLookupBaseUrlCompleted(UILLPSNBaseUrlRequestTask* RequestTask);

	/**
	 * Refreshes a previously cached base URL.
	 * Called on a timer set from OnLookupBaseUrlCompleted.
	 *
	 * @param Counter Base URL counter to refresh.
	 */
	virtual void RefreshBaseUrl(uint32 Counter);

	/**
	 * Unregisters a previously cached base URL.
	 * Called on a timer set from OnLookupBaseUrlCompleted.
	 *
	 * @param Counter Base URL counter to unregister.
	 */
	virtual void ExpireBaseUrl(uint32 Counter);

	/**
	 * @struct FBaseUrl
	 */
	struct FBaseUrl
	{
		uint32 Counter;
		FString ApiGroup;
		FString CountryCode;
		EOnlineEnvironment::Type OnlineEnvironment;
		FString URL;
		FTimerHandle TimerHandle_Refresh;
		FTimerHandle TimerHandle_Expiration;
	};

	// Base URL cache
	TArray<FBaseUrl> CachedBaseUrls;

	// Delegate to fire when base URL lookup completes
	DECLARE_MULTICAST_DELEGATE(FOnBaseUrlLookupCompleteDelegate);
	FOnBaseUrlLookupCompleteDelegate OnPSNBaseUrlLookupCompleteDelegate;

	// Pending base URL requests
	UPROPERTY()
	TArray<UILLPSNBaseUrlRequestTask*> BaseUrlRequests;

	// Multiplier against base URL expiration seconds before we expire them locally
	UPROPERTY(Config)
	float BaseUrlExpirationMultiplier;

	// Multiplier against base URL expiration seconds before we will refresh them
	UPROPERTY(Config)
	float BaseUrlRefreshMultiplier;

	// Counter used for identifying base URLs
	uint32 BaseUrlCounter;

	//////////////////////////////////////////////////
	// Session handling
public:
	/**
	 * Creates a PSN game session that mirrors the local platform one.
	 *
	 * @param CompletionDelegate Delegate to fire when done.
	 */
	virtual void CreateGameSession(FOnILLPSNCreateSessionCompleteDelegate CompletionDelegate);

	/**
	 * Destroys our existing PSN game session.
	 */
	virtual void DestroyGameSession();

	/** @return Current session identifier for our PSN game session. */
	FORCEINLINE const FString& GetGameSessionId() const { return GameSessionId; }

	/** @return true if we are creating a game session, but have not finished. */
	FORCEINLINE bool IsCreatingGameSession() const { return bCreatingGameSession; }

protected:
	friend class UILLPSNPostSessionRequestTask;

	/** Callback fired when LookupBaseUrls completes for CreateGameSession. */
	virtual void OnLookupBaseUrlForCreateGameSessionCompleted(FOnILLPSNCreateSessionCompleteDelegate CompletionDelegate);

	// Session identifier for the current game session
	FString GameSessionId;

	// Are we already working on game session creation?
	bool bCreatingGameSession;
};
