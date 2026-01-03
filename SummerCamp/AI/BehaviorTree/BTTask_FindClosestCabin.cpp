// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindClosestCabin.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCabin.h"
#include "SCCharacter.h"
#include "SCCounselorAIController.h"
#include "SCDoor.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCWindow.h"

UBTTask_FindClosestCabin::UBTTask_FindClosestCabin(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Cabin");

	AwayFromJasonDistance = 1000.f;
	MinJasonDistance = 1400.f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindClosestCabin::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn)
		return EBTNodeResult::Failed;

	// Clear interaction hack done in all of these...
	BotController->SetDesiredInteractable(nullptr);

	UWorld* World = GetWorld();
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	const ASCGameState* GameState = World->GetGameState<ASCGameState>();
	if (!GameMode || !GameState)
		return EBTNodeResult::Failed;

	ASCCounselorAIController* CounselorController = Cast<ASCCounselorAIController>(BotController);
	const bool bCheckDistanceToJason = (MinJasonDistance > 0.f && IsJasonStimulusValid(OwnerComp));
	const FVector Epicenter = CalculateEpicenter(OwnerComp);

	// Check if we have searched everything
	if (bExcludeSearched && CounselorController)
	{
		int32 NumCabins = 0;
		int32 SearchedCabins = 0;
		for (ASCCabin* Cabin : GameMode->GetAllCabins())
		{
			if (IsValid(Cabin))
			{
				++NumCabins;
				if (CounselorController->HasSearchedCabin(Cabin))
					++SearchedCabins;
			}
		}

		if (NumCabins > 0 && SearchedCabins >= NumCabins)
		{
			// Clear the searched cabin list when we have searched them all, so it can start over
			// Otherwise you end up counselors just wandering about a random point in the map with nothing to do
			CounselorController->ClearSearchedCabinets();
			CounselorController->ClearSearchedCabins();
		}
	}

	// Find the closest cabin to this bot
	ASCCabin* ClosestCabinActor = nullptr;
	float ClosestDistanceSq = (MaximumDistance > 0.f) ? FMath::Square(MaximumDistance) : FLT_MAX;
	for (ASCCabin* Cabin : GameMode->GetAllCabins())
	{
		if (!IsValid(Cabin))
			continue;

		// Skip Jason shacks
		if (Cabin->IsJasonShack())
			continue;

		// Optionally skip already-searched cabins
		if (bExcludeSearched && CounselorController && CounselorController->HasSearchedCabin(Cabin))
			continue;

		// Check our distance by finding the closest point on the bounds to us
		const FBox CabinBox = Cabin->GetComponentsBoundingBox();
		const FVector ClosestPointToOwner = CabinBox.GetClosestPointTo(Epicenter);
		const float DistanceToCabinSq = (Epicenter - ClosestPointToOwner).SizeSquared();
		if (DistanceToCabinSq >= ClosestDistanceSq)
			continue;

		// Avoid Jason if desired
		if (bCheckDistanceToJason && CounselorController)
		{
			const FVector JasonLocation = CounselorController->LastKillerStimulusLocation;
			const FVector ClosestPointToJason = CabinBox.GetClosestPointTo(JasonLocation);
			const float CabinToJasonDistSq = (ClosestPointToJason - JasonLocation).SizeSquared();
			if (CabinToJasonDistSq <= FMath::Square(MinJasonDistance))
				continue;
		}

		// Avoid cabins that Jason is in
		if (GameState->CurrentKiller && GameState->CurrentKiller->IsInsideCabin(Cabin))
			continue;

		ClosestCabinActor = Cabin;
		ClosestDistanceSq = DistanceToCabinSq;
	}

	// Find closest window/door
	FVector ClosestInsideEntryLocation(FNavigationSystem::InvalidLocation);
	USCInteractComponent* CloestEntryInteractable = nullptr;
	if (ClosestCabinActor)
	{
		// Look at all of the child doors/windows
		float ClosestEntryDistanceSq = FLT_MAX;
		AActor* ClosestEntryActor = nullptr;
		for (AActor* CabinChild : ClosestCabinActor->GetChildren())
		{
			ASCDoor* DoorChild = Cast<ASCDoor>(CabinChild);
			if (DoorChild && !DoorChild->InteractComponent->AcceptsPendingCharacterAI(BotController))
				continue;

			ASCWindow* WindowChild = Cast<ASCWindow>(CabinChild);
			if (WindowChild && !WindowChild->InteractComponent->AcceptsPendingCharacterAI(BotController))
				continue;

			if (DoorChild || WindowChild)
			{
				const float DistanceSq = (Epicenter - CabinChild->GetActorLocation()).SizeSquared();
				if (DistanceSq < ClosestEntryDistanceSq)
				{
					ClosestEntryDistanceSq = DistanceSq;
					ClosestEntryActor = CabinChild;
					if (DoorChild)
						CloestEntryInteractable = DoorChild->InteractComponent;
					else
						CloestEntryInteractable = WindowChild->InteractComponent;
				}
			}
		}

		if (ClosestEntryActor && CloestEntryInteractable)
		{
			// Move towards the indoors side of the ClosestEntryActor a bit
			const FVector EntryLocation = ClosestEntryActor->GetActorLocation();
			const FBox CabinBox = ClosestCabinActor->GetComponentsBoundingBox();
			FVector CabinCenterEntryZ = CabinBox.GetCenter();
			CabinCenterEntryZ.Z = EntryLocation.Z;
			const FVector EntryToCabinCenter = (CabinCenterEntryZ - EntryLocation).GetSafeNormal();
			ClosestInsideEntryLocation = EntryLocation + EntryToCabinCenter*CloestEntryInteractable->DistanceLimit;
		}
	}

	// Set the blackboard key with the location of the cabin
	if (FNavigationSystem::IsValidLocation(ClosestInsideEntryLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ClosestInsideEntryLocation);
			BotController->SetDesiredInteractable(CloestEntryInteractable);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
