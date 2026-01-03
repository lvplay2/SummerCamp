// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyBeaconHost.h"

AILLPartyBeaconHost::AILLPartyBeaconHost(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PartyBeaconPort = 16000;
}

bool AILLPartyBeaconHost::InitHost()
{
	// Only allow the command line to override the default port
	ListenPort = PartyBeaconPort;
	int32 PortOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("PartyBeaconPort="), PortOverride) && PortOverride != 0)
	{
		ListenPort = PortOverride;
	}

	return Super::InitHost();
}
