// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "StatModifiedValue.h"
#include "SCCounselorCharacter.h"

FStatModifiedValue::FStatModifiedValue()
	: BaseValue(0.f)
	, BakedValue(FLT_MAX)
{
	for (float& f : StatModifiers)
		f = 0.f;
}

float FStatModifiedValue::Get(const ASCCounselorCharacter* Counselor, const bool Reverse) const
{
	if (BakedValue == FLT_MAX || BakedCounselor != Counselor)
	{
		BakedCounselor = Counselor;
		BakedValue = BaseValue;
		for (uint8 iStat = 0; iStat < (uint8)ECounselorStats::MAX; ++iStat)
		{
			const float Adjustment = StatModifiers[iStat] * (Reverse ? -0.01f : 0.01f);
			BakedValue += (BaseValue * FMath::Lerp(-Adjustment, Adjustment, Counselor->GetStatAlpha((ECounselorStats)iStat)));
		}
	}

	return BakedValue;
}
