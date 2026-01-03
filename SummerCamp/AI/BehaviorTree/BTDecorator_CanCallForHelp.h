// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanCallForHelp.generated.h"

/**
 * @class UBTDecorator_CanCallForHelp
 * @brief Checks to see if the police and hunter have been called.
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_CanCallForHelp
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
