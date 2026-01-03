// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCCabin.h"

#include "SCGameMode.h"
#include "SCWorldSettings.h"

ASCCabin::ASCCabin(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCCabin::BeginPlay()
{
	Super::BeginPlay();

	// Update rain state
	UWorld* World = GetWorld();
	if (ASCWorldSettings* WorldSettings = World ? Cast<ASCWorldSettings>(World->GetWorldSettings()) : nullptr)
	{
		SetRaining(WorldSettings->GetIsRaining());
		WorldSettings->OnRainingStateChanged.AddUniqueDynamic(this, &ThisClass::SetRaining);
	}
}
