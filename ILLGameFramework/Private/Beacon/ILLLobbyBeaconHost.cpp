// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLobbyBeaconHost.h"

#include "ILLOnlineSessionClient.h"

AILLLobbyBeaconHost::AILLLobbyBeaconHost(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	LobbyBeaconPort = 17000;
}

bool AILLLobbyBeaconHost::InitHost(const FOnlineSessionSettings& SessionSettings)
{
	bIsLANBeacon = SessionSettings.bIsLANMatch;
	return InitHost();
}

bool AILLLobbyBeaconHost::InitHost()
{
	// Only allow the command line to override the default port
	ListenPort = LobbyBeaconPort;
	int32 PortOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("LobbyBeaconPort="), PortOverride) && PortOverride != 0)
	{
		ListenPort = PortOverride;
	}

	return Super::InitHost();
}
