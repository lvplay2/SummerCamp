// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSpecialMove_MoveToward.h"

#include "Kismet/KismetMathLibrary.h"

#include "ILLCharacter.h"
#include "ILLPlayerController.h"

UILLSpecialMove_MoveToward::UILLSpecialMove_MoveToward(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bMoveToward = true;
	MoveScale = 1.f;
	MoveThreshold = 10.f;

	bLookToward = true;
	LookTowardRate = 6.f;
	bSnapLookOnDestinationReached = true;
	LookPitchMode = EMoveTowardRotatorMode::Flatten;
}

void UILLSpecialMove_MoveToward::TickMove(const float DeltaSeconds)
{
	AILLCharacter* Character = GetOuterAILLCharacter();
	if (!Character)
	{
		return;
	}

	if (!bReachedDestination && bMoveToward)
	{
		FVector MoveDestination;
		if (GetMoveDestination(DeltaSeconds, MoveDestination))
		{
			const FVector MoveDelta = MoveDestination - Character->GetActorLocation();
			const float Threshold = MoveThreshold + Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
			if (MoveDelta.SizeSquared2D() > FMath::Square(Threshold))
			{
				if (Character->IsLocallyControlled())
					Character->AddMovementInput(MoveDelta.GetSafeNormal(), MoveScale, true);
			}
			else
			{
				bReachedDestination = true;
				DestinationReached(MoveDestination);
			}
		}
		else
		{
			MoveFailed();
		}
	}

	if (!Character->IsLocallyControlled())
	{
		return;
	}

	if (bLookToward && (!bReachedDestination || bLookAfterReachedDestination))
	{
		FVector LookDestination;
		if (GetLookDestination(DeltaSeconds, LookDestination))
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			Character->Controller->GetActorEyesViewPoint(ViewLocation, ViewRotation);

			// Find our new full TargetRot
			const FRotator CurrentRot = Character->Controller->GetControlRotation();
			FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(ViewLocation, LookDestination);

			// Handle LookPitchMode
			if (LookPitchMode == EMoveTowardRotatorMode::Epsilon)
			{
				TargetRot.Pitch = FMath::ClampAngle(CurrentRot.Pitch, TargetRot.Pitch - LookPitchEpsilon, TargetRot.Pitch + LookPitchEpsilon);
			}
			else if (LookPitchMode == EMoveTowardRotatorMode::Flatten)
			{
				TargetRot.Pitch = 0.f;
			}
			else
			{
				check(LookPitchMode == EMoveTowardRotatorMode::Free);
				TargetRot.Pitch = CurrentRot.Pitch;
			}

			// Handle LookYawMode
			if (LookYawMode == EMoveTowardRotatorMode::Epsilon)
			{
				TargetRot.Yaw = FMath::ClampAngle(CurrentRot.Yaw, TargetRot.Yaw - LookYawEpsilon, TargetRot.Yaw + LookYawEpsilon);
			}
			else if (LookYawMode == EMoveTowardRotatorMode::Free)
			{
				TargetRot.Yaw = 0.f; // FIXME: pjackson: 90? or something? untested constant
			}
			else
			{
				check(LookYawMode == EMoveTowardRotatorMode::Free);
				TargetRot.Yaw = CurrentRot.Yaw;
			}

			// Conditionally interpolate then assign the new control rotation
			if (!bReachedDestination || !bSnapLookOnDestinationReached)
			{
				TargetRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, LookTowardRate);
			}
			Character->Controller->SetControlRotation(TargetRot);
		}
	}
}

bool UILLSpecialMove_MoveToward::GetMoveDestination(const float DeltaSeconds, FVector& OutDestination)
{
	if (TargetActor && !TargetActor->IsPendingKill())
	{
		OutDestination = TargetActor->GetActorLocation();
		return true;
	}

	return false;
}

bool UILLSpecialMove_MoveToward::GetLookDestination(const float DeltaSeconds, FVector& OutDestination)
{
	return GetMoveDestination(DeltaSeconds, OutDestination);
}

void UILLSpecialMove_MoveToward::DestinationReached(const FVector& Destination)
{
	MarkPendingKill();
}

void UILLSpecialMove_MoveToward::MoveFailed()
{
	DestinationReached(FVector::ZeroVector);
}
