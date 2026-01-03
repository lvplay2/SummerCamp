// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCContextComponent.h"

// Sets default values for this component's properties
USCContextComponent::USCContextComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
}

void USCContextComponent::InitializeComponent()
{
	Super::InitializeComponent();

	DestinationReachedDelegate.Clear();
	DestinationReachedDelegate.AddDynamic(this, &USCContextComponent::BeginAction);
}

void USCContextComponent::BeginAction()
{
	KillerSpecialMove->BeginAction();
}

void USCContextComponent::EndAction()
{
	Killer = nullptr;
	KillerSpecialMove = nullptr;
}
