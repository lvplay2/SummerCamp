// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacterMovement.h"

#include "SCCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"

USCCharacterMovement::USCCharacterMovement(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, MaxSprintSpeed(450.f)
, MaxRunSpeed(280.f)
, MaxSlowRunSpeed(200.f)
, MaxCombatSpeed(104.f)
{
	MaxWalkSpeed = 104.f;
	MaxWalkSpeedCrouched = 80.f;
}

float USCCharacterMovement::GetMaxSpeed() const
{
	if (ASCCharacter* Owner = Cast<ASCCharacter>(PawnOwner))
	{
		const float SpeedModifier = Owner->GetSpeedModifier();
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Owner))
		{
			if (Killer->IsShiftActive())
			{
				return Killer->GetShiftSpeed() * SpeedModifier;
			}

			if (Killer->bUnderWater)
			{
				return FMath::Lerp(MaxSlowRunSpeed, Killer->GetUnderwaterSpeed(), Killer->GetUnderwaterDepthModifier()) * SpeedModifier;
			}
		}

		if (Owner->IsSprinting() && !IsCrouching())
		{
			return MaxSprintSpeed * SpeedModifier;
		}
		else if (Owner->IsRunning() && !Owner->IsCrouching())
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Owner))
			{
				// if you are out of stamina or wounded use a flat speed
				if (Counselor->bStaminaDepleted || Counselor->IsWounded())
				{
					return MaxSlowRunSpeed;
				}
			}

			return MaxRunSpeed * SpeedModifier;
		}

		if (Owner->InCombatStance())
		{
			return MaxCombatSpeed * SpeedModifier;
		}

		if (Owner->IsCrouching())
		{
			return MaxWalkSpeedCrouched * SpeedModifier;
		}

		return Super::GetMaxSpeed() * SpeedModifier;
	}

	return Super::GetMaxSpeed();
}

void USCCharacterMovement::PhysSwimming(float deltaTime, int32 Iterations)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(PawnOwner))
	{
		if (Character->IsKiller())
		{
			NavAgentProps.bCanSwim = false;
			// SwitchBack to walking immediately. This should never be called, make sure bCanSwim is false.
			SetMovementMode(EMovementMode::MOVE_Walking, 0);
			StartNewPhysics(deltaTime, Iterations);
			return;
		}
	}

	Super::PhysSwimming(deltaTime, Iterations);
}

bool USCCharacterMovement::IsSprinting() const
{
	return Cast<ASCCharacter>(CharacterOwner) && Cast<ASCCharacter>(CharacterOwner)->IsSprinting();
}

bool USCCharacterMovement::IsRunning() const
{
	return Cast<ASCCharacter>(CharacterOwner) && Cast<ASCCharacter>(CharacterOwner)->IsRunning();
}
