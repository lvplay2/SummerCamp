// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_WithinTimeOf.generated.h"

/**
 * @class UBTDecorator_WithinTimeOf
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UBTDecorator_WithinTimeOf
: public UBTDecorator_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// End UBTDecorator interface

protected:
	// Time limit away from BlackboardKey's value
	UPROPERTY(EditAnywhere, Category="Condition")
	float TimeLimit;
};
