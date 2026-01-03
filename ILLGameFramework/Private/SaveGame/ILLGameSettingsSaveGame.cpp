// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameSettingsSaveGame.h"

#include "ILLPlayerController.h"

UILLGameSettingsSaveGame::UILLGameSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLGameSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);
}

bool UILLGameSettingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	return false;
}
