// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSinglePlayerDamageType.h"

USCSinglePlayerDamageType::USCSinglePlayerDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShouldPlayHitReaction = false;
	bPlayDeathAnim = false;
}
