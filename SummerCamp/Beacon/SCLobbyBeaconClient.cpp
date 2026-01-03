// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCLobbyBeaconClient.h"

#include "SCGameState_Online.h"

ASCLobbyBeaconClient::ASCLobbyBeaconClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
#if PLATFORM_XBOXONE
	LobbyTemplateName = TEXT("DedicatedLobbyTrafficUdp");
	GameTemplateName = TEXT("DedicatedGameTrafficUdp");
#endif
}
