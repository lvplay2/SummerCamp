// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLAudioSettingsSaveGame.h"
#include "SCAudioSettingsSaveGame.generated.h"

/**
 * @class USCAudioSettingsSaveGame
 */
UCLASS()
class USCAudioSettingsSaveGame
: public UILLAudioSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	// End UILLPlayerSettingsSaveGame interface

	// Use the headset for world spatialized voice, instead of the normal audio output?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	bool bUseHeadsetForGameVoice;

	// Push to talk option when using a gamepad
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PlayerSettings|Audio")
	bool bStreamingMode;
};
