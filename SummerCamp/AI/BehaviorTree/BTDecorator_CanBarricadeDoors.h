// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanBarricadeDoors.generated.h"

/**
 * @class UBTDecorator_CanBarricadeDoors
 * @brief Checks to see if there are barricadeable doors in this counselor's current cabin.
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_CanBarricadeDoors 
: public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif
	// End UBTDecorator interface
};
