// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media
#include "SummerCamp.h"
#include "SCActorPreview.h"

ASCActorPreview::ASCActorPreview(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PreviewActor(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
	if (CaptureComponent)
	{
		CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		CaptureComponent->SetupAttachment(RootComponent);
	}
}

void ASCActorPreview::SpawnPreviewActor()
{
	DestroyPreviewActor();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Instigator;
	PreviewClass.LoadSynchronous(); // TODO: Make Async
	PreviewActor = GetWorld()->SpawnActor<AActor>(PreviewClass.Get(), SpawnParams);
	if (PreviewActor)
	{
		PreviewActor->SetActorTransform(FTransform::Identity);
		PreviewActor->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CaptureComponent->ShowOnlyActors.Add(PreviewActor);
		// this spawned actor should be local only
		PreviewActor->SetReplicates(false);
		PreviewActor->SetReplicateMovement(false);
	}
}

void ASCActorPreview::DestroyPreviewActor()
{
	if (PreviewActor)
	{
		if (CaptureComponent)
			CaptureComponent->ShowOnlyActors.Remove(PreviewActor);

		PreviewActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		PreviewActor->SetActorHiddenInGame(true);
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
}

void ASCActorPreview::BeginDestroy()
{
	DestroyPreviewActor();

	Super::BeginDestroy();
}