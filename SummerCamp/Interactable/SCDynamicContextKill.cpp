// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDynamicContextKill.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCContextKillComponent.h"
#include "InstancedFoliageActor.h"

TAutoConsoleVariable<int32> CVarDebugGrabKills(TEXT("sc.DebugGrabKills"), 0,
	TEXT("Displays debug information for grab kills.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

// Sets default values
ASCDynamicContextKill::ASCDynamicContextKill(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bReattachToKiller(true)
{
	KillBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("KillBounds"));
	KillBounds->SetupAttachment(RootComponent);
	KillBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KillBounds->SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);
	KillBounds->SetCollisionResponseToChannel(ECC_Boat, ECR_Ignore);
	KillBounds->bGenerateOverlapEvents = false;

	SetReplicateMovement(true);
}

void ASCDynamicContextKill::BeginPlay()
{
	Super::BeginPlay();

	ContextKillComponent->KillComplete.AddDynamic(this, &ASCDynamicContextKill::ReattachKill);

	// Since this can be actived from clients and the server would have NO IDEA what
	// we wanted to do, it's important to keep this kill awake for the whole match.
	// This can all change once we get the RPCs out of here and into the character, howerver.
	SetNetDormancy(DORM_Awake);
}

void ASCDynamicContextKill::ActivateKill(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	if (Character->IsInSpecialMove())
		return;

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
		{
			SERVER_DetachFromParent();
			ContextKillComponent->ActivateGrabKill(Killer);
		}
		break;
	}
}

void ASCDynamicContextKill::MULTICAST_DetachFromParent_Implementation()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

bool ASCDynamicContextKill::SERVER_DetachFromParent_Validate()
{
	return true;
}

void ASCDynamicContextKill::SERVER_DetachFromParent_Implementation()
{
	MULTICAST_DetachFromParent();
}

void ASCDynamicContextKill::ReattachKill(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (bReattachToKiller)
	{
		AttachToActor(Interactor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		// Move the volume to the ground.
		SetActorRelativeLocation(FVector(0.f, 0.f, -Interactor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	}
}

bool ASCDynamicContextKill::DoesKillVolumeFit(const AActor* Interactor) const
{
	if (const ASCKillerCharacter* Character = Cast<ASCKillerCharacter>(Interactor))
	{
		FCollisionQueryParams params;

		TArray<AActor*> AttachedActors;
		Character->GetAttachedActors(AttachedActors);
		AttachedActors.Add((ASCKillerCharacter*)Character);

		params.AddIgnoredActors(AttachedActors);
		if (ASCCounselorCharacter* Victim = Character->GetGrabbedCounselor())
		{
			Victim->GetAttachedActors(AttachedActors);
			AttachedActors.Add(Victim);
			params.AddIgnoredActors(AttachedActors);
		}
		params.AddIgnoredActor(this);

		TArray<FOverlapResult> OverlappingActors;
		const bool bBlocking = GetWorld()->OverlapMultiByChannel(OverlappingActors, KillBounds->GetComponentLocation(), KillBounds->GetComponentQuat(), ECC_WorldDynamic, KillBounds->GetCollisionShape(), params);

		if (CVarDebugGrabKills.GetValueOnGameThread() >= 1)
		{
			DrawDebugBox(GetWorld(), KillBounds->GetComponentLocation(), KillBounds->GetScaledBoxExtent(), KillBounds->GetComponentQuat(), FColor::Purple);

			for (FOverlapResult Overlap : OverlappingActors)
			{
				if (Overlap.bBlockingHit)
				{
					GEngine->AddOnScreenDebugMessage((uint64) -1, 0.f, FColor::Orange, FString::Printf(TEXT("%s Overlapping with %s"), *GetName(), *Overlap.Actor->GetName()));
					DrawDebugSphere(GetWorld(), Overlap.Actor.Get()->GetActorLocation(), 25.f, 8, FColor::Orange);
				}
			}
		}

		return !bBlocking;
	}

	// The interactor wasn't the killer?!?!?!
	return false;
}

float ASCDynamicContextKill::GetPreviewDistance() const
{
	if (InteractComponent)
		return InteractComponent->HighlightDistance;

	return 0.f;
}

void ASCDynamicContextKill::PlaceVolume(const FVector NewLocation, const FRotator NewRotation, AActor* Interactor, bool bForcePosition /* = false*/)
{
	PreviousLocation = GetActorLocation();
	PreviousRotation = GetActorRotation();

	SetActorLocationAndRotation(NewLocation, NewRotation);

	UpdateOverlaps(false);

	if (!DoesKillVolumeFit(Interactor) && !bForcePosition)
	{
		SetActorLocationAndRotation(PreviousLocation, PreviousRotation);
		UpdateOverlaps(false);
		return;
	}
}

void ASCDynamicContextKill::ForceDisableCustomRender()
{
	//DisableKillPreview(nullptr);
}
