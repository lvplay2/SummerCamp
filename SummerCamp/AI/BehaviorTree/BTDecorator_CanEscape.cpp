// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTDecorator_CanEscape.h"

#include "SCCounselorAIController.h"
#include "SCDriveableVehicle.h"
#include "SCEscapeVolume.h"
#include "SCGameMode.h"
#include "SCVehicleSeatComponent.h"

UBTDecorator_CanEscape::UBTDecorator_CanEscape(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Can Escape");
}

bool UBTDecorator_CanEscape::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();

	if (BotController && GameMode)
	{
		// Check police escapes
		for (ASCEscapeVolume* EscapeVolume : GameMode->GetAllEscapeVolumes())
		{
			if (IsValid(EscapeVolume) && EscapeVolume->EscapeType == ESCEscapeType::Police && EscapeVolume->CanEscapeHere())
				return true;
		}

		// Check repaired vehicles
		for (ASCDriveableVehicle* Vehicle : GameMode->GetAllSpawnedVehicles())
		{
			if (IsValid(Vehicle) && Vehicle->bIsRepaired && Vehicle->bFirstStartComplete && !Vehicle->bEscaping && !Vehicle->bEscaped)
			{
				// Make sure there is a seat for us
				for (USCVehicleSeatComponent* Seat : Vehicle->Seats)
				{
					if (!Seat->CounselorInSeat && Seat->AcceptsPendingCharacterAI(BotController))
						return true;
				}
			}
		}
	}

	return false;
}

#if WITH_EDITOR
FName UBTDecorator_CanEscape::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}
#endif
