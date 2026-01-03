// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSessionBeaconHost.h"
#include "ILLPartyBeaconHost.generated.h"

/**
 * @class AILLPartyBeaconHost
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLPartyBeaconHost
: public AILLSessionBeaconHost
{
	GENERATED_UCLASS_BODY()

	// Begin AOnlineBeaconHost interface
	virtual bool InitHost() override;
	// End AOnlineBeaconHost interface

protected:
	// Port to use for the party beacon, stomps over the ListenPort from the parent
	UPROPERTY(Config)
	int32 PartyBeaconPort;
};
