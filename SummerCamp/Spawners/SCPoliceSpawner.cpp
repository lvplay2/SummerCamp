// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPoliceSpawner.h"
#include "SCGameState.h"
#include "SCEscapeVolume.h"
#include "SCKillerCharacter.h"


// Sets default values
ASCPoliceSpawner::ASCPoliceSpawner(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpawnerIcon"));

	InterceptDirection = CreateDefaultSubobject<USceneComponent>(TEXT("InterceptDirection"));
	InterceptDirection->SetupAttachment(RootComponent);
	PrimaryActorTick.bStartWithTickEnabled = false;
}

// Called when the game starts or when spawned
void ASCPoliceSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (EscapeVolume)
	{
		EscapeVolume->DeactivateVolume();
	}
}

bool ASCPoliceSpawner::ActivateSpawner()
{
	// Make sure we have an escape zone and a police spawn class and that jason isn't in the way.
	if (!EscapeVolume || !PoliceSpawnClass || !EscapeVolume->IsUnobstructed())
	{
		return false;
	}

	// No, then spawn the police and activate the escape volume.
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = Instigator;
	AActor* Police = GetWorld()->SpawnActor<AActor>(PoliceSpawnClass, spawnParams);
	Police->SetActorLocationAndRotation(GetActorLocation(), GetActorRotation());

	OnPoliceSpawned();

	//J-Swan gets shot if he attempts to enter the Escape volume
	EscapeVolume->EscapeVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ASCPoliceSpawner::JasonTriggerOnBeginOverlap);

	// Listen for overlap events on the Jason trigger volume for Audio
	if (JasonTriggerVolume)
	{
		JasonTriggerVolume->OnActorBeginOverlap.AddUniqueDynamic(this, &ASCPoliceSpawner::PoliceSpotJasonBeginOverlap);
	}

	// Alert all the Players that the police has arrived with SFX.
	if (UWorld* World = GetWorld())
	{
		if (const ASCGameState* const GS = World->GetGameState<ASCGameState>())
		{
			for (ASCCharacter* PlayerCharacter : GS->PlayerCharacterList)
			{
				PlayerCharacter->MULTICAST_PlayPoliceArriveSFX();
			}
		}
	}

	// We want to activate the volume after everything else has been set up.
	EscapeVolume->MULTICAST_ActivateVolume();
	return true;
}

void ASCPoliceSpawner::JasonTriggerOnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
	{
		if (OtherComp == Killer->GetCapsuleComponent())
		{
			FVector TargetForward = FVector(FVector2D(InterceptDirection->GetComponentLocation() - EscapeVolume->EscapeVolume->GetComponentLocation()), 0.f).GetSafeNormal();
			Killer->StunnedByPolice(EscapeVolume->EscapeVolume, TargetForward, EscapeVolume->EscapeVolume->GetScaledSphereRadius());
		}
	}
}

void ASCPoliceSpawner::PoliceSpotJasonBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (Role == ROLE_Authority)
	{
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
		{
			Killer->MULTICAST_SpottedByPolice(PoliceSpotJasonAudio, JasonTriggerVolume->GetActorLocation());
		}
	}
}

