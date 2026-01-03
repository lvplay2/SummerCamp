// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindPowerGridRepair.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCFuseBox.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCPowerGridVolume.h"
#include "SCPowerPlant.h"


UBTTask_FindPowerGridRepair::UBTTask_FindPowerGridRepair(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Power Generator Repair Location");

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindPowerGridRepair::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;
	const ASCGameState* GameState = GetWorld()->GetGameState<ASCGameState>();
	const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
	const ESCGameModeDifficulty GameDifficulty = GameMode ? GameMode->GetModeDifficulty() : ESCGameModeDifficulty::Normal;
	const bool bCompareDistance = GameDifficulty == ESCGameModeDifficulty::Hard ? true : GameDifficulty == ESCGameModeDifficulty::Normal ? FMath::RandBool() : false;
	
	if (BotPawn && GameState)
	{
		BotController->SetDesiredInteractable(nullptr);

		if (USCPowerPlant* PowerPlant = GameState->GetPowerPlant())
		{
			for (ASCPowerGridVolume* PowerGrid : PowerPlant->GetPowerGrids())
			{
				if (PowerGrid->ContainsCounselor(BotPawn))
				{
					if (!PowerGrid->GetIsPowered())
					{
						const ASCFuseBox* FuseBox = PowerGrid->GetFuseBox();
						if (FuseBox && FuseBox->InteractComponent && FuseBox->InteractComponent->AcceptsPendingCharacterAI(BotController, bCompareDistance))
						{
							FVector InteractLocation;
							if (FuseBox->InteractComponent->BuildAIInteractionLocation(BotController, InteractLocation))
							{
								const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
								if (BlackboardKeyId != FBlackboard::InvalidKey)
								{
									BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, InteractLocation);
									BotController->SetDesiredInteractable(FuseBox->InteractComponent, false, true);
									return EBTNodeResult::Succeeded;
								}
							}
						}
					}

					break;
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}
