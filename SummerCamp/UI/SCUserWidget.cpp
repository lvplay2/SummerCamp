// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCUserWidget.h"

#include "SCHUD.h"

USCUserWidget::USCUserWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

ASCHUD* USCUserWidget::GetOwningPlayerSCHUD() const
{
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		return Cast<ASCHUD>(OwningPlayer->GetHUD());
	}

	return nullptr;
}

UILLUserWidget* USCUserWidget::GetCurrentMenu() const
{
	if (!GetOwningPlayerILLHUD() || !GetOwningPlayerILLHUD()->GetMenuStackComp())
		return nullptr;

	return GetOwningPlayerILLHUD()->GetMenuStackComp()->GetCurrentMenu();
}
