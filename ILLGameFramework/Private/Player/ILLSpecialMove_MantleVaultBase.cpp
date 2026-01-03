// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLSpecialMove_MantleVaultBase.h"

#include "ILLCharacterMovement.h"
#include "ILLMantleVaultEdge.h"
#include "ILLPlayerController.h"

UILLSpecialMove_MantleVaultBase::UILLSpecialMove_MantleVaultBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MoveTimeLimit = 2.f;

	MoveThreshold = 30.f;
	LookPitchMode = EMoveTowardRotatorMode::Free;
	LookYawMode = EMoveTowardRotatorMode::Epsilon;
	LookYawEpsilon = 20.f;
	bLookAfterReachedDestination = true;
	bLookToward = true;

	bCanRotatePitch = false;
	bCanRotateYaw = false;
}

void UILLSpecialMove_MantleVaultBase::BeginDestroy()
{
	AILLCharacter* Character = Cast<AILLCharacter>(GetOuter()); // GetOuterAILLCharacter doesn't work here, causes a crash in compile and continue
	if (Character && !Character->IsPendingKillPending() && !Character->HasAnyFlags(RF_BeginDestroyed))
	{
		// Re-enable capsule collision if that was not already done
		if (!Character->GetActorEnableCollision())
		{
			Character->SetActorEnableCollision(true);
			if (UILLCharacterMovement* MovementComp = Cast<UILLCharacterMovement>(Character->GetMovementComponent()))
			{
				MovementComp->SetMovementMode(MOVE_Falling);
			}
		}
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AnimComplete);
	}

	Super::BeginDestroy();
}

bool UILLSpecialMove_MantleVaultBase::GetMoveDestination(const float DeltaSeconds, FVector& OutDestination)
{
	if (AnimationList.Num() == 0)
		return false;

	AILLCharacter* Character = GetOuterAILLCharacter();
	if (!Character)
	{
		return false;
	}

	if (AILLMantleVaultEdge* TargetEdge = Cast<AILLMantleVaultEdge>(TargetActor))
	{
		OutDestination = TargetEdge->GetInteractableComponent()->GetInteractionLocation(Character);
		return true;
	}

	return false;
}

bool UILLSpecialMove_MantleVaultBase::GetLookDestination(const float DeltaSeconds, FVector& OutDestination)
{
	if (!bHasLookDestination && GetMoveDestination(DeltaSeconds, OutDestination))
	{
		if (AILLCharacter* Character = GetOuterAILLCharacter())
		{
			LookDestination = Character->GetActorLocation() + (OutDestination - Character->GetActorLocation()).GetSafeNormal() * 1000.f;
			bHasLookDestination = true;
		}
	}

	if (bHasLookDestination)
	{
		OutDestination = LookDestination;
		return true;
	}

	return false;
}

void UILLSpecialMove_MantleVaultBase::DestinationReached(const FVector& Destination)
{
	AILLCharacter* Character = GetOuterAILLCharacter();

	// Figure out what montage set we are going to play
	const FILLMantleVaultCharacterMontage* ChosenMontage = nullptr;
	if (AnimationList.Num() > 0 && ensure(!Destination.IsNearlyZero()))
	{
		const FVector FeetWorldLocation = Character->GetActorLocation() - FVector(0,0,Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const float VerticalDelta = FMath::Max(Destination.Z-FeetWorldLocation.Z, 0.f);
		float SmallestMontageDelta = FLT_MAX;
		for (const FILLMantleVaultCharacterMontage& HeightedMontage : AnimationList)
		{
			const float MontageVerticalDelta = (HeightedMontage.VerticalDelta >= VerticalDelta) ? HeightedMontage.VerticalDelta-VerticalDelta : VerticalDelta-HeightedMontage.VerticalDelta;
			if (SmallestMontageDelta >= MontageVerticalDelta)
			{
				SmallestMontageDelta = MontageVerticalDelta;
				ChosenMontage = &HeightedMontage;
			}
		}
	}
	if (!ChosenMontage)
	{
		MarkPendingKill();
		return;
	}

	// Disable capsule collision and start "flying"
	Character->SetActorEnableCollision(false);
	if (UILLCharacterMovement* MovementComp = Cast<UILLCharacterMovement>(Character->GetMovementComponent()))
	{
		MovementComp->SetMovementMode(MOVE_Flying);
	}

	// Play root motion animation
	const float Duration = Character->PlayAnimMontageEx(*ChosenMontage);

	// Play a camera shake for the person mantling/vaulting
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(Character->Controller))
	{
		if (PC->IsLocalPlayerController())
		{
			PC->ClientPlayCameraShake(ChosenMontage->CameraShake, 1);
		}
	}

	// Set a timer for when the animation completes
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AnimComplete, this, &UILLSpecialMove_MantleVaultBase::RootMotionSequenceComplete, Duration);

	// Do not time out until some time after the animation
	ResetTimeout(Duration + MoveTimeLimit);
}

void UILLSpecialMove_MantleVaultBase::RootMotionSequenceComplete()
{
	// Re-enable capsule collision and start falling
	AILLCharacter* Character = GetOuterAILLCharacter();
	Character->SetActorEnableCollision(true);
	if (UILLCharacterMovement* MovementComp = Cast<UILLCharacterMovement>(Character->GetMovementComponent()))
	{
		MovementComp->SetMovementMode(MOVE_Falling);
	}

	MarkPendingKill();
}
