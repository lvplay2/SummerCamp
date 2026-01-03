// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/ActorComponent.h"
#include "GenericQuadTree.h"
#include "SCGVCInteractibleWidgetManager.generated.h"

UCLASS( ClassGroup="SCVCInteractible", meta=(BlueprintSpawnableComponent) )
class SUMMERCAMP_API USCGVCInteractibleWidgetManager
: public UActorComponent
{
	GENERATED_BODY()

public:
	USCGVCInteractibleWidgetManager(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface
	
	// Delay between updates, performance setting
	UPROPERTY(EditDefaultsOnly, Category = "VC InteractableManagerComponent")
	float UpdateInterval;

	// If true, will use camera rotation (true), if false will only check orientation of item to interactor
	UPROPERTY(EditDefaultsOnly, Category = "VC InteractableManagerComponent")
	bool bUseCameraRotation;

	// If true, uses actor eye position for interaction checks, if false uses actor location (generally the pelvis)
	UPROPERTY(EditDefaultsOnly, Category = "VC InteractableManagerComponent")
	bool bUseEyeLocation;

	// If true, object behind the camera cannot be used
	UPROPERTY(EditDefaultsOnly, Category = "VC InteractableManagerComponent")
	bool bMustFaceBestInteractable;

	/** Broadcasts out of range and stop highlight events for any that received it previously, and broadcasts OnBestInteractableChanged for any removed.  */
	virtual void CleanupInteractions();

	/** Gets the view location/rotation for our interacting character */
	virtual void GetViewLocationRotation(FVector& OutLocation, FRotator& OutDirection) const;

	//////////////////////////////////////////////////
	// Interactable cache
public:
	/** The reach of the trace to identify interactibles*/
	UPROPERTY(EditAnywhere, Category = "VC InteractableManagerComponent")
	float m_ProximityReach = 200;

	/** Registers and interactable component with InteractableBoundsCache and InteractableTree. */
	virtual void RegisterInteractable(USCGVCInteractibleWidget* InteractablePip);

	/** Unregisters and interactable component with InteractableBoundsCache and InteractableTree. */
	virtual void UnregisterInteractable(USCGVCInteractibleWidget* InteractablePip);

private:
	/** Delegate fired when one of our InteractableBoundsCache members has a transform change. */
	UFUNCTION()
	virtual void OnInteractableTransformChanged(USCGVCInteractibleWidget* Interactable);

	FBox2D GetMaxAABB(USCGVCInteractibleWidget* Interactable);

	// Cache of interactable component bounds
	TMap<TWeakObjectPtr<USCGVCInteractibleWidget>, FBox2D> InteractableBoundsCache;

	// Quad-tree of all interactables
	TQuadTree<TWeakObjectPtr<USCGVCInteractibleWidget>, 4> InteractableTree;

	// Cache of CharacterOwner-relevant interactables from the last tick
	TArray<TWeakObjectPtr<USCGVCInteractibleWidget>> RelevantInteractables;

	// List of in-range interactables to be pulled into RelevantInteractables so they update their in-range status after going irrelevant
	TArray<TWeakObjectPtr<USCGVCInteractibleWidget>> InRangeInteractables;

	// List of highlighted interactables to be pulled into RelevantInteractables so they update their highlight status after going irrelevant
	TArray<TWeakObjectPtr<USCGVCInteractibleWidget>> HighlightedInteractables;

	// FIXME: pjackson
	bool bHasTicked;

	//////////////////////////////////////////////////
	// Best interactable tracking
public:
	// If true, will throw out the Z/pitch component when checking distance and view direction for the best interactable
	UPROPERTY(EditDefaultsOnly, Category = "Best Interactable")
	bool bIgnoreHeightInDistanceComparison;
};
