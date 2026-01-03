// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_CounselorMoveTo.h"

UBTTask_CounselorMoveTo::UBTTask_CounselorMoveTo(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Counselor Move To");
}

void UBTTask_CounselorMoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (ASCCounselorAIController* MyCounselor = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner()))
	{
		MyCounselor->SetMovePreferences(ESCCounselorMovementPreference::FearAndStamina, ESCCounselorMovementPreference::FearAndStamina);
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

EBTNodeResult::Type UBTTask_CounselorMoveTo::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const EBTNodeResult::Type Result = Super::PerformMoveTask(OwnerComp, NodeMemory);

	if (Result == EBTNodeResult::InProgress)
	{
		if (ASCCounselorAIController* MyCounselor = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner()))
		{
			MyCounselor->SetMovePreferences(RunPreference, SprintPreference);
		}
	}

	return Result;
}
