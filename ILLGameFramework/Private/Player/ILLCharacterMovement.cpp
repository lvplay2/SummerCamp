// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCharacterMovement.h"

UILLCharacterMovement::UILLCharacterMovement(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bCanWalkOffLedgesWhenCrouching = true;

	CustomSpeedMultiplier = 1.f;
	CustomAccelerationMultiplier = 1.f;
}

float UILLCharacterMovement::GetMaxSpeed() const
{
	return Super::GetMaxSpeed() * CustomSpeedMultiplier;
}

float UILLCharacterMovement::GetMaxAcceleration() const
{
	return Super::GetMaxAcceleration() * CustomAccelerationMultiplier;
}
