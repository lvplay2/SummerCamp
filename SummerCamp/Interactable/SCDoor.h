// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavLinkProxy.h"
#include "DestructibleComponent.h"

#include "ILLInteractableComponent.h"

#include "SCAnimDriverComponent.h"
#include "SCBarricade.h"
#include "SCContextComponent.h"
#include "SCContextKillActor.h"
#include "SCCounselorCharacter.h"

#include "SCDoor.generated.h"

class ASCCounselorCharacter;

class UNavModifierComponent;
class USCContextKillComponent;
class USCInteractComponent;
class USCSpecialMoveComponent;
class USCStatBadge;
class USCStatusIconComponent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCDoor
 */
UCLASS()
class SUMMERCAMP_API ASCDoor
: public ANavLinkProxy
{
	GENERATED_BODY()

protected:
	// Allow the character to modify us so they can handle our RPCs
	friend ASCCharacter;

public:
	ASCDoor(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor interface

	//////////////////////////////////////////////////////////////////////////////////
	// Components
public:
	// Root scene component
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Root;

	// Scene component to rotate the door mesh around
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USceneComponent* Pivot;

	// Door Static Mesh
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UDestructibleComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	UBoxComponent* DoorCollision;

	// Collision volume to keep vehicles from blocking the door
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	UBoxComponent* VehicleCollision;

	// Volume to notify AI when they have passed through the door
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="AI")
	UBoxComponent* AIPassthroughVolume;

	// Navigation modifier to specify the NavArea around this door
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="AI")
	UNavModifierComponent* NavigationModifier;

	// "Out" direction for testing if characters are inside or outside the door
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	UArrowComponent* OutArrow;

protected:
	/** Called when an actor has walked through a door */
	UFUNCTION()
	void OnActorPassedThrough(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Should AI close this door after they walk through it?
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bShouldAICloseDoor;

	// Should AI lock this door after they close it?
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bShouldAILockDoor;

	// List of AI characters that are allowed to close this door, if empty, all AI can close
	UPROPERTY(EditAnywhere, Category="AI")
	TArray<ASCCounselorCharacter*> CounselorsAllowedToCloseDoor;

	// List of AI characters that are allowed to lock this door, if empty, all AI can lock
	UPROPERTY(EditAnywhere, Category = "AI")
	TArray<ASCCounselorCharacter*> CounselorsAllowedToLockDoor;

public:
	/** @return true if AI should close this door after they walk through it */
	FORCEINLINE bool ShouldAICloseDoor() const { return bShouldAICloseDoor; }

	/** @return true if AI should lock this door after closing it */
	FORCEINLINE bool ShouldAILockDoor() const { return bShouldAILockDoor; }

	/** @return true if the specified character can close this door */
	FORCEINLINE bool CanCharacterCloseDoor(const ACharacter* Character) const
	{
		if (CounselorsAllowedToCloseDoor.Num() > 0 && Character && Character->PlayerState && Character->PlayerState->bIsABot)
			return CounselorsAllowedToCloseDoor.Contains(Character);

		return true;
	}

	/** @return true if the specified character can lock this door */
	FORCEINLINE bool CanCharacterLockDoor(const ACharacter* Character) const
	{
		if (CounselorsAllowedToLockDoor.Num() > 0 && Character && Character->PlayerState && Character->PlayerState->bIsABot)
			return CounselorsAllowedToLockDoor.Contains(Character);

		return true;
	}

	// ~Components
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Door destruction state
public:
	// Destructible mesh To swap in when health falls below LightDestructionThreshold
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* LightDestructionMesh;

	// If we fall below this health percentage we swap to our light destruction state
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	float LightDestructionThreshold;

	// Destructible mesh To swap in when health falls below LightDestructionThreshold
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* MediumDestructionMesh;

	// If we fall below this health percentage we swap to our light destruction state
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	float MediumDestructionThreshold;

	// Destructible mesh To swap in when health falls below HeavyDestructionThreshold
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	UDestructibleMesh* HeavyDestructionMesh;

	// If we fall below this health percentage we swap to our heavy destruction state
	UPROPERTY(EditDefaultsOnly, Category = "Default", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
	float HeavyDestructionThreshold;

	// ~Door destruction state
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Config
public:
	// Is the door lockable?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bIsLockable;

	// The amount of force to apply to the door pieces when Jason bursts through
	UPROPERTY(EditDefaultsOnly, Category = "Door")
	float BurstExplosionForce;

	// Current health of the door
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Health, Transient, BlueprintReadOnly, Category = "Door")
	int32 Health;

	// Starting base health for the door
	UPROPERTY(EditDefaultsOnly, Category = "Door")
	int32 BaseDoorHealth;

	UPROPERTY(EditDefaultsOnly, Category = "Door")
	float BreakExplosionForce;

	// Sound to play when door opens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Sound")
	USoundCue* OpenDoorSound;

	// Sound to play when door closes
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Door|Sound")
	USoundCue* CloseDoorSound;

	// Sound to play when locking/unlocked door
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Door|Sound")
	USoundCue* LockedDoorSound;

	UPROPERTY(EditDefaultsOnly, Category = "Door|Sound")
	USoundCue* DoorDestroySoundCue;

	UPROPERTY(EditDefaultsOnly, Category = "Door|Sound")
	USoundCue* DoorHitSoundCue;

	/** Replicate the new health and update the destruction state if neccessary */
	UFUNCTION()
	virtual void OnRep_Health(int32 OldHealth);

	// ~Config
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Context Kill
public:
	// Door context kill component
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCContextKillComponent* ContextKillComponent;

	// Door kill animation driver for door rotation during the kill
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* DoorKillDriver;

	/** Called when the kill finishes to make sure the door is flagged as open */
	UFUNCTION()
	void KillComplete(ASCCharacter* Interactor, ASCCharacter* Victim);

	// ~Context Kill
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Interaction
public:
	// Interact component for interactions
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCInteractComponent* InteractComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* OpenCloseDoorDriver;

	// Open the door towards the player from the inside, step ouside (killer only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* KillerOpenDoorInside_SpecialMoveHandler;

	// Open the door away from the player from the outside, step inside (killer only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* KillerOpenDoorOutside_SpecialMoveHandler;
	
	// Lock the door from inside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* LockDoorInside_SpecialMoveHandler;

	// Unlock the door from inside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* UnlockDoorInside_SpecialMoveHandler;

	// Unlock the door from outside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* UnlockDoorOutside_SpecialMoveHandler;

	// Open the door towards the player from the inside, step ouside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* OpenDoorInside_SpecialMoveHandler;

	// Open the door away from the player from the outside, step inside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* OpenDoorOutside_SpecialMoveHandler;

	// Open the door away from the player from the outside, move inside (wheelchair counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* OpenDoorWheelchairOutside_SpecialMoveHandler;

	// Close the door away from the player while facing it, no movement (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* CloseDoorInsideForward_SpecialMoveHandler;

	// Close the door away from the player while facing away from it, no movement (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* CloseDoorInsideBackward_SpecialMoveHandler;

	// Close the door towards the player while facing it, backs outside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* CloseDoorOutsideForward_SpecialMoveHandler;

	// Close the door towards the player while facing away from it, steps outside (counselor only)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* CloseDoorOutsideBackward_SpecialMoveHandler;

	// Killer bursts through the door from inside to outside
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BurstDoorFromInside_SpecialMoveHandler;

	// Killer bursts through the door from outside to inside
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BurstDoorFromOutside_SpecialMoveHandler;

	// Handler for burst destruction on the door
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BurstDoorDriver;

	// Handler for whacking away at the door from the inside
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BreakDownFromInside_SpecialMoveHandler;

	// Handler for whacking away at the door from the outside
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCSpecialMoveComponent* BreakDownFromOutside_SpecialMoveHandler;

	/**
	 * Decides if the Character can lock the door
	 * @param Interacting Character
	 * @param Character location
	 * @param Character Rotation
	 * @return True if can interact, otherwise false
	 */
	UFUNCTION()
	int32 CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	/**
	 * Triggered when interacting with this actor's interactable component
	 * @param Interacting Actor
	 * @param Interaction method (Press, Hold, DoublePress)
	 */
	UFUNCTION()
	virtual void OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod);

	/** Called when locking/unlocking the door */
	UFUNCTION()
	void OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState);

	/** @return true if this door has the specified interact component */
	bool HasInteractComponent(const USCInteractComponent* Interactable) const;

	/** @return true if any interact component on this door is locked or in use. */
	bool IsInteractionLockedInUse() const;

	/** @return the closest interact component to the specified character. */
	USCInteractComponent* GetClosestInteractComponent(const AILLCharacter* Character) const;

	// ~Interaction
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Blueprint Events
public:
	// These are used in the blueprints for the grendel doors to update materials
	UFUNCTION(BlueprintImplementableEvent, Category = "Door|Interaction")
	void OnLocked(bool nNewLocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "Door|Interaction")
	void OnBarricade(bool bNewBarricade);


	//////////////////////////////////////////////////////////////////////////////////
	// Barricading
public:
	/** Returns true when we have a barricade AND it's enabled */
	UFUNCTION(BlueprintPure, Category="Door")
	FORCEINLINE bool IsBarricaded() const { return LinkedBarricade && LinkedBarricade->IsEnabled(); }

	/** Returns true when we have a barricade (enabled or not) */
	UFUNCTION(BlueprintPure, Category="Door")
	FORCEINLINE bool HasBarricade() const { return LinkedBarricade != nullptr; }

	/** Returns true if we're barricaded or locked */
	UFUNCTION(BlueprintPure, Category="Door")
	FORCEINLINE bool IsLockedOrBarricaded() const { return IsBarricaded() || bIsLocked; }

	/** Sets the barricade so that we can hook into the special move notifies */
	void SetLinkedBarricade(ASCBarricade* Barricade);

	/** Called when a barricade is applied or removed */
	UFUNCTION()
	void OnBarricadeFinished(ASCCharacter* Interactor);

protected:
	// This barricade is very close to us and will bar entry through our threshold, set via BeginPlay in either the barricade or this door
	UPROPERTY()
	ASCBarricade* LinkedBarricade;
	// ~Barricading
	//////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////////////
	// State
protected:
	// Is the door open?
	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "Door")
	bool bIsOpen;

	// Is the door locked?
	UPROPERTY(ReplicatedUsing=OnLocked, BlueprintReadOnly, Category = "Door")
	bool bIsLocked;

	// Is this door destroyed
	UPROPERTY(BlueprintReadOnly, Category = "Door")
	bool bIsDestroyed;

	// Once this actor is destroyed by some asshole with an axe, wait this many seconds to cleanup the actor
	UPROPERTY(EditDefaultsOnly, Category = "Door")
	float LifespanAfterDestruction;

	/** Make sure that doors are open or closed as they come into relevancey */
	UFUNCTION()
	virtual void OnRep_IsOpen();

	// Timer info for destroying the mesh
	FTimerHandle TimerHandle_DestroyMesh;

	/** Callback for when our DestroyMesh timer has fired */
	virtual void OnDestroyDoorMesh();

	// ~State
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Break Down
public:
	// Door break down sync point for jumping to success or not
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BreakDownDriver;

	// Door break down sync point for looping or not
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
	USCAnimDriverComponent* BreakDownLoopDriver;

	UPROPERTY()
	class ASCKillerCharacter* KillerBreakingDown;

	/** Burst special move stop */
	UFUNCTION()
	void OnBurstComplete(ASCCharacter* Interactor);

	/** Break down special move start */
	UFUNCTION()
	void OnBreakDownStart(ASCCharacter* Interactor);

	/** Break down special move stop */
	UFUNCTION()
	void OnBreakDownStop(ASCCharacter* Interactor);

	/** Used to decide if this next swing is going to be the last, and we should switch to the success state */
	UFUNCTION()
	void BreakDownSync(FAnimNotifyData NotifyData);

	/** Used to process break down damage. */
	UFUNCTION()
	void BreakDownDamage(FAnimNotifyData NotifyData);

	/** Used to decide if the player still wants to attack the door or not */
	UFUNCTION()
	void BreakDownLoop(FAnimNotifyData NotifyData);

	/** If true, we have a killer knocking at our door */
	bool IsBeingBrokenDown() const { return KillerBreakingDown != nullptr; }

private:
	// Stops KillerBreakingDown from hitting the door anymore
	bool TryAbortBreakDown(bool bForce = false);

	// If true, we shouldn't abort
	bool bBreakDownSucceeding;

	// ~Break Down
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// Utilities
public:
	UFUNCTION(BlueprintPure, Category = "Door|Interaction")
	bool IsInteractorInside(const AActor* Interactor) const;

	UFUNCTION()
	virtual void OpenCloseDoorDriverBegin(FAnimNotifyData NotifyData);

	UFUNCTION()
	virtual void OpenCloseDoorDriverTick(FAnimNotifyData NotifyData);

	UFUNCTION()
	virtual void OpenCloseDoorDriverEnd(FAnimNotifyData NotifyData);

	/** Door rotation update for anim driver */
	UFUNCTION()
	virtual void DoorKillDriverTick(FAnimNotifyData NotifyData);

	/** Listener for the end of the animation driver to reset notify owner */
	UFUNCTION()
	virtual void DoorKillDriverEnd(FAnimNotifyData NotifyData);

	UFUNCTION()
	void BurstDestroyDoor(FAnimNotifyData NotifyData);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_DestroyDoor(AActor* DamageCauser, float Impulse);

	/** Allows us to destroy the door from either an RPC or serialization of Health */
	virtual void Local_DestroyDoor(AActor* DamageCauser, float Impulse);

	UFUNCTION(BlueprintImplementableEvent, Category = "Door|Interaction")
	void OnDestroyDoor(AActor* DamageCauser, float Impulse);

	UFUNCTION(BlueprintCallable, Category = "Door")
	FORCEINLINE bool IsLocked() const { return bIsLocked; }

	UFUNCTION(BlueprintCallable, Category = "Door")
	FORCEINLINE bool IsOpen() const { return bIsOpen; }
	
	UFUNCTION(BlueprintCallable, Category = "Door")
	FORCEINLINE bool IsOpeningOrClosing() const { return bIsDoorDriverActive; }

	UFUNCTION(BlueprintCallable, Category = "Door")
	FORCEINLINE bool IsDestroyed() const { return bIsDestroyed; }

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool CanOpenDoor() const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	USCStatusIconComponent* LockedIcon;

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetDoorOpenCloseDriverOwner(AActor* InOwner);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetBurstNotifyOwner(AActor* InOwner);

	/** Set notify owner for door kill Driver. */
	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_SetKillDriverNotifyOwner(AActor* InOwner);

	/** Called when the door is done opening/closing */
	UFUNCTION()
	virtual void OpenCloseDoorCompleted(ASCCharacter* Interactor);

	/** Called when the door is canceled while opening/closing */
	UFUNCTION()
	void OpenCloseDoorAborted(ASCCharacter* Interactor);

	/** Called when we switch from normal open to fast open */
	UFUNCTION()
	void OpenDoorSpedUp(ASCCharacter* Interactor);

	/** Called from lock door special move handlers */
	UFUNCTION()
	virtual void ToggleLock(ASCCharacter* Interactor);

	UFUNCTION(NetMulticast, Reliable)
	void MULTICAST_PlaySound(USoundCue* SoundToPlay);

	/** Update destruction mesh when we cross a destruction state threshold */
	UFUNCTION()
	void UpdateDestructionState(UDestructibleMesh* NewDestructionMesh);

	/** Blueprintable callback when the destruction state is changed */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door|Interaction")
	void OnUpdateDestructionState();

	/** Update the nav area for this door based on it's state */
	void UpdateNavArea();

	// Store off our list of Interact components. Since we can have multiple and we don't know which is which we need to unlock all of them when telling the character to let go.
	UPROPERTY()
	TArray<USCInteractComponent*> InteractComponents;

private:
	float StartAngle;
	float TargetAngle;
	bool bIsDoorDriverActive;

	// ~Utilities
	//////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Badges
public:
	UPROPERTY(EditDefaultsOnly, Category = "Scoring|Badges")
	TSubclassOf<USCStatBadge> BrokeDoorDownBadge;

	// ~Badges
	//////////////////////////////////////////////////////////////////////////////////
};
