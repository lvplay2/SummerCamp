// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCustomInput.h"

#include "ILLPlayerController.h"
#include "ILLLocalPlayer.h"

#include "SCInputMappingsSaveGame.h"
#include "SCRebindingWidget.h"

USCCustomInput::USCCustomInput(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCCustomInput::ApplySettings()
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(GetOwningILLPlayer()->GetLocalPlayer()))
	{
		if (USCInputMappingsSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCInputMappingsSaveGame>())
		{
			SaveGame->ApplySettings(GetOwningILLPlayer());
		}
	}
}

void USCCustomInput::DiscardSettings()
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(GetOwningILLPlayer()->GetLocalPlayer()))
	{
		if (USCInputMappingsSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCInputMappingsSaveGame>())
		{
			SaveGame->DiscardSettings(GetOwningILLPlayer());
		}
	}
}

void USCCustomInput::ResetSettings()
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(GetOwningILLPlayer()->GetLocalPlayer()))
	{
		if (USCInputMappingsSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCInputMappingsSaveGame>())
		{
			SaveGame->ResetSettings(GetOwningILLPlayer());
		}
	}
}
