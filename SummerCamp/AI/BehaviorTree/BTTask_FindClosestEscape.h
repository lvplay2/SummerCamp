// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_FindClosest.h"
#include "BTTask_FindClosestEscape.generated.h"

/**
 * @class UBTTask_FindClosestEscape
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindClosestEscape
: public UBTTask_FindClosest
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:
	// Navigation filter class
	UPROPERTY(EditAnywhere, Category="Condition")
	TSubclassOf<UNavigationQueryFilter> FilterClass;
};
