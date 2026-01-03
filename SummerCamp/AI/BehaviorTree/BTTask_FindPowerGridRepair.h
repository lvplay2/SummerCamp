// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPowerGridRepair.generated.h"

/**
 * @class UBTTask_FindPowerGridRepair
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindPowerGridRepair 
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface
};
