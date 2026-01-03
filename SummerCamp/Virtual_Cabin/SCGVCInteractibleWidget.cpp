// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCCharacter.h"
#include "SCGVCInteractibleWidget.h"

//extern ILLGAMEFRAMEWORK_API TAutoConsoleVariable<int32> CVarSCVCDebugInteractables;

USCGVCInteractibleWidget::USCGVCInteractibleWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	// Range
	, DistanceLimit(200.f)
		, bUseYawLimit(false)
		, MaxYawOffset(15.f)
		, bUsePitchLimit(false)
		, MaxPitchOffset(90.f)
		, bInRange(false)

		// Occlusion
		, bCheckForOcclusion(false)
		, OcclusionUpdateFrequency(30.f)
		, bPerformComplexOcclusionCheck(false)
		, RayTraceZOffset(0.f)
		, RayTraceChannel(ECollisionChannel::ECC_WorldDynamic)

		//Highlight
		, bIsOccluded(false)
{}

/////////////////////////////////////////
// Range

void USCGVCInteractibleWidget::OnInRangeBroadcast(ASCGVCCharacter* Interactor)
{
	if (OnInRange.IsBound())
	{
		OnInRange.Broadcast(Interactor);
	}
}

void USCGVCInteractibleWidget::OnOutOfRangeBroadcast(ASCGVCCharacter* Interactor)
{
	if (OnOutOfRange.IsBound())
	{
		OnOutOfRange.Broadcast(Interactor);
	}
}

bool USCGVCInteractibleWidget::IsInInteractRange(ASCGVCCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation, const bool bUseCameraRotation)
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

FVector USCGVCInteractibleWidget::GetInteractionLocation(ASCGVCCharacter* Character) const
{
	if (OverrideInteractionLocation.IsBound())
	{
		return OverrideInteractionLocation.Execute(Character);
	}

	return GetComponentLocation();
}

/////////////////////////////////////////
// Occlusion

bool USCGVCInteractibleWidget::IsOccluded(ASCGVCCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bIsOcclusionDirty == false)
		return bIsOccluded;

	bIsOcclusionDirty = false;

	if (OcclusionUpdateFrequency > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			const float FrameTime = World->GetTimeSeconds();
			if (FrameTime - LastOcclusionCheck > 1.f / OcclusionUpdateFrequency)
			{
				LastOcclusionCheck = FrameTime;
			}
			else
			{
				// Not time for an update, return if we were occluded last frame
				return bIsOccluded;
			}
		}
	}

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

	bIsOccluded = false;
	int32 Iterations = 16;
	FHitResult HitResult;
	while (GetWorld()->LineTraceSingleByChannel(HitResult, Start, GetInteractionLocation(Character), (ECollisionChannel)RayTraceChannel, TraceParams))
	{
		// Don't loop forever
		if (Iterations-- <= 0)
			break;

		// See if we're ignoring this actor
		if (AActor* HitActor = HitResult.Actor.Get())
		{
			bool bShouldIgnore = false;
			for (auto Class : IgnoreClasses)
			{
				if (HitActor->IsA(Class))
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

	return bIsOccluded;
}