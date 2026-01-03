// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerTrapWidget.h"
#include "SCKillerCharacter.h"

USCKillerTrapWidget::USCKillerTrapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanEverTick = true;
}

int32 USCKillerTrapWidget::GetTrapCount() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		return Killer->TrapCount;
	}

	return 0;
}

FLinearColor USCKillerTrapWidget::GetActiveColor() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (Killer->TrapCount <= 0)
			return EmptyColor;

		if (!Killer->CanPlaceTrap())
			return RechargeColor;
	}

	return ActiveColor;
}

FLinearColor USCKillerTrapWidget::GetActionMapColor() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (!Killer->CanPlaceTrap())
			return InactiveActionMapColor;
	}

	return ActiveActionMapColor;
}