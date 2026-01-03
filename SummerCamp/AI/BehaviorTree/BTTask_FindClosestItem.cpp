// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindClosestItem.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCDoubleCabinet.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCInteractComponent.h"

UBTTask_FindClosestItem::UBTTask_FindClosestItem(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Item");

	DistanceLimit = 3000.f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindClosestItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Check if we should fail for already having an equipped item
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
	if (bFailWhenItemEquipped && BotPawn->GetEquippedItem())
		return EBTNodeResult::Failed;

	UWorld* World = GetWorld();
	ASCGameState* GameState = World->GetGameState<ASCGameState>();
	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	if (!GameState || !GameMode)
		return EBTNodeResult::Failed;

	const bool bCheckDistanceToJason = (MinJasonDistance > 0.f && IsJasonStimulusValid(OwnerComp));
	const FVector Epicenter = CalculateEpicenter(OwnerComp);
	const ASCItem* EquippedItem = BotPawn->GetEquippedItem();
	const float MaxSearchDistanceSq = FMath::Square(DistanceLimit);
	const ESCGameModeDifficulty GameDifficulty = GameMode->GetModeDifficulty();

	// Find the closest item
	FVector ClosestItemLocation(FNavigationSystem::InvalidLocation);
	ASCItem* ClosestItemActor = nullptr;
	USCInteractComponent* ClosestItemInteractable = nullptr;
	for (ASCItem* Item : GameState->ItemList)
	{
		if (!IsValid(Item))
			continue;

		// Someone already has picked up this item
		if (Item->GetCharacterOwner())
			continue;

		// Check if the Item is even desired
		if (!BotController->DesiresItemPickup(Item))
			continue;

		// Compare against our current candidate
		if (ClosestItemActor && !BotController->CompareItems(ClosestItemActor, Item))
			continue;

		// Avoid items near Jason if desired
		if (bCheckDistanceToJason && (Item->GetActorLocation() - BotController->LastKillerStimulusLocation).SizeSquared() <= FMath::Square(MinJasonDistance))
			continue;

		// Verify it's allowed
		if (AllowedItemClasses.Num() > 0 && !AllowedItemClasses.ContainsByPredicate([&](TSubclassOf<ASCItem> AllowedClass) { return Item->IsA(AllowedClass); }))
			continue;

		// Skip cabinet items, UBTTask_FindSearchableCabinet takes care of those
		// Since Grendel has cabinets out in the open, we need to search all items
		/*if (Item->IsInCabinet())
			continue;*/

		// Get the interact component we want to interact with
		USCInteractComponent* Iteractable = Item->IsInCabinet() ? Item->CurrentCabinet->GetInteractComponent() : Item->GetInteractComponent();

		// Skip items other AI are looking at
		const bool bCompareDistance = GameDifficulty == ESCGameModeDifficulty::Hard ? true : GameDifficulty == ESCGameModeDifficulty::Normal ? FMath::RandBool() : false;
		if (!Iteractable->AcceptsPendingCharacterAI(BotController, bCompareDistance))
			continue;

		// Check distance
		const float DistanceToPartSq = (Epicenter - Iteractable->GetComponentLocation()).SizeSquared();
		if (DistanceToPartSq <= MaxSearchDistanceSq || GameState->ShouldAIIgnoreSearchDistances())
		{
			// Check navigation
			FVector InteractionLocation;
			if (Iteractable->BuildAIInteractionLocation(BotController, InteractionLocation))
			{
				ClosestItemLocation = InteractionLocation;
				ClosestItemActor = Item;
				ClosestItemInteractable = Iteractable;
			}
		}
	}

	// Set the blackboard key with the location of the part, and assign our desired interactable
	if (FNavigationSystem::IsValidLocation(ClosestItemLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ClosestItemLocation);
			BotController->SetDesiredInteractable(ClosestItemInteractable, false, true);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
