// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "BTTask_FindNextWaypoint.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "ILLCharacter.h"
#include "ILLAIController.h"
#include "ILLAIWaypoint.h"


UBTTask_FindNextWaypoint::UBTTask_FindNextWaypoint(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Next Waypoint");

	// Accept only ILLAIWaypoints
	WaypointKey.AddObjectFilter(this, *NodeName, AILLAIWaypoint::StaticClass());
}

EBTNodeResult::Type UBTTask_FindNextWaypoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AILLAIController* BotController = Cast<AILLAIController>(OwnerComp.GetAIOwner());
	AILLCharacter* BotPawn = BotController ? Cast<AILLCharacter>(BotController->GetPawn()) : nullptr;
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();

	if (BotBlackboard && BotPawn)
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
				AILLAIWaypoint* NextWaypoint = nullptr;

				if (CurrentWaypoint && CurrentWaypoint->IsActorAtWaypoint(BotPawn) && CurrentWaypoint->bSnapToWaypointAfterTasks)
				{
					const FVector CurrentPosition = BotPawn->GetActorLocation();
					const FVector DesiredPosition = CurrentWaypoint->GetActorLocation();
					if (!CurrentPosition.Equals(DesiredPosition, 0.5f))
						BotPawn->SetActorLocation(DesiredPosition);
				}

				if (AILLAIWaypoint* OverrideWaypoint = BotController->GetOverrideNextWaypoint())
				{
					// If we have an override next waypoint, move to that waypoint and clear our override
					NextWaypoint = OverrideWaypoint;
					BotController->SetOverrideNextWaypoint(nullptr);
				}
				else if (CurrentWaypoint)
				{
					// Make sure we've reached our current waypoint before trying to move to the next one
					if (CurrentWaypoint->IsActorAtWaypoint(BotPawn))
					{
						NextWaypoint = CurrentWaypoint->NextWaypoint;

						// Resume the tail waypoint if we have reached the end
						if (!NextWaypoint && BotController->bAssignedStartingWaypoint && !CurrentWaypoint->bIgnoreRestartWhileAtWaypoint)
						{
							NextWaypoint = CurrentWaypoint;
						}
					}
					else
					{
						// Keep using our current node since we haven't reached it yet
						return EBTNodeResult::Succeeded;
					}
				}
				else if (!BotController->bAssignedStartingWaypoint)
				{
					// Get the starting waypoint from the character
					NextWaypoint = BotController->StartingWaypoint;
					BotController->bAssignedStartingWaypoint = true;
				}

				// Set the next waypoint
				if (NextWaypoint)
				{
					BotBlackboard->SetValue<UBlackboardKeyType_Object>(WaypointKeyId, NextWaypoint);

					if (CurrentWaypoint)
						CurrentWaypoint->RemoveOverlappingActor(BotPawn);

					return EBTNodeResult::Succeeded;
				}

				UE_LOG(LogILLAI, Warning, TEXT("Failed to find next waypoint for %s"), *BotPawn->GetName());
			}
		}
	}

	return EBTNodeResult::Failed;
}
