// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"
#include "SCPlayerState_Hunt.h"
#include "SCCharacterPreferencesSaveGame.generated.h"

/**
 * @class USCCharacterPreferencesSaveGame
 */
UCLASS()
class USCCharacterPreferencesSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	// End UILLPlayerSettingsSaveGame interface

	// Stored character preference for the local player
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Game")
	ESCCharacterPreference CharacterPreference;
};
