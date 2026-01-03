// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNamedDamageType.h"

USCNamedDamageType::USCNamedDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShouldPlayHitReaction = false;
	bPlayDeathAnim = false;
}
