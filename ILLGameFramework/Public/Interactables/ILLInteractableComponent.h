// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/SceneComponent.h"
#include "ILLInteractableComponent.generated.h"

class AILLCharacter;
class UILLInteractableComponent;

/**
 * @enum EILLEditorInteractMethod (bitmask)
 */
UENUM(BlueprintType, meta = (Bitflags))
enum class EILLEditorInteractMethod : uint8 // nfaletra: This enum is for editor only. Do not try to reference these values in code (they won't match the values being set from editor)
{
	EIM_Press		UMETA(DisplayName = "Press"),
	EIM_Hold		UMETA(DisplayName = "Hold"),
	EIM_DoublePress	UMETA(Displayname = "Double Press"),

	EIM_MAX			UMETA(Hidden)
};

/**
 * @enum EILLInteractMethod
 */
UENUM()
enum class EILLInteractMethod : uint8 // FIXME: pjackson: This name is bad, but changing it requires cross-project coordination (everyone adding the rename to their ini after they integrate)
{
	Press		= 0x01,
	Hold		= 0x02,
	DoublePress	= 0x04,
};

/**
 * @enum EILLHoldInteractionState
 */
UENUM(BlueprintType)
enum class EILLHoldInteractionState : uint8
{
	NONE		= 0,
	Interacting,
	Canceled,
	Success,
	
	MAX
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnTransformChanged, UILLInteractableComponent*, Interactable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FILLInteractableOnInteractDelegate, AActor*, Interactor, EILLInteractMethod, InteractMethod);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnBecameBestDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnLostBestDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnInRangeDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnOutOfRangeDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnStartHighlightDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLInteractableOnStopHighlightDelegate, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FILLInteractableOnHoldStateChangeDelegate, AActor*, Interactor, EILLHoldInteractionState, NewState);

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FVector, FILLInteractableOverrideInteractionLocationDelegate, AILLCharacter*, Character);
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(int32, FILLInteractableCanInteractWithDelegate, AILLCharacter*, Character, const FVector&, ViewLocation, const FRotator&, ViewRotation);

DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FILLInteractableShouldHighlightDelegate, AILLCharacter*, Character, const FVector&, ViewLocation, const FRotator&, ViewRotation);

/**
 * @class UILLInteractableComponent
 */
UCLASS(ClassGroup="ILLInteractable", BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ILLGAMEFRAMEWORK_API UILLInteractableComponent
: public USceneComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin UActorComponent interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent interface

	// Begin USceneComponent interface
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;
	// End USceneComponent interface

	/** @return Bounds for the interactable. */
	virtual FBox2D GetInteractionAABB() const;

	/** Helper function that attempts to unregister this component with any interactable managers. */
	virtual void UnregisterWithManagers();

	// Subscribe to this delegate to be notified of transform updates (expensive!)
	UPROPERTY()
	FILLInteractableOnTransformChanged OnTransformChanged;

private:
	/** Manages internal variables so callbacks are only made when things change */
	friend class UILLInteractableManagerComponent;

	//////////////////////////////////////////////////
	// Interaction Type
public:
	// Handles easily disabling this interaction with this component
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="ILLInteractable")
	bool bIsEnabled;

	// Allow interaction when our owning actor is hidden?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable")
	bool bInteractWhenOwnerHidden;

	// The types of interaction this component handles. See EInteractMethod for options
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable", meta=(Bitmask, BitmaskEnum="EILLEditorInteractMethod"))
	int32 InteractMethods;

	// List of specific actors classes that should not be able to interact with this component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable")
	TArray<TSoftClassPtr<AActor>> ActorIgnoreList;

	// Amount of time to hold (For Hold Interaction Method) before the interaction is complete
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable")
	float HoldTimeLimit;

	// Subscribe to this delegate to handle bevavior in the containing actor class
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable")
	FILLInteractableOnInteractDelegate OnInteract;

	// Subscribe to this delegate to handle behaviour in the containing actor class
	UPROPERTY(BlueprintAssignable, Category = "ILLInteractable")
	FILLInteractableOnHoldStateChangeDelegate OnHoldStateChanged;

	FORCEINLINE bool IsLockedInUse() const { return bIsLockedInUse; }

	/**
	 * Native callback which broadcasts the delegate for when an actor interacts with this component
	 * @param Interactor - Actor who is currently interacting with this component
	 */
	virtual void OnInteractBroadcast(AActor* Interactor, EILLInteractMethod InteractMethod);

	/**
	 * Native callback which broadcasts the delegate for when an actor has begun a hold interaction
	 * with this component.
	 * @param Interactor - Actor who is currently interacting with this component
	 */
	virtual void OnHoldStateChangedBroadcast(AActor* Interactor, EILLHoldInteractionState NewState);

	// Subscribe to this delegate to override the interaction location for this component
	UPROPERTY()
	FILLInteractableOverrideInteractionLocationDelegate OverrideInteractionLocation;

	/** Bind this delegate to add additional checking (besides distance and rotation checks) */
	UPROPERTY(BlueprintReadOnly, Category="ILLInteractable")
	FILLInteractableCanInteractWithDelegate OnCanInteractWith;

	/**
	 * True if this interactable is available for interaction, does not check range (see IsInInteractRange)
	 * @param Character - Character who wants to interact with the component
	 * @return True if this component is enabled and the character is not in the ignore list
	 */
	virtual bool IsInteractionAllowed(AILLCharacter* Character);

	/**
	 * Flags for supported interaction
	 * @param Character - Character who wants to interact with the component
	 * @param ViewLocation - Eye position of the character
	 * @param ViewRotation - The character's rotation (used for yaw/pitch look limits)
	 * @return Flags for supported interaction
	 */
	virtual int32 CanInteractWithBroadcast(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

protected:
	// True when LockInteraction is called on the InteractableManager, returned to false when UnlockInteraction is called
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ILLInteractable")
	bool bIsLockedInUse;

	//////////////////////////////////////////////////
	// Range
public:
	// How close (in cm) an actor needs to be to interact with this component. Measured from the eyes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Range")
	float DistanceLimit;

	// If true the player must be looking at this component to use it
	UPROPERTY(BlueprintReadOnly, Category="ILLInteractable|Range", meta=(PinHiddenByDefault))
	bool bUseYawLimit;

	// Angle in degrees the character rotation may be offset from the direction towards this component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Range", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0", EditCondition="bUseYawLimit"))
	float MaxYawOffset;

	// If true the player must be looking at this component to use it
	UPROPERTY(BlueprintReadOnly, Category="ILLInteractable|Range", meta=(PinHiddenByDefault))
	bool bUsePitchLimit;

	// Angle in degrees the character rotation may be offset from the direction towards this component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Range", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0", EditCondition="bUsePitchLimit"))
	float MaxPitchOffset;

	// Called when this interactable component comes into range/view limits of an actor
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable|Range")
	FILLInteractableOnInRangeDelegate OnInRange;
	
	/**
	 * Native callback which broadcasts the delegate for when an actor comes into rage of this component
	 * @param Interactor - Actor who has entered the range of this component
	 */
	virtual void OnInRangeBroadcast(AActor* Interactor);

	// Called when this interactable component leaves range/view limits of an actor
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable|Range")
	FILLInteractableOnOutOfRangeDelegate OnOutOfRange;

	/**
	 * Native callback which broadcasts the delegate for when an actor leaves rage of this component
	 * @param Interactor - Actor who is leaving range of this component
	 */
	virtual void OnOutOfRangeBroadcast(AActor* Interactor);

	/**
	 * True if the component is in range for interaction, does not check class limitations (see CanInteractWith)
	 * @param Character - Character to check against
	 * @param ViewLocation - Position of the interacting character's eyes
	 * @param ViewRotation - Rotation of the interacting character
	 */
	virtual bool IsInInteractRange(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation, const bool bUseCameraRotation);

	/** @return Location to interact with this component from. */
	virtual FVector GetInteractionLocation(AILLCharacter* Character) const;

	//////////////////////////////////////////////////
	// Occlusion
public:
	// If true, will perform a simple raytrace to make sure this object is not occluded from the player (based off of the actors eye position), checks for both highlighting and interaction
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInteractable|Occlusion")
	bool bCheckForOcclusion;

	// If true, will perform a more expensive occlusion ray trace against polygon rather than collision geometery
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInteractable|Occlusion", meta=(EditCondition="bCheckForOcclusion"))
	bool bPerformComplexOcclusionCheck;

	// Offset from the actors eye position along the actors Z axis to begin the occlusion trace
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInteractable|Occlusion", meta=(EditCondition="bCheckForOcclusion"))
	float RayTraceZOffset;

	// Collision channel to use for occlusion
	UPROPERTY(EditDefaultsOnly, Category = "ILLInteractable|Occlusion", meta=(EditCondition="bCheckForOcclusion"))
	TEnumAsByte<ECollisionChannel> RayTraceChannel;

	// List of actor classes to ignore while checking for occlusion
	UPROPERTY(EditDefaultsOnly, Category = "ILLInteractable|Occlusion", meta=(EditCondition="bCheckForOcclusion"))
	TArray<TSoftClassPtr<AActor>> IgnoreClasses;

	/** Performs raytrace if occlusion has not been updated this frame, otherwise returns bIsOccluded */
	UFUNCTION(BlueprintCallable, Category = "ILLInteractable|Occlusion")
	bool IsOccluded(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

	//////////////////////////////////////////////////
	// Highlight
public:
	// If true, will do Highlight check using HighlightDistance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Highlight")
	bool bCanHighlight;

	// If true, this interactable will be hidden in game while it is locked onto.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ILLInteractable|Highlight")
	bool bHideWhileLocked;

	// Minimum distance (in cm) for highlight callback to be triggered, when inside this distance HighlightZDistance and bCheckForOcclusion will also be checked
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Highlight", meta=(EditCondition="bCanHighlight"))
	float HighlightDistance;

	// If true, HighlightZDistance will also be checked while this object is within the HighlightDistance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Highlight", meta=(PinHiddenByDefault))
	bool bUseSeperateZBounds;

	// Optional seperate world Z distance (in cm) so things on different levels don't highlight, can be larger than HighlightDistance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Highlight", meta=(EditCondition="bUseSeperateZBounds"))
	float HighlightZDistance;
	
	// If true the player must also be within the yaw limit for this object to highlight
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Highlight", meta=(PinHiddenByDefault))
	bool bUseYawHighlightLimit;

	// Angle in degrees the character rotation may be offset from the direction towards this component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ILLInteractable|Highlight", meta=(EditCondition="bUseYawHighlightLimit"))
	float YawHighlightLimit;

	// Called when testing if the interactable should highlight
	UPROPERTY(BlueprintReadOnly, Category = "ILLInteractable|Highlight")
	FILLInteractableShouldHighlightDelegate OnShouldHighlight;

	// Called when all highlight criteria is met (distance/occlusion/Z-Height)
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable|Highlight")
	FILLInteractableOnStartHighlightDelegate OnStartHighlight;

	/**
	 * Native callback which broadcasts the delegate when all highlight criteria is met (distance/occlusion/Z-Height)
	 * @param Interactor - Actor who wants the highlight
	 */
	virtual void StartHighlightBroadcast(AActor* Interactor);

	// Called when not all highlight criteria is met (distance/occlusion/Z-Height)
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable|Highlight")
	FILLInteractableOnStopHighlightDelegate OnStopHighlight;

	/**
	 * Native callback which broadcasts the delegate when not all highlight criteria is met (distance/occlusion/Z-Height)
	 * @param Interactor - Actor who no longer wants the highlight
	 */
	virtual void StopHighlightBroadcast(AActor* Interactor);

	/**
	 * True if this interactable passes all highlight checks (enabled, range, Z-height and Occlusion)
	 * @param Character - Character who is entering highlight range
	 * @param ViewLocator - Position of the character's eyes
	 * @param ViewRotation - Rotation of the character
	 * @return True if this component feels like it should be highlighted
	 */
	virtual bool ShouldHighlight(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);
	
	//////////////////////////////////////////////////
	// Best interactable notification
public:
	// Called when this interactable has became the best interactable for Actor
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable|Best")
	FILLInteractableOnBecameBestDelegate OnBecameBest;

	/** Broadcast OnBecameBest. */
	virtual void BroadcastBecameBest(AActor* Interactor) { OnBecameBest.Broadcast(Interactor); }

	// Called when this interactable lost being the best interactable for Actor
	UPROPERTY(BlueprintAssignable, Category="ILLInteractable|Best")
	FILLInteractableOnLostBestDelegate OnLostBest;

	/** Broadcast OnLostBest. */
	virtual void BroadcastLostBest(AActor* Interactor) { OnLostBest.Broadcast(Interactor); }
};
