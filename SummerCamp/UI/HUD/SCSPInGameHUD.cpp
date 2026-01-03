// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSPInGameHUD.h"

#include "SCHUDWidget.h"
#include "SCMenuStackInGameHUDComponent.h"
#include "SCMenuWidget.h"

ASCSPInGameHUD::ASCSPInGameHUD(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<USCMenuStackInGameHUDComponent>(ASCHUD::MenuStackCompName))
{
	ScoreboardClass = StaticLoadClass(USCUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/Scoreboard/SP_ScoreboardWidget.SP_ScoreboardWidget_C"));

	MatchEndMenuClass = StaticLoadClass(USCUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Menu/MatchEnd/SP_Results/SP_MatchEnd.SP_MatchEnd_C"));
}

void ASCSPInGameHUD::OnShowObjectiveEvent(const FString& Message)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnShowObjectiveEvent(Message);
	}
}

void ASCSPInGameHUD::OnSkullCompleted(const ESCSkullTypes skullType) const
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnSuccessSkull(skullType);
	}
}

void ASCSPInGameHUD::OnSkullFailed(const ESCSkullTypes skullType) const
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnFailedSkull(skullType);
	}
}
