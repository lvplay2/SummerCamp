// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCCounselorAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BrainComponent.h"
#include "Perception/PawnSensingComponent.h"

#if WITH_EDITOR
# include "MessageLog.h"
# include "UObjectToken.h"
#endif

#include "SCBridge.h"
#include "SCCabin.h"
#include "SCCBRadio.h"
#include "SCCharacterAIProperties.h"
#include "SCCounselorCharacter.h"
#include "SCCounselorAvoidTrapsQueryFilter.h"
#include "SCDoor.h"
#include "SCDoubleCabinet.h"
#include "SCDriveableVehicle.h"
#include "SCEscapePod.h"
#include "SCEscapeVolume.h"
#include "SCFirstAid.h"
#include "SCFuseBox.h"
#include "SCGameMode.h"
#include "SCGameMode_SPChallenges.h"
#include "SCGameState.h"
#include "SCGun.h"
#include "SCHidingSpot.h"
#include "SCIndoorComponent.h"
#include "SCInteractableManagerComponent.h"
#include "SCJasonOnlyNavigationQueryFilter.h"
#include "SCKillerCharacter.h"
#include "SCNoiseMaker.h"
#include "SCPamelaSweater.h"
#include "SCPamelaTape.h"
#include "SCPerkData.h"
#include "SCPhoneJunctionBox.h"
#include "SCPlayerState.h"
#include "SCPocketKnife.h"
#include "SCPolicePhone.h"
#include "SCRepairPart.h"
#include "SCSpecialMoveComponent.h"
#include "SCThrowingKnifePickup.h"
#include "SCTommyTape.h"
#include "SCTrap.h"
#include "SCVehicleSeatComponent.h"
#include "SCVehicleStarterComponent.h"
#include "SCVoiceOverComponent_Counselor.h"
#include "SCWeapon.h"
#include "SCWheelchairNavigationFilter.h"
#include "SCWindow.h"

namespace SCCounselorBlackboardKeys
{
	const FName NAME_CanEscape(TEXT("CanEscape"));
	const FName NAME_JasonCharacter(TEXT("JasonCharacter"));
	const FName NAME_IsBeingChased(TEXT("IsBeingChased"));
	const FName NAME_IsGrabbed(TEXT("IsGrabbed"));
	const FName NAME_IsIndoors(TEXT("IsIndoors"));
	const FName NAME_IsInsideSearchableCabin(TEXT("IsInsideSearchableCabin"));
	const FName NAME_IsKillerInSameCabin(TEXT("IsKillerInSameCabin"));
	const FName NAME_IsJasonAThreat(TEXT("IsJasonAThreat"));
	const FName NAME_LastKillerSeenTime(TEXT("LastKillerSeenTime"));
	const FName NAME_LastKillerSoundStimulusLocation(TEXT("LastKillerSoundStimulusLocation"));
	const FName NAME_LastKillerSoundStimulusTime(TEXT("LastKillerSoundStimulusTime"));
	const FName NAME_SeekWeaponWhileFleeing(TEXT("SeekWeaponWhileFleeing"));
	const FName NAME_ShouldFightBack(TEXT("ShouldFightBack"));
	const FName NAME_ShouldArmedFightBack(TEXT("ShouldArmedFightBack"));
	const FName NAME_ShouldMeleeFightBack(TEXT("ShouldMeleeFightBack"));
	const FName NAME_ShouldFleeKiller(TEXT("ShouldFleeKiller"));
	const FName NAME_ShouldJukeKiller(TEXT("ShouldJukeKiller"));
	const FName NAME_ShouldWaitForPassengers(TEXT("ShouldWaitForPassengers"));
}

ASCCounselorAIController::ASCCounselorAIController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	LastKillerStimulusLocation = FNavigationSystem::InvalidLocation;

	bCanSprint = true;
	bCanRun = true;

	TimerDelegate_FailedRepair = FTimerDelegate::CreateUObject(this, &ASCCounselorAIController::OnFailedRepair);
}

bool ASCCounselorAIController::InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset)
{
	if (Super::InitializeBlackboard(BlackboardComp, BlackboardAsset))
	{
		CanEscapeKey							= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_CanEscape);
		JasonCharacterKey						= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_JasonCharacter);
		IsBeingChasedKey						= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_IsBeingChased);
		IsGrabbedKey							= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_IsGrabbed);
		IsIndoorsKey							= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_IsIndoors);
		IsInsideSearchableCabinKey				= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_IsInsideSearchableCabin);
		IsKillerInSameCabinKey					= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_IsKillerInSameCabin);
		IsJasonAThreatKey						= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_IsJasonAThreat);
		LastKillerSeenTimeKey					= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_LastKillerSeenTime);
		LastKillerSoundStimulusLocationKey		= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_LastKillerSoundStimulusLocation);
		LastKillerSoundStimulusTimeKey			= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_LastKillerSoundStimulusTime);
		SeekWeaponWhileFleeingKey				= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_SeekWeaponWhileFleeing);
		ShouldFightBackKey						= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_ShouldFightBack);
		ShouldArmedFightBackKey					= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_ShouldArmedFightBack);
		ShouldMeleeFightBackKey					= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_ShouldMeleeFightBack);
		ShouldFleeKillerKey						= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_ShouldFleeKiller);
		ShouldJukeKillerKey						= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_ShouldJukeKiller);
		ShouldWaitForPassengersKey				= BlackboardAsset.GetKeyID(SCCounselorBlackboardKeys::NAME_ShouldWaitForPassengers);
		return true;
	}

	return false;
}

void ASCCounselorAIController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	if (InPawn)
	{
		if (InPawn->IsA<ASCCounselorCharacter>())
		{
			if (BrainComponent && !BrainComponent->IsRunning())
			{
				UWorld* World = GetWorld();
				ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;
				if (GameMode)
				{
					RunBehaviorTree(GameMode->CounselorBehaviorTree);
				}
			}
		}
		else if (ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(InPawn))
		{
			if (Vehicle->VehicleType == EDriveableVehicleType::Car)
				RunBehaviorTree(Vehicle->BehaviorTree);
			// TODO: Add Boat
		}
	}
}

bool ASCCounselorAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	// Make sure we have an usable pawn before setting our BT
	APawn* BotPawn = GetPawn();
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(BotPawn);
	const bool bHasUsablePawn = Counselor ? Counselor->IsAlive() && !Counselor->HasEscaped() : BotPawn != nullptr;
	if (bHasUsablePawn && Super::RunBehaviorTree(BTAsset))
	{
		if (IsUsingOfflineBotsLogic())
		{
			if (UWorld* World = GetWorld())
			{
				if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
				{
					// Stop any conversations with this counselor
					GameState->GetConversationManager()->AbortConversationByCounselor(Counselor);

					// Trigger the counselors scream
					if (USCVoiceOverComponent_Counselor* VOComp = Cast<USCVoiceOverComponent_Counselor>(Counselor->VoiceOverComponent))
					{
						VOComp->PlayKillerInRangeVO(0);
					}
				}
			}

			// Notify that we have gone into offline bots logic
			if (Counselor->OnOfflineAIMode.IsBound())
				Counselor->OnOfflineAIMode.Broadcast();

			const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
			if (MyProperties && MyProperties->bUseDefaultPreferencesForOfflineLogic)
			{
				// Don't close doors when leaving a building
				bCloseDoorWhenExitingBuilding = false;
				
				// Set counselors to close and lock exterior doors
				SetDoorPreferences(ESCCounselorCloseDoorPreference::ExteriorOnly, ESCCounselorLockDoorPreference::ExteriorOnly);

				// Do LOS check before entering a hiding spot
				bCheckJasonLOSBeforeHiding = true;
			}
		}

		return true;
	}

	return false;
}

void ASCCounselorAIController::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
{
	if (Query.NavData.Get())
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn()))
		{
			if (Counselor->IsInWheelchair())
			{
				// Wheelchair counselors should ignore Windows, too
				Query.QueryFilter = USCWheelchairNavigationFilter::GetQueryFilter<USCWheelchairNavigationFilter>(*Query.NavData);
			}
			else
			{
				ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
				if (GameMode && !GameMode->IsA<ASCGameMode_SPChallenges>() && FMath::RandBool()) // TODO: dhumphries - Take composure into account or maybe having a pocket knife
				{
					// Ignore Car, trap, and Jason only areas
					Query.QueryFilter = USCCounselorAvoidTrapsQueryFilter::GetQueryFilter<USCCounselorAvoidTrapsQueryFilter>(*Query.NavData);
				}
				else
				{
					// Ignore Car and Jason only areas
					Query.QueryFilter = USCCounselorNavigationQueryFilter::GetQueryFilter<USCCounselorNavigationQueryFilter>(*Query.NavData);
				}
			}
		}
		else if (ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(GetPawn()))
		{
			// Cars only ignore Jason nav areas for now, I guess?
			Query.QueryFilter = USCJasonOnlyNavigationQueryFilter::GetQueryFilter<USCJasonOnlyNavigationQueryFilter>(*Query.NavData);
		}
	}

	Super::FindPathForMoveRequest(MoveRequest, Query, OutPath);
}

void ASCCounselorAIController::SetBotProperties(TSubclassOf<USCCharacterAIProperties> PropertiesClass)
{
	Super::SetBotProperties(PropertiesClass);

	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (PropertiesClass && !MyProperties && GetPawn())
	{
#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("No counselor AI properties set for \"%s\""), *GetPawn()->GetName()))))
			->AddToken(FUObjectToken::Create(GetPawn()->GetClass()))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(", using default values in code."))));
#endif
		UE_LOG(LogSCCharacterAI, Error, TEXT("ASCCounselorAIController::SetBotProperties: No counselor AI properties set for \"%s\", using default values in code."), *GetPawn()->GetName());

		return;
	}

	// Give our counselor any items or perks from their properties
	GiveCounselorItemsAndPerks();

	bCloseDoorWhenExitingBuilding = MyProperties->bCloseDoorWhenExitingBuilding;
	SetDoorPreferences(MyProperties->ShouldCloseDoors, MyProperties->ShouldLockDoors);
	bCheckJasonLOSBeforeHiding = MyProperties->bCheckJasonLOSBeforeHiding;
}

void ASCCounselorAIController::ThrottledTick(float TimeSeconds/* = 0.f*/)
{
	// Calculate the DeltaTime before the Super call updates LastThrottledTickTime
	UWorld* World = GetWorld();
	if (TimeSeconds == 0.f)
		TimeSeconds = World->GetTimeSeconds();
	const float DeltaTime = TimeSeconds-LastThrottledTickTime;
	Super::ThrottledTick(TimeSeconds);

	if (!Blackboard)
		return;

	// We might be driving a car, let's update some shit for that
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(GetPawn());
	if (!Counselor && Vehicle)
	{
		Counselor = Cast<ASCCounselorCharacter>(Vehicle->Driver);
	}

	if (!Counselor || Counselor->bIsDying || Counselor->IsInContextKill())
		return;

	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	ASCGameState* GameState = World->GetGameState<ASCGameState>();
	if (!GameMode || !GameState)
		return;

	const float Hard25Higher = GameMode->GetCurrentDifficultyUpwardAlpha(.25f);
	const float Easy25Higher = GameMode->GetCurrentDifficultyDownwardAlpha(.25f);

	// Update door locking/unlocking time
	if (TimeUntilAbleToLockDoors > 0.f)
		TimeUntilAbleToLockDoors -= DeltaTime;

	// Check on the flashlight
	bool bWantsFlashlight = false;
	switch (FlashlightMode)
	{
	case ESCCounselorFlashlightMode::Fear:
	{
		// If we're grabbed we don't want to fuck with the flashlight.
		if (Counselor->bIsGrabbed)
		{
			bWantsFlashlight = Counselor->IsFlashlightOn();
			break;
		}

		// Determine minimum fear for the flashlight
		float MinFearForFlashlight = 100.f;
		if (Counselor->IsIndoors())
			MinFearForFlashlight = MyProperties->MinFearForFlashlightIndoors;
		else
			MinFearForFlashlight = MyProperties->MinFearForFlashlightOutdoors;

		// Incorporate difficulty, making them use the flashlight less in easier difficulties
		MinFearForFlashlight *= Easy25Higher;

		// Determine our answer
		bWantsFlashlight = (Counselor->GetFear() >= MinFearForFlashlight);
	}
	break;

	case ESCCounselorFlashlightMode::On:
		bWantsFlashlight = true;
		break;
	}

	// Don't turn our flashlight on if we are hiding
	if (Counselor->GetCurrentHidingSpot())
		bWantsFlashlight = false;

	if (Counselor->IsFlashlightOn() != bWantsFlashlight)
	{
		Counselor->ToggleFlashlight();
	}

	// Determine validity of current stimulus
	bWasKillerRecentlySeen = HasKillerBeenSeenRecently();
	bPerfersWeaponsOverParts = ShouldPreferWeaponOverPart();
	const float LastJasonStimulusDistanceSq = bWasKillerRecentlySeen ? (LastKillerStimulusLocation - Counselor->GetActorLocation()).SizeSquared() : FLT_MAX;

	// Check if we should use a Med Spray
	if (Counselor->Health <= (float)MyProperties->MinHealthToUseMedSpray*Hard25Higher && Counselor->HasItemInInventory(ASCFirstAid::StaticClass()))
	{
		if (!bBehaviorsBlocked && (!bWasKillerRecentlySeen || LastJasonStimulusDistanceSq >= FMath::Square(MyProperties->MinDistanceFromJasonToUseMedSpray*Hard25Higher)))
		{
			const int32 MedSprayIndex = Counselor->GetIndexOfSmallItem(ASCFirstAid::StaticClass());
			if (MedSprayIndex >= 0)
				Counselor->SERVER_OnUseSmallItem(MedSprayIndex);
		}
	}

	// Consider Jason as chasing us when the character chase stuff is active and we have seen him recently
	const bool bIsBeingChased = bWasKillerRecentlySeen && Counselor->IsBeingChased();
	if (bIsBeingChased)
	{
		LastKillerChasingTime = TimeSeconds;
	}

	// Check if Jason is in the same cabin as us
	const bool bIsJasonInSameCabin = bWasKillerRecentlySeen && (Counselor->IsIndoors() && bWasKillerInSameCabin);

	// Check if we are both outside
	const bool bIsJasonOutsideToo = bWasKillerRecentlySeen && (!Counselor->IsIndoors() && bWasKillerOutside);

	// Update line of sight checks before they're needed
	if (DeltaTime > 0.f)
	{
		TimeUntilKillerLOSUpdate -= DeltaTime;
		if (TimeUntilKillerLOSUpdate <= 0.f)
		{
			bWasKillerInLOS = LineOfSightTo(GameState->CurrentKiller);
			TimeUntilKillerLOSUpdate = 1.f;
		}
	}

	// Consider Jason a threat if he has been seen recently and is within a distance requirement, and we are in the same cabin or both outside
	const bool bInSameSpace = (bIsJasonInSameCabin || bIsJasonOutsideToo) && bWasKillerInLOS;
	bIsJasonAThreat = bInSameSpace && (LastJasonStimulusDistanceSq <= FMath::Square(MyProperties->JasonThreatDistance*Hard25Higher));
	if (!bIsJasonAThreat)
	{
		bIsJasonAThreat = (
				(LastKillerChasingTime != 0.f && TimeSeconds-LastKillerChasingTime < MyProperties->JasonThreatSeenRecently*Hard25Higher) || // Recently chased us?
				(LastKillerSeenTime != 0.f && TimeSeconds-LastKillerSeenTime < MyProperties->JasonThreatSeenRecently*Hard25Higher) || // Recently seen the killer?
				(LastKillerSeenTime != 0.f && LastKillerHeardTime != 0.f && TimeSeconds-LastKillerHeardTime < MyProperties->JasonThreatSeenRecently*Hard25Higher) // Recently heard the killer after seeing him ever?
			);
	}

	// Check if we should try and distract Jason
	if (Counselor->HasItemInInventory(ASCNoiseMaker::StaticClass()))
	{
		if (!bBehaviorsBlocked && bWasKillerRecentlySeen && bInSameSpace && LastJasonStimulusDistanceSq <= FMath::Square(MyProperties->MaxDistanceFromJasonToUseFirecrackers*Easy25Higher))
		{
			const int32 FirecrackerIndex = Counselor->GetIndexOfSmallItem(ASCNoiseMaker::StaticClass());
			if (FirecrackerIndex >= 0)
				Counselor->SERVER_OnUseSmallItem(FirecrackerIndex);
		}
	}

	// Check if we should fight back
	bool bShouldArmedFightBack = false;
	bool bShouldMeleeFightBack = false;
	ASCItem* CurrentItem = Counselor->GetEquippedItem();
	if (!bBehaviorsBlocked && Counselor->CanAttack() && bWasKillerRecentlySeen && bInSameSpace)
	{
		if (bPerformingArmedFightBack || bPerformingMeleeFightBack)
		{
			// Resume the previous fight back sequence
			bShouldArmedFightBack = bPerformingArmedFightBack;
			bShouldMeleeFightBack = bPerformingMeleeFightBack;
		}
		// Require outdoor combat, at least for now, and avoid fighting back too often
		else if (CurrentItem && TimeSeconds >= NextFightBackTime && !GameState->CurrentKiller->IsGrabKilling())
		{
			// Armed fight back requires a Gun... and other stuff
			if (CurrentItem->IsA(ASCGun::StaticClass()))
			{
				// Make sure we start this far enough away
				if (LastJasonStimulusDistanceSq >= FMath::Square(MyProperties->ArmedFightBackMinimumDistance*Hard25Higher))
				{
					// Make sure he is not too far away
					if (LastJasonStimulusDistanceSq <= FMath::Square(MyProperties->ArmedFightBackMaximumDistance*Hard25Higher))
					{
						bShouldArmedFightBack = true;
					}
				}
			}
			// Melee fight back requires having a non-gun weapon equipped
			else if (CurrentItem->IsA(ASCWeapon::StaticClass()))
			{
				// Make sure we start this far enough away to turn around
				if (LastJasonStimulusDistanceSq >= FMath::Square(MyProperties->MeleeFightBackMinimumDistance*Easy25Higher))
				{
					// Make sure he is not too far away
					if (LastJasonStimulusDistanceSq <= FMath::Square((Counselor->IsWounded() ? MyProperties->MeleeFightBackMaximumDistanceWounded : MyProperties->MeleeFightBackMaximumDistance)*Easy25Higher))
					{
						bShouldMeleeFightBack = true;
					}
				}
				else
				{
					// Fallback to allowing up-close attacks when facing towards them
					const FVector DirToJason = (LastKillerStimulusLocation - Counselor->GetActorLocation()).GetSafeNormal();
					const float ForwardDot = FVector::DotProduct(Counselor->GetActorForwardVector(), DirToJason);
					if (ForwardDot >= FMath::DegreesToRadians(MyProperties->MeleeFightBackCloseFacingAngle))
					{
						bShouldMeleeFightBack = true;
					}
				}
			}
		}

		// Delay the next attempt on success, or when continued so it's a delay after the end of the fight back
		if (bShouldMeleeFightBack)
		{
			NextFightBackTime = TimeSeconds + FMath::RandRange(MyProperties->MeleeFightBackCooldownMinimum*Easy25Higher, MyProperties->MeleeFightBackCooldownMaximum*Easy25Higher);
		}
	}
	bPerformingArmedFightBack = bShouldArmedFightBack;
	bPerformingMeleeFightBack = bShouldMeleeFightBack;
	const bool bShouldFightBack = (bPerformingArmedFightBack || bPerformingMeleeFightBack);

	// Check if we should be performing our flee or one of it's sub-behaviors (fight back, juke)
	const bool bShouldJukeKiller = !bShouldFightBack && bInSameSpace && (LastJasonStimulusDistanceSq <= FMath::Square(MyProperties->JasonJukeDistance*Hard25Higher));
	bShouldFleeKiller = (bShouldFightBack || bShouldJukeKiller || bIsJasonAThreat || bIsBeingChased);

	// Check if we should be seeking a weapon while fleeing
	const bool bSeekWeaponWhileFleeing = bShouldFleeKiller && !bShouldJukeKiller && (!CurrentItem || !CurrentItem->IsA(ASCWeapon::StaticClass())) && bPerfersWeaponsOverParts;

	// Update dynamic Blackboard values
	Blackboard->SetValue<UBlackboardKeyType_Bool>(CanEscapeKey, CanEscape());
	Blackboard->SetValue<UBlackboardKeyType_Object>(JasonCharacterKey, UBlackboardKeyType_Object::FDataType(GameState->CurrentKiller));
	Blackboard->SetValue<UBlackboardKeyType_Bool>(IsBeingChasedKey, bIsBeingChased);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(IsGrabbedKey, Counselor->bIsGrabbed);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(IsKillerInSameCabinKey, bIsJasonInSameCabin);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(IsJasonAThreatKey, bIsJasonAThreat);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(SeekWeaponWhileFleeingKey, bSeekWeaponWhileFleeing);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(ShouldFightBackKey, bShouldFightBack);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(ShouldArmedFightBackKey, bPerformingArmedFightBack);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(ShouldMeleeFightBackKey, bPerformingMeleeFightBack);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(ShouldFleeKillerKey, bShouldFleeKiller);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(ShouldJukeKillerKey, bShouldJukeKiller);
	Blackboard->SetValue<UBlackboardKeyType_Bool>(ShouldWaitForPassengersKey, ShouldWaitForPassengers(Vehicle));

	// Update indoor status
	if (Counselor->IsIndoors())
	{
		Blackboard->SetValue<UBlackboardKeyType_Bool>(IsIndoorsKey, true);

		bool bInsideSearchableCabin = false;
		for (USCIndoorComponent* IndoorComp : Counselor->OverlappingIndoorComponents)
		{
			if (ASCCabin* Cabin = Cast<ASCCabin>(IndoorComp->GetOwner()))
			{
				if (!SearchedCabins.Contains(Cabin))
				{
					bInsideSearchableCabin = true;
					break;
				}
			}
		}
		Blackboard->SetValue<UBlackboardKeyType_Bool>(IsInsideSearchableCabinKey, bInsideSearchableCabin);
	}
	else
	{
		Blackboard->SetValue<UBlackboardKeyType_Bool>(IsIndoorsKey, false);
		Blackboard->SetValue<UBlackboardKeyType_Bool>(IsInsideSearchableCabinKey, false);
	}

	if (DeltaTime > 0.f)
	{
		// Wait for our run/sprint ease-off to expire
		TimeUntilRunningStateCheck -= DeltaTime;
		if (TimeUntilRunningStateCheck <= 0.f)
		{
			bool bStaminaLow = false;

			// Check if we want to be sprinting
			bool bWantsToSprint = false;
			if (bCanSprint && !Counselor->IsWounded())
			{
				if (!Counselor->bStaminaDepleted && Counselor->GetStamina() >= MyProperties->MinStaminaToSprint)
				{
					switch (SprintPreference)
					{
					case ESCCounselorMovementPreference::FearAndStamina: bWantsToSprint = (Counselor->GetFear() >= MyProperties->MinFearToSprint); break;
					case ESCCounselorMovementPreference::Stamina: bWantsToSprint = true; break;
					}
				}
				else
				{
					bStaminaLow = true;
				}
			}
			if (bWantsToSprint != Counselor->IsSprinting())
			{
				Counselor->SetSprinting(bWantsToSprint);
			}

			// Check if we should be running instead
			bool bWantsToRun = false;
			if (bCanRun && !bWantsToSprint)
			{
				if (!Counselor->bStaminaDepleted && Counselor->GetStamina() >= MyProperties->MinStaminaToRun)
				{
					switch (RunPreference)
					{
					case ESCCounselorMovementPreference::FearAndStamina: bWantsToRun = (Counselor->GetFear() >= MyProperties->MinFearToRun); break;
					case ESCCounselorMovementPreference::Stamina: bWantsToRun = true; break;
					}
				}
				else
				{
					bStaminaLow = true;
				}
			}
			if (bWantsToRun != Counselor->IsRunning())
			{
				Counselor->SetRunning(bWantsToRun);
			}

			// When our stamina is low, ease-off for a period of time
			if (bStaminaLow)
			{
				TimeUntilRunningStateCheck = FMath::RandRange(MyProperties->MinRunSprintStateEaseOff, MyProperties->MaxRunSprintStateEaseOff);;
			}
			else
			{
				// Otherwise just ease-off for a little while, randomly to avoid sync-ups
				TimeUntilRunningStateCheck = FMath::RandRange(.1f, .3f);
			}

			// Incorporate difficulty
			TimeUntilRunningStateCheck *= Easy25Higher;
		}
	}
}

void ASCCounselorAIController::OnSensorSeePawn(APawn* SeenPawn)
{
	// Don't take stimulus if pawn sensing isn't enabled
	if (PawnSensingComponent && !PawnSensingComponent->bEnableSensingUpdates)
		return;

	Super::OnSensorSeePawn(SeenPawn);

	if (ASCKillerCharacter* SeenKiller = Cast<ASCKillerCharacter>(SeenPawn))
	{
		// Check against their stealth
		if (SeenKiller->IsStealthVisibleFor(this))
		{
			ReceivedKillerStimulus(SeenKiller, SeenKiller->GetActorLocation(), true, 1.f);
		}
	}
}

void ASCCounselorAIController::OnSensorHearNoise(APawn* HeardPawn, const FVector& Location, float Volume)
{
	// Don't take stimulus if pawn sensing isn't enabled
	if (PawnSensingComponent && !PawnSensingComponent->bEnableSensingUpdates)
		return;

	Super::OnSensorHearNoise(HeardPawn, Location, Volume);

	if (ASCKillerCharacter* HeardKiller = Cast<ASCKillerCharacter>(HeardPawn))
	{
		// Check against their stealth
		if (HeardKiller->IsStealthHearbleBy(this, Location, Volume))
		{
			ReceivedKillerStimulus(HeardKiller, Location, false, Volume);
		}
	}
}

void ASCCounselorAIController::OnPawnDied()
{
	// Clear our timers
	GetWorldTimerManager().ClearTimer(TimerHandle_LockDoor);
	GetWorldTimerManager().ClearTimer(TimerHandle_FailRepair);

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
		{
			// Stop any conversations with this counselor on death
			GameState->GetConversationManager()->AbortConversationByCounselor(Cast<ASCCounselorCharacter>(GetOwner()));
		}
	}

	Super::OnPawnDied();
}

void ASCCounselorAIController::PawnInteractAccepted(USCInteractComponent* Interactable)
{
	const ASCCharacter* MyCharacter = Cast<ASCCharacter>(GetPawn());
	const AActor* InteractableOwner = Interactable ? Interactable->GetOwner() : nullptr;
	if (InteractableOwner && InteractableOwner->IsA<ASCDoor>() && MyCharacter && MyCharacter->IsHoldInteractButtonPressed() && MyCharacter->InteractTimerPassedMinHoldTime())
	{
		// We have finished interacting with the door, clear our desired interactable
		SetDesiredInteractable(nullptr);
	}

	Super::PawnInteractAccepted(Interactable);
}

void ASCCounselorAIController::InteractWithDoor(USCInteractComponent* InteractableComp, const ASCDoor* Door)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (!Door || !Counselor || !MyProperties)
		return;

	if (InteractableComp == DesiredInteractable && bBarricadeDesiredInteractable)
	{
		if (Door->IsOpen())
		{
			// Close the door
			SimulateInteractButtonPress();
		}
		else if (!Counselor->IsHoldInteractButtonPressed() && TimeUntilAbleToLockDoors <= 0.f)
		{
			// Lock the door
			Counselor->OnInteract1Pressed();
			TimeUntilAbleToLockDoors = MyProperties->TimeBetweenLockingInteractions;
		}
		return;
	}

	if (Door->IsLockedOrBarricaded() && Door->IsInteractorInside(Counselor) && TimeUntilAbleToLockDoors <= 0.f)
	{
		// Unlock the door
		Counselor->OnInteract1Pressed();
		TimeUntilAbleToLockDoors = MyProperties->TimeBetweenLockingInteractions;

		return;
	}

	Super::InteractWithDoor(InteractableComp, Door);

	if (!Counselor->IsInWheelchair())
	{
		if ((Door->ShouldAICloseDoor() && Door->CanCharacterCloseDoor(Counselor)) || CloseDoorPreference == ESCCounselorCloseDoorPreference::All)
		{
			bNeedsToCloseDoor = true;
		}
		else if (CloseDoorPreference == ESCCounselorCloseDoorPreference::ExteriorOnly)
		{
			bNeedsToCloseDoor = bWasOutsideWhenOpeningDoor;
		}
	}
}

void ASCCounselorAIController::InteractWithWindow(USCInteractComponent* InteractableComp, const ASCWindow* Window)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (!Counselor || !Window || !MyProperties || !Blackboard)
		return;

	if (InteractableComp == Window->InteractComponent && IsMovingTowardInteractable(InteractableComp))
	{
		UWorld* World = GetWorld();
		ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
		const float Hard25Higher = GameMode->GetCurrentDifficultyUpwardAlpha(.25f);

		// Check to see if we should dive through a window
		if (Counselor->GetFear() < MyProperties->MinFearForWindowDive*Hard25Higher)
		{
			// Skip window-diving when low on fear
			Counselor->SetSprinting(false);
		}
		else if (Counselor->Health < MyProperties->MinHealthForWindowDive*Hard25Higher)
		{
			// Skip window-diving when low on health
			Counselor->SetSprinting(false);
		}
		else if (!Blackboard->GetValue<UBlackboardKeyType_Bool>(ShouldFleeKillerKey))
		{
			// When not being fleeing, try not to window-dive
			Counselor->SetSprinting(false);
		}

		// Open and climb through the window
		SimulateInteractButtonPress();
	}
}

void ASCCounselorAIController::InteractWithHidingSpot(USCInteractComponent* InteractableComp, const ASCHidingSpot* HidingSpot)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (!Counselor || !HidingSpot || !Blackboard)
		return;

	if (InteractableComp == DesiredInteractable)
	{
		UWorld* World = GetWorld();
		const float TimeSeconds = World->GetTimeSeconds();
		if (Counselor->GetCurrentHidingSpot() == HidingSpot)
		{
			ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
			const float Hard25Higher = GameMode ? GameMode->GetCurrentDifficultyUpwardAlpha(.25f) : 1.f;

			// We are already hiding here
			bool bIsJasonClose = bIsJasonAThreat;
			if (!bIsJasonClose)
			{
				bIsJasonClose = (bWasKillerRecentlySeen || (LastKillerHeardTime != 0.f && TimeSeconds-LastKillerHeardTime <= MyProperties->HideJasonHeardMinTime*Hard25Higher))
					&& (bWasKillerInSameCabin || (LastKillerStimulusLocation-Counselor->GetActorLocation()).SizeSquared() <= MyProperties->JasonThreatDistance*Hard25Higher);
			}

			// TODO: maybe add hold breath

			// Should we leave?
			const bool bShouldExitHideSpot = !bIsJasonClose && (TimeSeconds-TimeEnteredHidingSpot >= MyProperties->MinTimeToHide*Hard25Higher);
			if (bShouldExitHideSpot)
			{
				// Leave the hiding spot
				SimulateInteractButtonPress();
				SetDesiredInteractable(nullptr);
			}
		}
		else if (!InteractableComp->IsLockedInUse())
		{
			// Make sure Jason can't see us before we enter
			ASCGameState* GameState = World->GetGameState<ASCGameState>();
			ASCKillerCharacter* Killer = GameState ? GameState->CurrentKiller : nullptr;
			const bool bCanJasonSeeUs = [&]()
			{
				if (bCheckJasonLOSBeforeHiding && Killer && Killer->Controller)
				{
					if (bAIIgnorePlayers && Killer->Controller->IsPlayerController())
						return false;

					return Killer->Controller->LineOfSightTo(Counselor);
				}
					
				return false;
			}();

			const bool bIsSafeToHide = Killer ? !bCanJasonSeeUs && Counselor->GetSquaredDistanceTo(Killer) > FMath::Square(MyProperties->MinDistanceFromJasonToHide) : true;
			if (bIsSafeToHide)
			{
				// Let's hide!
				SimulateInteractButtonPress();
				TimeEnteredHidingSpot = TimeSeconds;
			}
			else
			{
				// Jason can see us, so we can't hide here
				SetDesiredInteractable(nullptr);
			}
		}
	}
}

void ASCCounselorAIController::InteractWithCabinet(USCInteractComponent* InteractableComp, const ASCCabinet* Cabinet)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || !Cabinet)
		return;

	if (InteractableComp == DesiredInteractable)
	{
		auto InteractWithDrawer = [&](USCInteractComponent* InteractComponent, ASCItem* ItemInDrawer, const bool bIsFullyOpened)
		{
			if (InteractComponent->IsLockedInUse() && Counselor->GetInteractableManagerComponent()->GetLockedInteractable() == InteractComponent)
			{
				if (bIsFullyOpened)
				{
					if (DesiresItemPickup(ItemInDrawer))
					{
						// Grab the item
						if (!Counselor->IsHoldInteractButtonPressed())
						{
							UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s grabbing item from cabinet %s (%s)"), *GetNameSafe(Counselor), *GetNameSafe(InteractComponent), *GetNameSafe(Cabinet));
							Counselor->OnInteract1Pressed();
						}
					}
					else if (ItemInDrawer)
					{
						// Close the drawer
						UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s closing non-empty cabinet %s (%s)"), *GetNameSafe(Counselor), *GetNameSafe(InteractComponent), *GetNameSafe(Cabinet));
						SimulateInteractButtonPress();

						// Clear our desired interactable
						if (!Counselor->bAttemptingInteract && Counselor->GetInteractableManagerComponent()->GetLockedInteractable() == nullptr)
							SetDesiredInteractable(nullptr);
					}
				}
			}
			else if (!Counselor->bAttemptingInteract)
			{
				// Open the drawer
				UE_LOG(LogSCCharacterAI, Verbose, TEXT("%s opening cabinet %s (%s)"), *GetNameSafe(Counselor), *GetNameSafe(InteractComponent), *GetNameSafe(Cabinet));
				SimulateInteractButtonPress();
			}
		};

		// Double cabinet special-case handling
		if (const ASCDoubleCabinet* DoubleCabinet = Cast<ASCDoubleCabinet>(Cabinet))
		{
			if (InteractableComp == DoubleCabinet->GetInteractComponent())
			{
				InteractWithDrawer(InteractableComp, DoubleCabinet->ContainedItem, DoubleCabinet->IsDrawerFullyOpened());
			}
			else if (ensure(InteractableComp == DoubleCabinet->GetInteractComponent2()))
			{
				InteractWithDrawer(InteractableComp, DoubleCabinet->ContainedItem2, DoubleCabinet->IsDrawer2FullyOpened());
			}
		}
		else
		{
			check(InteractableComp == Cabinet->GetInteractComponent());
			InteractWithDrawer(InteractableComp, Cabinet->ContainedItem, Cabinet->IsDrawerFullyOpened());
		}
	}
}

void ASCCounselorAIController::InteractWithItem(USCInteractComponent* InteractableComp, const ASCItem* Item)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || !Item)
		return;

	// See if we should disarm a Jason trap
	if (const ASCTrap* Trap = Cast<ASCTrap>(Item))
	{
		if (Trap->WasPlacedByKiller() && Counselor->HasItemInInventory(ASCPocketKnife::StaticClass()))
		{
			// Crouch down and disarm the trap
			Counselor->SetStance(ESCCharacterStance::Crouching);
			Counselor->OnInteract1Pressed();
			return;
		}

		if (Trap->GetIsArmed())
			return;
	}

	// Pick up the item
	if (DesiredInteractable == InteractableComp)
		SimulateInteractButtonPress();
}

void ASCCounselorAIController::InteractWithRepairable(USCInteractComponent* InteractableComp, const AActor* RepairableObject)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || !RepairableObject || !InteractableComp)
		return;

	// Skip repairing when we know we need to flee
	if (bShouldFleeKiller)
	{
		if (Counselor->IsHoldInteractButtonPressed())
		{
			// Stop repairing
			InteractableComp->OnHoldStateChanged.RemoveDynamic(this, &ASCCounselorAIController::RepairInteractStateChange);
			Counselor->OnInteract1Released();
			return;
		}
		else if (Counselor->IsInVehicle() && Counselor->GetVehicle()->CanEnterExitVehicle())
		{
			// Get out of the vehicle and ruuuuunnnnn!
			SimulateInteractButtonPress();
			return;
		}
	}

	const ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(RepairableObject);
	if (Vehicle && InteractableComp->IsA<USCVehicleSeatComponent>())
	{
		if (!Counselor->IsInVehicle())
		{
			if (DesiredInteractable == InteractableComp)
			{
				// Get in the vehicle
				SimulateInteractButtonPress();
			}
		}
		else if (!Vehicle->Driver)
		{
			// Get out and try to move to the drivers seat
			for (USCVehicleSeatComponent* Seat : Vehicle->Seats)
			{
				if (Seat->IsDriverSeat())
				{
					if (!Seat->CounselorInSeat && Seat->AcceptsPendingCharacterAI(this))
					{
						// Get out of the vehicle
						SimulateInteractButtonPress();

						// Mark the seat as claimed so only 1 passenger needs to get out
						SetDesiredInteractable(Seat);
					}

					break;
				}
			}
		}

		return;
	}

	if (Counselor->IsDriver()) // I'm driving a vehicle! Make sure DesiredInteractable is the Starter Component and not the Seat
	{
		if (DesiredInteractable != InteractableComp)
		{
			SetDesiredInteractable(InteractableComp);
		}
	}

	if (DesiredInteractable != InteractableComp)
		return;

	// Vehicles
	if (Vehicle)
	{
		if (InteractableComp->bIsEnabled)
		{
			if (!InteractableComp->IsLockedInUse())
			{
				if (!Vehicle->bIsRepaired)
				{
					StartRepair(InteractableComp);
				}
				else if (InteractableComp->IsA<USCVehicleSeatComponent>())
				{
					// Get in the vehicle
					if (!Counselor->IsInVehicle())
						SimulateInteractButtonPress();
					// Get out of the vehicle
					else if (Vehicle->IsVehicleDisabled())
						SimulateInteractButtonPress();
				}
			}
			else if (InteractableComp->IsA<USCVehicleStarterComponent>() && !Counselor->IsHoldInteractButtonPressed())
			{
				// Start the vehicle
				Counselor->OnInteract1Pressed();
			}
		}
	}
	else
	{
		if (InteractableComp->bIsEnabled && !InteractableComp->IsLockedInUse())
		{
			// Early out if this has already been repaired
			USCRepairComponent* RepairComponent = Cast<USCRepairComponent>(InteractableComp);
			if (RepairComponent && RepairComponent->IsRepaired())
				return;

			// Repair the phone box or power box
			StartRepair(InteractableComp);
		}
	}
}

void ASCCounselorAIController::InteractWithCommunicationDevice(USCInteractComponent* InteractableComp, const AActor* ComsDevice)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || !ComsDevice || DesiredInteractable != InteractableComp)
		return;

	// Skip repairing when we know we need to flee
	if (bShouldFleeKiller)
	{
		// Stop repairing
		if (Counselor->IsHoldInteractButtonPressed())
		{
			InteractableComp->OnHoldStateChanged.RemoveDynamic(this, &ASCCounselorAIController::RepairInteractStateChange);
			Counselor->OnInteract1Released();
		}
		return;
	}

	if (InteractableComp->bIsEnabled && !InteractableComp->IsLockedInUse() && !Counselor->IsHoldInteractButtonPressed())
	{
		// Call for help
		Counselor->OnInteract1Pressed();
	}
}

void ASCCounselorAIController::InteractWithRadio(USCInteractComponent* InteractableComp, const AActor* Radio)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || !Radio || DesiredInteractable != InteractableComp)
		return;

	// Turn the radio on/off
	if (!Counselor->bAttemptingInteract)
	{
		SimulateInteractButtonPress();
		SetDesiredInteractable(nullptr);
	}
}

void ASCCounselorAIController::GiveCounselorItemsAndPerks()
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();

	if (Counselor && GameMode && MyProperties)
	{
		if (!bHasRecievedSpawnItems)
		{
			if (GameMode->IsA<ASCGameMode_SPChallenges>())
			{
				Counselor->GiveItem(MyProperties->LargeItem);
				Counselor->GiveItem(MyProperties->SmallItems[0]);
				Counselor->GiveItem(MyProperties->SmallItems[1]);
				Counselor->GiveItem(MyProperties->SmallItems[2]);
			}
			else if (GameMode->IsA<ASCGameMode_Offline>())
			{
				const FCounslorGameDifficultySettings& CurrentSettings = MyProperties->DifficultySettings[(int32)GameMode->GetModeDifficulty()];

				// Large item
				if ((CurrentSettings.bAlwaysStartWithLargeItem || (!CurrentSettings.bAlwaysStartWithLargeItem && FMath::RandBool())) && CurrentSettings.PossibleLargeItems.Num())
				{
					Counselor->GiveItem(CurrentSettings.PossibleLargeItems[FMath::RandHelper(CurrentSettings.PossibleLargeItems.Num())]);
				}

				// Small items
				if (CurrentSettings.bUseSpecifiedSmallItems)
				{
					Counselor->GiveItem(CurrentSettings.SmallItems[0]);
					Counselor->GiveItem(CurrentSettings.SmallItems[1]);
					Counselor->GiveItem(CurrentSettings.SmallItems[2]);
				}
				else if (CurrentSettings.PossibleSmallItems.Num())
				{
					TArray<TSubclassOf<ASCItem>> SmallItems = CurrentSettings.PossibleSmallItems;
					for (int32 i = 0, End = FMath::RandRange(0, 3); i < End && SmallItems.Num(); ++i)
					{
						int32 ItemIndex = FMath::RandHelper(SmallItems.Num());
						Counselor->GiveItem(SmallItems[ItemIndex]);
						SmallItems.RemoveAt(ItemIndex);
					}
				}

				// Perks
				if (CurrentSettings.bUseSpecifiedPerks)
				{
					TArray<USCPerkData*> ActivePerks;
					if (CurrentSettings.Perks[0])
						ActivePerks.Add(CurrentSettings.Perks[0]->GetDefaultObject<USCPerkData>());
					if (CurrentSettings.Perks[1])
						ActivePerks.Add(CurrentSettings.Perks[1]->GetDefaultObject<USCPerkData>());
					if (CurrentSettings.Perks[2])
						ActivePerks.Add(CurrentSettings.Perks[2]->GetDefaultObject<USCPerkData>());

					if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
					{
						PS->SetActivePerks(ActivePerks);
					}
				}
				else if (CurrentSettings.PossiblePerks.Num())
				{
					TArray<USCPerkData*> ActivePerks;
					TArray<TSubclassOf<USCPerkData>> Perks = CurrentSettings.PossiblePerks;
					for (int i = 0, End = FMath::RandRange(0, 3); i < End && Perks.Num(); ++i)
					{
						int32 PerkIndex = FMath::RandHelper(Perks.Num());
						ActivePerks.Add(Perks[PerkIndex]->GetDefaultObject<USCPerkData>());
						Perks.RemoveAt(PerkIndex);
					}

					if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
					{
						PS->SetActivePerks(ActivePerks);
					}
				}
			}

			bHasRecievedSpawnItems = true;
		}
	}
}

void ASCCounselorAIController::ForceKillerVisualStimulus()
{
	UWorld* World = GetWorld();
	ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr;
	if (!GameState || !GameState->CurrentKiller)
		return;

	ReceivedKillerStimulus(GameState->CurrentKiller, GameState->CurrentKiller->GetActorLocation(), true, 1.f);
}

bool ASCCounselorAIController::HasKillerBeenSeenRecently() const
{
	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;
	ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr;
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (!GameMode || !GameState || !MyProperties)
		return false;

	// Handle AIIgnorePlayers and CurrentKiller
	const bool bIgnoreCurrentKiller = !GameState->CurrentKiller || (bAIIgnorePlayers && GameState->CurrentKiller->Controller && GameState->CurrentKiller->Controller->IsPlayerController());

	// Check the age of visual stimulus
	const float Hard25Higher = GameMode->GetCurrentDifficultyUpwardAlpha(.25f);
	const bool bKillerLocationStale = (LastKillerStimulusTime == 0.f || !FNavigationSystem::IsValidLocation(LastKillerStimulusLocation));
	const bool bCurrentKillerCanBeSensed = !bIgnoreCurrentKiller && !bKillerLocationStale;
	return bCurrentKillerCanBeSensed && (LastKillerSeenTime != 0.f && World->GetTimeSeconds()-LastKillerSeenTime < MyProperties->JasonThreatSeenTimeout*Hard25Higher);
}

void ASCCounselorAIController::ReceivedKillerStimulus(ASCKillerCharacter* Killer, const FVector& Location, const bool bVisual, const float Loudness, const bool bShared/* = false*/)
{
	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;
	ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr;
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (!GameMode || !GameState || !Counselor || !MyProperties || !Killer)
		return;

	// Let the game state know we spotted a killer, but only if we're alive and not in the middle of being murdered
	if (Counselor->IsAlive() && !Counselor->bIsGrabbed && !Counselor->IsInContextKill())
		GameState->SpottedKiller(Killer, Counselor);

	// Store off stimulus info
	LastKillerStimulusLocation = Location;
	LastKillerStimulusTime = World->GetTimeSeconds();
	if (bVisual)
	{
		LastKillerSeenTime = LastKillerStimulusTime;
		if (Blackboard)
		{
			Blackboard->SetValue<UBlackboardKeyType_Float>(LastKillerSeenTimeKey, LastKillerSeenTime);
		}
	}
	else
	{
		LastKillerHeardTime = LastKillerStimulusTime;
		if (Blackboard)
		{
			Blackboard->SetValue<UBlackboardKeyType_Vector>(LastKillerSoundStimulusLocationKey, Location);
			Blackboard->SetValue<UBlackboardKeyType_Float>(LastKillerSoundStimulusTimeKey, LastKillerHeardTime);
		}
	}

	// Update cabin status
	bWasKillerOutside = (Killer && !Killer->IsIndoors());
	bWasKillerInSameCabin = !bWasKillerOutside && (Killer && Killer->IsIndoorsWith(Counselor));

	// Delay fighting back a while after he was stunned
	if (Killer->IsStunned())
	{
		const float Easy25Higher = GameMode->GetCurrentDifficultyDownwardAlpha(.25f);
		NextFightBackTime = LastKillerStimulusTime + FMath::RandRange(MyProperties->FightBackStunDelayMinimum*Easy25Higher, MyProperties->FightBackStunDelayMaximum*Easy25Higher);
	}

	// Share with the class
	if (!bShared)
	{
		const float ShareSkill = FMath::Lerp(.75f, 1.f, Counselor->GetStatAlpha(ECounselorStats::Composure));
		const float ShareDifficulty = ShareSkill * GameMode->GetCurrentDifficultyUpwardAlpha(.3f); // Share at a larger range the harder the difficulty
		const float ShareDistanceLimit = ShareDifficulty * (bVisual ? MyProperties->ShareJasonVisualStimulusDistance : MyProperties->ShareJasonNoiseStimulusDistance);
		const float ShareStimulusDistanceSq = FMath::Square(ShareDistanceLimit);
		for (ASCCharacter* OtherCharacter : GameState->PlayerCharacterList)
		{
			if (OtherCharacter == Counselor)
				continue;

			if (ASCCounselorAIController* OtherCounselor = Cast<ASCCounselorAIController>(OtherCharacter->Controller))
			{
				// Get the distance between the two characters
				const FVector CounselorLocation = Counselor->GetActorLocation();
				const FVector OtherCharacterLocation = OtherCharacter->GetActorLocation();
				const float SquaredDistanceToOtherCharacter = (CounselorLocation - OtherCharacterLocation).SizeSquared();

				// Setup trace params
				FHitResult HitResult;
				static const FName NAME_StimulusTrace(TEXT("StimulusTrace"));
				FCollisionQueryParams QueryParams(NAME_StimulusTrace);
				QueryParams.AddIgnoredActor(Counselor);
				QueryParams.AddIgnoredActor(OtherCharacter);

				// Test to see if we should share the stimulus
				if (SquaredDistanceToOtherCharacter < ShareStimulusDistanceSq && !World->LineTraceSingleByChannel(HitResult, CounselorLocation, OtherCharacterLocation, ECC_Visibility, QueryParams))
				{
					// Always share as audio stimulus for them to look towards
					OtherCounselor->ReceivedKillerStimulus(Killer, Location, bVisual, Loudness*.5f, true);
				}
			}
		}
	}

	// Notify killer stealth
	Killer->CounselorReceivedStimulus(bVisual, Loudness, bShared);

	// Force an immediate update
	ThrottledTick(0.f);
}

void ASCCounselorAIController::SetShouldCrouch(bool bCrouch)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());

	if (!Counselor)
		return;

	Counselor->SetStance(bCrouch ? ESCCharacterStance::Crouching : ESCCharacterStance::Standing);
}

void ASCCounselorAIController::SetFearLevel(float FearLevel)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());

	if (!Counselor)
		return;

	Counselor->SetFear(FearLevel);
}

void ASCCounselorAIController::OnPassedThroughDoor(const ASCDoor* DoorPassedThrough)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	
	if (!DoorPassedThrough || !Counselor || Counselor->bIsGrabbed)
		return;

	// Close the door if we are coming into a building
	if (DoorPassedThrough->IsOpen() && !DoorPassedThrough->IsInteractionLockedInUse() && (bNeedsToCloseDoor || (!Counselor->IsIndoors() && bCloseDoorWhenExitingBuilding)))
	{
		if (bWasOutsideWhenOpeningDoor && DoorPassedThrough->IsInteractorInside(Counselor))
		{
			// Get the door close interact location with the counselor's half height as an offset
			const float ZOffset = Counselor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FVector DesiredLocation = DoorPassedThrough->CloseDoorInsideForward_SpecialMoveHandler->GetDesiredLocation(Counselor) + (FVector::UpVector * ZOffset);

			// Place the character at the close door interaction location
			Counselor->SetActorLocation(DesiredLocation);
		}

		// Close the door
		if (!DoorPassedThrough->IsInteractionLockedInUse())
			Counselor->OnInteract(DoorPassedThrough->GetClosestInteractComponent(Counselor), EILLInteractMethod::Press);

		bNeedsToCloseDoor = false;
		bWasOutsideWhenOpeningDoor = false;

		// Lock the door, if we can
		const bool bShouldLockDoor = (DoorPassedThrough->ShouldAILockDoor() && DoorPassedThrough->CanCharacterLockDoor(Counselor)) || LockDoorPreference != ESCCounselorLockDoorPreference::Never || Counselor->IsBeingChased();
		if (bShouldLockDoor && (DoorPassedThrough->HasBarricade() || DoorPassedThrough->bIsLockable) && DoorPassedThrough->IsInteractorInside(Counselor))
		{
			// Pause pathing so we can lock the door
			GetPathFollowingComponent()->PauseMove(FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Reset);

			// Lock the door once it's closed
			FTimerDelegate LockDoorDelegate;
			static const FName NAME_OnLockDoor(TEXT("OnLockDoor"));
			LockDoorDelegate.BindUFunction(this, NAME_OnLockDoor, DoorPassedThrough);
			GetWorldTimerManager().SetTimer(TimerHandle_LockDoor, LockDoorDelegate, 1.4f, false);
		}
	}
}

void ASCCounselorAIController::OnLockDoor(ASCDoor* CurrentDoor)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();

	if (!Counselor || !CurrentDoor || !MyProperties)
		return;

	// Face the door
	FRotator DesiredRotation = Counselor->GetActorRotation();
	const FRotator RotationToDoor = CurrentDoor->OutArrow->GetComponentRotation();
	DesiredRotation.Yaw = RotationToDoor.Yaw;
	Counselor->SetActorRotation(DesiredRotation);

	// Lock the door
	Counselor->OnInteract1Pressed();
	TimeUntilAbleToLockDoors = MyProperties->TimeBetweenLockingInteractions;

	// Resume path following
	GetPathFollowingComponent()->ResumeMove();
}

void ASCCounselorAIController::ExitHidingSpot()
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	ASCHidingSpot* CurrentHidingSpot = Counselor ? Counselor->GetCurrentHidingSpot() : nullptr;

	if (CurrentHidingSpot)
	{
		// Leave the hiding spot
		SimulateInteractButtonPress();
		SetDesiredInteractable(nullptr);
	}
}

bool ASCCounselorAIController::HasSearchedCabinet(const ASCCabinet* Cabinet) const
{
	if (Cabinet)
	{
		if (SearchedCabinetInteractComponents.Contains(Cabinet->GetInteractComponent()))
		{
			if (const ASCDoubleCabinet* DoubleCabinet = Cast<ASCDoubleCabinet>(Cabinet))
			{
				if (SearchedCabinetInteractComponents.Contains(DoubleCabinet->GetInteractComponent2()))
					return true;
			}
			else
			{
				return true;
			}
		}
	}

	return false;
}

bool ASCCounselorAIController::CompareItems(const ASCItem* Current, const ASCItem* Other) const
{
	if (!Current)
		return (Other != nullptr);
	if (!Other)
		return false;

	if (Current->bIsLarge == Other->bIsLarge)
	{
		// Is this a large item comparison?
		if (Current->bIsLarge)
		{
			// Both are large items
			// Decide what we should do with weapons
			if (const ASCWeapon* OtherWeapon = Cast<ASCWeapon>(Other))
			{
				// See if we should swap weapons
				if (const ASCWeapon* CurrentWeapon = Cast<ASCWeapon>(Current))
				{
					if (const ASCGun* CurrentGun = Cast<ASCGun>(CurrentWeapon))
					{
						// Ignore other weapons when we have a gun
						return false;
					}
					else if (!Cast<ASCGun>(Other))
					{
						const FWeaponStats& CurrentStats = CurrentWeapon->WeaponStats;
						const FWeaponStats& OtherStats = OtherWeapon->WeaponStats;

						const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
						if (GameMode && GameMode->GetModeDifficulty() == ESCGameModeDifficulty::Hard)
						{
							// We always want a higher stun chance in hard mode
							if (OtherStats.StunChance > CurrentStats.StunChance)
								return true;
						}

						// Make sure OtherStats is the same or exceeds CurrentStats in any way
						if (CurrentStats.Damage > OtherStats.Damage || CurrentStats.StunChance > OtherStats.StunChance || CurrentStats.Durability > CurrentStats.Durability)
							return false;

						// Make sure OtherStats is not identical to CurrentStats
						if (CurrentStats.Damage == OtherStats.Damage && OtherStats.StunChance == OtherStats.StunChance && CurrentStats.Durability == OtherStats.Durability)
							return false;
					}
				}
				// See if we should drop our repair part for a weapon
				else if (const ASCRepairPart* CurrentPart = Cast<ASCRepairPart>(Current))
				{
					if (!bPerfersWeaponsOverParts)
					{
						// We prefer repair parts over weapons right now
						return false;
					}
				}
			}
			// Decide what to do with repair parts
			else if (const ASCRepairPart* OtherPart = Cast<ASCRepairPart>(Other))
			{
				if (const ASCWeapon* CurrentWeapon = Cast<ASCWeapon>(Current))
				{
					if (bPerfersWeaponsOverParts)
					{
						// We do not want to lose our weapon
						return false;
					}
				}
				else if (const ASCRepairPart* CurrentPart = Cast<ASCRepairPart>(Current))
				{
					// Repair parts that are closer to their closest repair goal are the most valuable
					if (CurrentPart->GetClosestRepairGoalDistanceSquared() < OtherPart->GetClosestRepairGoalDistanceSquared())
						return false;
				}
			}
		}
		else
		{
			// Both are small items
			// TODO: pjackson: When small inventory is full, compare against all small inventory items and decide what to swap
		}
	}

	return true;
}

bool ASCCounselorAIController::DesiresItemPickup(ASCItem* Item) const
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
	if (!Counselor || !Item || !GameMode)
		return false;

	// Ignore special items in general
	if (Item->bIsSpecial)
		return false;

	// Always ignore throwing knives
	if (Item->IsA(ASCThrowingKnifePickup::StaticClass()))
		return false;

	// See if we should place a trap
	if (ASCTrap* Trap = Cast<ASCTrap>(Item))
	{
		if (!Trap->GetIsArmed())
		{
			if (Counselor->OverlappingIndoorComponents.Num())
			{
				if (ASCCabin* CurrentCabin = Cast<ASCCabin>(Counselor->OverlappingIndoorComponents[0]->GetOwner()))
				{
					for (AActor* CabinChild : CurrentCabin->GetChildren())
					{
						// Make sure all the doors are locked before we try to place a trap
						ASCDoor* CabinDoor = Cast<ASCDoor>(CabinChild);
						if (CabinDoor && !CabinDoor->IsLockedOrBarricaded())
							return false;
					}
				}
			}
			else
			{
				// Ignore traps that are outside
				return false;
			}
		}
		else
		{
			// Ignore armed traps
			return false;
		}
	}

	// Ignore Pamela and Tommy tapes
	if (Item->IsA<ASCPamelaTape>() || Item->IsA<ASCTommyTape>())
		return false;

	// Check interaction
	if (!Item->IsInCabinet())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Counselor->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		if (!Item->GetInteractComponent()->IsInteractionAllowed(Counselor))
			return false;
		if (!Item->CanInteractWith(Counselor, ViewLocation, ViewRotation))
			return false;
	}

	// Check the character
	if (!Counselor->CanTakeItem(Item))
		return false;

	// Try not to be greedy, only taking 1 of an item by class
	if (Counselor->NumberOfItemTypeInInventory(Item->GetClass()) > 0)
		return false;

	// Ignore repair parts that have no goals
	if (ASCRepairPart* RepairPart = Cast<ASCRepairPart>(Item))
	{
		// Always grab car keys
		if (RepairPart->GetClass() != GameMode->GetCarKeysClass())
		{
			if (RepairPart->GetRepairGoalCache().Num() == 0)
				return false;
		}
	}

	// Compare our currently equipped item to it
	return CompareItems(Counselor->GetEquippedItem(), Item);
}

bool ASCCounselorAIController::ShouldPreferWeaponOverPart() const
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
	if (!Counselor || !MyProperties)
		return false;

	// Prefer a weapon when Jason is a threat and we have enough Strength for it to matter
	const uint8 StrengthStat = Counselor->Stats[static_cast<uint8>(ECounselorStats::Strength)];
	return (bWasKillerRecentlySeen && StrengthStat >= MyProperties->WeaponsOverPartsMinStrength);
}

void ASCCounselorAIController::StartRepair(USCInteractComponent* InteractComponent)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || !InteractComponent || !InteractComponent->bIsEnabled)
		return;

	// Determine how well this counselor will do this repair
	// Calculate the number of repair failures we will do
	const float IntelligenceAlpha = Counselor->GetStatAlpha(ECounselorStats::Intelligence);
	const float InteractTime = InteractComponent->GetHoldTimeLimit(Counselor);
	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	const int32 NumInteractPips = InteractComponent->Pips.Num();
	const float NumIntelligenceRepairFailures = (1.f-IntelligenceAlpha) * NumInteractPips * .5f; // Fail up to half the pips when dumb as shit
	const float NumDifficultyRepairFailures = GameMode->GetCurrentDifficultyDownwardAlpha(1.5f, 1.5f); // Fail a little more in easy
	NumRepairFails = FMath::RoundToInt(FMath::RandRange(0.f, NumIntelligenceRepairFailures + NumDifficultyRepairFailures));
	if (NumRepairFails > 0)
	{
		// Calculate the time between each repair failure (random delay between the first pip time and the second)
		const float PipTime = InteractTime / float(NumInteractPips);
		RepairFailTime = FMath::RandRange(PipTime, PipTime*2.f);

		// Calculate how much progress will be lost with each failure (one pip worth, half progress at max)
		LostRepairProgress = FMath::Clamp(PipTime / InteractTime, 0.f, .5f);

		// Offset the first failure by the duration of one pip
		GetWorldTimerManager().SetTimer(TimerHandle_FailRepair, TimerDelegate_FailedRepair, FMath::FRand()*PipTime + RepairFailTime, false);
	}

	// Start repairing
	if (!InteractComponent->OnHoldStateChanged.IsAlreadyBound(this, &ASCCounselorAIController::RepairInteractStateChange))
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCCounselorAIController::RepairInteractStateChange);
	Counselor->OnInteract1Pressed();
}

void ASCCounselorAIController::OnFailedRepair()
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || Counselor->bIsGrabbed || Counselor->bIsDying)
		return;

	USCInteractComponent* InteractComponent = Counselor->GetInteractable();
	if (!InteractComponent)
		return;

	// Set our repair progress back
	Counselor->SetInteractTimePercent(FMath::Max(Counselor->GetInteractTimePercent() - LostRepairProgress, 0.f));

	// Play fail anim
	Counselor->FailureInteractAnim();

	// Notify the killer
	Counselor->Native_NotifyKillerOfMinigameFail(InteractComponent->NotifyKillerMinigameFailCue);

	// Restart the fail timer if needed
	if (--NumRepairFails > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_FailRepair, TimerDelegate_FailedRepair, RepairFailTime, false);
	}
}

void ASCCounselorAIController::RepairInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (NewState == EILLHoldInteractionState::Interacting)
		return;

	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetPawn());
	if (!Counselor || Interactor != Counselor)
		return;

	USCInteractComponent* InteractComponent = Counselor->GetInteractable();
	if (!InteractComponent)
		return;

	// Stop listening for interact changes
	InteractComponent->OnHoldStateChanged.RemoveDynamic(this, &ASCCounselorAIController::RepairInteractStateChange);

	// Force release interact button and clear our desired interactable
	if (NewState == EILLHoldInteractionState::Success)
	{
		SetDesiredInteractable(nullptr);
		Counselor->OnInteract1Released();
	}
}

bool ASCCounselorAIController::ShouldWaitForPassengers(ASCDriveableVehicle* Vehicle) const
{
	// Could be a null vehicle, but we want to update the value in blackboard anyway
	if (!Vehicle)
		return false;

	// If Jason is a threat, gtfo
	if (Blackboard->GetValue<UBlackboardKeyType_Bool>(IsJasonAThreatKey))
		return false;

	// Loop through open vehicle seats and see if another bot is trying to get into it!
	for (USCVehicleSeatComponent* Seat : Vehicle->Seats)
	{
		if (Seat->GetPendingCharacterAI() && !Seat->CounselorInSeat)
		{
			return true;
		}
	}

	// Don't drive off if someone is still getting in
	if (Vehicle->CounselorsInteractingWithVehicle.Num() > 0)
		return true;

	return false;
}

bool ASCCounselorAIController::IsUsingOfflineBotsLogic()
{
	const UWorld* World = GetWorld();
	const ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;
	const UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(GetBrainComponent());
	if (GameMode && BTComp)
		return GameMode->CounselorBehaviorTree == BTComp->GetCurrentTree();

	return false;
}

bool ASCCounselorAIController::CanEscape() const
{
	if (const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		// Check police escapes
		for (ASCEscapeVolume* EscapeVolume : GameMode->GetAllEscapeVolumes())
		{
			if (IsValid(EscapeVolume) && EscapeVolume->EscapeType == ESCEscapeType::Police && EscapeVolume->CanEscapeHere())
				return true;
		}

		const USCCounselorAIProperties* const MyProperties = GetBotPropertiesInstance<USCCounselorAIProperties>();
		const ASCKillerCharacter* Killer = Blackboard ? Cast<ASCKillerCharacter>(Blackboard->GetValue<UBlackboardKeyType_Object>(JasonCharacterKey)) : nullptr;
		const float JasonThreatDistanceSq = FMath::Square(MyProperties->JasonThreatDistance);

		// Check repaired vehicles
		for (ASCDriveableVehicle* Vehicle : GameMode->GetAllSpawnedVehicles())
		{
			if (IsValid(Vehicle) && Vehicle->bIsRepaired && (Vehicle->bFirstStartComplete || Vehicle->WasForceRepaired()) && !Vehicle->bEscaping && !Vehicle->bEscaped)
			{
				// Make sure Jason isn't close to the vehicle
				const float JasonDistanceToVehicleSq = Vehicle->GetSquaredDistanceTo(Killer);
				if (!bAIIgnorePlayers && Killer && JasonDistanceToVehicleSq <= JasonThreatDistanceSq)
					continue;

				// Make sure there is a seat for us
				for (USCVehicleSeatComponent* Seat : Vehicle->Seats)
				{
					if (!Seat->CounselorInSeat && Seat->AcceptsPendingCharacterAI(this))
						return true;
				}
			}
		}

		// Check escape pods
		for (ASCEscapePod* EscapePod : GameMode->GetAllEscapePods())
		{
			if (IsValid(EscapePod) && (EscapePod->PodState == EEscapeState::EEscapeState_Active || EscapePod->PodState == EEscapeState::EEscapeState_Repaired))
			{
				// Make sure the pod is active or close to becoming active
				const float ActivationTime = EscapePod->GetRemainingActivationTime();
				if (ActivationTime >= 0.f && ActivationTime <= 10.f)
					return true;
			}
		}

		// Check the bridge
		const ASCGameState* GameState = GetWorld()->GetGameState<ASCGameState>();
		if (GameState && GameState->bNavCardRepaired && GameState->bCoreRepaired && GameState->Bridge && !GameState->Bridge->bDetached)
		{
			return true;
		}
	}

	return false;
}
