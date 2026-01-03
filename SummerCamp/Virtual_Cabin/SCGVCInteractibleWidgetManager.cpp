// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCCharacter.h"
#include "SCGVCInteractible.h"
#include "SCGVCInteractibleWidget.h"
#include "SCGVCInteractibleWidgetManager.h"

#ifndef INTERACTABLE_TREE_TILESIZE
# define INTERACTABLE_TREE_TILESIZE 30000.f
#endif

#ifndef INTERACTABLE_TREE_MINQUADSIZE
# define INTERACTABLE_TREE_MINQUADSIZE 1000.f
#endif

#ifndef RELEVANT_INTERACTABLE_SLACK
# define RELEVANT_INTERACTABLE_SLACK 8
#endif

// Sets default values for this component's properties
USCGVCInteractibleWidgetManager::USCGVCInteractibleWidgetManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUseCameraRotation(true)
	, bUseEyeLocation(true)

	// Interactable quad tree
	, InteractableTree(FBox2D(FVector2D(-INTERACTABLE_TREE_TILESIZE * 2.f, -INTERACTABLE_TREE_TILESIZE * 2.f), FVector2D(INTERACTABLE_TREE_TILESIZE * 2.f, INTERACTABLE_TREE_TILESIZE * 2.f)), INTERACTABLE_TREE_MINQUADSIZE)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bAutoActivate = true;
}

void USCGVCInteractibleWidgetManager::InitializeComponent()
{
	Super::InitializeComponent();

	SetComponentTickEnabled(false);
}

void USCGVCInteractibleWidgetManager::UninitializeComponent()
{
	Super::UninitializeComponent();

	// Cleanup previous interactions
	CleanupInteractions();
}

void USCGVCInteractibleWidgetManager::OnUnregister()
{
	Super::OnUnregister();

	InteractableBoundsCache.Empty();
	InteractableTree.Empty();
	RelevantInteractables.Empty();
}


void USCGVCInteractibleWidgetManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ASCGVCCharacter* OwnerCharacter = Cast<ASCGVCCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
		return;

	UWorld* World = GetWorld();
	if (!bHasTicked)
	{
		for (TObjectIterator<USCGVCInteractibleWidget> It; It; ++It)
		{
			USCGVCInteractibleWidget* InteractablePip = *It;
			if (InteractablePip->GetWorld() == World)
			{
				RegisterInteractable(InteractablePip);
			}
		}
		bHasTicked = true;
	}

	if (!OwnerCharacter->CanInteractAtAll())
	{
		CleanupInteractions();
		return;
	}

	USCGVCInteractibleWidget* NewBestInteractableComponent = nullptr;
	int32 NewBestInteractableMethodFlags = 0;

	// Get orientation and owner info
	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewLocationRotation(ViewLocation, ViewRotation);
	FVector CameraLocation;
	
	CameraLocation = OwnerCharacter->GetActorLocation();
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
	RelevantInteractables.Empty(FMath::Max(RelevantInteractables.Num() + InRangeInteractables.Num() + HighlightedInteractables.Num(), RELEVANT_INTERACTABLE_SLACK));
	
	for (const TPair<TWeakObjectPtr<USCGVCInteractibleWidget>, FBox2D>& Entry : InteractableBoundsCache)
	{
		RelevantInteractables.Add(Entry.Key);
	}

	// Determine the best interactable
	float BestScore = -FLT_MAX;
	for (int32 InteractableIndex = 0; InteractableIndex < RelevantInteractables.Num(); ++InteractableIndex)
	{
		USCGVCInteractibleWidget* Interactable = RelevantInteractables[InteractableIndex].Get();
		if (!IsValid(Interactable))
			continue;

		Interactable->SetOcclusionDirty();

		const FVector Location = Interactable->GetInteractionLocation(OwnerCharacter);
		FVector PlayerToComponent = Location - ViewLocation;
		if (bIgnoreHeightInDistanceComparison)
		{
			PlayerToComponent.Z = 0.f;
		}

		PlayerToComponent.Normalize();
		const float CameraDot = (Location - CameraLocation).GetSafeNormal() | ViewDirection;

		// Perform highlight check
		bool bShouldHighlight = false;

		// Perform range checks
		if (Interactable->IsInInteractRange(OwnerCharacter, ViewLocation, ViewRotation, bUseCameraRotation) == false)
		{
			if (Interactable->bInRange)
			{
				if (ASCGVCInteractible* parentInteractible = Cast<ASCGVCInteractible>(Interactable->GetOwner()))
				{
					parentInteractible->m_isPlayerInProximity = false;
				}

				Interactable->bInRange = false;
				Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
				InRangeInteractables.Remove(Interactable);
			}

			continue;
		}

		if (Interactable->bInRange == false)
		{
			if (ASCGVCInteractible* parentInteractible = Cast<ASCGVCInteractible>(Interactable->GetOwner()))
			{
				parentInteractible->m_isPlayerInProximity = true;
			}

			Interactable->bInRange = true;
			Interactable->OnInRangeBroadcast(OwnerCharacter);
			InRangeInteractables.AddUnique(Interactable);
		}
	}
}

void USCGVCInteractibleWidgetManager::CleanupInteractions()
{
	// Check for previous in-range or highlighted interactables and clean up the interaction
	ASCGVCCharacter* OwnerCharacter = Cast<ASCGVCCharacter>(GetOwner());
	for (int32 InteractableIndex = 0; InteractableIndex < RelevantInteractables.Num(); ++InteractableIndex)
	{
		USCGVCInteractibleWidget* Interactable = RelevantInteractables[InteractableIndex].Get();
		if (!IsValid(Interactable))
			continue;

		if (Interactable->bInRange)
		{
			if (ASCGVCInteractible* parentInteractible = Cast<ASCGVCInteractible>(Interactable->GetOwner()))
			{
				parentInteractible->m_isPlayerInProximity = false;
			}
			Interactable->bInRange = false;
			Interactable->OnOutOfRangeBroadcast(OwnerCharacter);
			InRangeInteractables.Remove(Interactable);
		}
	}
}

void USCGVCInteractibleWidgetManager::GetViewLocationRotation(FVector& OutLocation, FRotator& OutDirection) const
{
	if (ASCGVCCharacter* OwnerCharacter = Cast<ASCGVCCharacter>(GetOwner()))
	{
		OwnerCharacter->GetActorEyesViewPoint(OutLocation, OutDirection);
		OutDirection.Normalize();

		if (bUseEyeLocation == false)
			OutLocation = OwnerCharacter->GetActorLocation();
	}
}

FBox2D USCGVCInteractibleWidgetManager::GetMaxAABB(USCGVCInteractibleWidget* Interactable)
{
	if (!Interactable)
		return FBox2D();

	const float HorizontalPadding = 10.f; // Pad by 10cm
	const float Radius = FMath::Max(Interactable->DistanceLimit, m_ProximityReach) + HorizontalPadding;
	const FVector2D Location(Interactable->GetComponentLocation());
	const FVector2D Offset(Radius, Radius);
	const FBox2D AABB(Location - Offset, Location + Offset);
	return AABB;
}

void USCGVCInteractibleWidgetManager::RegisterInteractable(USCGVCInteractibleWidget* Interactable)
{
	ASCGVCCharacter* OwnerCharacter = Cast<ASCGVCCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
		return;

	if (!InteractableBoundsCache.Contains(Interactable))
	{
		const FBox2D MaxAABB = GetMaxAABB(Interactable);

		// Add it to the master list
		if (InteractableBoundsCache.Num() == 0)
			InteractableBoundsCache.Reserve(1024); // Reserve a bunch of room up front
		InteractableBoundsCache.Add(Interactable, MaxAABB);

		// Insert it into the quad tree
		InteractableTree.Insert(Interactable, MaxAABB);
	}
}

void USCGVCInteractibleWidgetManager::UnregisterInteractable(USCGVCInteractibleWidget* Interactable)
{
	if (InteractableBoundsCache.Contains(Interactable))
	{
		InteractableBoundsCache.Remove(Interactable);

		// Remove it from the quad tree
		const FBox2D MaxAABB = GetMaxAABB(Interactable);
		InteractableTree.Remove(Interactable, MaxAABB);
	}
}

void USCGVCInteractibleWidgetManager::OnInteractableTransformChanged(USCGVCInteractibleWidget* Interactable)
{
	if (IsValid(Interactable))
	{
		const FBox2D MaxAABB = GetMaxAABB(Interactable);

		// Remove it and update the InteractableBoundsCache
		if (FBox2D* OldAABB = InteractableBoundsCache.Find(Interactable))
		{
			InteractableTree.Remove(Interactable, *OldAABB);
			*OldAABB = MaxAABB;
		}

		// Re-insert it into the quad tree
		InteractableTree.Insert(Interactable, MaxAABB);
	}
}