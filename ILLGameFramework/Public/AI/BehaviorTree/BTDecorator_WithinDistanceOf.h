// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_WithinDistanceOf.generated.h"

/**
 * @class UBTDecorator_WithinDistanceOf
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UBTDecorator_WithinDistanceOf
: public UBTDecorator_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// End UBTDecorator interface

protected:
	// Distance limit to BlackboardKey
	UPROPERTY(EditAnywhere, Category="Condition")
	float MaxDistance;
};
