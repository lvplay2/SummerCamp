// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/WidgetComponent.h"
#include "SCGVCInteractibleWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCVCInteractableOnInRangeDelegate, ASCGVCCharacter*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCVCInteractableOnOutOfRangeDelegate, ASCGVCCharacter*, Interactor);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FVector, FSCVCInteractableOverrideInteractionLocationDelegate, ASCGVCCharacter*, Character);


UCLASS()
class SUMMERCAMP_API USCGVCInteractibleWidget
: public UWidgetComponent
{
	GENERATED_BODY()

public:
	USCGVCInteractibleWidget(const FObjectInitializer& Objectinitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VC Interactible Component")
	float m_FadeDuration = 0.2f;

	UFUNCTION(BlueprintImplementableEvent, Category = "VC Interactible Component")
	void OnShow();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "VC Interactible Component")
	void OnHide();

	//////////////////////////

	// Subscribe to this delegate to override the interaction location for this component
	UPROPERTY()
	FSCVCInteractableOverrideInteractionLocationDelegate OverrideInteractionLocation;

	// How close (in cm) an actor needs to be to interact with this component. Measured from the eyes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Range")
	float DistanceLimit;

	// If true the player must be looking at this component to use it
	UPROPERTY(BlueprintReadOnly, Category = "VC Interactible|Range", meta = (PinHiddenByDefault))
	bool bUseYawLimit;

	// Angle in degrees the character rotation may be offset from the direction towards this component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Range", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0", EditCondition = "bUseYawLimit"))
	float MaxYawOffset;

	// If true the player must be looking at this component to use it
	UPROPERTY(BlueprintReadOnly, Category = "VC Interactible|Range", meta = (PinHiddenByDefault))
	bool bUsePitchLimit;

	// Angle in degrees the character rotation may be offset from the direction towards this component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Range", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0", EditCondition = "bUsePitchLimit"))
	float MaxPitchOffset;

	// Called when this interactable component comes into range/view limits of an actor
	UPROPERTY(BlueprintAssignable, Category = "VC Interactible|Range")
	FSCVCInteractableOnInRangeDelegate OnInRange;

	/**
	* Native callback which broadcasts the delegate for when an actor comes into rage of this component
	* @param Interactor - Actor who has entered the range of this component
	*/
	virtual void OnInRangeBroadcast(ASCGVCCharacter* Interactor);

	// Called when this interactable component leaves range/view limits of an actor
	UPROPERTY(BlueprintAssignable, Category = "VC Interactible|Range")
	FSCVCInteractableOnOutOfRangeDelegate OnOutOfRange;

	/**
	* Native callback which broadcasts the delegate for when an actor leaves rage of this component
	* @param Interactor - Actor who is leaving range of this component
	*/
	virtual void OnOutOfRangeBroadcast(ASCGVCCharacter* Interactor);

	/**
	* True if the component is in range for interaction, does not check class limitations (see CanInteractWith)
	* @param Character - Character to check against
	* @param ViewLocation - Position of the interacting character's eyes
	* @param ViewRotation - Rotation of the interacting character
	*/
	virtual bool IsInInteractRange(ASCGVCCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation, const bool bUseCameraRotation);

	/** @return Location to interact with this component from. */
	virtual FVector GetInteractionLocation(ASCGVCCharacter* Character) const;

	// Managed by the InteractableManager to determine if InRange/OutOfRange callbacks should be made
	UPROPERTY(BlueprintReadOnly, Category = "VC Interactible|Range")
	bool bInRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible Pip")
	bool m_ShowInteractibleEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VC Interactible Pip")
	bool m_ShowBackPackPip = false;

	// If true, will perform a simple raytrace to make sure this object is not occluded from the player (based off of the actors eye position), checks for both highlighting and interaction
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Occlusion")
	bool bCheckForOcclusion;

	// Number of times per second occlusion should be checked (when enabled), lower values can provide a large performance increase
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Occlusion", meta = (EditCondition = "bCheckForOcclusion"))
	float OcclusionUpdateFrequency;

	// If true, will perform a more expensive occlusion ray trace against polygon rather than collision geometery
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Occlusion", meta = (EditCondition = "bCheckForOcclusion"))
	bool bPerformComplexOcclusionCheck;

	// Offset from the actors eye position along the actors Z axis to begin the occlusion trace
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VC Interactible|Occlusion", meta = (EditCondition = "bCheckForOcclusion"))
	float RayTraceZOffset;

	// Collision channel to use for occlusion
	UPROPERTY(EditDefaultsOnly, Category = "VC Interactible|Occlusion", meta = (EditCondition = "bCheckForOcclusion"))
	TEnumAsByte<ECollisionChannel> RayTraceChannel;

	// List of actor classes to ignore while checking for occlusion
	UPROPERTY(EditDefaultsOnly, Category = "VC Interactible|Occlusion", meta = (EditCondition = "bCheckForOcclusion"))
	TArray<TSubclassOf<AActor>> IgnoreClasses;

	// Should be called once a frame by the InteractableManagerComponent to force a refresh of occlusion
	FORCEINLINE void SetOcclusionDirty() { bIsOcclusionDirty = true; }

	/** Performs raytrace if occlusion has not been updated this frame, otherwise returns bIsOccluded */
	UFUNCTION(BlueprintCallable, Category = "VC Interactible|Occlusion")
	bool IsOccluded(ASCGVCCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation);

private:
	bool bIsOcclusionDirty;
	// Internal value to keep track of occlusion between updates, managed by OcclusiontUpdateFrequency
	bool bIsOccluded;
	// Internal value to check if we should update the occluded this frame, if not will just use bIsOccluded
	float LastOcclusionCheck;
};
