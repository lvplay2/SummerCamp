// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLBackendUser.h"
#include "ILLBackendServer.generated.h"

class UILLGameInstance;

/**
 * @class UILLBackendServer
 */
UCLASS(Within=ILLBackendRequestManager)
class ILLGAMEFRAMEWORK_API UILLBackendServer
: public UILLBackendUser
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual UWorld* GetWorld() const override;
	// End UObject interface

	/** @return Game instance pulled from the outer UILLBackendRequestManager. */
	UILLGameInstance* GetGameInstance() const;

	/** @return Timer manager instance from the outer UILLBackendRequestManager. */
	FTimerManager& GetTimerManager() const;

	/** @return Known hostname of this server. Derived from Steam. */
	virtual FString GenerateHostName() const;

	//////////////////////////////////////////////////
	// Dedicated server reporting
public:
	/** Sends a server report to the backend. If one is already being waited on, queues another one to immediately follow it. */
	virtual void SendReport();

protected:
	virtual void GenerateServerReport(TSharedPtr<FJsonObject> JsonObject);
	virtual FString GenerateHeartbeat();
	virtual void OnServerReport(int32 ResponseCode, const FString& ResponseContents);

	bool bPendingReportResponse;
	bool bReportQueuedAfterResponse;

	//////////////////////////////////////////////////
	// Dedicated server heartbeat
protected:
	/** Sends a heartbeat to the backend. */
	virtual void SendBackendServerHeartbeat();

	/** Server heartbeat response callback. */
	virtual void OnServerHeartbeatResponse(int32 ResponseCode, const FString& ResponseContents);

	// Interval at which SendBackendServerHeartbeat is called
	UPROPERTY(Config)
	int32 DedicatedArbiterHeartbeatInterval;

	// Timer handle for the Arbiter service heartbeat
	FTimerHandle TimerHandle_ArbiterHeartbeat;
};
