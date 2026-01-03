// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSessionBeaconHost.h"
#include "ILLLobbyBeaconHost.generated.h"

class FOnlineSessionSettings;

/**
 * @class AILLLobbyBeaconHost
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLLobbyBeaconHost
: public AILLSessionBeaconHost
{
	GENERATED_UCLASS_BODY()
	
	/** Initializes this Lobby beacon with the session settings that will be used. */
	virtual bool InitHost(const FOnlineSessionSettings& SessionSettings);

protected:
	// Begin AOnlineBeaconHost interface
	virtual bool InitHost() override;
	// End AOnlineBeaconHost interface

	// Port to use for the Lobby beacon, stomps over the ListenPort from the parent
	UPROPERTY(Config)
	int32 LobbyBeaconPort;
};
