// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCRepairPartSpawner.h"

#include "SCActorSpawnerComponent.h"
#include "SCRepairPart.h"

ASCRepairPartSpawner::ASCRepairPartSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	ActorSpawner = CreateDefaultSubobject<USCActorSpawnerComponent>(TEXT("ActorSpawner"));
	ActorSpawner->SetupAttachment(RootComponent);
}

bool ASCRepairPartSpawner::SpawnPart(UClass* PartClass)
{
	if (SpawnedPart)
		return false;

	if (ActorSpawner->SpawnActor(PartClass))
	{
		SpawnedPart = Cast<ASCRepairPart>(ActorSpawner->GetSpawnedActor());
		return true;
	}

	return false;
}
