// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_CharacterMoveTo.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "SCCharacter.h"
#include "SCWaypoint.h"

UBTTask_CharacterMoveTo::UBTTask_CharacterMoveTo(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Character Move To");

	BreadCrumbDistance = 2400.f;
	bAutomaticBreadCrumbDistance = true;
}

void UBTTask_CharacterMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ASCCharacterAIController* MyController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	UPathFollowingComponent* MyPathFollowingComp = MyController ? MyController->GetPathFollowingComponent() : nullptr;
	ASCCharacter* MyCharacter = MyController ? Cast<ASCCharacter>(MyController->GetPawn()) : nullptr;
	UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();

	// Check to see if we should immediately move to the next waypoint
	if (MyPathFollowingComp && MyCharacter && MyBlackboard)
	{
		const ASCWaypoint* CurrentWaypoint = Cast<ASCWaypoint>(MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.SelectedKeyName));
		if (CurrentWaypoint && !CurrentWaypoint->HasActionsToExecute() && CurrentWaypoint->NextWaypoint)
		{
			// Check to see if we have reached our CurrentWaypoint
			const UCapsuleComponent* CapsuleComponent = MyCharacter->GetCapsuleComponent();
			const float CapsuleRadius = CapsuleComponent ? CapsuleComponent->GetScaledCapsuleRadius() : 35.f;
			if (MyPathFollowingComp->HasReached(*CurrentWaypoint, EPathFollowingReachMode::OverlapAgent, AcceptableRadius + CapsuleRadius))
			{
				// Update our current goal with the next waypoint, so there is no pause when moving to the next waypoint
				MyBlackboard->SetValue<UBlackboardKeyType_Object>(BlackboardKey.SelectedKeyName, CurrentWaypoint->NextWaypoint);

				// Make sure waypoint events fire even though we aren't stopping here
				CurrentWaypoint->NotifyPassingThroughWaypoint(MyCharacter);
			}
		}
	}

	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
}

void UBTTask_CharacterMoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (ASCCharacterAIController* MyController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner()))
	{
		MyController->OnMoveTaskFinished(TaskResult, bAlreadyAtGoal);
	}
}

EBTNodeResult::Type UBTTask_CharacterMoveTo::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (bAutomaticBreadCrumbDistance)
	{
		if (ASCCharacterAIController* MyController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner()))
		{
			BreadCrumbDistance = MyController->NavigationInvokerRadius*.8f;
		}
	}

	return Super::PerformMoveTask(OwnerComp, NodeMemory);
}
