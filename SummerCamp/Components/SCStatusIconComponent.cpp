// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCStatusIconComponent.h"
#include "SCStatusIconWidget.h"

USCStatusIconComponent::USCStatusIconComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bIsHidden(true)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USCStatusIconComponent::InitializeComponent()
{
	Super::InitializeComponent();

	StatusIconWidget = Cast<USCStatusIconWidget>(Widget);
}

void USCStatusIconComponent::ShowIcon()
{
	StatusIconWidget = Cast<USCStatusIconWidget>(Widget);
	SetComponentTickEnabled(true);

	if (StatusIconWidget)
	{
		StatusIconWidget->ShowIcon();
		bIsHidden = false;
	}
}

void USCStatusIconComponent::HideIcon()
{
	StatusIconWidget = Cast<USCStatusIconWidget>(Widget);
	//SetComponentTickEnabled(false);

	if (StatusIconWidget)
	{
		StatusIconWidget->HideIcon();
		bIsHidden = true;
	}
}
