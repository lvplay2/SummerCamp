// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_CarHasEscapeRoute.generated.h"

/**
 * @class UBTDecorator_CarHasEscapeRoute
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_CarHasEscapeRoute
: public UBTDecorator
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// End UBTDecorator interface

protected:
	// Blackboard object storing the current car escape route
	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector CarEscapeRouteKey;
};
