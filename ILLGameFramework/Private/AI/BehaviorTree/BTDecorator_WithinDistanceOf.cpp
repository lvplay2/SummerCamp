// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "BTDecorator_WithinDistanceOf.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTDecorator_WithinDistanceOf::UBTDecorator_WithinDistanceOf(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Within Distance Of");

	// Accept only actors and vectors
	BlackboardKey.AddObjectFilter(this, *NodeName, AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

bool UBTDecorator_WithinDistanceOf::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	AAIController* MyController = OwnerComp.GetAIOwner();

	if (MyController && MyBlackboard && MyController->GetPawn())
	{
		FBlackboard::FKey MyID = MyBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		TSubclassOf<UBlackboardKeyType> TargetKeyType = MyBlackboard->GetKeyType(MyID);
		bool bGotTarget = false;

		FVector TargetLocation;
		AActor* EnemyActor = NULL;
		if (TargetKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(MyID);
			EnemyActor = Cast<AActor>(KeyValue);
			if (EnemyActor)
			{
				TargetLocation = EnemyActor->GetActorLocation();
				bGotTarget = true;
			}
		}
		else if (TargetKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			TargetLocation = MyBlackboard->GetValue<UBlackboardKeyType_Vector>(MyID);
			bGotTarget = true;
		}

		if (bGotTarget)
		{
			const float CurrentDistance = (MyController->GetPawn()->GetActorLocation() - TargetLocation).SizeSquared();
			return (CurrentDistance < FMath::Square(MaxDistance));
		}
	}

	return false;
}
