// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindCarDestination.generated.h"

/**
 * @class UBTTask_FindVehicleDestination
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindCarDestination
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:
	///////////////////////////////////////////////
	// Blackboard Keys

	// Key to store Vehicle Destination
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector CarDestinationKey;

	// Key to store Current Road
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector CarEscapeRouteKey;

	// Key to store Current Road Spline Index
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector EscapeIndexKey;
};
