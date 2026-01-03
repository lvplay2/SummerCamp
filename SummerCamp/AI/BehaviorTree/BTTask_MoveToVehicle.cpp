// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_MoveToVehicle.h"

#include "AIController.h"

#include "SCDriveableVehicle.h"
#include "SCWheeledVehicleMovementComponent.h"

UBTTask_MoveToVehicle::UBTTask_MoveToVehicle(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Move To Vehicle");
}

void UBTTask_MoveToVehicle::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// Kill Vehicle inputs if the move to failed or was aborted
	if (TaskResult == EBTNodeResult::Failed || TaskResult == EBTNodeResult::Aborted)
	{
		AAIController* BotController = OwnerComp.GetAIOwner();
		ASCDriveableVehicle* BotVehicle = BotController ? Cast<ASCDriveableVehicle>(BotController->GetPawn()) : nullptr;
		if (BotVehicle)
		{
			if (USCWheeledVehicleMovementComponent* VM = Cast<USCWheeledVehicleMovementComponent>(BotVehicle->GetVehicleMovement()))
				VM->FullStop();
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
