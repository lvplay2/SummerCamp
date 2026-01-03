// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerKnifeWidget.h"
#include "SCKillerCharacter.h"

USCKillerKnifeWidget::USCKillerKnifeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanEverTick = true;
}

int32 USCKillerKnifeWidget::GetKnifeCount() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		return Killer->NumKnives;
	}

	return 0;
}

FLinearColor USCKillerKnifeWidget::GetActiveColor() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (Killer->NumKnives <= 0)
			return EmptyColor;

		if (!Killer->CanThrowKnife())
			return RechargeColor;
	}

	return ActiveColor;
}


ESlateVisibility USCKillerKnifeWidget::GetUseActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (Killer->IsThrowingKnife())
			return ESlateVisibility::Collapsed;
	}

	return ESlateVisibility::HitTestInvisible;
}

ESlateVisibility USCKillerKnifeWidget::GetCancelActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (Killer->IsThrowingKnife())
			return ESlateVisibility::HitTestInvisible;
	}

	return ESlateVisibility::Collapsed;
}

FLinearColor USCKillerKnifeWidget::GetActionMapColor() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (!Killer->CanThrowKnife())
			return InactiveActionMapColor;
	}

	return ActiveActionMapColor;
}