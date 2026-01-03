// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNavArea_LockedDoor.h"

USCNavArea_LockedDoor::USCNavArea_LockedDoor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DefaultCost = 500000.f;
	FixedAreaEnteringCost = 10000.f;
	DrawColor = FColor::Yellow;
}