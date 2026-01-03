// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_FindClosest.h"
#include "BTTask_FindClosestItem.generated.h"

class ASCItem;

/**
 * @class UBTTask_FindClosestItem
 * @brief Finds the closest item or repair part (car/boat parts or phone box fuse) within the specified distance to the Counselor AI.
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindClosestItem 
: public UBTTask_FindClosest
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

	// How far away an item can be to be considered
	UPROPERTY(EditAnywhere, Category="Condition")
	TArray<TSubclassOf<ASCItem>> AllowedItemClasses;

	// How far away an item can be to be considered
	UPROPERTY(EditAnywhere, Category="Condition")
	float DistanceLimit;

	// Exclude items when Jason is within this many units of them, when >0
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinJasonDistance;

	// Fail if we already have an equipped item?
	UPROPERTY(EditAnywhere, Category="Condition")
	bool bFailWhenItemEquipped;
};
