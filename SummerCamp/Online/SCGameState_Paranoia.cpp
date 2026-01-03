// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameState_Paranoia.h"

ASCGameState_Paranoia::ASCGameState_Paranoia(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	bPlayJasonAbilityUnlockSounds = false;
}