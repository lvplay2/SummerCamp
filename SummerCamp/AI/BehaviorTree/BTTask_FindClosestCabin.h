// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_FindClosest.h"
#include "BTTask_FindClosestCabin.generated.h"

/**
 * @class UBTTask_FindClosestCabin
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindClosestCabin
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

	// Maximum distance a cabin can be away from us, when >0
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaximumDistance;

	// Exclude cabins containing Jason or when Jason is within this many units of the outside, when >0
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinJasonDistance;

	// Exclude cabins we have already flagged as searched
	UPROPERTY(EditAnywhere, Category="Condition")
	bool bExcludeSearched;
};
