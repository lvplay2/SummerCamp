// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCAudioSettingsSaveGame.h"
#include "SCPlayerController.h"

USCAudioSettingsSaveGame::USCAudioSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundClass> MasterObj(TEXT("SoundClass'/Game/Sounds/SoundClasses/Master'"));
	MasterSoundClass = MasterObj.Object;

	static ConstructorHelpers::FObjectFinder<USoundClass> MenuObj(TEXT("SoundClass'/Game/Sounds/SoundClasses/Menu'"));
	MenuSoundClass = MenuObj.Object;

	static ConstructorHelpers::FObjectFinder<USoundClass> MusicObj(TEXT("SoundClass'/Game/Sounds/SoundClasses/Music'"));
	MusicSoundClass = MusicObj.Object;
	VolumeMusic = .75f;

	static ConstructorHelpers::FObjectFinder<USoundClass> SoundEffectsObj(TEXT("SoundClass'/Game/Sounds/SoundClasses/SoundEffects'"));
	SoundEffectsSoundClass = SoundEffectsObj.Object;

	static ConstructorHelpers::FObjectFinder<USoundClass> VoiceObj(TEXT("SoundClass'/Game/Sounds/SoundClasses/Voice'"));
	VoiceSoundClass = VoiceObj.Object;

	bUseHeadsetForGameVoice = true;
}

void USCAudioSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	if (bAndSave)
	{
		if (ASCPlayerController* SCPlayerController = Cast<ASCPlayerController>(PlayerController))
		{
			SCPlayerController->UpdateStreamerSettings(bStreamingMode);
		}
	}
}

bool USCAudioSettingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	USCAudioSettingsSaveGame* Other = CastChecked<USCAudioSettingsSaveGame>(OtherSave);
	return(Super::HasChangedFrom(OtherSave)
		|| bUseHeadsetForGameVoice != Other->bUseHeadsetForGameVoice
		|| bStreamingMode != Other->bStreamingMode);
}
