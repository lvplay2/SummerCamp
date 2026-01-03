// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLAIController.h"
#include "BTTask_SwitchBehaviorTree.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_SwitchBehaviorTree::UBTTask_SwitchBehaviorTree(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Switch BehaviorTree");
}

EBTNodeResult::Type UBTTask_SwitchBehaviorTree::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AILLAIController* Controller = Cast<AILLAIController>(OwnerComp.GetAIOwner());

	if (Controller)
	{
		Controller->ChangeBehaviorTree(BehaviorTree);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
