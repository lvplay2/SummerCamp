// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindTrapPlaceArea.generated.h"

/**
* @class UBTTask_FindTrapPlaceArea
* @brief Finds an area around a door to place a trap
*/
UCLASS()
class SUMMERCAMP_API UBTTask_FindTrapPlaceArea
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface
};
