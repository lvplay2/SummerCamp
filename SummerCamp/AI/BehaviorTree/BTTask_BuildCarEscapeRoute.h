// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_BuildCarEscapeRoute.generated.h"

/**
 * @class UBTTask_BuildCarEscapeRoute
 */
UCLASS()
class SUMMERCAMP_API UBTTask_BuildCarEscapeRoute
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
	
	// Key to store the car's escape path
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector CarEscapeRouteKey;

	// Key to store Current Road Spline Index
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector EscapeIndexKey;
};
