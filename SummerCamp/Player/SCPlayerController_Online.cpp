// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController_Online.h"

#include "SCGameInstance.h"

ASCPlayerController_Online::ASCPlayerController_Online(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPlayerController_Online::ClientReceiveP2PHostAccountId_Implementation(const FILLBackendPlayerIdentifier& HostAccountId)
{
	USCGameInstance* GameInstance = CastChecked<USCGameInstance>(GetGameInstance());
	GameInstance->LastP2PQuickPlayHostAccountId = HostAccountId;
}
