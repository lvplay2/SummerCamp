// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCStunDamageType.h"

USCStunDamageType::USCStunDamageType(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bHasStunRecovery(true)
, bCanBeBlocked(true)
{
	PercentPerWiggle.BaseValue = 5.f;
	PercentPerWiggle.StatModifiers[(uint8)ECounselorStats::Strength] = 25.f;

	StaminaPerWiggle.BaseValue = 1.f;
	StaminaPerWiggle.StatModifiers[(uint8)ECounselorStats::Strength] = 25.f;

	DepletionRate.BaseValue = 0.02f;
	DepletionRate.StatModifiers[(uint8)ECounselorStats::Strength] = 25.f;
	DepletionRate.StatModifiers[(uint8)ECounselorStats::Luck] = 5.f;
}
