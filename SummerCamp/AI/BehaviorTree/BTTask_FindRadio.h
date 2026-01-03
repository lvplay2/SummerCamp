// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindRadio.generated.h"

/**
 * @class UBTTask_FindRadio
 */
UCLASS()
class SUMMERCAMP_API UBTTask_FindRadio 
: public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()
	
	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface
};
