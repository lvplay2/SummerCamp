// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerMorphPoint.h"

ASCKillerMorphPoint::ASCKillerMorphPoint(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	OverlapVolume = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("MarkupArea"));
	OverlapVolume->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	OverlapVolume->SetCollisionObjectType(ECC_MorphPoint);
	OverlapVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	OverlapVolume->SetCollisionResponseToChannel(ECC_MorphPoint, ECollisionResponse::ECR_Overlap);
	OverlapVolume->bHiddenInGame = true;
	OverlapVolume->ShapeColor = FColor(238, 49, 36);

	GetCapsuleComponent()->SetCapsuleHalfHeight(100.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);

	PrimaryActorTick.bStartWithTickEnabled = false;

	GroundDistanceLimit = 20.f;
}
