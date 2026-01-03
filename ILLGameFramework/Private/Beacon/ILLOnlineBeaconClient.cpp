// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLOnlineBeaconClient.h"

AILLOnlineBeaconClient::AILLOnlineBeaconClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NetDriverName = NAME_BeaconNetDriver;
}

void AILLOnlineBeaconClient::WasKicked(const FText& KickReason)
{
	UE_LOG(LogBeacon, Log, TEXT("%s: %s \"%s\""), ANSI_TO_TCHAR(__FUNCTION__), *GetName(), *KickReason.ToString());

	ClientWasKicked(KickReason);
}

void AILLOnlineBeaconClient::ClientWasKicked_Implementation(const FText& KickReason)
{
	UE_LOG(LogBeacon, Log, TEXT("%s: %s \"%s\""), ANSI_TO_TCHAR(__FUNCTION__), *GetName(), *KickReason.ToString());

	BeaconClientKicked.ExecuteIfBound(KickReason);
}
