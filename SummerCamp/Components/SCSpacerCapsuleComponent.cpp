// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSpacerCapsuleComponent.h"
#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"


USCSpacerCapsuleComponent::USCSpacerCapsuleComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, SpacerType(ESpacerPushType::SP_Nearest)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USCSpacerCapsuleComponent::BeginPlay()
{
	Super::BeginPlay();

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DefaultCollision = GetCollisionResponseToChannels();
	FullRadius = GetUnscaledCapsuleRadius();
}

void USCSpacerCapsuleComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (GetUnscaledCapsuleRadius() < FullRadius)
	{
		CurrentGrowTime += DeltaSeconds;
		SetCapsuleRadius(FMath::Lerp(0.f, FullRadius, FMath::Min(CurrentGrowTime / TotalGrowTime, 1.f)));
	}
	else
	{
		SetComponentTickEnabled(false);
	}
}

bool USCSpacerCapsuleComponent::ActivateSpacer(ASCCharacter* Interactor, float GrowTime /* = 0.f*/)
{
	// Activate and set collision to overlap to get characters in volume
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	if (SpacerInteractor)
		SpacerInteractor->GetCapsuleComponent()->IgnoreComponentWhenMoving(this, false);

	if (Interactor)
		Interactor->GetCapsuleComponent()->IgnoreComponentWhenMoving(this, true);

	SpacerInteractor = Interactor;

	// Get our list of overlapping actors
	TArray<AActor*> OverlappingActors;
	TArray<FOverlapInfo> Overlaps;
	GetOverlappingActors(OverlappingActors);
	
	for (AActor* Actor : OverlappingActors)
	{
		// See if any of our overlaps are blockers
		for (TSoftClassPtr<AActor> Blocker : BlockingActors)
		{
			if (Blocker.Get() && Actor->IsA(Blocker.Get()))
			{
				DeactivateSpacer();
				return true;
			}
		}

		// Add the overlap info to our array for later.
		Actor->UpdateOverlaps(false);
		GetOverlapsWithActor(Actor, Overlaps);
	}

	// If the spacer type is grow we want to set our radius and clear timers.
	if (SpacerType == ESpacerPushType::SP_Grow)
	{
		SetCapsuleRadius(0.1f);
		SetComponentTickEnabled(true);
		TotalGrowTime = FMath::Max(GrowTime, KINDA_SMALL_NUMBER);
		CurrentGrowTime = 0.f;
		SetCollisionResponseToChannels(DefaultCollision);
		return false;
	}

	// Push The overlapping actors out of the way if they're in our pushable actors list.
	const FVector CapsuleLocation = GetComponentLocation();
	for (FOverlapInfo infos : Overlaps)
	{
		if (infos.OverlapInfo.GetActor() == Interactor)
			continue;

		AActor* Actor = infos.OverlapInfo.GetActor();
		for (TSoftClassPtr<AActor> ActorClass : PushableActors)
		{
			if (ActorClass.Get() && !Actor->IsA(ActorClass.Get()))
				continue;

			// Get the actor's location and simple collision cylider so we can move them outside the volume.
			float ActorRadius = 0.f;
			float ActorHeight = 0.f;
			Actor->GetSimpleCollisionCylinder(ActorRadius, ActorHeight);
			FVector ActorLocation = Actor->GetActorLocation();

			if (SpacerType == ESpacerPushType::SP_StraightBack)
			{
				ActorLocation = CapsuleLocation + (GetForwardVector() * (GetScaledCapsuleRadius() + ActorRadius));
			}
			else if (SpacerType == ESpacerPushType::SP_Nearest)
			{
				const FVector VecTo = ActorLocation - CapsuleLocation;
				const float DistToMove = (GetScaledCapsuleRadius() + ActorRadius) - VecTo.Size2D();
				ActorLocation += VecTo.GetSafeNormal2D() * DistToMove;
			}

			Actor->SetActorLocation(ActorLocation);
			break;
		}
	}

	SetCollisionResponseToChannels(DefaultCollision);
	return false;
}

void USCSpacerCapsuleComponent::DeactivateSpacer()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (SpacerInteractor)
	{
		SpacerInteractor->GetCapsuleComponent()->IgnoreComponentWhenMoving(this, false);
		SpacerInteractor = nullptr;
	}

	// reset collision so blocking works again
	SetCollisionResponseToChannels(DefaultCollision);
}
