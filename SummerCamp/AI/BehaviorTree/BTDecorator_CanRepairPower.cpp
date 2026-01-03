// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CanRepairPower.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCPowerGridVolume.h"
#include "SCPowerPlant.h"

UBTDecorator_CanRepairPower::UBTDecorator_CanRepairPower(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Can Repair Power Gird");
}

bool UBTDecorator_CanRepairPower::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = BotController ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;
	const ASCGameState* GameState = GetWorld()->GetGameState<ASCGameState>();

	if (BotPawn && GameState)
	{
		if (USCPowerPlant* PowerPlant = GameState->GetPowerPlant())
		{
			for (ASCPowerGridVolume* PowerGrid : PowerPlant->GetPowerGrids())
			{
				if (PowerGrid->ContainsCounselor(BotPawn))
				{
					return !PowerGrid->GetIsPowered();
				}
			}
		}
	}

	return false;
}
