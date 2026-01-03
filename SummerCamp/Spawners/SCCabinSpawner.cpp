// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCabinSpawner.h"

#include "SCActorSpawnerComponent.h"

ASCCabinSpawner::ASCCabinSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	ActorSpawner = CreateDefaultSubobject<USCActorSpawnerComponent>(TEXT("ActorSpawner"));
	ActorSpawner->SetupAttachment(RootComponent);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->bHiddenInGame = true;
	PreviewMesh->SetupAttachment(RootComponent);
}

bool ASCCabinSpawner::SpawnCabin()
{
	if (bCabinSpawned)
		return false;

	bCabinSpawned = ActorSpawner->SpawnActor();

	return bCabinSpawned;
}
