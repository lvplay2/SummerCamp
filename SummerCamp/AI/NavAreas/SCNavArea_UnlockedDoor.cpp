// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNavArea_UnlockedDoor.h"

USCNavArea_UnlockedDoor::USCNavArea_UnlockedDoor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DefaultCost = 5.f;
	DrawColor = FColor::Cyan;
}
