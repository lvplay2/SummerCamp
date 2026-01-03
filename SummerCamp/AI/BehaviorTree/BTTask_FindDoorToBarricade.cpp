// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindDoorToBarricade.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCabin.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCDoor.h"
#include "SCIndoorComponent.h"
#include "SCInteractComponent.h"

UBTTask_FindDoorToBarricade::UBTTask_FindDoorToBarricade(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Door to Barricade");

	MinimumDistance = 200.f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindDoorToBarricade::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(OwnerComp.GetAIOwner());
	ASCCounselorCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCounselorCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn)
		return EBTNodeResult::Failed;

	// Clear interaction hack done in all of these...
	BotController->SetDesiredInteractable(nullptr);

	// Check requirements
	if (BotPawn->IsInWheelchair())
		return EBTNodeResult::Failed;
	if (!BotPawn->IsIndoors())
		return EBTNodeResult::Failed;

	FVector DoorLocation(FNavigationSystem::InvalidLocation);
	float ClosestDoorDistSq = FLT_MAX;
	USCInteractComponent* DesiredDoorInteractable = nullptr;

	// Find all lockable doors in the cabin we are in
	for (const USCIndoorComponent* IndoorComponent : BotPawn->OverlappingIndoorComponents)
	{
		if (ASCCabin* Cabin = Cast<ASCCabin>(IndoorComponent->GetOwner()))
		{
			for (AActor* Child : Cabin->GetChildren())
			{
				ASCDoor* Door = Cast<ASCDoor>(Child);
				if (!Door || Door->IsLockedOrBarricaded() || !Door->HasBarricade() || Door->IsDestroyed())
					continue;

				if (Door->InteractComponent->AcceptsPendingCharacterAI(BotController))
				{
					FVector InteractLocation;
					if (Door->InteractComponent->BuildAIInteractionLocation(BotController, InteractLocation))
					{
						const float DistanceToDoorSq = (InteractLocation - BotPawn->GetActorLocation()).SizeSquared();
						if (DistanceToDoorSq < ClosestDoorDistSq && DistanceToDoorSq >= FMath::Square(MinimumDistance))
						{
							ClosestDoorDistSq = DistanceToDoorSq;
							DoorLocation = InteractLocation;
							DesiredDoorInteractable = Door->InteractComponent;
						}
					}
				}
			}
		}
	}

	// Set the blackboard key with the location of the door to barricade, and assign our desired interactable
	if (FNavigationSystem::IsValidLocation(DoorLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, DoorLocation);
			BotController->SetDesiredInteractable(DesiredDoorInteractable, true);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
