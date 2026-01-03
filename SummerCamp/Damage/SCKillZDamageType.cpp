// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillZDamageType.h"

USCKillZDamageType::USCKillZDamageType(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bShouldPlayHitReaction = false;
	bCausedByWorld = true;
}
