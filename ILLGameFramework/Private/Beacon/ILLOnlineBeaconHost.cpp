// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLOnlineBeaconHost.h"

AILLOnlineBeaconHost::AILLOnlineBeaconHost(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NetDriverName = NAME_BeaconNetDriver;
}
