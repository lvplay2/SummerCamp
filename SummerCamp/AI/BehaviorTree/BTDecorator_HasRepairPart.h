// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasRepairPart.generated.h"

/**
 * @class UBTDecorator_HasRepairPart
 */
UCLASS()
class SUMMERCAMP_API UBTDecorator_HasRepairPart
: public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	// Begin UBTDecorator interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif
	// End UBTDecorator interface

	// Include the small item inventory repair parts?
	UPROPERTY(EditAnywhere)
	bool bIncludeSmallItems;
};
