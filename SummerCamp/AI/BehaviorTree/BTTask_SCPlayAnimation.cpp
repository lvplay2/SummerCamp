// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_SCPlayAnimation.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

#include "SCCharacter.h"
#include "SCCharacterAIController.h"

UBTTask_SCPlayAnimation::UBTTask_SCPlayAnimation(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Play Animation");
	// instantiating to be able to use Timers
	bCreateNodeInstance = true;

	TimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnAnimationTimerDone);
}

EBTNodeResult::Type UBTTask_SCPlayAnimation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = EBTNodeResult::Failed;

	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	MyCharacter = BotController ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr;
	MyOwnerComp = &OwnerComp;

	if (MyCharacter)
	{
		Result = PlayAnimation(MyCharacter->GetIsFemale() ? FemaleAnimationToPlay : MaleAnimationToPlay);
	}

	return Result;
}

EBTNodeResult::Type UBTTask_SCPlayAnimation::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* BotController = OwnerComp.GetAIOwner())
	{
		// Clear our timer
		if (TimerHandle.IsValid())
			BotController->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

		// Stop our animation
		if (ASCCharacter* BotPawn = Cast<ASCCharacter>(BotController->GetPawn()))
			BotPawn->StopAnimMontage(CurrentPlayingAnimation);
	}

	CurrentPlayingAnimation = nullptr;
	MyCharacter = nullptr;
	MyOwnerComp = nullptr;
	TimerHandle.Invalidate();

	return EBTNodeResult::Aborted;
}

FString UBTTask_SCPlayAnimation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: Female: '%s'\nMale: '%s'"), *Super::GetStaticDescription(), *GetNameSafe(FemaleAnimationToPlay), *GetNameSafe(MaleAnimationToPlay));
}

#if WITH_EDITOR
FName UBTTask_SCPlayAnimation::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.PlaySound.Icon");
}
#endif

EBTNodeResult::Type UBTTask_SCPlayAnimation::PlayAnimation(UAnimMontage* MontageToPlay)
{
	if (MontageToPlay && MyCharacter)
	{
		// Reset timer handle
		TimerHandle.Invalidate();
		CurrentPlayingAnimation = MontageToPlay;

		const float FinishDelay = MyCharacter->PlayAnimMontage(MontageToPlay);
		const int32 SectionCount = MontageToPlay->CompositeSections.Num();
		bool bIsLoopingMontage = false;
		if (SectionCount != INDEX_NONE)
			bIsLoopingMontage = MontageToPlay->CompositeSections[SectionCount - 1].NextSectionName != NAME_None;

		if (FinishDelay > 0)
		{
			// No need to handle the finish if this is a looping animation
			if (!bIsLoopingMontage)
				MyCharacter->GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, FinishDelay, false);

			return EBTNodeResult::InProgress;
		}
		else
		{
			UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s> Instant success due to having a valid MontageToPlay and Character with SkelMesh, but 0-length animation"), *GetNodeName());
			CurrentPlayingAnimation = nullptr;
			MyCharacter = nullptr;
			MyOwnerComp = nullptr;

			// we're done here, report success so that BT can pick next task
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}

void UBTTask_SCPlayAnimation::OnAnimationTimerDone()
{
	if (MyOwnerComp)
	{
		// Stop the montage if it's still playing
		if (MyCharacter && MyCharacter->IsAnimMontagePlaying(CurrentPlayingAnimation))
		{
			MyCharacter->StopAnimMontage(CurrentPlayingAnimation);
		}

		FinishLatentTask(*MyOwnerComp, EBTNodeResult::Succeeded);

		CurrentPlayingAnimation = nullptr;
		MyCharacter = nullptr;
		MyOwnerComp = nullptr;
		TimerHandle.Invalidate();
	}
}

