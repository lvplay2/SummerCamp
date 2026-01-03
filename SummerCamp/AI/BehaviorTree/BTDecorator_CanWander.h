// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanWander.generated.h"

/**
 * @class UBTDecorator_CanWander
 * @brief Checks to see if we are allowed to wander.
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_CanWander
: public UBTDecorator
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif
	// End UBTDecorator interface

protected:
	// Minimum time we have not been moving before this returns true
	UPROPERTY(EditAnywhere, Category="Condition")
	float MinimumTimeWithoutVelocity;
};
