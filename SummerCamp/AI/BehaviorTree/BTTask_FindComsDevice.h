// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindComsDevice.generated.h"

/**
 * @class UBTTask_FindComsDevice
 * @brief Finds the closest Hunter Radio or Police phone within the specfied distance limit
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindComsDevice : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

	// How far away a communication device can be to be considered
	UPROPERTY(EditAnywhere)
	float DistanceLimit;
};
