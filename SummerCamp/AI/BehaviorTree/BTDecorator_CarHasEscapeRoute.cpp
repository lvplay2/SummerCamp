// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CarHasEscapeRoute.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "Components/SplineComponent.h"

UBTDecorator_CarHasEscapeRoute::UBTDecorator_CarHasEscapeRoute(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Car Has Escape Route");

	// Accept only Spline Components
	CarEscapeRouteKey.AddObjectFilter(this, *NodeName, USplineComponent::StaticClass());
}

bool UBTDecorator_CarHasEscapeRoute::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// If we already built a path, fuck off
	if (UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent())
	{
		const FBlackboard::FKey CarEscapeRouteKeyId = BotBlackboard->GetKeyID(CarEscapeRouteKey.SelectedKeyName);
		if (CarEscapeRouteKeyId != FBlackboard::InvalidKey)
		{
			return Cast<USplineComponent>(BotBlackboard->GetValue<UBlackboardKeyType_Object>(CarEscapeRouteKeyId)) != nullptr;
		}
	}

	return false;
}
