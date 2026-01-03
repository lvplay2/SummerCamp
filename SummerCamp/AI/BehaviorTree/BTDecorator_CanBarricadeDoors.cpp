// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CanBarricadeDoors.h"

#include "SCCabin.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCDoor.h"
#include "SCInteractComponent.h"


UBTDecorator_CanBarricadeDoors::UBTDecorator_CanBarricadeDoors(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Can Barricade Doors");
}

bool UBTDecorator_CanBarricadeDoors::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = BotController ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;

	if (BotPawn && !BotPawn->IsInWheelchair())
	{
		return true;
	}

	return false;
}

#if WITH_EDITOR
FName UBTDecorator_CanBarricadeDoors::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}
#endif
