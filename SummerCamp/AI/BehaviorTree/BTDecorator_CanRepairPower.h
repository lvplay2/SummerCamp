// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanRepairPower.generated.h"

/**
* @class UBTDecorator_CanRepairPower
* @brief Checks to see if we are in an area without power.
*/
UCLASS()
class SUMMERCAMP_API UBTDecorator_CanRepairPower 
: public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	// End UBTDecorator interface
};
