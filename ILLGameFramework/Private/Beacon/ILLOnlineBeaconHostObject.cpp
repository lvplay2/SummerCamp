// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLOnlineBeaconHostObject.h"

#include "ILLOnlineBeaconClient.h"

AILLOnlineBeaconHostObject::AILLOnlineBeaconHostObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NetDriverName = NAME_BeaconNetDriver;
}

AILLOnlineBeaconClient* AILLOnlineBeaconHostObject::FindClientByEncryptionToken(const FString& EncryptionToken) const
{
	for (AOnlineBeaconClient* ClientActor : ClientActors)
	{
		if (ClientActor->GetNetConnection() && ClientActor->GetNetConnection()->EncryptionToken == EncryptionToken)
			return CastChecked<AILLOnlineBeaconClient>(ClientActor);
	}

	return nullptr;
}
