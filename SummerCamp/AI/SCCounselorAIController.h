// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "SCCharacterAIController.h"
#include "SCCounselorAIController.generated.h"

class ASCCabin;
class ASCKillerCharacter;
class ASCDriveableVehicle;
class USCCounselorAIProperties;
class USCInteractComponent;

enum class EILLHoldInteractionState : uint8;

/**
 * @enum ESCCounselorFlashlightMode
 */
UENUM(BlueprintType)
enum class ESCCounselorFlashlightMode : uint8
{
	Off, // Always have the flashlight off
	Fear, // Use fear to determine if the flashlight should be on
	On // Always have the flashlight on
};

static const TCHAR* FlashlightModeToString(const ESCCounselorFlashlightMode Mode)
{
	switch (Mode)
	{
	case ESCCounselorFlashlightMode::Off: return TEXT("Off");
	case ESCCounselorFlashlightMode::Fear: return TEXT("Fear");
	case ESCCounselorFlashlightMode::On: return TEXT("On");
	}

	return TEXT("UNKNOWN");
}

/**
 * @enum ESCCounselorMovementPreference
 */
UENUM(BlueprintType)
enum class ESCCounselorMovementPreference : uint8
{
	FearAndStamina, // Use fear and stamina based running/sprinting
	Stamina, // Use stamina based running/sprinting
	Forbid // Disallow running/sprinting for this move request
};

/**
* @enum ESCCounselorCloseDoorPreference
*/
UENUM(BlueprintType)
enum class ESCCounselorCloseDoorPreference : uint8
{
	// Only close exterior doors
	ExteriorOnly,
	// Close all doors
	All,
	// Born in a barn
	Never
};

/**
* @enum ESCCounselorLockDoorPreference
*/
UENUM(BlueprintType)
enum class ESCCounselorLockDoorPreference : uint8
{
	// Only barricade or lock exterior doors
	ExteriorOnly,
	// Barricade or lock all doors
	All,
	// Never lock
	Never
};

/**
 * @class ASCCounselorAIController
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCCounselorAIController
: public ASCCharacterAIController
{
	GENERATED_UCLASS_BODY()

	// Begin AController interface
protected:
	virtual bool InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset) override;
	virtual void Possess(APawn* InPawn) override;
	// End AController interface

	// Begin AAIController interface
public:
	virtual bool RunBehaviorTree(UBehaviorTree* BTAsset) override;
	virtual void FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const override;
	// End AAIController interface

	// Begin ASCCharacterAIController interface
public:
	virtual void SetBotProperties(TSubclassOf<USCCharacterAIProperties> PropertiesClass) override;
	virtual void ThrottledTick(float TimeSeconds = 0.f) override;
	virtual void OnSensorSeePawn(APawn* SeenPawn) override;
	virtual void OnSensorHearNoise(APawn* HeardPawn, const FVector& Location, float Volume) override;
	virtual void OnPawnDied() override;
	virtual void PawnInteractAccepted(USCInteractComponent* Interactable);

protected:
	virtual void InteractWithDoor(USCInteractComponent* InteractableComp, const ASCDoor* Door) override;
	virtual void InteractWithWindow(USCInteractComponent* InteractableComp, const ASCWindow* Window) override;
	virtual void InteractWithHidingSpot(USCInteractComponent* InteractableComp, const ASCHidingSpot* HidingSpot) override;
	virtual void InteractWithCabinet(USCInteractComponent* InteractableComp, const ASCCabinet* Cabinet) override;
	virtual void InteractWithItem(USCInteractComponent* InteractableComp, const ASCItem* Item) override;
	virtual void InteractWithRepairable(USCInteractComponent* InteractableComp, const AActor* RepairableObject) override;
	virtual void InteractWithCommunicationDevice(USCInteractComponent* InteractableComp, const AActor* ComsDevice) override;
	virtual void InteractWithRadio(USCInteractComponent* InteractableComp, const AActor* Radio) override;
	// End ASCCharacterAIController interface

	//////////////////////////////////////////////////
	// Inventory / Perks
protected:
	/** Give this counselor their items and perks from their properties */
	void GiveCounselorItemsAndPerks();
	
	//////////////////////////////////////////////////
	// Killer stimulus management
public:
	/** Triggers a fake visual stimulus event on whoever the CurrentKiller in the GameState is. Useful for single player scripting. */
	UFUNCTION(BlueprintCallable, Category="JasonStimulus")
	virtual void ForceKillerVisualStimulus();

	/** @return true if the killer has been seen within JasonThreatSeenTimeout seconds. */
	virtual bool HasKillerBeenSeenRecently() const;

	/**
	 * Processes visual and noise stimulus for the Killer.
	 *
	 * @param Killer Killer the caused the stimulus.
	 * @param Location Location for the stimulus.
	 * @param bVisual Visual stimulus? Noise stimulus when false.
	 * @param Loudness Loudness of the stimulus, 1.0 for visual.
	 * @param bShared Shared from another counselor in range?
	 */
	void ReceivedKillerStimulus(ASCKillerCharacter* Killer, const FVector& Location, const bool bVisual, const float Loudness, const bool bShared = false);

	// Last location we received killer stimulus at
	FVector LastKillerStimulusLocation;

	// World time seconds the killer was last chasing us
	float LastKillerChasingTime;

	// World time seconds that we last received killer stimulus
	float LastKillerStimulusTime;

	// World time seconds that we last received noise killer stimulus
	float LastKillerHeardTime;

	// World time seconds that we last received visual killer stimulus
	float LastKillerSeenTime;

	// Was the Killer inside the same cabin as us when we last received stimulus?
	bool bWasKillerInSameCabin;

	// Was the Killer outside last time we received stimulus?
	bool bWasKillerOutside;

	// Has the Killer been seen recently?
	bool bWasKillerRecentlySeen;

	// World time seconds that we last updated killer line of sight
	float TimeUntilKillerLOSUpdate;

	// Was there a line of sight to our killer?
	bool bWasKillerInLOS;
	
	//////////////////////////////////////////////////
	// FightBack
protected:
	// Next time that we can attempt a FightBack sequence
	float NextFightBackTime;

	// Are we performing the armed fight back sequence?
	bool bPerformingArmedFightBack;

	// Are we performing the melee fight back sequence?
	bool bPerformingMeleeFightBack;

	//////////////////////////////////////////////////
	// Flashlight control
public:
	// Flashlight mode
	ESCCounselorFlashlightMode FlashlightMode;

	/** Set this counselors flashlight mode */
	UFUNCTION(BlueprintCallable, Category="Flashlight")
	void SetFlashlightMode(ESCCounselorFlashlightMode Mode) { FlashlightMode = Mode; }

	//////////////////////////////////////////////////
	// Run/Sprint
public:
	/** Set if the counselor can run. */
	UFUNCTION(BlueprintCallable, Category="Running", meta=(DeprecatedFunction)) // Deprecated to disable any previous BP hookups for this
	void AllowCounselorToRun(bool bCanCounselorRun)
	{
		bCanRun = bCanCounselorRun;
		TimeUntilRunningStateCheck = 0.f; // Update next tick
	}

	/** Set if this counselor can sprint. */
	UFUNCTION(BlueprintCallable, Category="Running|Sprint", meta=(DeprecatedFunction)) // Deprecated to disable any previous BP hookups for this
	void AllowCounselorToSprint(bool bCanCounselorSprint)
	{
		bCanSprint = bCanCounselorSprint;
		TimeUntilRunningStateCheck = 0.f; // Update next tick
	}

	/** Sets the run preference for this counselor's current move. */
	void SetMovePreferences(const ESCCounselorMovementPreference InRunPreference, const ESCCounselorMovementPreference InSprintPreference)
	{
		RunPreference = InRunPreference;
		SprintPreference = InSprintPreference;
		TimeUntilRunningStateCheck = 0.f; // Update next tick
	}

protected:
	// Time until we next update our running/sprinting states
	float TimeUntilRunningStateCheck;

	// Preference to run
	ESCCounselorMovementPreference RunPreference;

	// Preference to sprint
	ESCCounselorMovementPreference SprintPreference;

	// Can this counselor run?
	bool bCanRun;

	// Can this counselor sprint?
	bool bCanSprint;

	//////////////////////////////////////////////////
	// Stance
public:
	/** Set if the counselor should be crouching */
	UFUNCTION(BlueprintCallable, Category="Stance")
	void SetShouldCrouch(bool bCrouch);

	//////////////////////////////////////////////////
	// Fear
public:
	/** Set this counselor's fear level */
	UFUNCTION(BlueprintCallable, Category="Fear")
	void SetFearLevel(float FearLevel);

	//////////////////////////////////////////////////
	// Door Interaction
public:
	/** Called when we have passed through a door */
	void OnPassedThroughDoor(const ASCDoor* DoorPassedThrough);

	/** Sets if this Bot should close doors when exiting a building */
	UFUNCTION(BlueprintCallable, Category="Doors")
	void SetShouldCloseDoorsWhenExiting(bool bCloseDoors) { bCloseDoorWhenExitingBuilding = bCloseDoors; }

	/** Sets the close door preference */
	UFUNCTION(BlueprintCallable, Category="Doors")
	void SetCloseDoorPreference(ESCCounselorCloseDoorPreference ClosePreference)
	{
		CloseDoorPreference = ClosePreference;
	}

	/** Sets the lock door preference */
	UFUNCTION(BlueprintCallable, Category="Doors")
	void SetLockDoorPreference(ESCCounselorLockDoorPreference LockPreference)
	{
		LockDoorPreference = LockPreference;
	}

	/** Sets the door preferences */
	UFUNCTION(BlueprintCallable, Category="Doors")
	void SetDoorPreferences(ESCCounselorCloseDoorPreference ClosePreference, ESCCounselorLockDoorPreference LockPreference)
	{
		CloseDoorPreference = ClosePreference;
		LockDoorPreference = LockPreference;
	}

private:
	/** Called when our LockDoor timer fires */
	UFUNCTION()
	void OnLockDoor(ASCDoor* CurrentDoor);

	// Timer handle for locking the door;
	FTimerHandle TimerHandle_LockDoor;

	// Do we need to close the door we just entered
	bool bNeedsToCloseDoor;

	// Should we close doors when leaving a building
	bool bCloseDoorWhenExitingBuilding;

	// Time until we can lock/unlock doors
	float TimeUntilAbleToLockDoors;

	// Preference for closing doors
	ESCCounselorCloseDoorPreference CloseDoorPreference;

	// Preference for locking doors
	ESCCounselorLockDoorPreference LockDoorPreference;

	//////////////////////////////////////////////////
	// Hiding Spot Interaction
public:
	/** Force this Counselor to exit their current hiding spot */
	UFUNCTION(BlueprintCallable, Category="Hiding Spot")
	void ExitHidingSpot();

protected:
	// Time when we entered a hiding spot
	float TimeEnteredHidingSpot;

	// Should we do a LOS check from Jason before entering the hiding spot
	bool bCheckJasonLOSBeforeHiding;

	//////////////////////////////////////////////////
	// Cabin Interaction
public:
	/** Resets the SearchedCabins list. */
	FORCEINLINE void ClearSearchedCabins() { SearchedCabins.Empty(); }

	/** @return true if this counselor has already searched Cabin. */
	FORCEINLINE bool HasSearchedCabin(const ASCCabin* Cabin) const { return SearchedCabins.Contains(Cabin); }

	/** Flags Cabin as searched by this counselor. */
	FORCEINLINE void MarkCabinSearched(ASCCabin* Cabin) { SearchedCabins.AddUnique(Cabin); }

protected:
	// List of cabins that have been searched
	UPROPERTY(Transient)
	TArray<ASCCabin*> SearchedCabins;

	//////////////////////////////////////////////////
	// Cabinet Interaction
public:
	/** Resets the SearchedCabinetInteractComponents list. */
	FORCEINLINE void ClearSearchedCabinets() { SearchedCabinetInteractComponents.Empty(); }

	/** @return true if we have already searched Cabinet. */
	bool HasSearchedCabinet(const ASCCabinet* Cabinet) const;

	/** @return true if we have already searched Cabinet. */
	bool HasSearchedCabinet(const USCInteractComponent* CabinetInteract) const { return SearchedCabinetInteractComponents.Contains(CabinetInteract); }

	/** Marks a cabinet as searched and clears the desired interactable. */
	FORCEINLINE void SetCabinetSearched(const USCInteractComponent* CabinetInteract)
	{
		SearchedCabinetInteractComponents.AddUnique(CabinetInteract);
		SetDesiredInteractable(nullptr);
	}

private:
	// List of cabinets searched
	UPROPERTY(Transient)
	TArray<const USCInteractComponent*> SearchedCabinetInteractComponents;
	
	//////////////////////////////////////////////////
	// Item Interaction
public:
	/** @return true if Other should be taken over Current. */
	virtual bool CompareItems(const ASCItem* Current, const ASCItem* Other) const;

	/** @return true if Item should be considered for picking up. */
	virtual bool DesiresItemPickup(ASCItem* Item) const;

protected:
	/** @return true if this counselor should prefer a weapon over a repair part. */
	virtual bool ShouldPreferWeaponOverPart() const;

	// Do we currently prefer weapons over parts?
	bool bPerfersWeaponsOverParts;

	// Have we been given our items we should spawn with?
	bool bHasRecievedSpawnItems;

	//////////////////////////////////////////////////
	// Repair Interaction
private:
	/** Start repairing */
	void StartRepair(USCInteractComponent* InteractComponent);

	/** Called when the counselor has "failed" a mini game interaction */
	UFUNCTION()
	void OnFailedRepair();

	/** Handle the state changes for our current repair interaction */
	UFUNCTION()
	void RepairInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState);

	// The number of times this counselor will fail it's current repair interaction
	int32 NumRepairFails;

	// How often this counselor will fail it's current repair interaction
	float RepairFailTime;

	// How much progress should be loss per repair fail
	float LostRepairProgress;

	// Timer handle to trigger OnFailedRepair
	FTimerHandle TimerHandle_FailRepair;

	// Timer delegate for failing a repair
	FTimerDelegate TimerDelegate_FailedRepair;

	//////////////////////////////////////////////////
	// Vehicle
private:
	bool ShouldWaitForPassengers(ASCDriveableVehicle* Vehicle) const;

	//////////////////////////////////////////////////
	// Blackboard
protected:
	// Cached Blackboard keys
	FBlackboard::FKey AreBehaviorsBlockedKey;
	FBlackboard::FKey CanEscapeKey;
	FBlackboard::FKey JasonCharacterKey;
	FBlackboard::FKey IsBeingChasedKey;
	FBlackboard::FKey IsGrabbedKey;
	FBlackboard::FKey IsIndoorsKey;
	FBlackboard::FKey IsInsideSearchableCabinKey;
	FBlackboard::FKey IsKillerInSameCabinKey;
	FBlackboard::FKey IsJasonAThreatKey;
	FBlackboard::FKey LastKillerSeenTimeKey;
	FBlackboard::FKey LastKillerSoundStimulusLocationKey;
	FBlackboard::FKey LastKillerSoundStimulusTimeKey;
	FBlackboard::FKey SeekWeaponWhileFleeingKey;
	FBlackboard::FKey ShouldFightBackKey;
	FBlackboard::FKey ShouldArmedFightBackKey;
	FBlackboard::FKey ShouldMeleeFightBackKey;
	FBlackboard::FKey ShouldFleeKillerKey;
	FBlackboard::FKey ShouldJukeKillerKey;
	FBlackboard::FKey ShouldWaitForPassengersKey;

	// Cached Blackboard values
	bool bIsJasonAThreat;
	bool bShouldFleeKiller;

public:
	/** @return true if this controller wants to flee the killer */
	FORCEINLINE bool ShouldFleeKiller() const { return bShouldFleeKiller; }

	/** @return true if this controller is using offline bots logic */
	UFUNCTION(BlueprintPure, Category="BehaviorTree")
	bool IsUsingOfflineBotsLogic();

	/** @return true if there is an escape option for this bot */
	bool CanEscape() const;
};
