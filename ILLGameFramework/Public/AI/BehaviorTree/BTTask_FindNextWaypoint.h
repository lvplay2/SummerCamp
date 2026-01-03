// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindNextWaypoint.generated.h"

/**
 * @class UBTTask_FindNextWaypoint
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UBTTask_FindNextWaypoint
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:

	// Navigation filter class
	UPROPERTY(EditAnywhere, Category="Condition")
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	// Blackboard actor to store the next waypoint
	UPROPERTY(EditAnywhere, Category="Condition")
	FBlackboardKeySelector WaypointKey;
};
