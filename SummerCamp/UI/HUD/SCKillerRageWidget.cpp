// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerRageWidget.h"
#include "SCKillerCharacter.h"

USCKillerRageWidget::USCKillerRageWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanEverTick = true;
}

float USCKillerRageWidget::GetRagePercentage() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		return Killer->GetRageUnlockPercentage();
	}

	return 0.0f;
}

FLinearColor USCKillerRageWidget::GetFillColor() const
{
	if (GetRagePercentage() >= 1.0f)
		return UnlockedColor;

	return LockedColor;
}