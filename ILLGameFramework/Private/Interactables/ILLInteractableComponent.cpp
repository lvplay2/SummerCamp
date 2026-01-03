// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLInteractableComponent.h"

#include "ILLCharacter.h"
#include "ILLInteractableManagerComponent.h"

extern ILLGAMEFRAMEWORK_API TAutoConsoleVariable<int32> CVarILLDebugInteractables;

UILLInteractableComponent::UILLInteractableComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)

// Interaction Type
, bIsEnabled(true)
, InteractMethods((int32)EILLInteractMethod::Press)

// Range
, DistanceLimit(200.f)
, MaxYawOffset(15.f)
, MaxPitchOffset(90.f)

// Occlusion
, RayTraceChannel(ECollisionChannel::ECC_WorldDynamic)

// Highlight
, HighlightDistance(500.f)
, HighlightZDistance(100.f)
, YawHighlightLimit(90.f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostUpdateWork;
	SetIsReplicated(true);
}

void UILLInteractableComponent::BeginDestroy()
{
	UnregisterWithManagers();

	Super::BeginDestroy();
}

void UILLInteractableComponent::OnRegister()
{
	Super::OnRegister();

	if (UWorld* World = GetWorld())
	{
		for (TObjectIterator<UILLInteractableManagerComponent> It(RF_ClassDefaultObject); It; ++It)
		{
			UILLInteractableManagerComponent* Manager = *It;
			if (Manager->GetWorld() == World)
			{
				Manager->RegisterInteractable(this);
			}
		}
	}
}

void UILLInteractableComponent::OnUnregister()
{
	UnregisterWithManagers();

	Super::OnUnregister();
}

void UILLInteractableComponent::DestroyComponent(bool bPromoteChildren/* = false*/)
{
	if (!IsBeingDestroyed()) // Avoid extra calls
	{
		UnregisterWithManagers();
	}

	Super::DestroyComponent(bPromoteChildren);
}

void UILLInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterWithManagers();
}

void UILLInteractableComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport/* = ETeleportType::None*/)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

	OnTransformChanged.Broadcast(this);
}

FBox2D UILLInteractableComponent::GetInteractionAABB() const
{
	const float Radius = DistanceLimit;
	const FVector2D Location(GetComponentLocation());
	const FVector2D Offset(Radius, Radius);
	const FBox2D AABB(Location - Offset, Location + Offset);
	return AABB;
}

void UILLInteractableComponent::UnregisterWithManagers()
{
	for (TObjectIterator<UILLInteractableManagerComponent> It(RF_ClassDefaultObject); It; ++It)
	{
		UILLInteractableManagerComponent* Manager = *It;
		Manager->UnregisterInteractable(this);
	}
}

/////////////////////////////////////////
// Replication

void UILLInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UILLInteractableComponent, bIsEnabled);
	DOREPLIFETIME(UILLInteractableComponent, bIsLockedInUse);
}


/////////////////////////////////////////
// Interaction Type

void UILLInteractableComponent::OnInteractBroadcast(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	if (OnInteract.IsBound())
	{
		OnInteract.Broadcast(Interactor, InteractMethod);
	}
}

void UILLInteractableComponent::OnHoldStateChangedBroadcast(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (OnHoldStateChanged.IsBound())
	{
		OnHoldStateChanged.Broadcast(Interactor, NewState);
	}
}

bool UILLInteractableComponent::IsInteractionAllowed(AILLCharacter* Character)
{
	// Block interaction if we're disabled
	if (!bIsEnabled)
	{
		return false;
	}

	// If the owner matters, check that it's both valid and not hidden
	if (!bInteractWhenOwnerHidden && (!IsValid(GetOwner()) || GetOwner()->bHidden))
	{
		return false;
	}

	// Skip over any interactables that ignore this actor type
	for (const TSoftClassPtr<AActor>& ActorClass : ActorIgnoreList)
	{
		if (ActorClass.Get() && Character->IsA(ActorClass.Get()))
		{
			return false;
		}
	}

	return true;
}

int32 UILLInteractableComponent::CanInteractWithBroadcast(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (OnCanInteractWith.IsBound())
	{
		return OnCanInteractWith.Execute(Character, ViewLocation, ViewRotation);
	}

	return InteractMethods; // Return the supported interact methods by default
}

/////////////////////////////////////////
// Range

void UILLInteractableComponent::OnInRangeBroadcast(AActor* Interactor)
{
	if (OnInRange.IsBound())
	{
		OnInRange.Broadcast(Interactor);
	}
}

void UILLInteractableComponent::OnOutOfRangeBroadcast(AActor* Interactor)
{
	if (OnOutOfRange.IsBound())
	{
		OnOutOfRange.Broadcast(Interactor);
	}
}

bool UILLInteractableComponent::IsInInteractRange(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation, const bool bUseCameraRotation)
{
	const FVector ActorLocation = Character->GetActorLocation();
	const FVector InteractLocation = GetInteractionLocation(Character);
	const FVector ToComponent = ActorLocation - InteractLocation;
	if (ToComponent.SizeSquared() > FMath::Square(DistanceLimit))
		return false;

	const FRotator ComponentRotation = GetComponentRotation();
	FRotator ToInteractorRotLocal = ToComponent.Rotation() - ComponentRotation;
	ToInteractorRotLocal.Normalize();
	if (bUseYawLimit && FMath::Abs(ToInteractorRotLocal.Yaw) > MaxYawOffset)
		return false;

	if (bUsePitchLimit && FMath::Abs(ToInteractorRotLocal.Pitch) > MaxPitchOffset)
		return false;

	if (bUseCameraRotation)
	{
		FRotator ToViewRotLocal = (-ComponentRotation.Vector()).Rotation() - ViewRotation;
		ToViewRotLocal.Normalize();
		if (bUseYawLimit && FMath::Abs(ToViewRotLocal.Yaw) > MaxYawOffset)
			return false;

		if (bUsePitchLimit && FMath::Abs(ToViewRotLocal.Pitch) > MaxPitchOffset)
			return false;
	}

	if (bCheckForOcclusion && IsOccluded(Character, ViewLocation, ViewRotation))
		return false;

	return true;
}

FVector UILLInteractableComponent::GetInteractionLocation(AILLCharacter* Character) const
{
	if (OverrideInteractionLocation.IsBound())
	{
		return OverrideInteractionLocation.Execute(Character);
	}

	return GetComponentLocation();
}

/////////////////////////////////////////
// Occlusion

bool UILLInteractableComponent::IsOccluded(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// Perform trace to retrieve hit info
	static FName InteractionOcclusionTag = FName(TEXT("InteractionOcclusionTrace"));
	FCollisionQueryParams TraceParams(InteractionOcclusionTag, bPerformComplexOcclusionCheck);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.AddIgnoredActor(GetOwner());
	TArray<AActor*> AttachedActors;
	Character->GetAttachedActors(AttachedActors);
	AttachedActors.Add(Character);
	TraceParams.AddIgnoredActors(AttachedActors);

	const FVector Start = ViewLocation + ViewRotation.Quaternion().GetAxisZ() * RayTraceZOffset;

	bool bIsOccluded = false;
	int32 Iterations = 16;
	FHitResult HitResult;
	while (Iterations-- > 0 && GetWorld()->LineTraceSingleByChannel(HitResult, Start, GetInteractionLocation(Character), (ECollisionChannel)RayTraceChannel, TraceParams))
	{
		// See if we're ignoring this actor
		if (AActor* HitActor = HitResult.Actor.Get())
		{
			bool bShouldIgnore = false;
			for (const TSoftClassPtr<AActor>& Class : IgnoreClasses)
			{
				if (Class.Get() && HitActor->IsA(Class.Get()))
				{
					bShouldIgnore = true;
					break;
				}
			}

			if (bShouldIgnore)
			{
				TraceParams.AddIgnoredActor(HitActor);
				continue;
			}
		}

		// Didn't hit an actor, or we're not ignoring them. Occluded!
		bIsOccluded = true;
		break;
	}

	if (CVarILLDebugInteractables.GetValueOnGameThread() >= 2)
	{
		const float Time = .5f;
		DrawDebugDirectionalArrow(GetWorld(), Start, GetInteractionLocation(Character), 25.f, bIsOccluded ? FColor::Red : FColor::Green, false, Time);
		if (bIsOccluded)
		{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 5.f, 8, FColor::Cyan, false, Time);

			if (CVarILLDebugInteractables.GetValueOnGameThread() >= 4)
			{
				if (AActor* InteractionActor = GetOwner())
				{
					DrawDebugString(GetWorld(), HitResult.ImpactPoint,
						FString::Printf(TEXT("%s:%s is occluded by %s:%s"), *InteractionActor->GetName(), *GetName(), HitResult.Actor.Get() ? *HitResult.Actor->GetName() : TEXT("Unknown"), HitResult.Component.Get() ? *HitResult.Component->GetName() : TEXT("Unknown")),
						nullptr, FColor::Cyan, Time, false);
				}
			}
		}
	}

	return bIsOccluded;
}


/////////////////////////////////////////
// Highlight

void UILLInteractableComponent::StartHighlightBroadcast(AActor* Interactor)
{
	if (OnStartHighlight.IsBound())
	{
		OnStartHighlight.Broadcast(Interactor);
	}
}

void UILLInteractableComponent::StopHighlightBroadcast(AActor* Interactor)
{
	if (OnStopHighlight.IsBound())
	{
		OnStopHighlight.Broadcast(Interactor);
	}
}

bool UILLInteractableComponent::ShouldHighlight(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bCanHighlight == false)
		return false;

	const FVector ActorLocation = Character->GetActorLocation();

	// TODO: Maybe move this after the range check?
	if (OnShouldHighlight.IsBound())
	{
		if (OnShouldHighlight.Execute(Character, ActorLocation, ViewRotation) == false)
			return false;
	}

	// Check our distance limit
	const FVector InteractionLocation = GetInteractionLocation(Character);
	if (bUseSeperateZBounds)
	{
		if (FMath::Abs(InteractionLocation.Z - ActorLocation.Z) > HighlightZDistance
		|| FVector::DistSquaredXY(InteractionLocation, ActorLocation) > FMath::Square(HighlightDistance))
		{
			return false;
		}
	}
	else
	{
		if (FVector::DistSquared(InteractionLocation, ActorLocation) > FMath::Square(HighlightDistance))
		{
			return false;
		}
	}

	// Check our yaw limits
	if (bUseYawHighlightLimit)
	{
		FRotator ToInteractorRotLocal = (ActorLocation - InteractionLocation).Rotation() - GetComponentRotation();
		ToInteractorRotLocal.Normalize();
		if (FMath::Abs(ToInteractorRotLocal.Yaw) > YawHighlightLimit)
			return false;
	}

	// Check our occlusion
	if (bCheckForOcclusion && IsOccluded(Character, ViewLocation, ViewRotation))
		return false;

	return true;
}
