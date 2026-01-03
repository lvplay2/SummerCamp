// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCParanoiaHUD.h"
#include "SCMenuStackInGameHUDComponent.h"
#include "SCUserWidget.h"

ASCParanoiaHUD::ASCParanoiaHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USCMenuStackInGameHUDComponent>(ASCHUD::MenuStackCompName))
{
	MatchEndMenuClass = StaticLoadClass(USCUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Menu/MatchEnd/ParanoiaMatchEnd.ParanoiaMatchEnd_C"));
}

void ASCParanoiaHUD::DisplayScoreboard(const bool bShow, const bool bLockUnlock/* = false*/)
{
}