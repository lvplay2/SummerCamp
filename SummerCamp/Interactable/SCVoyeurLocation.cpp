// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVoyeurLocation.h"

#include "SCKillerCharacter.h"

ASCVoyeurLocation::ASCVoyeurLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	RootComponent = CreateDefaultSubobject<USphereComponent>("InteractArea");

	SplineCamera = CreateDefaultSubobject<USCSplineCamera>("VoyeurCam");
	SplineCamera->SetupAttachment(RootComponent);

	VoyeurInteract = CreateDefaultSubobject<USCInteractComponent>("VoyeurInteract");
	VoyeurInteract->InteractMethods = (int32)EILLInteractMethod::Press;
	VoyeurInteract->bCanHighlight = true;
	VoyeurInteract->SetupAttachment(RootComponent);
}

void ASCVoyeurLocation::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (VoyeurInteract)
	{
		VoyeurInteract->OnCanInteractWith.BindDynamic(this, &ASCVoyeurLocation::CanInteractWith);
	}
}

int32 ASCVoyeurLocation::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (Killer->GetGrabbedCounselor() != nullptr)
			return 0;
	}

	return VoyeurInteract->InteractMethods;
}