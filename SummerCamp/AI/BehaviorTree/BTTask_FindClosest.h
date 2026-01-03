// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindClosest.generated.h"

/**
 * @class UBTTask_FindClosest
 * @brief Base class which allows some basic features for filtering nearby actors.
 */
UCLASS(Abstract)
class SUMMERCAMP_API UBTTask_FindClosest
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

protected:
	/** @return true if the last Jason stimulus is considered valid. */
	virtual bool IsJasonStimulusValid(UBehaviorTreeComponent& OwnerComp) const;

	/** @return Search epicenter based on the OwnerComp and Jason. */
	virtual FVector CalculateEpicenter(UBehaviorTreeComponent& OwnerComp) const;

	// Project search epicenter this many units away from the last Jason stimulus
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float AwayFromJasonDistance;

	// Maximum age of stimulus from Jason to perform AwayFromJasonDistance checks
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxJasonStimulusAge;
};
