// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "BTTask_MoveToExtended.h"
#include "BTTask_MoveToVehicle.generated.h"

UCLASS(Config=Game)
class SUMMERCAMP_API UBTTask_MoveToVehicle
: public UBTTask_MoveToExtended
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	// End UBTTaskNode interface
};
