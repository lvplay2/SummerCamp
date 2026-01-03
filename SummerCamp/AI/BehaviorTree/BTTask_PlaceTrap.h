// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PlaceTrap.generated.h"

class ASCCharacter;

/**
* @class UBTTask_PlaceTrap
* @brief Finds an area around a door to place a trap
*/
UCLASS()
class SUMMERCAMP_API UBTTask_PlaceTrap
: public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:

	/** Called when the animation has finished playing */
	void OnPlaceTrapTimerDone();

	// Timer info for when the animation is complete
	FTimerDelegate TimerDelegate_PlaceTrap;
	FTimerHandle TimerHandle_PlaceTrap;

	// Keep track of who is executing this task
	UPROPERTY()
	UBehaviorTreeComponent* MyOwnerComp;

	// Keep track of our character
	UPROPERTY()
	ASCCharacter* MyCharacter;
};