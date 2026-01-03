// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindSearchableCabinet.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCabin.h"
#include "SCDoubleCabinet.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCIndoorComponent.h"

UBTTask_FindSearchableCabinet::UBTTask_FindSearchableCabinet(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Searchable Cabinet");

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindSearchableCabinet::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// Find all searchable cabinets in the cabin we are in
	TMap<USCInteractComponent*, FVector> SearchableCabinets;
	FVector DrawerLocation(FNavigationSystem::InvalidLocation);
	USCInteractComponent* DesiredCabinetInteractable = nullptr;
	for (const USCIndoorComponent* IndoorComponent : BotPawn->OverlappingIndoorComponents)
	{
		if (ASCCabin* Cabin = Cast<ASCCabin>(IndoorComponent->GetOwner()))
		{
			bool bHasSearchableCabinets = false;
			for (AActor* Child : Cabin->GetChildren())
			{
				ASCCabinet* Cabinet = Cast<ASCCabinet>(Child);
				if (!Cabinet || BotController->HasSearchedCabinet(Cabinet))
					continue;

				auto EvaluateCabinetDrawer = [&](USCInteractComponent* InteractComponent, const bool bFullyOpened) -> void
				{
					if (bFullyOpened)
						return;
					if (BotController->HasSearchedCabinet(InteractComponent))
						return;

					// Track as searchable even if AcceptsPendingCharacterAI fails below, since that may block temporarily and we don't want the entire cabin ignored because of that
					bHasSearchableCabinets = true;

					// Cooldown test
					if (InteractComponent->AcceptsPendingCharacterAI(BotController))
					{
						// Navigation test
						FVector InteractionLocation;
						if (InteractComponent->BuildAIInteractionLocation(BotController, InteractionLocation))
						{
							SearchableCabinets.Add(InteractComponent, InteractionLocation);
						}
					}
				};

				EvaluateCabinetDrawer(Cabinet->GetInteractComponent(), Cabinet->IsDrawerPhysicallyOpened());

				if (ASCDoubleCabinet* DoubleCabinet = Cast<ASCDoubleCabinet>(Cabinet))
					EvaluateCabinetDrawer(DoubleCabinet->GetInteractComponent2(), DoubleCabinet->IsDrawer2PhysicallyOpened());
			}

			// No cabinets to search? Flag the cabin as searched
			if (!bHasSearchableCabinets)
				BotController->MarkCabinSearched(Cabin);
		}
	}

	// Find the closest searchable cabinet
	float ClosestCabinetDistance = FLT_MAX;
	for (TPair<USCInteractComponent*, FVector> Cabinet : SearchableCabinets)
	{
		const float DistanceToCabinet = (BotPawn->GetActorLocation() - Cabinet.Value).SizeSquared();
		if (DistanceToCabinet < ClosestCabinetDistance)
		{
			ClosestCabinetDistance = DistanceToCabinet;
			DrawerLocation = Cabinet.Value;
			DesiredCabinetInteractable = Cabinet.Key;
		}
	}

	// Set the blackboard key with the location of the drawer, and assign this our desired interactable
	if (FNavigationSystem::IsValidLocation(DrawerLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, DrawerLocation);
			BotController->SetDesiredInteractable(DesiredCabinetInteractable);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
