// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_FindClosest.h"
#include "BTTask_FindClosestHidingSpot.generated.h"

/**
 * @class UBTTask_FindClosestHidingSpot
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindClosestHidingSpot
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

	// Maximum number of people that can be hiding in one building
	UPROPERTY(EditAnywhere, Category="Condition")
	int32 MaxHidingCounselorsPerCabin;

	// Exclude cabins containing Jason or when Jason is within this many units of the outside, when >0
	UPROPERTY(EditAnywhere, Category = "Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinJasonDistance;
};
