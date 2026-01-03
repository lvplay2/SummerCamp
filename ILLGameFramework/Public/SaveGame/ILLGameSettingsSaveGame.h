// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"
#include "ILLGameSettingsSaveGame.generated.h"

/**
 * @class UILLGameSettingsSaveGame
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLGameSettingsSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	// End UILLPlayerSettingsSaveGame interface
};
