// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindClosestHidingSpot.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCCabin.h"
#include "SCCharacter.h"
#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCHidingSpot.h"
#include "SCIndoorComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"

UBTTask_FindClosestHidingSpot::UBTTask_FindClosestHidingSpot(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Hiding Spot");

	AwayFromJasonDistance = 200.f;
	MaxHidingCounselorsPerCabin = 2;
	MinJasonDistance = 1400.f;

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindClosestHidingSpot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn)
		return EBTNodeResult::Failed;

	// Clear interaction hack done in all of these...
	BotController->SetDesiredInteractable(nullptr);

	// Check requirements
	if (BotPawn->IsInWheelchair())
		return EBTNodeResult::Failed;

	UWorld* World = GetWorld();
	const ASCGameState* GameState = World->GetGameState<ASCGameState>();
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	if (!GameState || !GameMode)
		return EBTNodeResult::Failed;

	// Make sure we aren't already in a hide spot
	if (const ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(BotPawn))
	{
		if (Counselor->GetCurrentHidingSpot() != nullptr)
			return EBTNodeResult::Failed;
	}

	const FVector Epicenter = CalculateEpicenter(OwnerComp);
	const bool bCheckDistanceToJason = (MinJasonDistance > 0.f && IsJasonStimulusValid(OwnerComp));

	// Find the closest hiding spot
	FVector ClosestHidingSpotLocation(FNavigationSystem::InvalidLocation);
	float ClosestDistanceSq = FLT_MAX;
	USCInteractComponent* DesiredHidingSpot = nullptr;
	TMap<ASCCabin*, int> CabinHidingCount;
	for (ASCHidingSpot* HidingSpot : GameMode->GetAllHidingSpots())
	{
		if (!IsValid(HidingSpot))
			continue;

		if (HidingSpot->IsDestroyed())
			continue;
		if (ASCCounselorCharacter* HidingCounselor = HidingSpot->GetHidingCounselor())
		{
			if (HidingCounselor->OverlappingIndoorComponents.Num() > 0)
			{
				// Use the first indoor component as the cabin they are in
				if (ASCCabin* Cabin = Cast<ASCCabin>(HidingCounselor->OverlappingIndoorComponents[0]->GetOwner()))
				{
					// Keep track of how many people are hiding in this cabin
					if (CabinHidingCount.Contains(Cabin))
						CabinHidingCount[Cabin]++;
					else
						CabinHidingCount.Add(Cabin, 1);
				}
			}

			continue;
		}
		if (!HidingSpot->GetInteractComponent()->AcceptsPendingCharacterAI(BotController))
			continue;

		ASCCabin* OwningCabin = Cast<ASCCabin>(HidingSpot->GetOwner());
		if (OwningCabin)
		{
			// Make sure there aren't too many counselors hiding in this cabin
			if (CabinHidingCount.FindRef(OwningCabin) >= MaxHidingCounselorsPerCabin)
				continue;
		}

		// Avoid Jason if desired
		ASCCounselorAIController* CounselorController = Cast<ASCCounselorAIController>(BotController);
		if (bCheckDistanceToJason && CounselorController)
		{
			const FVector JasonLocation = CounselorController->LastKillerStimulusLocation;
			const FBox HideSpotBox = HidingSpot->GetComponentsBoundingBox();
			const FVector ClosestPointToJason = HideSpotBox.GetClosestPointTo(JasonLocation);
			const float CabinToJasonDistSq = (ClosestPointToJason - JasonLocation).SizeSquared();
			if (CabinToJasonDistSq <= FMath::Square(MinJasonDistance))
				continue;

			if (OwningCabin && GameState->CurrentKiller && GameState->CurrentKiller->IsInsideCabin(OwningCabin))
				continue;				
		}

		// Check distance
		const float DistanceToHidingSpotSq = (Epicenter - HidingSpot->GetInteractComponent()->GetComponentLocation()).SizeSquared();
		if (DistanceToHidingSpotSq < ClosestDistanceSq)
		{
			// Check navigation
			FVector InteractionLocation;
			if (HidingSpot->GetInteractComponent()->BuildAIInteractionLocation(BotController, InteractionLocation))
			{
				ClosestDistanceSq = DistanceToHidingSpotSq;
				ClosestHidingSpotLocation = InteractionLocation;
				DesiredHidingSpot = HidingSpot->GetInteractComponent();
			}
		}
	}

	// Set the blackboard key with the location of the hiding spot, and assign this our desired interactable
	if (FNavigationSystem::IsValidLocation(ClosestHidingSpotLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ClosestHidingSpotLocation);
			BotController->SetDesiredInteractable(DesiredHidingSpot);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
