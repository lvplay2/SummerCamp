// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "BehaviorTree/BehaviorTreeTypes.h"
#include "SCAIController.h"
#include "SCCharacterAIController.generated.h"

class ASCCabinet;
class ASCDoor;
class ASCHidingSpot;
class ASCItem;
class ASCWindow;
class UBTTask_FindClosest;
class UPawnSensingComponent;
class USCCharacterAIProperties;
class USCInteractComponent;

SUMMERCAMP_API DECLARE_LOG_CATEGORY_EXTERN(LogSCCharacterAI, Log, All);

/**
 * @class ASCCharacterAIController
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCCharacterAIController
: public ASCAIController
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	// End AActor interface

	// Begin AController interface
	virtual void SetPawn(APawn* InPawn) override;
protected:
	virtual bool InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset) override;
public:
	virtual bool ShouldPostponePathUpdates() const override;
	virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	// End AController interface

	// Begin AAIController interface
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	virtual FVector GetFocalPointOnActor(const AActor *Actor) const override;
	// End AAIController interface

	/** Notification that our Pawn has died. */
	virtual void OnPawnDied();
	
	//////////////////////////////////////////////////
	// Tick throttling
public:
	/** Notification for when our pawn goes indoors or outdoors, to update the BehaviorTree immediately. */
	virtual void PawnIndoorsChanged(const bool bIsInside)
	{
		ThrottledTick();
	}

	/** Notification for when our pawn starts a special move, to update the BehaviorTree immediately. */
	virtual void PawnSpecialMoveStarted()
	{
		ThrottledTick();
	}

	/** Notification for when our pawn stops a special move, to update the BehaviorTree immediately. */
	virtual void PawnSpecialMoveFinished(const bool bWasAborted)
	{
		ThrottledTick();
	}

protected:
	/** Called from Tick every ThrottledTickDelay seconds. */
	virtual void ThrottledTick(float TimeSeconds = 0.f);

	// Time between ThrottledTick calls, in seconds
	float ThrottledTickDelay;

	// Last time we called ThrottledTick
	float LastThrottledTickTime;
	
	//////////////////////////////////////////////////
	// Navigation
public:
	/** Notification for whenever a UBTTask_CharacterMoveTo completes. */
	virtual void OnMoveTaskFinished(const EBTNodeResult::Type TaskResult, const bool bAlreadyAtGoal);

	// Radius around the character to invoke navigation
	float NavigationInvokerRadius;

	// Multiplied against NavigationInvokerTileRemovalRadiusScale to get the tile removal radius 
	float NavigationInvokerTileRemovalRadiusScale;

	// Time we have spent not moving
	float TimeWithoutVelocity;

	//////////////////////////////////////////////////
	// Sensing
public:
	// Pawn sensing component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sensing")
	UPawnSensingComponent* PawnSensingComponent;

	/** @return true if the specified pawn is visible to our PawnSensingComponent */
	virtual bool CanSeePawn(APawn* PawnToSee) const;

protected:
	/** Notification for when the PawnSensingComponent sights SeenPawn. */
	UFUNCTION()
	virtual void OnSensorSeePawn(APawn* SeenPawn);

	/** Notification for when the PawnSensingComponent hears Pawn. */
	UFUNCTION()
	virtual void OnSensorHearNoise(APawn* HeardPawn, const FVector& Location, float Volume);

	//////////////////////////////////////////////////
	// Stun/Grab wiggle
public:
	/** Is this AI attempting to break out of a grab/stun */
	UFUNCTION(BlueprintPure, Category = "Stun|Wiggle")
	bool IsWiggling() const { return bIsWiggling; }

protected:
	// Is this AI wiggling to break out of a grab/stun
	bool bIsWiggling;

	// Time until wiggle tick
	float WiggleDelay;

	// Time since last wiggle
	float WiggleDelta;

	//////////////////////////////////////////////////
	// Interactables
public:
	/** Notification from ASCCharacter::OnInteract before the interaction processing begins. */
	virtual void PawnInteractStarted(USCInteractComponent* Interactable)
	{
		ThrottledTick();
	}

	/** Notification from ASCCharacter::OnInteractAccepted. */
	virtual void PawnInteractAccepted(USCInteractComponent* Interactable);

	/** Set this character's desired interactable.
	 * @param ObjectToInteractWith - The object this AI should interact with (only objects with an SCInteractComponent will work)
	 * @param BlackboardKey - If specified, the blackboard entry for the key will be set with the location of the interactable
	 * @return true if the specified object has been set as the desired interactable
	 */
	UFUNCTION(BlueprintCallable, Category="Interactables")
	bool SetDesiredInteractable(const AActor* ObjectToInteractWith, const FName BlackboardKey, const bool bBarricadeDoor = false);

	/** Set this character's desired interactable. */
	virtual void SetDesiredInteractable(USCInteractComponent* Interactable, const bool bBarricadeDoor = false, const bool bForce = false); // FIXME: pjackson: bBarricadeDoor is kinda... gross and specific

	/** Notify this character that another character has taken their desired interactable (since they were closer to it, or other reasons) */
	virtual void DesiredInteractableTaken();

	/** Get this character's desired interactable. */
	FORCEINLINE USCInteractComponent* GetDesiredInteractable() const { return DesiredInteractable; }

	/** Clears this character's current and desired interactables */
	FORCEINLINE void ClearInteractables()
	{
		SetDesiredInteractable(nullptr);
	}

	/** Simulates interactable button press */
	void SimulateInteractButtonPress();

protected:
	/** Check to see if we have any interactables, and if we should interact with them. */
	virtual void UpdateInteractables(const float DeltaTime);

	/** Check to see if the current interactable is in front of this character. */
	bool IsMovingTowardInteractable(const USCInteractComponent* InteractableComp) const;

	/** Determine what we should do with the door we can interact with. */
	virtual void InteractWithDoor(USCInteractComponent* InteractableComp, const ASCDoor* Door);

	/** Determine what we should do with the window we can interact with. */
	virtual void InteractWithWindow(USCInteractComponent* InteractableComp, const ASCWindow* Window) {}

	/** Determine what we should do with the hiding spot we can interact with. */
	virtual void InteractWithHidingSpot(USCInteractComponent* InteractableComp, const ASCHidingSpot* HidingSpot) {}

	/** Determine what we should do with the cabinet we can interact with. */
	virtual void InteractWithCabinet(USCInteractComponent* InteractableComp, const ASCCabinet* Cabinet) {}

	/** Determine what we should do with the item we can interact with. */
	virtual void InteractWithItem(USCInteractComponent* InteractableComp, const ASCItem* Item) {}

	/** Determine what we should do with the repairable object we can interact with. */
	virtual void InteractWithRepairable(USCInteractComponent* InteractableComp, const AActor* RepairableObject) {}

	/** Determine what we should do with the communication device we can interact with. */
	virtual void InteractWithCommunicationDevice(USCInteractComponent* InteractableComp, const AActor* ComsDevice) {}

	/** Determine what we should do with the radio we can interact with. */
	virtual void InteractWithRadio(USCInteractComponent* InteractableComp, const AActor* Radio) {}

	// Were we outside when we opened a door
	bool bWasOutsideWhenOpeningDoor;

	// Desired object to interact with
	UPROPERTY(Transient)
	USCInteractComponent* DesiredInteractable;

	// Barricade the DesiredInteractable?
	bool bBarricadeDesiredInteractable;

	// How long have we been holding the interact button
	float InteractionHoldTime;

	//////////////////////////////////////////////////
	// Look at Target
public:

	/** Have this AI look at a the specified actor. */
	UFUNCTION(BlueprintCallable, Category="Look At")
	void LookAtTarget(AActor* Actor);

	/** Have this AI look at a specified location */
	UFUNCTION(BlueprintCallable, Category="Look At")
	void LookAtLocation(const FVector Location);

	/** Stop looking at our focal location */
	UFUNCTION(BlueprintCallable, Category="Look At")
	void ClearLookAt();

	//////////////////////////////////////////////////
	// Collision
public:

	/** Add an Actor to ignore by Pawn's movement collision */
	UFUNCTION(BlueprintCallable, Category="Collision")
	void IgnoreCollisionOnActor(AActor* OtherActor);

	/** Remove an Actor to ignore by Pawn's movement collision */
	UFUNCTION(BlueprintCallable, Category="Collision")
	void RemoveIgnoreCollisionOnActor(AActor* OtherActor);

	//////////////////////////////////////////////////
	// Character Properties
public:
	/** Set the properties for this bot */
	virtual void SetBotProperties(TSubclassOf<USCCharacterAIProperties> PropertiesClass);

	/** @return Class default object instance of CharacterPropertiesClass. */
	template<class T>
	FORCEINLINE const T* const GetBotPropertiesInstance(bool bCastChecked = false) const
	{
		const USCCharacterAIProperties* const CDO = CharacterPropertiesClass ? CharacterPropertiesClass->GetDefaultObject<USCCharacterAIProperties>() : nullptr;
		if (bCastChecked)
		{ 
			return CastChecked<T>(CDO, ECastCheckedType::NullAllowed);
		}
		else
		{
			return Cast<T>(CDO);
		}
	}

protected:
	// Properties for this AI
	UPROPERTY(Transient)
	TSubclassOf<USCCharacterAIProperties> CharacterPropertiesClass;

	//////////////////////////////////////////////////
	// Blackboard
protected:
	// Cached Blackboard keys
	FBlackboard::FKey AreBehaviorsBlockedKey;
	FBlackboard::FKey CurrentWaypointKey;
	FBlackboard::FKey DesiredInteractableKey;

	// Cached Blackboard values
	bool bBehaviorsBlocked;
};
