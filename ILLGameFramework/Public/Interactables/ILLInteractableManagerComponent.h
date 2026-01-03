// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/ActorComponent.h"
#include "GenericQuadTree.h"
#include "ILLInteractableManagerComponent.generated.h"

class UILLInteractableComponent;

enum class EILLInteractMethod : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FILLInteractableManagerOnBestInteractableComponentChanged, UILLInteractableComponent*, OldInteractable, UILLInteractableComponent*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FILLInteractableManagerOnClientLockedInteractable, UILLInteractableComponent*, LockingInteractable, const EILLInteractMethod, InteractMethod, const bool, bSuccess);

extern ILLGAMEFRAMEWORK_API TAutoConsoleVariable<int32> CVarILLDebugInteractables;

/**
 * @class UILLInteractableManagerComponent
 */
UCLASS(Within=ILLCharacter, ClassGroup="ILLInteractable", meta=(BlueprintSpawnableComponent))
class ILLGAMEFRAMEWORK_API UILLInteractableManagerComponent
: public UActorComponent
{
	GENERATED_BODY()

public:
	UILLInteractableManagerComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface
	
	// If true, will use camera rotation (true), if false will only check orientation of item to interactor
	UPROPERTY(EditDefaultsOnly, Category = "InteractableManagerComponent")
	bool bUseCameraRotation;

	// If true, uses actor eye position for interaction checks, if false uses actor location (generally the pelvis)
	UPROPERTY(EditDefaultsOnly, Category = "InteractableManagerComponent")
	bool bUseEyeLocation;

	// If true, object behind the camera cannot be used
	UPROPERTY(EditDefaultsOnly, Category = "InteractableManagerComponent")
	bool bMustFaceBestInteractable;

	/** Polls for interact-ability with relevant components, updating BestInteractable and friends. Called from TickComponent. */
	virtual void PollInteractions();

	/** Broadcasts out of range and stop highlight events for any that received it previously, and broadcasts OnBestInteractableChanged for any removed.  */
	virtual void CleanupInteractions();

	/** Gets the view location/rotation for our interacting character */
	virtual void GetViewLocationRotation(FVector& OutLocation, FRotator& OutDirection) const;

protected:
	// Delay between updates, performance setting
	float UpdateInterval;
	float AIUpdateInterval;

	// DeltaTime eats away at this until an update is allowed
	float TimeUntilUpdate;

	//////////////////////////////////////////////////
	// Interactable cache
public:
	/** Registers and interactable component with InteractableBoundsCache and InteractableTree. */
	virtual void RegisterInteractable(UILLInteractableComponent* Interactable);

	/** Unregisters and interactable component with InteractableBoundsCache and InteractableTree. */
	virtual void UnregisterInteractable(UILLInteractableComponent* Interactable);

private:
	/** Delegate fired when one of our InteractableBoundsCache members has a transform change. */
	void OnInteractableTransformChanged(USceneComponent* RootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	// Cache of interactable component bounds
	TMap<TWeakObjectPtr<UILLInteractableComponent>, FBox2D> InteractableBoundsCache;

	// Quad-tree of all interactables
	TQuadTree<TWeakObjectPtr<UILLInteractableComponent>, 4> InteractableTree;

	// Interactables that need to be re-added to the tree due to transform changes
	TArray<TWeakObjectPtr<UILLInteractableComponent>> DirtyInteractables;

	// Cache of CharacterOwner-relevant interactables from the last tick
	TArray<TWeakObjectPtr<UILLInteractableComponent>> RelevantInteractables;

	// List of in-range interactables to be pulled into RelevantInteractables so they update their in-range status after going irrelevant
	TArray<TWeakObjectPtr<UILLInteractableComponent>> InRangeInteractables;

	// List of highlighted interactables to be pulled into RelevantInteractables so they update their highlight status after going irrelevant
	TArray<TWeakObjectPtr<UILLInteractableComponent>> HighlightedInteractables;

	// FIXME: pjackson
	bool bHasTicked;

	//////////////////////////////////////////////////
	// Best interactable tracking
public:
	// If true, will throw out the Z/pitch component when checking distance and view direction for the best interactable
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable")
	bool bIgnoreHeightInDistanceComparison;

	// How much emphasis should distance be on selecting the best interactable, combined weights may be larger than 1
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable", meta=(ClampMin=0.0, UIMin=0.0, ClampMax=1.0, UIMax=1.0))
	float DistanceWeight;

	// How much emphasis should view direction be on selecting the best interactable, combined weights may be larger than 1
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable", meta=(ClampMin=0.0, UIMin=0.0, ClampMax=1.0, UIMax=1.0))
	float ViewWeight;

	// How much emphasis should charater body direction be on selecting the best interactable, combined weights may be larger than 1
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable", meta=(ClampMin=0.0, UIMin=0.0, ClampMax=1.0, UIMax=1.0))
	float CharacterRotationWeight;

	// How much emphasis should movement direction be on selecting the best interactable (ignored when standing still), combined weights may be larger than 1
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable", meta=(ClampMin=0.0, UIMin=0.0, ClampMax=1.0, UIMax=1.0))
	float MovementDirectionWeight;

	// The old best interactable will get this bump to try and keep the interactable stable rather than switching
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable", meta=(ClampMin=0.0, UIMin=0.0, ClampMax=1.0, UIMax=1.0))
	float BestInteractableStabilityScore;

	/** Best Interactable Component we can interact with */
	FORCEINLINE UILLInteractableComponent* GetBestInteractableComponent() const { return LockedComponent ? LockedComponent : BestInteractableComponent; }

	/** Bitmask of available interaction methods for the best interactable component */
	virtual int32 GetBestInteractableMethodFlags() const;

	/** Best Actor we can interact with. */
	virtual AActor* GetBestInteractableActor() const;

	// Interactable changed delegate
	UPROPERTY()
	FILLInteractableManagerOnBestInteractableComponentChanged OnBestInteractableComponentChanged;

protected:
	/** Triggers OnBestInteractableComponentChanged then notifies OldInteractable and NewInteractable. */
	virtual void BroadcastBestInteractable(UILLInteractableComponent* OldInteractable, UILLInteractableComponent* NewInteractable);

	// Last set of best interactable targets (includes one of each interact type)
	UPROPERTY(Transient)
	UILLInteractableComponent* BestInteractableComponent;

	// Last found flags
	UPROPERTY(Transient)
	int32 BestInteractableMethodFlags;

	//////////////////////////////////////////////////
	// Interaction locking
public:
	// TODO: Add support for locking interaction with more than one component? API comes across as allowing it right now (it does not)

	/**
	 * Used to set the active component and lock interaction with any other component if requested component is not already locked, cleans up all other interactions and causes BestInteractableComponentChanged to be called
	 * @param Component - Component that interaction has begun on (LockedComponent will be set to this if none is set)
	 * @param bForce - Forces the interaction, overriding LockedComponent if set
	 * @return False if LockedComponent is already set and bForce is false, or the passed in Component is null or locked
	 */
	UFUNCTION(BlueprintCallable, Category = "InteractableManagerComponent")
	virtual bool LockInteraction(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bForce = false);

	/**
	 * Ends interaction with the LockedComponent, cleans up all other interactions and causes BestInteractableComponentChanged to be called
	 * @param Component - Component we want to unlock interaction with, MUST be the one we used when calling LockInteraction
	 * @param bForce - Bypasses the component check and clears whatever LockComponent we have currently
	 */
	UFUNCTION(BlueprintCallable, Category = "InteractableManagerComponent")
	virtual void UnlockInteraction(UILLInteractableComponent* Component, const bool bForce = false);

	/**
	 * Locks the passed in interaction locally (does not network, does not set bIsLocked on the component)
	 * This is useful for an interaction you want to stay available but don't want to block
	 * @param Component - Component we want to maintain interaction with
	 * @return False when the component is not set to the LockedComponent, this can happen when another component is locked or the component passed in is already locked
	 */
	UFUNCTION(BlueprintCallable, Category = "InteractableManagerComponent")
	virtual bool LockInteractionLocally(UILLInteractableComponent* Component);

	/** 
	 * Attempt to interact with the given interact component 
	 * @param Component Component that we want to interact with.
	 * @param InteractMethod Interaction method to use if we can interact with the component
	 */
	UFUNCTION()
	virtual void AttemptInteract(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, bool bAutoLock = false);

	/** Gets the active interaction component */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractableManagerComponent")
	virtual UILLInteractableComponent* GetLockedInteractable() const { return LockedComponent; }

	// Called whenever CLIENT_LockInteraction is called, if the second param is false the lock failed and the client should do something else with its life.
	UPROPERTY()
	FILLInteractableManagerOnClientLockedInteractable OnClientLockedInteractable;

protected:
	/** Called on server to ensure interaction locked is serialized */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_SetInteractionLocked(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bShouldLock);

	/**
	 * Client call to notify the owner that they now have a locked interact component
	 * @param Component Component that the interaction has begin on (Locked Component will be set to this)
	 * @param bCanLock True if the server has locked the component for us.
	 */
	UFUNCTION(Client, Reliable)
	virtual void CLIENT_LockInteraction(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bCanLock);

	/**
	 * Client call to notify the owner that they have unlocked the locked interact component
	 * @param Component Component that the interaction has been released from
	 * @param bCanLock True if the server has locked the component for us.
	 */
	UFUNCTION(Client, Reliable)
	virtual void CLIENT_UnlockInteraction(UILLInteractableComponent* Component);

	/** Called on the server to ensure we're checking against the most up to date lock state */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void SERVER_AttemptInteract(UILLInteractableComponent* Component, EILLInteractMethod InteractMethod, bool bAutoLock = false);

	/** Update our LockedInteractable and BestInteractable */
	UFUNCTION()
	void UpdateLockComponent(class AILLCharacter* OwnerCharacter);

	// Component the player is currently locked into using, prevents interaction with other components and pretends to be the best interactable
	UPROPERTY(Transient)
	UILLInteractableComponent* LockedComponent;

	//////////////////////////////////////////////////
	// Debug
protected:
	/** Draws debug info for the manager */
	void DebugDrawInfo() const;

	/** Draws interactables for debug purposes */
	void DebugDrawInteractable(const UILLInteractableComponent* Interactable, const AActor* Interactor, FColor Color) const;
};
