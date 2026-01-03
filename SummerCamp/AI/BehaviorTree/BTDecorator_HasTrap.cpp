// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_HasTrap.h"

#include "SCCharacterAIController.h"
#include "SCCounselorCharacter.h"
#include "SCTrap.h"

UBTDecorator_HasTrap::UBTDecorator_HasTrap(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Has Trap");
}

bool UBTDecorator_HasTrap::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	ASCCharacterAIController* MyController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = MyController ? Cast<ASCCounselorCharacter>(MyController->GetPawn()) : nullptr;
	if (!BotPawn)
		return false;

	// See if we have a trap
	if (ASCTrap* CurrentTrap = Cast<ASCTrap>(BotPawn->GetEquippedItem()))
		return true;

	return false;
}

#if WITH_EDITOR
FName UBTDecorator_HasTrap::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}
#endif	// WITH_EDITOR
