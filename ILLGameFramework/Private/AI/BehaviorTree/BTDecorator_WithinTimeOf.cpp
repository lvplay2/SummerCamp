// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "BTDecorator_WithinTimeOf.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"

UBTDecorator_WithinTimeOf::UBTDecorator_WithinTimeOf(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Within Time Of");

	// Accept only floats
	BlackboardKey.AddFloatFilter(this, *NodeName);
}

bool UBTDecorator_WithinTimeOf::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	AAIController* MyController = OwnerComp.GetAIOwner();
	UWorld* World = MyController ? MyController->GetWorld() : nullptr;

	if (World && MyBlackboard)
	{
		FBlackboard::FKey MyID = MyBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		TSubclassOf<UBlackboardKeyType> TargetKeyType = MyBlackboard->GetKeyType(MyID);

		if (TargetKeyType == UBlackboardKeyType_Float::StaticClass())
		{
			const float KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Float>(MyID);
			if (KeyValue > 0.f) // Always return false for uninitialized values
			{
				const float CurrentTime = World->GetTimeSeconds();
				if (CurrentTime-KeyValue <= TimeLimit)
					return true;
			}
		}
	}

	return false;
}
