// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCHelpTextWidget.h"

#include "SCGameSettingsSaveGame.h"
#include "SCLocalPlayer.h"
#include "SCPlayerState.h"

USCHelpTextWidget::USCHelpTextWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MaxLevelForHelpText = 10;
}

bool USCHelpTextWidget::ShouldShowHelpText() const
{
	if (ASCPlayerState* PS = Cast<ASCPlayerState>(GetOwningPlayer()->PlayerState))
	{
		if (PS->GetPlayerLevel() > MaxLevelForHelpText)
			return false;
	}

	USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(GetOwningLocalPlayer());
	if (USCGameSettingsSaveGame* GameSettings = LocalPlayer->GetLoadedSettingsSave<USCGameSettingsSaveGame>())
	{
		return GameSettings->bShowHelpText;
	}

	return true;
}
