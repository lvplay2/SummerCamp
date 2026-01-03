// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindRepairPartGoal.generated.h"

/**
 * @class UBTTask_FindRepairPartGoal
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindRepairPartGoal 
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

	// How far away a repair can be to be considered
	UPROPERTY(EditAnywhere)
	float DistanceLimit;
};
