// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCTeddyBearTriggerVolume.h"

#include "SCCharacter.h"
#include "SCPlayerState.h"


// Sets default values
ASCTeddyBearTriggerVolume::ASCTeddyBearTriggerVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TriggerArea = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("MarkupArea"));
	RootComponent = TriggerArea;

	TriggerArea->bHiddenInGame = true;
}

void ASCTeddyBearTriggerVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Handle overlaps
	if (!TriggerArea->OnComponentBeginOverlap.IsAlreadyBound(this, &ASCTeddyBearTriggerVolume::OnBeginOverlap))
	{
		TriggerArea->OnComponentBeginOverlap.AddDynamic(this, &ASCTeddyBearTriggerVolume::OnBeginOverlap);
	}
}

void ASCTeddyBearTriggerVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ASCCharacter* OverlappingCharacter = Cast<ASCCharacter>(OtherActor))
	{
		if (ASCPlayerState* PS = OverlappingCharacter->GetPlayerState())
		{
			PS->FoundTeddyBear();
		}
	}
}

