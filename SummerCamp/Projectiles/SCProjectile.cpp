// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"

#include "SCCharacter.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"

ASCProjectile::ASCProjectile(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = true;
	bReplicateMovement = true;

	Root = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	RootComponent = Root;

	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	ParticleSystem->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->bForceSubStepping = true;
}

void ASCProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
		{
			GS->ProjectileList.AddUnique(this);
		}
	}
}

void ASCProjectile::Destroyed()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
		{
			GS->ProjectileList.Remove(this);
		}
	}

	Super::Destroyed();
}

void ASCProjectile::SetIgnoredActor(AActor* Actor)
{
	Root->IgnoreActorWhenMoving(Actor, true);
}

void ASCProjectile::RemoveIgnoredActor(AActor* Actor)
{
	Root->IgnoreActorWhenMoving(Actor, false);
}

void ASCProjectile::InitVelocity(const FVector& Velocity)
{
	ProjectileMovement->Activate();
	ProjectileMovement->Velocity = Velocity;

	// check and see if Jason can take damage, if not ignore him
	if (GetWorld())
	{
		if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (GS->CurrentKiller && !GS->CurrentKiller->CanTakeDamage())
			{
				GS->CurrentKiller->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
				SetIgnoredActor(GS->CurrentKiller);
			}
		}
	}
}

void ASCProjectile::TriggerScoreEvent(TSubclassOf<USCScoringEvent> ScoreEventClass) const
{
	if (Role == ROLE_Authority)
	{
		if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
		{
			GameMode->HandleScoreEvent(Instigator->PlayerState, ScoreEventClass);
		}
	}
}
