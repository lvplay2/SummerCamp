// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"
#include "ILLAudioSettingsSaveGame.generated.h"

/**
 * @class UILLAudioSettingsSaveGame
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLAudioSettingsSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	// End UILLPlayerSettingsSaveGame interface

	/** @return Current system volume of SoundClass. */
	virtual float GetSystemSoundClassVolume(USoundClass* SoundClass) const;

	/** Changes the volume of SoundClass to NewVolume. */
	virtual void SetSystemSoundClassVolume(USoundClass* SoundClass, float NewVolume);

	// Sound class for "Master" volume
	USoundClass* MasterSoundClass;

	// Volume for the "Master" sound class
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	float VolumeMaster;

	// Sound class for "Menu" volume
	USoundClass* MenuSoundClass;

	// Volume for the "Menu" sound class
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	float VolumeMenu;

	// Sound class for "Music" volume
	USoundClass* MusicSoundClass;

	// Volume for the "Music" sound class
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	float VolumeMusic;

	// Sound class for "SoundEffects" volume
	USoundClass* SoundEffectsSoundClass;

	// Volume for the "SoundEffects" sound class
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	float VolumeSoundEffects;

	// Sound class for "VoiceOver" volume
	USoundClass* VoiceOverSoundClass;

	// Volume for the "VoiceOver" sound class
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	float VolumeVoiceOver;

	// Sound class for Voice volume
	USoundClass* VoiceSoundClass;

	// Volume for the "Voice" sound class
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	float VolumeVoice;

	// Push to talk option
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	bool bUsePushToTalk;

	// Push to talk sound option
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Audio")
	bool bPlayPushToTalkSound;
};
