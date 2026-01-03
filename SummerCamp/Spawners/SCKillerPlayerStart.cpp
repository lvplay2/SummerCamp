// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerPlayerStart.h"

ASCKillerPlayerStart::ASCKillerPlayerStart(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Keep in sync with ASCKillerCharacter's capsule size!
	GetCapsuleComponent()->InitCapsuleSize(34.f, 100.f);
}
