// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BTTask_ExecuteWaypointActions.generated.h"

/**
 * @class UBTTask_ExecuteWaypointActions
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UBTTask_ExecuteWaypointActions
: public UBTTask_RunBehavior
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	// End UBTTaskNode interface

protected:

	// Navigation filter class
	UPROPERTY(EditAnywhere, Category="Condition")
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	// Blackboard actor to store the next waypoint
	UPROPERTY(EditAnywhere, Category="Condition")
	FBlackboardKeySelector WaypointKey;

	// How long we've been at this waypoint
	float TimeAtWaypoint;
};
