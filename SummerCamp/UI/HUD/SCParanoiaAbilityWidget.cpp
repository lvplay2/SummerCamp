// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCParanoiaAbilityWidget.h"
#include "SCPlayerController_Paranoia.h"

USCParanoiaAbilityWidget::USCParanoiaAbilityWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanEverTick = true;
}

float USCParanoiaAbilityWidget::GetUseAbilityPercentage() const
{
	if (ASCPlayerController_Paranoia* PC = Cast<ASCPlayerController_Paranoia>(GetOwningPlayer()))
	{
		return PC->GetUseAbilityPercentage();
	}

	return 0.0f;
}