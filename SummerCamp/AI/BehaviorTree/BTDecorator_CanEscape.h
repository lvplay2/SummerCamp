// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanEscape.generated.h"

/**
 * @class UBTDecorator_CanEscape
 * @brief Checks to see if there is a valid escape option.
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_CanEscape 
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
