// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBackendPlayer.h"

#include "SCBackendInventory.h"

USCBackendPlayer::USCBackendPlayer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCBackendPlayer::NotifyArbitrated()
{
	Super::NotifyArbitrated();

	BackendInventory = NewObject<USCBackendInventory>(this);
}
