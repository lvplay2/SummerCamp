// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLInteractableManagerComponent.h"

#include "AIController.h"

#include "ILLCharacter.h"
#include "ILLInteractableComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogILLInteract, Log, All);

const static FColor DEBUG_COLOR(255, 187, 187, 255);

#ifndef INTERACTABLE_TREE_TILESIZE
# define INTERACTABLE_TREE_TILESIZE 200000.f
#endif

#ifndef INTERACTABLE_TREE_MINQUADSIZE
# define INTERACTABLE_TREE_MINQUADSIZE 1000.f
#endif

#ifndef INTERACTABLE_BOUNDS_SLACK
# define INTERACTABLE_BOUNDS_SLACK (INTERACTABLE_TREE_MINQUADSIZE*.5f)
#endif

#ifndef RELEVANT_INTERACTABLE_SLACK
# define RELEVANT_INTERACTABLE_SLACK 8
#endif

TAutoConsoleVariable<int32> CVarILLDebugInteractablesShowAll(
	TEXT("ill.DebugInteractablesShowAll"),
	0,
	TEXT("If enabled, will show dark red interactled (blocked due to actor type)")
);

TAutoConsoleVariable<int32> CVarILLDebugInteractables(
	TEXT("ill.DebugInteractables"),
	0,
	TEXT("Displays debug information for interactables, use ill.DebugInteractablesRange to manage display range\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n")
	TEXT(" 2: Show Ray Traces\n")
	TEXT(" 3: Show Names (uses same colors as below, setting a range is HIGHLY recommended)\n")
	TEXT(" 4: Also show names for occlusion\n\n")
	TEXT("Dark Red cannot be interacted with.\n")
	TEXT("Red is out of range.\n")
	TEXT("Yellow has no declared interactions (Press/Hold/Double Press).\n")
	TEXT("Green can be interacted with.\n")
	TEXT("Cyan spheres are where a raytrace for occlusion is colliding and failing the interaction test.\n")
	TEXT("White shows when an interactable is locked in use with a player.\n")
	TEXT("The pulsing sphere is the current BestInteractableComponent.\n")
);

TAutoConsoleVariable<float> CVarILLDebugInteractablesRange(
	TEXT("ill.DebugInteractablesRange"),
	15.f,
	TEXT("Used to limit the range at which interactables are drawn\n")
	TEXT(" < 0: No limit)\n")
	TEXT(" 0: Only show in-range interactables\n")
	TEXT(" > 0: Range in meters to draw interactables (defaults to 15m)\n")
);

FBox2D GetMaxAABB(UILLInteractableComponent* Interactable)
{
	const float Radius = FMath::Max(Interactable->DistanceLimit, Interactable->HighlightDistance) + INTERACTABLE_BOUNDS_SLACK;
	const FVector2D Location(Interactable->GetComponentLocation());
	const FVector2D Offset(Radius, Radius);
	const FBox2D AABB(Location - Offset, Location + Offset);
	return AABB;
}

FString InteractableToString(const UILLInteractableComponent* Interactable)
{
	if (!Interactable)
		return TEXT("NONE");

	return FString::Printf(TEXT("%s:%s"), *GetNameSafe(Interactable->GetOwner()), *Interactable->GetName());
}

UILLInteractableManagerComponent::UILLInteractableManagerComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bUseCameraRotation(true)
, bUseEyeLocation(true)

// Interactable quad tree
, InteractableTree(FBox2D(FVector2D(-INTERACTABLE_TREE_TILESIZE, -INTERACTABLE_TREE_TILESIZE), FVector2D(INTERACTABLE_TREE_TILESIZE, INTERACTABLE_TREE_TILESIZE)), INTERACTABLE_TREE_MINQUADSIZE)

// Best interactable tracking
, bIgnoreHeightInDistanceComparison(true)
, DistanceWeight(0.9f)
, ViewWeight(0.7f)
, CharacterRotationWeight(0.5f)
, MovementDirectionWeight(0.8f)
, BestInteractableStabilityScore(0.1f)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bAutoActivate = true;

	UpdateInterval = 1.f/8.f; // 8FPS
	AIUpdateInterval = 1.f/3.f; // 3FPS
}

void UILLInteractableManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	SetComponentTickEnabled(false);
}

void UILLInteractableManagerComponent::UninitializeComponent()
{
	Super::UninitializeComponent();

	// Cleanup previous interactions
	CleanupInteractions();

	UnlockInteraction(LockedComponent);
}

void UILLInteractableManagerComponent::OnUnregister()
{
	Super::OnUnregister();

	InteractableBoundsCache.Empty();
	InteractableTree.Empty();
	RelevantInteractables.Empty();
}

void UILLInteractableManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
		return;

	// Don't throttle updates if we're debugging, makes for flickering lines/text
	if (CVarILLDebugInteractables.GetValueOnGameThread() < 1)
	{
		TimeUntilUpdate -= DeltaTime;
		if (TimeUntilUpdate > 0.f)
		{
			return;
		}
	}

	PollInteractions();
}

void UILLInteractableManagerComponent::PollInteractions()
{
	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
		return;

	// Delay our next update from TickComponent
	UWorld* World = GetWorld();
	TimeUntilUpdate = Cast<AAIController>(OwnerCharacter->Controller) ? AIUpdateInterval : UpdateInterval;

	// OPTIMIZE: pjackson: Gross.
	if (!bHasTicked)
	{
		for (TObjectIterator<UILLInteractableComponent> It; It; ++It)
		{
			UILLInteractableComponent* Interactable = *It;
			if (Interactable->GetWorld() == World)
			{
				RegisterInteractable(Interactable);
			}
		}
		bHasTicked = true;
	}

	if (LockedComponent)
	{
		if (LockedComponent->bHideWhileLocked)
			CleanupInteractions();

		FVector ViewLocation;
		FRotator ViewRotation;
		GetViewLocationRotation(ViewLocation, ViewRotation);
		const int32 OldFlags = BestInteractableMethodFlags;
		BestInteractableMethodFlags = LockedComponent->CanInteractWithBroadcast(OwnerCharacter, ViewLocation, ViewRotation);
		if (BestInteractableMethodFlags != OldFlags)
		{
			if (BestInteractableMethodFlags > 0)
			{
				if (BestInteractableComponent != LockedComponent)
				{
					BroadcastBestInteractable(BestInteractableComponent, LockedComponent);
					BestInteractableComponent = LockedComponent;
				}
			}
			else if (BestInteractableComponent == LockedComponent)
			{
				BroadcastBestInteractable(BestInteractableComponent, nullptr);
				BestInteractableComponent = nullptr;
			}
		}
		return;
	}

	if (!OwnerCharacter->CanInteractAtAll())
	{
		CleanupInteractions();
		return;
	}

	// Update dirty interactables
	for (int32 DirtyIndex = 0; DirtyIndex < DirtyInteractables.Num(); )
	{
		if (!DirtyInteractables[DirtyIndex].IsValid())
		{
			DirtyInteractables.RemoveAt(DirtyIndex);
			continue;
		}

		UILLInteractableComponent* Interactable = DirtyInteractables[DirtyIndex].Get();
		const FBox2D MaxAABB = GetMaxAABB(Interactable);

		if (FBox2D* OldAABB = InteractableBoundsCache.Find(Interactable))
		{
			// Are we within location tolerance?
			if ((OldAABB->GetCenter()-MaxAABB.GetCenter()).SizeSquared() <= FMath::Square(INTERACTABLE_BOUNDS_SLACK))
			{
				// And are we within size tolerance?
				if (FMath::Abs(OldAABB->GetArea()-MaxAABB.GetArea()) <= FMath::Square(INTERACTABLE_BOUNDS_SLACK))
				{
					// Hold off for more movement
					++DirtyIndex;
					continue;
				}
			}

			// Remove it and update the InteractableBoundsCache
			InteractableTree.Remove(Interactable, *OldAABB);
			*OldAABB = MaxAABB;
		}

		// Re-insert it into the quad tree
		InteractableTree.Insert(Interactable, MaxAABB);
		DirtyInteractables.RemoveAt(DirtyIndex);
	}

	UILLInteractableComponent* NewBestInteractableComponent = nullptr;
	int32 NewBestInteractableMethodFlags = 0;

	// Get orientation and owner info
	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewLocationRotation(ViewLocation, ViewRotation);
	FVector CameraLocation;
	if (Cast<AAIController>(OwnerCharacter->Controller))
		CameraLocation = ViewLocation;
	else
		CameraLocation = OwnerCharacter->GetCachedCameraInfo().Location;
	FVector ViewDirection = ViewRotation.Vector();
	FVector CharacterDirection = OwnerCharacter->GetActorRotation().Vector();
	FVector MovementDirection = OwnerCharacter->GetVelocity();
	if (bIgnoreHeightInDistanceComparison)
	{
		ViewDirection.Z = CharacterDirection.Z = MovementDirection.Z = 0.f;
		ViewDirection.Normalize();
		CharacterDirection.Normalize();
	}
	const bool bIsMoving = MovementDirection.SizeSquared() > KINDA_SMALL_NUMBER;
	if (bIsMoving)
	{
		MovementDirection.Normalize();
	}

	// Update RelevantInteractables
	RelevantInteractables.Empty(FMath::Max(RelevantInteractables.Num()+InRangeInteractables.Num()+HighlightedInteractables.Num(), RELEVANT_INTERACTABLE_SLACK));
	if (CVarILLDebugInteractables.GetValueOnGameThread() < 1 || CVarILLDebugInteractablesRange.GetValueOnGameThread() == 0.0f)
	{
		// Collect RelevantInteractables based on our character's bounds
		const FVector2D OwnerLocation(OwnerCharacter->GetActorLocation());
		const FVector2D OwnerSize(OwnerCharacter->GetSimpleCollisionRadius(), OwnerCharacter->GetSimpleCollisionHalfHeight()*2.f);
		const FBox2D OwnerAABB(OwnerLocation - OwnerSize, OwnerLocation + OwnerSize);
		InteractableTree.GetElements(OwnerAABB, RelevantInteractables);

		// Add previously in-range and highlighted interactables so their status updates
		for (TWeakObjectPtr<UILLInteractableComponent> PreviouslyInRange : InRangeInteractables)
			RelevantInteractables.AddUnique(PreviouslyInRange);
		for (TWeakObjectPtr<UILLInteractableComponent> PreviouslyHighlighted : HighlightedInteractables)
			RelevantInteractables.AddUnique(PreviouslyHighlighted);
	}
	else
	{
		for (const TPair<TWeakObjectPtr<UILLInteractableComponent>, FBox2D>& Entry : InteractableBoundsCache)
		{
			RelevantInteractables.Add(Entry.Key);
		}
	}

	// Determine the best interactable
	float BestScore = -FLT_MAX;
	for (int32 InteractableIndex = 0; InteractableIndex < RelevantInteractables.Num(); ++InteractableIndex)
	{
		UILLInteractableComponent* Interactable = RelevantInteractables[InteractableIndex].Get();
		if (!IsValid(Interactable))
			continue;

		if (!Interactable->IsInteractionAllowed(OwnerCharacter))
		{
			if (InRangeInteractables.Contains(Interactable))
			{
				Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
				InRangeInteractables.Remove(Interactable);
			}

			if (HighlightedInteractables.Contains(Interactable))
			{
				Interactable->StopHighlightBroadcast(OwnerCharacter);
				HighlightedInteractables.Remove(Interactable);
			}

			if (CVarILLDebugInteractablesShowAll.GetValueOnGameThread() >= 1)
				DebugDrawInteractable(Interactable, OwnerCharacter, FColor(64, 0, 0));

			continue;
		}

		const FVector Location = Interactable->GetInteractionLocation(OwnerCharacter);
		FVector PlayerToComponent = Location - ViewLocation;
		if (bIgnoreHeightInDistanceComparison)
		{
			PlayerToComponent.Z = 0.f;
		}

		PlayerToComponent.Normalize();
		const float CameraDot = (Location - CameraLocation).GetSafeNormal() | ViewDirection;

		// Components locked by other players shouldn't highlight/be usable
		const bool bLockedByOtherPlayer = Interactable->bIsLockedInUse && LockedComponent != Interactable;
		const int32 InteractableMethodFlags = bLockedByOtherPlayer ? 0 : Interactable->CanInteractWithBroadcast(OwnerCharacter, ViewLocation, ViewRotation);

		// Perform highlight check
		bool bShouldHighlight = false;
		if (bMustFaceBestInteractable == false || CameraDot > 0.f)
		{
			bShouldHighlight = Interactable->ShouldHighlight(OwnerCharacter, ViewLocation, ViewRotation) && InteractableMethodFlags != 0;
		}

		if (HighlightedInteractables.Contains(Interactable) != bShouldHighlight)
		{
			if (bShouldHighlight)
			{
				Interactable->StartHighlightBroadcast(OwnerCharacter);
				HighlightedInteractables.AddUnique(Interactable);
			}
			else
			{
				Interactable->StopHighlightBroadcast(OwnerCharacter);
				HighlightedInteractables.Remove(Interactable);
			}
		}

		// Perform range checks
 		if (Interactable->IsInInteractRange(OwnerCharacter, ViewLocation, ViewRotation, bUseCameraRotation) == false)
		{
			if (InRangeInteractables.Contains(Interactable))
			{
				Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
				InRangeInteractables.Remove(Interactable);
			}

			DebugDrawInteractable(Interactable, OwnerCharacter, FColor::Red);

			continue;
		}

		if (!InRangeInteractables.Contains(Interactable))
		{
			Interactable->OnInRangeBroadcast(OwnerCharacter);
			InRangeInteractables.AddUnique(Interactable);
		}

		if (InteractableMethodFlags == 0)
		{
			DebugDrawInteractable(Interactable, OwnerCharacter, FColor::Yellow);
			continue;
		}

		// Should the object be in front of the camera?
		if (bMustFaceBestInteractable == false || CameraDot > 0.f)
		{
			// Scale by the component's own range
			const float Distance2D = (ViewLocation - Location).Size2D();
			const float DistanceScore = 1.f - (Distance2D / Interactable->DistanceLimit);

			// Directional scores
			const float ViewDot = PlayerToComponent | ViewDirection;
			const float CharacterDot = PlayerToComponent | CharacterDirection;
			const float VelocityDot = bIsMoving ? PlayerToComponent | MovementDirection : 0.f;

			float OverallScore = (DistanceScore * DistanceWeight) + (ViewDot * ViewWeight) + (CharacterDot * CharacterRotationWeight) + (VelocityDot * MovementDirectionWeight);

			// Stability!
			if (Interactable == BestInteractableComponent)
				OverallScore += BestInteractableStabilityScore;

			if (CVarILLDebugInteractables.GetValueOnGameThread() > 1)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan,
					FString::Printf(TEXT("%s @ %.3f (%.3f, %.3f, %.3f, %.3f)"),
						*InteractableToString(Interactable), OverallScore,
						Distance2D, ViewDot, CharacterDot, VelocityDot),
					false);
			}

			if (OverallScore > BestScore)
			{
				BestScore = OverallScore;
				NewBestInteractableComponent = Interactable;
				NewBestInteractableMethodFlags = InteractableMethodFlags;
			}
		}

		// Draw regardless
		DebugDrawInteractable(Interactable, OwnerCharacter, FColor::Green);
	}

	// Broadcast events
	UILLInteractableComponent* OldBestInteractableComponent = GetBestInteractableComponent();
	BestInteractableMethodFlags = NewBestInteractableMethodFlags;
	if (NewBestInteractableComponent != OldBestInteractableComponent)
	{
		BestInteractableComponent = NewBestInteractableComponent;
		BroadcastBestInteractable(OldBestInteractableComponent, NewBestInteractableComponent);
	}

	DebugDrawInfo();
}

void UILLInteractableManagerComponent::CleanupInteractions()
{
	// Check for previous in-range or highlighted interactables and clean up the interaction
	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	for (int32 InteractableIndex = 0; InteractableIndex < RelevantInteractables.Num(); ++InteractableIndex)
	{
		UILLInteractableComponent* Interactable = RelevantInteractables[InteractableIndex].Get();
		if (!IsValid(Interactable))
			continue;

		if (InRangeInteractables.Contains(Interactable))
		{
			Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
			InRangeInteractables.Remove(Interactable);
		}

		if (HighlightedInteractables.Contains(Interactable))
		{
			Interactable->StopHighlightBroadcast(OwnerCharacter);
			HighlightedInteractables.Remove(Interactable);
		}
	}

	// Broadcast best interactable changes to NULL
	if (BestInteractableComponent)
	{
		UILLInteractableComponent* OldBestInteractableComponent = BestInteractableComponent;
		BestInteractableMethodFlags = 0;
		BestInteractableComponent = nullptr;
		BroadcastBestInteractable(OldBestInteractableComponent, nullptr);
	}
}

void UILLInteractableManagerComponent::GetViewLocationRotation(FVector& OutLocation, FRotator& OutDirection) const
{
	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	OwnerCharacter->GetActorEyesViewPoint(OutLocation, OutDirection);
	OutDirection.Normalize();

	if (bUseEyeLocation == false)
		OutLocation = OwnerCharacter->GetActorLocation();
}

void UILLInteractableManagerComponent::RegisterInteractable(UILLInteractableComponent* Interactable)
{
	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
		return;

	if (!InteractableBoundsCache.Contains(Interactable))
	{
		const FBox2D MaxAABB = GetMaxAABB(Interactable);

		// Add it to the master list
		if (InteractableBoundsCache.Num() == 0)
			InteractableBoundsCache.Reserve(1024); // Reserve a bunch of room up front
		InteractableBoundsCache.Add(Interactable, MaxAABB);

		// Listen for transform changes
		Interactable->TransformUpdated.AddUObject(this, &ThisClass::OnInteractableTransformChanged);

		// Insert it into the quad tree
		InteractableTree.Insert(Interactable, MaxAABB);
	}
}

void UILLInteractableManagerComponent::UnregisterInteractable(UILLInteractableComponent* Interactable)
{
	if (InteractableBoundsCache.Contains(Interactable))
	{
		InteractableBoundsCache.Remove(Interactable);

		// Remove it from the quad tree
		const FBox2D MaxAABB = GetMaxAABB(Interactable);
		InteractableTree.Remove(Interactable, MaxAABB);
	}
}

void UILLInteractableManagerComponent::OnInteractableTransformChanged(USceneComponent* RootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	if (UILLInteractableComponent* Interactable = Cast<UILLInteractableComponent>(RootComponent))
	{
		if (IsValid(Interactable))
		{
			DirtyInteractables.AddUnique(Interactable);
		}
	}
}

int32 UILLInteractableManagerComponent::GetBestInteractableMethodFlags() const
{
	return BestInteractableMethodFlags;
}

AActor* UILLInteractableManagerComponent::GetBestInteractableActor() const
{
	return LockedComponent ? LockedComponent->GetOwner() : (BestInteractableComponent ? BestInteractableComponent->GetOwner() : nullptr);
}

void UILLInteractableManagerComponent::BroadcastBestInteractable(UILLInteractableComponent* OldInteractable, UILLInteractableComponent* NewInteractable)
{
	if (CVarILLDebugInteractables.GetValueOnGameThread() >= 1)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, DEBUG_COLOR,
			FString::Printf(TEXT("%d. Best interactable changed from %s to %s"),
				GFrameCounter, *InteractableToString(OldInteractable), *InteractableToString(NewInteractable)),
			false);
	}

	OnBestInteractableComponentChanged.Broadcast(OldInteractable, NewInteractable);

	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	if (OldInteractable)
		OldInteractable->BroadcastLostBest(OwnerCharacter);

	if (NewInteractable)
		NewInteractable->BroadcastBecameBest(OwnerCharacter);
}

bool UILLInteractableManagerComponent::LockInteraction(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bForce /*= false*/)
{
	// Passing in a null component is always invalid, use EndInteraction to clear
	if (Component == nullptr || (LockedComponent && bForce == false))
		return false;

	// We may have an active component due to bForce, make sure to free it up (NOTE: doesn't support interacting with more than one object!)
	if (LockedComponent && Component != LockedComponent)
	{
		UnlockInteraction(LockedComponent);
	}

	SERVER_SetInteractionLocked(Component, InteractMethod, true);

	return true;
}

void UILLInteractableManagerComponent::UnlockInteraction(UILLInteractableComponent* Component, const bool bForce /*= false*/)
{
	// Passing in a null component is always invalid, use EndInteraction to clear
	if (!Component || !LockedComponent)
		return;

	SERVER_SetInteractionLocked(Component, (EILLInteractMethod)0, false);

	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	if (bForce && OwnerCharacter->Role != ROLE_Authority)
	{
		CLIENT_UnlockInteraction(Component);
	}
}

bool UILLInteractableManagerComponent::LockInteractionLocally(UILLInteractableComponent* Component)
{
	// Gotta have an unlocked component and can't already be locked to another component
	if (!Component || Component->IsLockedInUse() || LockedComponent)
		return false;

	LockedComponent = Component;
	// DO NOT SET LOCKED IN USE

	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	UpdateLockComponent(OwnerCharacter);

	return true;
}

void UILLInteractableManagerComponent::AttemptInteract(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bAutoLock /* = false*/)
{
	// Server controls interactions
	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());
	if (OwnerCharacter->Role != ROLE_Authority)
	{
		SERVER_AttemptInteract(Component, InteractMethod, bAutoLock);
		return;
	}

	// You have to interact with SOMETHING
	if (Component == nullptr)
	{
		CLIENT_LockInteraction(nullptr, InteractMethod, false);
		return;
	}

	// We can't interact with this since someone else has it locked or we have something else locked
	if (Component != LockedComponent && (Component->IsLockedInUse() || LockedComponent))
	{
		CLIENT_LockInteraction(Component, InteractMethod, false);
		UE_LOG(LogILLInteract, Verbose, TEXT("UILLInteractableManagerComponent::AttemptInteract %s could not lock %s due to having %s locked already"), *GetNameSafe(GetOwner()), *InteractableToString(Component), *InteractableToString(LockedComponent));
		return;
	}

	// Check if we're actually able to use this object at all
	FVector ViewLocation;
	FRotator ViewDirection;
	GetViewLocationRotation(ViewLocation, ViewDirection);
	if ((Component->CanInteractWithBroadcast(OwnerCharacter, ViewLocation, ViewDirection) & (int32)InteractMethod) == 0)
	{
		// If we already have this locked, don't unlock it, but also don't allow the interaction
		CLIENT_LockInteraction(Component, InteractMethod, Component == LockedComponent);
		UE_CLOG(Component != LockedComponent, LogILLInteract, Verbose, TEXT("UILLInteractableManagerComponent::AttemptInteract %s could not lock %s due to having no available interactions."), *GetNameSafe(GetOwner()), *InteractableToString(Component));
		return;
	}

	// Lock it up! Probably!
	if (bAutoLock)
	{
		LockInteraction(Component, InteractMethod);

		// Lock failed, don't let us interact
		if (LockedComponent == nullptr)
			return;
	}

	UE_LOG(LogILLInteract, Verbose, TEXT("UILLInteractableManagerComponent::AttemptInteract %s interacted with %s"), *GetNameSafe(GetOwner()), *InteractableToString(Component));

	// Finally perform the interaction
	OwnerCharacter->OnInteractAccepted(Component, InteractMethod);
	Component->OnInteractBroadcast(OwnerCharacter, InteractMethod);
}


bool UILLInteractableManagerComponent::SERVER_SetInteractionLocked_Validate(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bShouldLock)
{
	return true;
}

void UILLInteractableManagerComponent::SERVER_SetInteractionLocked_Implementation(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bShouldLock)
{
	if (!Component) // early out to prevent potential crash
		return;

	if (bShouldLock)
	{
		// We already have this locked, don't need to do anything more
		if (LockedComponent == Component)
		{
			CLIENT_LockInteraction(Component, InteractMethod, true);
			return;
		}

		// If the component is already locked GTFO.
		if (Component->IsLockedInUse())
		{
			CLIENT_LockInteraction(Component, InteractMethod, false);
			UE_LOG(LogILLInteract, Verbose, TEXT("UILLInteractableManagerComponent::SERVER_SetInteractionLocked %s could not lock %s due to it already be locked."), *GetNameSafe(GetOwner()), *InteractableToString(Component));
			return;
		}

		// If this is a listen server, we're setting this twice... oh well
		CLIENT_LockInteraction(Component, InteractMethod, true);
		UE_LOG(LogILLInteract, Verbose, TEXT("UILLInteractableManagerComponent::SERVER_SetInteractionLocked %s locked %s"), *GetNameSafe(GetOwner()), *InteractableToString(Component));

		Component->GetOwner()->ForceNetUpdate();
		LockedComponent = Component;
		LockedComponent->bIsLockedInUse = true;
	}
	else
	{
		// This isn't the component we locked
		if (LockedComponent != Component)
			return;

		// Let the client know if we're unlocking the component
		CLIENT_UnlockInteraction(Component);

		if (LockedComponent)
		{
			LockedComponent->GetOwner()->ForceNetUpdate();
			LockedComponent->bIsLockedInUse = false;
			LockedComponent = nullptr;
		}
	}
}

void UILLInteractableManagerComponent::CLIENT_LockInteraction_Implementation(UILLInteractableComponent* Component, const EILLInteractMethod InteractMethod, const bool bCanLock)
{
	OnClientLockedInteractable.Broadcast(Component, InteractMethod, bCanLock);

	// Passing in a null component is always invalid, use EndInteraction to clear
	if (!bCanLock)
		return;

	if (LockedComponent && LockedComponent != Component)
	{
		UnlockInteraction(LockedComponent);
	}

	AILLCharacter* OwnerCharacter = Cast<AILLCharacter>(GetOwner());

	Component->GetOwner()->ForceNetUpdate();
	LockedComponent = Component;
	LockedComponent->bIsLockedInUse = true;

	UpdateLockComponent(OwnerCharacter);

	if (CVarILLDebugInteractables.GetValueOnGameThread() >= 1)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, DEBUG_COLOR,
			FString::Printf(TEXT("%d. Interaction locked to %s"),
				GFrameCounter, *InteractableToString(LockedComponent)),
			false);
	}
}

void UILLInteractableManagerComponent::CLIENT_UnlockInteraction_Implementation(UILLInteractableComponent* Component)
{
	if (LockedComponent && LockedComponent == Component)
	{
		LockedComponent->GetOwner()->ForceNetUpdate();
		LockedComponent->bIsLockedInUse = false;
		LockedComponent = nullptr;
	}

	if (CVarILLDebugInteractables.GetValueOnGameThread() >= 1)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, DEBUG_COLOR,
			FString::Printf(TEXT("%d. Interaction unlocked from %s"),
				GFrameCounter, *InteractableToString(Component)),
			false);
	}

	TimeUntilUpdate = 0.f;
	TickComponent(0.f, ELevelTick::LEVELTICK_All, nullptr);
}

bool UILLInteractableManagerComponent::SERVER_AttemptInteract_Validate(UILLInteractableComponent* Component, EILLInteractMethod InteractMethod, const bool bAutoLock /* = false*/)
{
	return true;
}

void UILLInteractableManagerComponent::SERVER_AttemptInteract_Implementation(UILLInteractableComponent* Component, EILLInteractMethod InteractMethod, const bool bAutoLock /* = false*/)
{
	AttemptInteract(Component, InteractMethod, bAutoLock);
}

void UILLInteractableManagerComponent::UpdateLockComponent(AILLCharacter* OwnerCharacter)
{
	// Get the view location
	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewLocationRotation(ViewLocation, ViewRotation);

	if (OwnerCharacter->IsLocallyControlled() && LockedComponent->IsInteractionAllowed(OwnerCharacter))
	{
		// Force in-range and highlight onto LockedComponent
		for (int32 InteractableIndex = 0; InteractableIndex < RelevantInteractables.Num(); ++InteractableIndex)
		{
			UILLInteractableComponent* Interactable = RelevantInteractables[InteractableIndex].Get();
			if (!IsValid(Interactable))
				continue;

			if (Interactable == LockedComponent)
			{
				const bool bIsInRange = Interactable->IsInInteractRange(OwnerCharacter, ViewLocation, ViewRotation, bUseCameraRotation);
				if (InRangeInteractables.Contains(Interactable) != bIsInRange)
				{
					if (bIsInRange)
					{
						Interactable->OnInRangeBroadcast(OwnerCharacter);
						InRangeInteractables.AddUnique(Interactable);
					}
					else
					{
						Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
						InRangeInteractables.Remove(Interactable);
					}
				}

				const bool bShouldHighlight = Interactable->ShouldHighlight(OwnerCharacter, ViewLocation, ViewRotation);
				if (HighlightedInteractables.Contains(Interactable) != bShouldHighlight)
				{
					if (bShouldHighlight)
					{
						Interactable->StartHighlightBroadcast(OwnerCharacter);
						HighlightedInteractables.AddUnique(Interactable);
					}
					else
					{
						Interactable->StopHighlightBroadcast(OwnerCharacter);
						HighlightedInteractables.Remove(Interactable);
					}
				}
				continue;
			}

			if (InRangeInteractables.Contains(Interactable))
			{
				Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
				InRangeInteractables.Remove(Interactable);
			}

			if (HighlightedInteractables.Contains(Interactable))
			{
				Interactable->StopHighlightBroadcast(OwnerCharacter);
				HighlightedInteractables.Remove(Interactable);
			}
		}
	}

	// Lock the BestInteractableComponent to LockedComponent
	if (LockedComponent != BestInteractableComponent)
	{
		// Assign BestInteractableMethodFlags before hand because some things rely on that
		UILLInteractableComponent* OldBestInteractableComponent = BestInteractableComponent;
		BestInteractableComponent = LockedComponent;
		BestInteractableMethodFlags = LockedComponent->CanInteractWithBroadcast(OwnerCharacter, ViewLocation, ViewRotation);
		BroadcastBestInteractable(OldBestInteractableComponent, BestInteractableComponent);
	}
}

void UILLInteractableManagerComponent::DebugDrawInfo() const
{
	if (CVarILLDebugInteractables.GetValueOnGameThread() < 1)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT("ill.DebugInteractables %d"), CVarILLDebugInteractables.GetValueOnGameThread()), false);
	const int32 DebugRange = CVarILLDebugInteractablesRange.GetValueOnGameThread();
	if (DebugRange > 0)
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT("ill.DebugInteractablesRange %d meters"), DebugRange), false);
	else if (DebugRange == 0)
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, TEXT("ill.DebugInteractablesRange In-Range Only"), false);
	else
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, TEXT("ill.DebugInteractablesRange Unbound!"), false);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT("ill.DebugInteractablesShowAll %s"), CVarILLDebugInteractablesShowAll.GetValueOnGameThread() >= 1 ? TEXT("Enabled") : TEXT("Disabled")), false);
	if (UILLInteractableComponent* BestInteractable = GetBestInteractableComponent())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT("Best Interactable: %s %s"), *InteractableToString(BestInteractable), BestInteractable->bIsLockedInUse ? TEXT("(Locked)") : TEXT("")), false);

		const int32 InteractionFlags = GetBestInteractableMethodFlags();
		if (InteractionFlags & (int32)EILLInteractMethod::Press)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT(" - Press")), false);
		if (InteractionFlags & (int32)EILLInteractMethod::Hold)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT(" - Hold")), false);
		if (InteractionFlags & (int32)EILLInteractMethod::DoublePress)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, FString::Printf(TEXT(" - Double Press")), false);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DEBUG_COLOR, TEXT("Best Interactable: NONE"), false);
	}
}

void UILLInteractableManagerComponent::DebugDrawInteractable(const UILLInteractableComponent* Interactable, const AActor* Interactor, FColor Color) const
{
	if (CVarILLDebugInteractables.GetValueOnGameThread() < 1)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// If this interactable is locked in use (regardless of who by), make it white
	if (Interactable->IsLockedInUse())
		Color = FColor::White;

	// Drawing debug lines is pricey, opt out of interactables that are too far (see help for functionality)
	const float MaxRange = CVarILLDebugInteractablesRange.GetValueOnGameThread();
	if (MaxRange >= 0.f)
	{
		const float Distance = (Interactor->GetActorLocation() - Interactable->GetComponentLocation()).SizeSquared();
		if (MaxRange == 0.f)
		{
			if (Distance > FMath::Square(Interactable->DistanceLimit))
				return;
		}
		else
		{
			// Convert from meters to centimeters
			if (Distance > FMath::Square(MaxRange*100.f))
				return;
		}
	}

	if (const FBox2D* Bounds = InteractableBoundsCache.Find(Interactable))
	{
		DrawDebugBox(World, FVector(Bounds->GetCenter(), Interactable->GetComponentLocation().Z), FVector(Bounds->GetExtent(), FMath::Max(Interactable->DistanceLimit, Interactable->HighlightDistance)), FColor(Color.R, Color.G, Color.B, 127));
	}
	else
	{
		DrawDebugPoint(World, Interactable->GetComponentLocation(), 20.f, FColor::Red);
	}

	// Draw a cone for yaw/pitch limits, sphere for no limit
	if (Interactable->bUseYawLimit || Interactable->bUsePitchLimit)
	{
		const float YawRad = Interactable->bUseYawLimit ? FMath::DegreesToRadians(Interactable->MaxYawOffset) : 0.f;
		const float PitchRad = Interactable->bUsePitchLimit ? FMath::DegreesToRadians(Interactable->MaxPitchOffset) : 0.f;

		DrawDebugCone(World, Interactable->GetComponentLocation(), Interactable->GetComponentRotation().RotateVector(FVector::ForwardVector), Interactable->DistanceLimit, YawRad, PitchRad, 8, Color, false, 0.f, 0, 0.5f);
	}
	else
	{
		DrawDebugSphere(World, Interactable->GetComponentLocation(), Interactable->DistanceLimit, 8, Color, false, 0.f, 0, 0.5f);
	}

	// Print names of interactable components, USE RANGE
	if (CVarILLDebugInteractables.GetValueOnGameThread() >= 3)
	{
		if (AActor* InteractionActor = Interactable->GetOwner())
		{
			const FVector ActorLocation = InteractionActor->GetActorLocation();
			DrawDebugString(World, Interactable->GetRelativeTransform().GetTranslation() + FVector(0.f, 0.f, 25.f),
				InteractableToString(Interactable), InteractionActor, Color, 0.f, false);
		}
	}
}
