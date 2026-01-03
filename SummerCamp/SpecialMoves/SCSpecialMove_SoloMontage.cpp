// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSpecialMove_SoloMontage.h"

#include "Animation/AnimMontage.h"
#include "Kismet/KismetMathLibrary.h"
#include "MessageLog.h"
#include "UObjectToken.h"

#include "SCKillerCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCAnimInstance.h"

#if WITH_EDITOR
TAutoConsoleVariable<int32> CVarFailMove(TEXT("sc.AlwaysFailSpecialMoves"), 0,
	TEXT("When true, movement to a special move will always fail.\n")
	TEXT("0: Disabled\n")
	TEXT("1: Fail on destination reached\n")
	TEXT("2: Fail half way through an interaction\n")
);
#endif

USCSpecialMove_SoloMontage::USCSpecialMove_SoloMontage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bInterpToInteraction(true)
, InterpolationTime(0.15f)
, SpeedUpTimelimit(0.f)
, bCallOnFinishedDelegates(true)
, FinishAfterSectionName(NAME_None)
, bRunToDestination(false)
, DesiredTransform(FTransform::Identity)
, TickedTime(0.f)
, bActionStarted(false)
, MontagePlayRate(1.0f)
{
	bMoveToward = true;
	MoveScale = 1.f;
	MoveThreshold = 15.f;
	MoveTimeLimit = 0.f;

	bLookToward = true;
	LookTowardRate = 12.f;
	bSnapLookOnDestinationReached = true;
	bReachedDestination = false;
	bMoveFinished = false;
	bSweepTestOnLerp = true;
}

void USCSpecialMove_SoloMontage::SetDesiredTransform(FVector InLocation, FRotator InRotation, bool InRequiresLocation)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

#if WITH_EDITOR
	if (MoveThreshold <= KINDA_SMALL_NUMBER)
	{
		FMessageLog("PIE").Warning()
			->AddToken(FUObjectToken::Create(GetClass()))
			->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(" has a very small move threshold (%.3f), this will make it very difficult to use!"), MoveThreshold))));
	}
#endif

	ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
	if (Character == nullptr)
		return;

	if (USCAnimInstance* AI = Cast<USCAnimInstance>(Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr))
	{
		AI->AllowRootMotion();
	}

	const float ZOffset = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CharacterRunningState = Character->IsRunning();
	bRequiresLocation = InRequiresLocation;
	if (bRequiresLocation)
	{
		DesiredTransform = FTransform(InRotation, InLocation, FVector::OneVector);
		DesiredTransform.SetTranslation(DesiredTransform.GetTranslation() + FVector::UpVector * ZOffset);
	}
	else
	{
		// We're moving to the character rather than the other way around
		DesiredTransform = Character->GetTransform();
	}
}

void USCSpecialMove_SoloMontage::BeginDestroy()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DestroyMove);
#if WITH_EDITOR
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AlwaysFailAbort);
#endif
	}

	Super::BeginDestroy();
}

void USCSpecialMove_SoloMontage::BeginMove(AActor* InTargetActor, FOnILLSpecialMoveCompleted::FDelegate InCompletionDelegate)
{
	Super::BeginMove(InTargetActor, InCompletionDelegate);

	ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
	if (!Character)
		return;

	if (bInterpToInteraction)
	{
		StartingTransform = Character->GetActorTransform();
	}
	else if (Character->IsRunning() != bRunToDestination)
	{
		Character->SetRunning(bRunToDestination);
	}
}

void USCSpecialMove_SoloMontage::TickMove(const float DeltaSeconds)
{
	if (bMoveFinished)
		return;

	ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
	if (!Character)
		return;

	if (!bRequiresLocation)
	{
		DestinationReached(FVector::ZeroVector);
		return;
	}

	TickedTime += DeltaSeconds;

	FRotator TargetRot = FRotator(DesiredTransform.GetRotation());
	if (bLookToward)
	{
		if ((bLookAfterReachedDestination && bReachedDestination) || !bLookAfterReachedDestination)
		{
			TargetRot = FMath::Lerp(Character->GetActorRotation(), TargetRot, TickedTime / LookTowardRate);
			Character->SetActorRotation(TargetRot);
		}
	}

	if (!bReachedDestination && bMoveToward)
	{
		const FVector MoveDelta = DesiredTransform.GetTranslation() - Character->GetActorLocation();
		const float Threshold = MoveThreshold;
		if (bInterpToInteraction)
		{
			const float Alpha = FMath::Clamp(TickedTime / FMath::Max(InterpolationTime, SMALL_NUMBER), 0.f, 1.f);

			if (Alpha >= 1.f)
			{
				bReachedDestination = !Character->SetActorTransform(DesiredTransform, bSweepTestOnLerp);

#if WITH_EDITOR
				if (CVarFailMove.GetValueOnGameThread() == 1)
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Special move %s failed to reach desitnation because sc.AlwaysFailSpecialMoves is 1!"), *GetClass()->GetName()));

					// Should replicate failing code below
					CancelSpecialMove();
				}
				else
#endif
				// Could not reach our destination
				if (!bReachedDestination)
				{
					// If we can't get within 1/4 meter, cancel. Otherwise just snap
					const float ResultingOffset = (Character->GetActorLocation() - DesiredTransform.GetLocation()).SizeSquared2D();
					if (ResultingOffset > FMath::Square(Threshold))
					{
						CancelSpecialMove();
					}
					else
					{
						Character->SetActorTransform(DesiredTransform);
						bReachedDestination = true;
					}
				}
			}
			else
			{
				FHitResult SweepHit;
				FTransform Interper;
				Interper.Blend(StartingTransform, DesiredTransform, Alpha);
				Character->SetActorTransform(Interper, bSweepTestOnLerp, &SweepHit);

				// Found a collision, push out
				if (SweepHit.bBlockingHit)
					Character->SetActorLocation(SweepHit.Location);
			}
		}
		else if (MoveDelta.SizeSquared2D() > FMath::Square(Threshold))
		{
			// We're going to try and locomote to our interaction point
			if (Character->IsLocallyControlled())
				Character->AddMovementInput(MoveDelta.GetSafeNormal2D(), MoveScale);
		}
		else
		{
			bReachedDestination = true;
			if (bSnapLookOnDestinationReached)
			{
				Character->SetActorRotation(DesiredTransform.GetRotation());
			}
		}
	}

	if ((Character->GetActorRotation().Equals(FRotator(DesiredTransform.GetRotation()), 5.f) || !bLookToward) && (bReachedDestination || !bMoveToward))
	{
		DestinationReached(DesiredTransform.GetLocation());
	}
}

void USCSpecialMove_SoloMontage::DestinationReached(const FVector& Destination)
{
	if (bMoveFinished)
		return;

	bMoveFinished = true;
	bReachedDestination = true;
	TickedTime = 0.f;

	if (DestinationReachedDelegate.IsBound())
	{
		DestinationReachedDelegate.Broadcast();
		DestinationReachedDelegate.Clear();
		return;
	}

	if (ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	BeginAction();
}

void USCSpecialMove_SoloMontage::BeginAction()
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	// Don't double start, and prevent starting actions we aborted before getting in position
	if (bActionStarted || bSpecialComplete)
		return;

	bActionStarted = true;
	ActionStart_Timestamp = GetWorld()->GetTimeSeconds();

	// play montage on our character
	ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
	if (!Character)
	{
		ConditionalBeginDestroy();
		return;
	}

	Character->SetRunning(CharacterRunningState);

	UAnimMontage* ChosenMontage = GetContextMontage();

	UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInst && ChosenMontage)
	{
		float PlayTime = AnimInst->Montage_Play(ChosenMontage, MontagePlayRate, EMontagePlayReturnType::Duration);

		// Catchall to ensure the special move is completed when the montage stops playing
		FOnMontageEnded MontageEndDelegate;
		MontageEndDelegate.BindUObject(this, &USCSpecialMove_SoloMontage::OnMontageEnded);
		AnimInst->Montage_SetEndDelegate(MontageEndDelegate, ChosenMontage);

		// wait for montage to be over
		int32 index = !FinishAfterSectionName.IsNone() ? ChosenMontage->GetSectionIndex(FinishAfterSectionName) : INDEX_NONE;
		if (index != INDEX_NONE)
		{
			PlayTime = 0.f;
			for (; index >= 0; --index)
			{
				PlayTime += ChosenMontage->GetSectionLength(index) / (MontagePlayRate * ChosenMontage->RateScale);
			}
		}

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_DestroyMove, this, &USCSpecialMove_SoloMontage::ForceDestroy, PlayTime);

#if WITH_EDITOR
		if (CVarFailMove.GetValueOnGameThread() == 2)
		{
			FTimerDelegate Delegate;

			Delegate.BindLambda([this]()
			{
				if (IsValid(this))
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Special move %s canceled mid move because sc.AlwaysFailSpecialMoves is 2!"), *GetClass()->GetName()));
					CancelSpecialMove();
				}
			});

			GetWorld()->GetTimerManager().SetTimer(TimerHandle_AlwaysFailAbort, Delegate, PlayTime * 0.5f, false);
		}
#endif
	}
	else
	{
		ForceDestroy();
	}
}

void USCSpecialMove_SoloMontage::CancelSpecialMove()
{
	// This special is already being canceled, stop it.
	if (bCanceling)
		return;

	bCanceling = true;
	// If our cancel is bound call that and clear the completion delegate (so actors don't cleanup twice), otherwise the completion delegate can handle things
	if (CancelSpecialMoveDelegate.IsBound())
	{
		CancelSpecialMoveDelegate.Broadcast();
		CancelSpecialMoveDelegate.Clear();

		CompletionDelegate.Clear();
	}

	if (bActionStarted)
	{
		// stop montage on our character
		ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
		if (!Character)
		{
			ConditionalBeginDestroy();
			return;
		}

		Character->SetRunning(CharacterRunningState);

		UAnimMontage* ChosenMontage = GetContextMontage();

		UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInst && ChosenMontage)
		{
			AnimInst->Montage_Stop(0.f, ChosenMontage);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DestroyMove);
	ForceDestroy();
}

void USCSpecialMove_SoloMontage::ResetTimer(float Time)
{
	if (!GetWorld())
		return;

	if (Time >= 0.f)
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_DestroyMove, this, &USCSpecialMove_SoloMontage::ForceDestroy, Time);
	else
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DestroyMove);

#if WITH_EDITOR
	if (CVarFailMove.GetValueOnGameThread() == 2)
	{
		FTimerDelegate Delegate;

		Delegate.BindLambda([this]()
		{
			if (IsValid(this))
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Special move %s canceled mid move because sc.AlwaysFailSpecialMoves is 2!"), *GetClass()->GetName()));
				CancelSpecialMove();
			}
		});

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_AlwaysFailAbort, Delegate, Time <= 0.f ? 0.5f : Time * 0.5f, false);
	}
#endif
}

float USCSpecialMove_SoloMontage::GetTimeRemaining() const
{
	if (!GetWorld() || !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_DestroyMove))
		return -1.f;

	return GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_DestroyMove);
}

float USCSpecialMove_SoloMontage::GetTimeElapsed() const
{
	if (!GetWorld() || !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_DestroyMove))
		return -1.f;

	return GetWorld()->GetTimerManager().GetTimerElapsed(TimerHandle_DestroyMove);
}

float USCSpecialMove_SoloMontage::GetTimePercentage() const
{
	const float TimeRemaining = GetTimeRemaining();
	if (TimeRemaining < 0.f)
		return -1.f;

	const float TimeElapsed = GetTimeElapsed();
	const float TotalTime = TimeRemaining + TimeElapsed;

	return TimeElapsed / TotalTime;
}

bool USCSpecialMove_SoloMontage::TrySpeedUp(bool bForce /*= false*/)
{
	// First test if this special move would ever go faster
	if (CanSpeedUp() == false)
		return false;

	// Don't do it twice
	if (bIsPlayingFast)
		return false;

	// Make sure we do it within the set time limit (so you can't wait a super long time like an asshole)
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bForce == false && CurrentTime - ActionStart_Timestamp > SpeedUpTimelimit)
		return false;

	// General safety checks...
	ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
	if (Character == nullptr)
		return false;

	UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInst == nullptr)
		return false;

	// We can go faster!
	bIsPlayingFast = true;

	// Stop listening for the end event on our current playing montage
	if (FAnimMontageInstance* MontageInst = AnimInst->GetActiveInstanceForMontage(ContextMontage))
	{
		MontageInst->OnMontageEnded.Unbind();
	}

	// Stop the current montage and play the new one at the same percentage through as the old one
	const float CurrentAlpha = AnimInst->Montage_GetPosition(ContextMontage) / FMath::Max(ContextMontage->GetPlayLength(), KINDA_SMALL_NUMBER);
	AnimInst->Montage_Stop(0.f, ContextMontage);
	AnimInst->Montage_Play(ContextMontage_Fast);
	AnimInst->Montage_SetPosition(ContextMontage_Fast, ContextMontage_Fast->GetPlayLength() * CurrentAlpha);

	// Catchall to ensure the special move is completed when the montage stops playing
	FOnMontageEnded MontageEndDelegate;
	MontageEndDelegate.BindUObject(this, &USCSpecialMove_SoloMontage::OnMontageEnded);
	AnimInst->Montage_SetEndDelegate(MontageEndDelegate, ContextMontage_Fast);

	// Also fix our timer so we die at the right time
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_DestroyMove, this, &USCSpecialMove_SoloMontage::ForceDestroy, ContextMontage_Fast->GetPlayLength() * (1.f - CurrentAlpha));

	if (SpeedUpAppliedDelegate.IsBound())
	{
		SpeedUpAppliedDelegate.Broadcast();
		SpeedUpAppliedDelegate.Clear();
	}

	return true;
}

void USCSpecialMove_SoloMontage::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter());
	UAnimInstance* AnimInst = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInst)
	{
		if (FAnimMontageInstance* MontageInst = AnimInst->GetActiveInstanceForMontage(Montage))
		{
			MontageInst->OnMontageEnded.Unbind();
		}
	}

	ForceDestroy();
}

void USCSpecialMove_SoloMontage::ForceDestroy()
{
#if WITH_EDITOR
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AlwaysFailAbort);
#endif

	if (bSpecialComplete)
		return;

	bSpecialComplete = true;
	DestinationReachedDelegate.Clear();
	CancelSpecialMoveDelegate.Clear();
	SpeedUpAppliedDelegate.Clear();

	if (bCallOnFinishedDelegates)
		CompletionDelegate.Broadcast();
	CompletionDelegate.Clear();

	if (ASCCharacter* Character = Cast<ASCCharacter>(GetOuterAILLCharacter()))
	{
		Character->OnSpecialMoveCompleted();
	}
}
