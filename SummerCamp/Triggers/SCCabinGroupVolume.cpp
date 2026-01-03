// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCabinGroupVolume.h"

ASCCabinGroupVolume::ASCCabinGroupVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;

	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	RootComponent = Volume;

	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Volume->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Volume->bHiddenInGame = true;

	PrimaryActorTick.bStartWithTickEnabled = false;
}
