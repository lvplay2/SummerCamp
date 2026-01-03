// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindTrapPlaceArea.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCabin.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCDoor.h"
#include "SCIndoorComponent.h"
#include "SCInteractComponent.h"

UBTTask_FindTrapPlaceArea::UBTTask_FindTrapPlaceArea(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Trap Placement Area");

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindTrapPlaceArea::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn)
		return EBTNodeResult::Failed;

	// Check requirements
	if (BotPawn->IsInWheelchair())
		return EBTNodeResult::Failed;
	if (!BotPawn->IsIndoors())
		return EBTNodeResult::Failed;

	FVector DoorLocation(FNavigationSystem::InvalidLocation);
	float ClosestDoorDistSq = FLT_MAX;

	// Find all locked doors in the cabin we are in
	for (const USCIndoorComponent* IndoorComponent : BotPawn->OverlappingIndoorComponents)
	{
		if (ASCCabin* Cabin = Cast<ASCCabin>(IndoorComponent->GetOwner()))
		{
			for (AActor* Child : Cabin->GetChildren())
			{
				ASCDoor* Door = Cast<ASCDoor>(Child);
				if (!Door || !Door->IsLockedOrBarricaded() || Door->IsDestroyed())
					continue;

				FVector InteractLocation;
				if (Door->InteractComponent->BuildAIInteractionLocation(BotController, InteractLocation))
				{
					const float DistanceToDoorSq = (InteractLocation - BotPawn->GetActorLocation()).SizeSquared();
					if (DistanceToDoorSq < ClosestDoorDistSq)
					{
						ClosestDoorDistSq = DistanceToDoorSq;
						DoorLocation = InteractLocation;
					}
				}
			}
		}
	}

	// Set the blackboard key with the location of the door we want to place a trap in front of
	if (FNavigationSystem::IsValidLocation(DoorLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, DoorLocation);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
