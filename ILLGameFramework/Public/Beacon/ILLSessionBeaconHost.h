// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLOnlineBeaconHost.h"
#include "ILLSessionBeaconHost.generated.h"

class AILLSessionBeaconHostObject;

/**
 * @class AILLSessionBeaconHost
 * @brief Manages the listen socket for a session beacon.
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLSessionBeaconHost
: public AILLOnlineBeaconHost
{
	GENERATED_UCLASS_BODY()

	// Begin AOnlineBeaconHost interface
	virtual bool InitHost() override;
	// End AOnlineBeaconHost interface

protected:
	// Time beacon will wait for packets after establishing a connection before giving up in Shipping
	UPROPERTY(Config)
	float BeaconConnectionTimeoutShipping;

	// Forcibly spawn a LAN beacon?
	bool bIsLANBeacon;
};
