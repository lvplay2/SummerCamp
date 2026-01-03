// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVehicleSpawner.h"

#include "SCActorSpawnerComponent.h"

ASCVehicleSpawner::ASCVehicleSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	RootComponent = Billboard;

	ActorSpawner = CreateDefaultSubobject<USCActorSpawnerComponent>(TEXT("ActorSpawner"));
	ActorSpawner->SetupAttachment(RootComponent);

	PrimaryActorTick.bStartWithTickEnabled = false;
}

bool ASCVehicleSpawner::SpawnVehicle(TSubclassOf<AActor> ClassOverride /*=nullptr*/)
{
	if (SpawnedVehicle)
	{
		return false;
	}

	if (ActorSpawner->SpawnActor(ClassOverride))
	{
		SpawnedVehicle = ActorSpawner->GetSpawnedActor();
		return true;
	}

	return false;
}
