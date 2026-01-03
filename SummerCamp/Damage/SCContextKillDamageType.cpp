// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCContextKillDamageType.h"

USCContextKillDamageType::USCContextKillDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShouldPlayHitReaction = false;
	bPlayDeathAnim = false;
}
