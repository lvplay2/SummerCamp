// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_HasRepairPart.h"

#include "SCCharacterAIController.h"
#include "SCCounselorCharacter.h"
#include "SCInventoryComponent.h"
#include "SCRepairPart.h"

UBTDecorator_HasRepairPart::UBTDecorator_HasRepairPart(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Has Repair Part");
	bIncludeSmallItems = true;
}

bool UBTDecorator_HasRepairPart::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	ASCCharacterAIController* MyController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = MyController ? Cast<ASCCounselorCharacter>(MyController->GetPawn()) : nullptr;
	if (!BotPawn)
		return false;

	// Get a list of all of our current repair items
	if (ASCRepairPart* CurrentRepairPart = Cast<ASCRepairPart>(BotPawn->GetEquippedItem()))
		return true;

	if (bIncludeSmallItems)
	{
		for (AILLInventoryItem* Item : BotPawn->GetSmallInventory()->GetItemList())
		{
			if (Item->IsA<ASCRepairPart>())
				return true;
		}
	}

	return false;
}

#if WITH_EDITOR
FName UBTDecorator_HasRepairPart::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}
#endif	// WITH_EDITOR
