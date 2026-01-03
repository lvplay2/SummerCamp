// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CanWander.h"

#include "SCCharacter.h"
#include "SCCharacterAIController.h"

UBTDecorator_CanWander::UBTDecorator_CanWander(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Can Wander");

	MinimumTimeWithoutVelocity = 5.f;
}

bool UBTDecorator_CanWander::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// Only wander after being idle a few seconds
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	if (BotController && BotController->TimeWithoutVelocity > MinimumTimeWithoutVelocity)
	{
		return true;
	}

	return false;
}

#if WITH_EDITOR
FName UBTDecorator_CanWander::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}
#endif
