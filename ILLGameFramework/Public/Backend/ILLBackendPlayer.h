// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLBackendPlayerIdentifier.h"
#include "ILLBackendUser.h"
#include "ILLLocalPlayer.h"
#include "ILLBackendPlayer.generated.h"

class UILLGameInstance;

enum class EILLBackendArbiterHeartbeatResult : uint8;
enum class EILLBackendMatchmakingStatusType : uint8;

DECLARE_MULTICAST_DELEGATE_TwoParams(FILLOnBackendPlayerArbiterHeartbeatComplete, UILLBackendPlayer* /*BackendPlayer*/, EILLBackendArbiterHeartbeatResult /*HeartbeatResult*/);

/**
 * @class UILLBackendPlayer
 */
UCLASS(Within=ILLLocalPlayer)
class ILLGAMEFRAMEWORK_API UILLBackendPlayer
: public UILLBackendUser
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	virtual UWorld* GetWorld() const override;
	// End UObject interface

	// Begin UILLBackendUser interface
	virtual void SetupRequest(FHttpRequestPtr HttpRequest) override;
	// End UILLBackendUser interface

	/** @return Game instance pulled from the outer UILLLocalPlayer. */
	UILLGameInstance* GetGameInstance() const;

	/** @return AccountID. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetAccountId() const { return AccountId; }

	/** @return SessionToken. */
	FORCEINLINE const FString& GetSessionToken() const { return SessionToken; }

	/** @return true if we are logged in and arbitrated with the backend. */
	FORCEINLINE bool IsLoggedIn() const { return bLoggedIn; }

	/** @return Delegate first when the arbiter heartbeat completes. */
	FORCEINLINE FILLOnBackendPlayerArbiterHeartbeatComplete& OnBackendPlayerArbiterHeartbeatComplete() { return BackendPlayerArbiterHeartbeatComplete; }

	/** First step in user authentication with the backend. */
	virtual void NotifyAuthenticated(const FILLBackendPlayerIdentifier& InAccountId, const FString& InSessionToken);

	/** Final step in user authentication with the backend. */
	virtual void NotifyArbitrated();

	/** Called when arbitration fails. */
	virtual void NotifyArbitrationFailed();

	/** Called when this user is logged out of the BRM. */
	virtual void NotifyLoggedOut();

	/**
	 * After establishing authentication/arbiter privileges this will be called every UserArbiterHeartbeatInterval seconds.
	 * Can also be invoked directly for establishing that we have a backend connection.
	 */
	virtual void SendArbiterHeartbeat();

	// Last received matchmaking status specifically for us
	EILLBackendMatchmakingStatusType LastMatchmakingStatus;

protected:
	/** Callback function for our Arbiter heartbeat. */
	virtual void ArbiterHeartbeatCompleted(EILLBackendArbiterHeartbeatResult HeartbeatResult);

	/** Post map load callback when we are logged in. */
	virtual void OnPostLoadMap(UWorld* World);

	/** Logged in delta-checker. Sets up bindings for suspend/re-activate. */
	virtual void SetLoggedIn(bool bIsLoggedIn);

	// Delegate for when arbiter heartbeat completes
	FILLOnBackendPlayerArbiterHeartbeatComplete BackendPlayerArbiterHeartbeatComplete;

private:
	// Interval at which SendBackendServerHeartbeat is called
	UPROPERTY(Config)
	int32 UserArbiterHeartbeatInterval;

	// Interval at which SendBackendServerHeartbeat is called for a reconnection attempt
	UPROPERTY(Config)
	int32 MaxArbiterRetries;

	// Delay before an arbiter retry attempt
	UPROPERTY(Config)
	float ArbiterRetryDelay;

	// Timer handle for the Arbiter service heartbeat
	FTimerHandle TimerHandle_ArbiterHeartbeat;

	// Backend account identifier
	FILLBackendPlayerIdentifier AccountId;

	// Session token received during authentication
	FString SessionToken;

	// Number of times we have attempted to reestablish our presence with the arbiter
	int32 ArbiterRetries;

	// true if we are retrying to establish an arbiter connection
	bool bRetryingArbiter;

	// true if we are logged in
	bool bLoggedIn;

	// true if we are already waiting on an arbiter heartbeat
	bool bWaitingOnHeartbeat;
};
