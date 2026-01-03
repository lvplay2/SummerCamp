// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLAudioSettingsSaveGame.h"

#include "ILLPlayerController.h"

UILLAudioSettingsSaveGame::UILLAudioSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SaveType = EILLPlayerSettingsSaveType::Platform;

	VolumeMaster = 1.f;
	VolumeMenu = 1.f;
	VolumeMusic = 1.f;
	VolumeSoundEffects = 1.f;
	VolumeVoiceOver = 1.f;
	VolumeVoice = 1.f;
	bUsePushToTalk = true;
	bPlayPushToTalkSound = true;
}

void UILLAudioSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	SetSystemSoundClassVolume(MasterSoundClass, VolumeMaster);
	SetSystemSoundClassVolume(MenuSoundClass, VolumeMenu);
	SetSystemSoundClassVolume(MusicSoundClass, VolumeMusic);
	SetSystemSoundClassVolume(SoundEffectsSoundClass, VolumeSoundEffects);
	SetSystemSoundClassVolume(VoiceOverSoundClass, VolumeVoiceOver);
	SetSystemSoundClassVolume(VoiceSoundClass, VolumeVoice);

	if (PlayerController)
	{
		PlayerController->UpdatePushToTalkTransmit();
	}
}

bool UILLAudioSettingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	UILLAudioSettingsSaveGame* Other = CastChecked<UILLAudioSettingsSaveGame>(OtherSave);
	return(VolumeMaster != Other->VolumeMaster
		|| VolumeMenu != Other->VolumeMenu
		|| VolumeMusic != Other->VolumeMusic
		|| VolumeSoundEffects != Other->VolumeSoundEffects
		|| VolumeVoiceOver != Other->VolumeVoiceOver
		|| VolumeVoice != Other->VolumeVoice
		|| bUsePushToTalk != Other->bUsePushToTalk
		|| bPlayPushToTalkSound != Other->bPlayPushToTalkSound);
}

float UILLAudioSettingsSaveGame::GetSystemSoundClassVolume(USoundClass* SoundClass) const
{
	if (SoundClass)
	{
		return SoundClass->Properties.Volume;
	}

	return 0.0f;
}

void UILLAudioSettingsSaveGame::SetSystemSoundClassVolume(USoundClass* SoundClass, float Volume)
{
	if (SoundClass)
	{
		SoundClass->Properties.Volume = Volume;
	}
}
