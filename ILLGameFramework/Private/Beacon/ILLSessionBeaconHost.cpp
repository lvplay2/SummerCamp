// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSessionBeaconHost.h"

AILLSessionBeaconHost::AILLSessionBeaconHost(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	BeaconConnectionTimeoutShipping = 60.f;
}

bool AILLSessionBeaconHost::InitHost()
{
	FURL URL(nullptr, TEXT(""), TRAVEL_Absolute);

	if (bIsLANBeacon)
		URL.AddOption(TEXT("bIsLanBeacon"));
	else
		URL.AddOption(TEXT("bIsBeacon"));

	// Allow the command line to override the default port
	int32 PortOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("BeaconPort="), PortOverride) && PortOverride != 0)
	{
		ListenPort = PortOverride;
	}

	URL.Port = ListenPort;
	if(URL.Valid)
	{
		if (InitBase() && NetDriver)
		{
			FString Error;
			if (NetDriver->InitListen(this, URL, false, Error))
			{
				ListenPort = URL.Port;
				NetDriver->SetWorld(GetWorld());
				NetDriver->Notify = this;
				NetDriver->InitialConnectTimeout = BeaconConnectionInitialTimeout;
				NetDriver->ConnectionTimeout = UE_BUILD_SHIPPING ? BeaconConnectionTimeoutShipping : BeaconConnectionTimeout;
				return true;
			}
			else
			{
				// error initializing the network stack...
				UE_LOG(LogBeacon, Log, TEXT("AOnlineBeaconHost::InitHost failed"));
				OnFailure();
			}
		}
	}

	return false;
}
