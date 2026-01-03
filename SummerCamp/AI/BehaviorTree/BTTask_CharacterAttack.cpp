// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_CharacterAttack.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "SCCounselorCharacter.h"
#include "SCCharacterAIController.h"
#include "SCGameMode.h"
#include "SCGun.h"
#include "SCKillerCharacter.h"

UBTTask_CharacterAttack::UBTTask_CharacterAttack(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bNotifyTick = true;
	NodeName = TEXT("Character Attack");

	// Accept only characters
	BlackboardKey.AddObjectFilter(this, *NodeName, ASCCharacter::StaticClass());

	MinGunAimTime = .75f;
	MaxGunAimTime = 1.5f;

	MinRangeEasy = -15.f;
	MaxRangeEasy = 15.f;

	MinRangeNormal = -5.f;
	MaxRangeNormal = 5.f;
}

EBTNodeResult::Type UBTTask_CharacterAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	if (ASCCharacter* Character = BotController ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr)
	{
		ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character);
		if (Character->CanAttack() && !Character->IsInWheelchair())
		{
			// Snap to face target
			const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
			ASCKillerCharacter* TargetCharacter = Cast<ASCKillerCharacter>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKeyId));
			if (Counselor && TargetCharacter)
			{
				FRotator RotationToEnemy = (TargetCharacter->GetActorLocation() - Counselor->GetActorLocation()).Rotation();
				RotationToEnemy.Roll = 0.f;
				Counselor->Controller->SetControlRotation(RotationToEnemy);
			}

			if (Counselor)
			{
				const ASCWeapon* CurrentWeapon = Character->GetCurrentWeapon();
				if (CurrentWeapon && CurrentWeapon->IsA<ASCGun>())
				{
					// Start aiming then tick a bit before firing
					Counselor->OnAimPressed();
					ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
					const float Easy50Higher = GameMode->GetCurrentDifficultyDownwardAlpha(.5f);
					TimeUntilFire = FMath::RandRange(MinGunAimTime, MaxGunAimTime) * Easy50Higher;
					return EBTNodeResult::InProgress;
				}
			}

			// Attack
			Character->OnAttack();
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}

void UBTTask_CharacterAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (OwnerComp.IsPaused())
		return;

	if (TimeUntilFire > 0.f)
	{
		UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
		ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
		ASCCounselorCharacter* Counselor = BotController ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;

		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		ASCKillerCharacter* TargetCharacter = Cast<ASCKillerCharacter>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKeyId));

		if (Counselor && TargetCharacter)
		{
			// Fail the task when the TargetCharacter shifts
			if (TargetCharacter->IsShiftActive())
			{
				// Stop aiming!
				Counselor->OnAimReleased();
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			// Lock on to target
			FRotator RotationToEnemy = (TargetCharacter->GetActorLocation() - Counselor->GetActorLocation()).Rotation();
			RotationToEnemy.Roll = 0.f;
			float RotationMultiplier = 2.f;
			UpdateAccuracyForDifficulty(RotationToEnemy, RotationMultiplier);
			Counselor->Controller->SetControlRotation(FMath::Lerp(Counselor->Controller->GetControlRotation(), RotationToEnemy, DeltaSeconds*RotationMultiplier));
		}

		// Check if we should fire
		TimeUntilFire -= DeltaSeconds;
		if (TimeUntilFire <= 0.f)
		{
			if (Counselor)
			{
				// Fire and release aim
				Counselor->OnLargeItemPressed();
				Counselor->OnAimReleased();
				Counselor->OnLargeItemReleased();
			}

			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

void UBTTask_CharacterAttack::UpdateAccuracyForDifficulty(FRotator& Rotation, float& RotationSpeedMultiplier)
{
	if (const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		switch (GameMode->GetModeDifficulty())
		{
		case ESCGameModeDifficulty::Easy:
			RotationSpeedMultiplier = 1.25f;
			Rotation += FRotator(FMath::RandRange(MinRangeEasy, MaxRangeEasy), FMath::RandRange(MinRangeEasy, MaxRangeEasy), 0.f);
			break;
		case ESCGameModeDifficulty::Normal:
			RotationSpeedMultiplier = 2.f;
			if (FMath::RandBool())
			{
				Rotation += FRotator(FMath::RandRange(MinRangeNormal, MaxRangeNormal), FMath::RandRange(MinRangeNormal, MaxRangeNormal), 0.f);
			}
			break;
		case ESCGameModeDifficulty::Hard:
			RotationSpeedMultiplier = 2.25f;
			break;
		default:
			break;
		}
	}
}
