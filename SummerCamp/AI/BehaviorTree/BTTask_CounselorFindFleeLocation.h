// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_CounselorFindFleeLocation.generated.h"

/**
 * @class UBTTask_CounselorFindFleeLocation
 */
UCLASS()
class SUMMERCAMP_API UBTTask_CounselorFindFleeLocation 
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:
	// Navigation filter class
	UPROPERTY(EditAnywhere, Category="Condition")
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	// Distance to attempt to flee away
	UPROPERTY(EditAnywhere, Category="Condition")
	float FleeDistance;

	// Distance around the point FleeDistance units away that is acceptable
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float AcceptableRadius;

	// Preferred angle away from Jason to flee
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float JasonJukeHorizontalRange;

	// Fallback minimum angle away from Jason to flee when JasonJukeHorizontalRange fails an attempt
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float JasonJukeFallbackHorizontalOffset;
};
