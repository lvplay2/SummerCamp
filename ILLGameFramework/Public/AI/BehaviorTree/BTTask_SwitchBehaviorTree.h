// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SwitchBehaviorTree.generated.h"

/**
 * @class UBTTask_SwitchBehaviorTree
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API UBTTask_SwitchBehaviorTree
: public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	// Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// End UBTTaskNode interface

protected:
	// BehaviorTree to switch the AIController into using
	UPROPERTY(EditAnywhere, Category="Blackboard")
	UBehaviorTree* BehaviorTree;
};
