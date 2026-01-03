// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCImposterOutfit.h"
#include "SCMinimapIconComponent.h"

ASCImposterOutfit::ASCImposterOutfit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ShowMaskTime = 300.f;
}

void ASCImposterOutfit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCImposterOutfit, bWasUsed);
}

void ASCImposterOutfit::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(ShowMinimapIconTimerHandle, this, &ASCImposterOutfit::OnShowIcon, ShowMaskTime, false);
}


void ASCImposterOutfit::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	Super::OnInteract(Interactor, InteractMethod);
	bWasUsed = true;// set flag to true, we dont kill the timer here because this function is only called from the server
}

void ASCImposterOutfit::OnShowIcon()
{
	if (!bWasUsed)
		MinimapIconComponent->SetVisibility(true);
}