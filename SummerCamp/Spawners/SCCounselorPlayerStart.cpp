// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorPlayerStart.h"

ASCCounselorPlayerStart::ASCCounselorPlayerStart(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Keep in sync with ASCCounselorCharacter's capsule size!
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
}
