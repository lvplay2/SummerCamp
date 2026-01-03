// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CanCallForHelp.h"

#include "SCCBRadio.h"
#include "SCGameMode.h"
#include "SCPhoneJunctionBox.h"

UBTDecorator_CanCallForHelp::UBTDecorator_CanCallForHelp(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Can Call For Help");
}

bool UBTDecorator_CanCallForHelp::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UWorld* World = GetWorld();
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();

	// Police phone
	for (const ASCPhoneJunctionBox* PhoneBox : GameMode->GetAllPhoneJunctionBoxes())
	{
		if (IsValid(PhoneBox) && !PhoneBox->IsPoliceCalled() && PhoneBox->HasFuse())
		{
			return true;
		}
	}

	// CB Radio
	for (const ASCCBRadio* CBRadio : GameMode->GetAllCBRadios())
	{
		if (IsValid(CBRadio) && !CBRadio->IsHunterCalled())
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
FName UBTDecorator_CanCallForHelp::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}
#endif
