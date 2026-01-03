// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "BTTask_ExecuteWaypointActions.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "ILLCharacter.h"
#include "ILLAIController.h"
#include "ILLAIWaypoint.h"

UBTTask_ExecuteWaypointActions::UBTTask_ExecuteWaypointActions(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Execute Waypoint Actions");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	// Accept only ILLAIWaypoints
	WaypointKey.AddObjectFilter(this, *NodeName, AILLAIWaypoint::StaticClass());
}

EBTNodeResult::Type UBTTask_ExecuteWaypointActions::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AILLAIController* BotController = Cast<AILLAIController>(OwnerComp.GetAIOwner());
	AILLCharacter* BotPawn = Cast<AILLCharacter>(BotController->GetPawn());
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	TimeAtWaypoint = 0.f;

	if (BotController && BotBlackboard && BotPawn)
	{
		// Make sure our selected key is valid
		const FBlackboard::FKey WaypointKeyId = BotBlackboard->GetKeyID(WaypointKey.SelectedKeyName);
		if (WaypointKeyId != FBlackboard::InvalidKey)
		{
			const TSubclassOf<UBlackboardKeyType> WaypointKeyType = BotBlackboard->GetKeyType(WaypointKeyId);
			if (WaypointKeyType == UBlackboardKeyType_Object::StaticClass())
			{
				// Get the current value for our waypoint in the blackboard
				AILLAIWaypoint* CurrentWaypoint = Cast<AILLAIWaypoint>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(WaypointKeyId));
				if (CurrentWaypoint && !CurrentWaypoint->IsActorAtWaypoint(BotPawn))
				{
					// Keep track of bots at this waypoint
					CurrentWaypoint->AddOverlappingActor(BotPawn);

					// Ignore collision of specified actors
					for (AActor* Actor : CurrentWaypoint->IgnoreCollisionOnActors)
						BotPawn->MoveIgnoreActorAdd(Actor);

					// See if we need to do execute a BT or adjust alignment or positioning
					if (CurrentWaypoint->HasActionsToExecute())
					{
						BotController->bCanExecuteSynchronizedWaypointActions = false;
						return EBTNodeResult::InProgress;
					}
				}
				
				BotController->bCanExecuteSynchronizedWaypointActions = true;
				return EBTNodeResult::Succeeded;
			}
		}
	}

	return EBTNodeResult::Failed;
}

void UBTTask_ExecuteWaypointActions::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AILLAIController* BotController = Cast<AILLAIController>(OwnerComp.GetAIOwner());
	AILLCharacter* BotPawn = Cast<AILLCharacter>(BotController->GetPawn());
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	const AILLAIWaypoint* CurrentWaypoint = Cast<AILLAIWaypoint>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(BotBlackboard->GetKeyID(WaypointKey.SelectedKeyName)));
	TimeAtWaypoint += DeltaSeconds;

	if (CurrentWaypoint)
	{
		// See if we have timed out on synchronizing with other waypoints
		if (CurrentWaypoint->bSynchronizeWithOtherWaypoints && CurrentWaypoint->SynchronizeTimeout > 0.f)
		{
			if (TimeAtWaypoint >= CurrentWaypoint->SynchronizeTimeout)
			{
				BotController->SetOverrideNextWaypoint(CurrentWaypoint->TimeoutWaypoint);
				UE_LOG(LogILLAI, Warning, TEXT("%s failed to synchronize at %s. Moving to TimeoutWaypoint."), *BotPawn->GetName(), *CurrentWaypoint->GetName());
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}
		}

		if (CurrentWaypoint->bMoveBotToArrow)
		{
			// Move this AI's pawn to the waypoint's arrow
			const FVector BotPos = BotPawn->GetActorLocation();
			const FVector DesiredPos = CurrentWaypoint->DirectionToFace->GetComponentLocation();
			if (!BotPos.Equals(DesiredPos, 0.5f))
			{
				if (CurrentWaypoint->bSnapPosition)
				{
					BotPawn->SetActorLocation(DesiredPos);
				}
				else
				{
					// Check for move to timeout
					if (CurrentWaypoint->MoveToArrowTimeout > 0 && TimeAtWaypoint >= CurrentWaypoint->MoveToArrowTimeout)
					{
						if (CurrentWaypoint->bSnapPositionOnMoveTimeout)
						{
							BotPawn->SetActorLocation(DesiredPos);
						}
						else
						{
							UE_LOG(LogILLAI, Warning, TEXT("%s failed to move to %s's DirectionToFaceArrow in %.2f seconds."), *BotPawn->GetName(), *CurrentWaypoint->GetName(), CurrentWaypoint->MoveToArrowTimeout);
							FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
							return;
						}
					}
					else
					{
						// Move towards our desired location
						BotPawn->AddMovementInput((DesiredPos - BotPos).GetSafeNormal2D(), CurrentWaypoint->MoveRate);
						return;
					}
				}
			}
		}

		if (CurrentWaypoint->bAlignBotWithArrow)
		{
			// Align this AI's pawn with the waypoint's arrow
			const FRotator BotCurrentRotation = BotPawn->GetActorRotation();
			const FRotator DirectionToFace = CurrentWaypoint->DirectionToFace->GetComponentRotation();
			if (!BotCurrentRotation.Equals(DirectionToFace, 0.5f))
			{
				if (CurrentWaypoint->bSnapRotation)
				{
					BotPawn->SetActorRotation(DirectionToFace);
				}
				else
				{
					BotPawn->SetActorRotation(FMath::Lerp(BotCurrentRotation, DirectionToFace, DeltaSeconds * CurrentWaypoint->RotationRate));
					return;
				}
			}
		}

		BotController->bCanExecuteSynchronizedWaypointActions = true;
		if (CurrentWaypoint->bSynchronizeWithOtherWaypoints)
		{
			// Make sure we are synchronized with our other waypoints
			for (const FSynchronizedWaypointInfo& SynchInfo : CurrentWaypoint->WaypointsToSynchronizeWith)
			{
				if (!SynchInfo.IsSynchronized())
					return;
			}
		}

		if (CurrentWaypoint->BehaviorToExecute && OwnerComp.GetCurrentTree() != CurrentWaypoint->BehaviorToExecute)
		{
			// Run the waypoint's behavior tree once
			BehaviorAsset = CurrentWaypoint->BehaviorToExecute;
			Super::ExecuteTask(OwnerComp, NodeMemory);
		}
		else
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
	else
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

void UBTTask_ExecuteWaypointActions::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	AILLAIController* BotController = Cast<AILLAIController>(OwnerComp.GetAIOwner());
	AILLCharacter* BotPawn = Cast<AILLCharacter>(BotController->GetPawn());
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	const AILLAIWaypoint* CurrentWaypoint = Cast<AILLAIWaypoint>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(BotBlackboard->GetKeyID(WaypointKey.SelectedKeyName)));

	if (BotPawn && CurrentWaypoint)
	{
		// Snap position back
		if (CurrentWaypoint->bSnapToWaypointAfterTasks)
		{
			const FVector CurrentPosition = BotPawn->GetActorLocation();
			const FVector DesiredPosition = CurrentWaypoint->GetActorLocation();
			if (!CurrentPosition.Equals(DesiredPosition, 0.5f))
				BotPawn->SetActorLocation(DesiredPosition);
		}

		// Remove collision ignores
		for (AActor* Actor : CurrentWaypoint->IgnoreCollisionOnActors)
			BotPawn->MoveIgnoreActorRemove(Actor);

	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
