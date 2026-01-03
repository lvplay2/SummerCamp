// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "BTTask_FindClosestEscape.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

#include "SCBridge.h"
#include "SCBridgeButton.h"
#include "SCCharacter.h"
#include "SCCharacterAIController.h"
#include "SCCharacterAIProperties.h"
#include "SCDriveableVehicle.h"
#include "SCEscapePod.h"
#include "SCEscapeVolume.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"
#include "SCVehicleSeatComponent.h"

UBTTask_FindClosestEscape::UBTTask_FindClosestEscape(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Closest Escape");

	// Accept only vectors
	BlackboardKey.AddVectorFilter(this, *NodeName);
}

EBTNodeResult::Type UBTTask_FindClosestEscape::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BotBlackboard = OwnerComp.GetBlackboardComponent();
	ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(OwnerComp.GetAIOwner());
	ASCCharacter* BotPawn = (BotBlackboard && BotController) ? Cast<ASCCharacter>(BotController->GetPawn()) : nullptr;
	if (!BotPawn || !BotPawn->CanInteractAtAll())
		return EBTNodeResult::Failed;

	// Clear interaction hack done in all of these...
	BotController->SetDesiredInteractable(nullptr);

	UWorld* World = GetWorld();
	const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	const ASCGameState* GameState = World->GetGameState<ASCGameState>();
	if (!GameMode || !GameState)
		return EBTNodeResult::Failed;

	const FVector Epicenter = CalculateEpicenter(OwnerComp);

	// Find the closest escape or repaired vehicle
	FVector ClosestEscapeLocation(FNavigationSystem::InvalidLocation);
	float ClosestEscapeDistanceSq = FLT_MAX;
	USCInteractComponent* DesiredEscapeInteractable = nullptr;
	const ESCGameModeDifficulty GameDifficulty = GameMode->GetModeDifficulty();
	const bool bCompareDistance = GameDifficulty == ESCGameModeDifficulty::Hard ? true : GameDifficulty == ESCGameModeDifficulty::Normal ? FMath::RandBool() : false;

	// Check police escapes
	for (ASCEscapeVolume* EscapeVolume : GameMode->GetAllEscapeVolumes())
	{
		if (IsValid(EscapeVolume) && EscapeVolume->EscapeType == ESCEscapeType::Police && EscapeVolume->CanEscapeHere())
		{
			const FVector EscapeLocation = EscapeVolume->GetActorLocation();
			const float DistanceToEscapeSq = (Epicenter - EscapeLocation).SizeSquared();
			if (DistanceToEscapeSq < ClosestEscapeDistanceSq)
			{
				ClosestEscapeDistanceSq = DistanceToEscapeSq;
				ClosestEscapeLocation = EscapeLocation;
			}
		}
	}

	// Check for repaired vehicles
	if (!BotPawn->GetVehicle())
	{
		const USCCounselorAIProperties* const BotProperties = BotController->GetBotPropertiesInstance<USCCounselorAIProperties>();
		const ASCKillerCharacter* Killer = GameState->CurrentKiller;
		const float JasonThreatDistanceSq = BotProperties ? FMath::Square(BotProperties->JasonThreatDistance) : 0.f;

		for (ASCDriveableVehicle* Vehicle : GameMode->GetAllSpawnedVehicles())
		{
			if (IsValid(Vehicle) && Vehicle->bIsRepaired && (Vehicle->bFirstStartComplete || Vehicle->WasForceRepaired()) && !Vehicle->bEscaping && !Vehicle->bEscaped)
			{
				// Make sure Jason isn't close to the vehicle
				const float JasonDistanceToVehicleSq = Vehicle->GetSquaredDistanceTo(Killer);
				if (!BotController->bAIIgnorePlayers && Killer && JasonDistanceToVehicleSq <= JasonThreatDistanceSq)
					continue;

				// Find a seat
				for (USCVehicleSeatComponent* Seat : Vehicle->Seats)
				{
					if (IsValid(Seat) && !Seat->CounselorInSeat && Seat->AcceptsPendingCharacterAI(BotController, bCompareDistance))
					{
						// Check the distance
						const float DistanceToSeat = (Epicenter - Seat->GetComponentLocation()).SizeSquared();
						if (DistanceToSeat < ClosestEscapeDistanceSq)
						{
							// Check navigation
							FVector InteractionLocation;
							if (Seat->BuildAIInteractionLocation(BotController, InteractionLocation))
							{
								ClosestEscapeDistanceSq = DistanceToSeat;
								ClosestEscapeLocation = InteractionLocation;
								DesiredEscapeInteractable = Seat;

								// Always go to driver's seat regardless of distance from other seats in this vehicle
								if (Seat->IsDriverSeat())
									break;
							}
						}
					}
				}
			}
		}
	}

	// Check Escape Pods
	for (ASCEscapePod* EscapePod : GameMode->GetAllEscapePods())
	{
		if (IsValid(EscapePod) && (EscapePod->PodState == EEscapeState::EEscapeState_Active || EscapePod->PodState == EEscapeState::EEscapeState_Repaired))
		{
			// Make sure the pod is active or close to becoming active
			const float ActivationTime = EscapePod->GetRemainingActivationTime();
			if (ActivationTime >= 0.f && ActivationTime <= 10.f)
			{
				if (EscapePod->InteractComponent->AcceptsPendingCharacterAI(BotController, bCompareDistance))
				{
					const float DistanceToEscapePod = (Epicenter - EscapePod->InteractComponent->GetComponentLocation()).SizeSquared();
					if (DistanceToEscapePod < ClosestEscapeDistanceSq)
					{
						FVector EscapePodLocation;
						if (EscapePod->InteractComponent->BuildAIInteractionLocation(BotController, EscapePodLocation))
						{
							ClosestEscapeDistanceSq = DistanceToEscapePod;
							ClosestEscapeLocation = EscapePodLocation;
							DesiredEscapeInteractable = EscapePod->InteractComponent;
						}
					}
				}
			}
		}
	}

	// Check the Bridge
	if (GameState->bNavCardRepaired && GameState->bCoreRepaired && GameState->Bridge && !GameState->Bridge->bDetached)
	{
		bool bFoundUsableButton = false;
		for (ASCBridgeButton* Button : GameState->Bridge->BridgeButtons)
		{
			if (Button->InteractComponent->AcceptsPendingCharacterAI(BotController, bCompareDistance))
			{
				bFoundUsableButton = true;
				const float DistanceToButton = (Epicenter - Button->InteractComponent->GetComponentLocation()).SizeSquared();
				if (DistanceToButton < ClosestEscapeDistanceSq)
				{
					FVector ButtonLocation;
					if (Button->InteractComponent->BuildAIInteractionLocation(BotController, ButtonLocation))
					{
						ClosestEscapeDistanceSq = DistanceToButton;
						ClosestEscapeLocation = ButtonLocation;
						DesiredEscapeInteractable = Button->InteractComponent;
					}
				}
			}
		}

		if (!bFoundUsableButton)
		{
			// If we didn't find a button to push, lets check if we should try and escape with the people pushing the button
			const FVector BridgeLocation = GameState->Bridge->GetActorLocation();
			const float DistanceToBridge = (Epicenter - BridgeLocation).SizeSquared();
			if (DistanceToBridge < ClosestEscapeDistanceSq)
			{
				ClosestEscapeDistanceSq = DistanceToBridge;
				ClosestEscapeLocation = BridgeLocation;
				DesiredEscapeInteractable = nullptr;
			}
		}
	}

	// Set the blackboard key with the location of the escape location or vehicle seat, and assign this our desired interactable
	if (FNavigationSystem::IsValidLocation(ClosestEscapeLocation))
	{
		const FBlackboard::FKey BlackboardKeyId = BotBlackboard->GetKeyID(BlackboardKey.SelectedKeyName);
		if (BlackboardKeyId != FBlackboard::InvalidKey)
		{
			BotBlackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKeyId, ClosestEscapeLocation);
			BotController->SetDesiredInteractable(DesiredEscapeInteractable, false, true);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
