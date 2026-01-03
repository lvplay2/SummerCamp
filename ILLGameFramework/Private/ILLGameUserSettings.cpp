// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameUserSettings.h"

#include "ILLPlayerController.h"

FORCEINLINE FString FILLSettingsResolutionInfo::ToString() const
{
	return FString::Printf(TEXT("%4d x %4d"), Width, Height);
}

FORCEINLINE FILLSettingsResolutionInfo::operator FIntPoint() const
{
	return FIntPoint(Width, Height);
}

FString UILLUserSettingsBlueprintLibrary::ToString(const FILLSettingsResolutionInfo& Info)
{
	return Info.ToString();
}

FIntPoint UILLUserSettingsBlueprintLibrary::ToIntPoint(const FILLSettingsResolutionInfo& Info)
{
	return Info;
}

UILLGameUserSettings::UILLGameUserSettings(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, Gamma(2.2f), LastConfirmedGamma(Gamma)
, VolumeMaster(1.f), LastConfirmedVolumeMaster(VolumeMaster)
, VolumeMenu(1.f), LastConfirmedVolumeMenu(VolumeMenu)
, VolumeMusic(1.f), LastConfirmedVolumeMusic(VolumeMusic)
, VolumeSFX(1.f), LastConfirmedVolumeSFX(VolumeSFX)
, VolumeVO(1.f), LastConfirmedVolumeVO(VolumeVO)
, VolumeVoice(1.f), LastConfirmedVolumeVoice(VolumeVoice)
, bUsePushToTalk(true), bLastConfirmedPushToTalk(true)
, bStreamingMode(false), bLastConfirmedStreamingMode(false)
, bHoldToCrouch(true), bLastConfirmedHoldToCrouch(bHoldToCrouch)
, bHoldForCombat(true), bLastConfirmedHoldForCombat(bHoldForCombat)
, bInvertedMouseLook(false), bLastConfirmedInvertedMouseLook(bInvertedMouseLook)
, MouseSensitivity(0.07f), LastConfirmedMouseSensitivity(MouseSensitivity)
, bControllerEnabled(false), bLastConfirmedControllerEnabled(bControllerEnabled)
, bControllerVibrationEnabled(true), bLastConfirmedControllerVibrationEnabled(bControllerVibrationEnabled)
, bInvertedControllerLook(false), bLastConfirmedInvertedControllerLook(bInvertedControllerLook)
, HorizontalSensitivity(2.0f), LastConfirmedHorizontalSensitivity(HorizontalSensitivity)
, VerticalSensitivity(2.0f), LastConfirmedVerticalSensitivity(VerticalSensitivity)
{
	MinResolutionScale = Scalability::MinResolutionScale;

	FScreenResolutionArray ResArray;
	if (RHIGetAvailableResolutions(ResArray, true))
	{
		for (FScreenResolutionRHI Resolution : ResArray)
			ResolutionList.Add(FILLSettingsResolutionInfo(Resolution.Width, Resolution.Height));
	}
}

void UILLGameUserSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// Confirm all settings to match last saved
	ConfirmAllSettings();
}

void UILLGameUserSettings::LoadSettings(bool bForceReload/* = false*/)
{
	Super::LoadSettings(bForceReload);

	// Confirm all settings to match last saved
	ConfirmAllSettings();
}

void UILLGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	// Video settings
	SetGamma(Gamma);

	// Volume settings
	SetVolumeMaster(VolumeMaster);
	SetVolumeMenu(VolumeMenu);
	SetVolumeMusic(VolumeMusic);
	SetVolumeSFX(VolumeSFX);
	SetVolumeVO(VolumeVO);
	SetVolumeVoice(VolumeVoice);
	SetPushToTalk(bUsePushToTalk);
	SetStreamingMode(bStreamingMode);

	// Mouse settings
	SetInvertedMouseLook(bInvertedMouseLook);
	SetMouseSensitivity(MouseSensitivity);

	// Controller settings
	SetControllerEnabled(bControllerEnabled);
	SetInvertedControllerLook(bInvertedControllerLook);
	SetControllerVibrationEnabled(bControllerVibrationEnabled);
	SetHorizontalSensitivity(HorizontalSensitivity);
	SetVerticalSensitivity(VerticalSensitivity);

	// Action button settings
	SetHoldForCombat(bHoldForCombat);
	SetHoldToCrouch(bHoldToCrouch);

	// Save out
	SaveSettings();

	// Confirm all settings to match last saved
	ConfirmAllSettings();
}

bool UILLGameUserSettings::IsDirty() const
{
	return Super::IsDirty() || IsGammaDirty() || AnyVolumeSettingDirty() || AnyMouseSettingsDirty() || AnyControllerSettingsDirty() || AnyActionButtonSettingsDirty();
}

void UILLGameUserSettings::ResetToCurrentSettings()
{
	Super::ResetToCurrentSettings();

	SetGamma(LastConfirmedGamma);

	SetVolumeMaster(LastConfirmedVolumeMaster);
	SetVolumeMenu(LastConfirmedVolumeMenu);
	SetVolumeMusic(LastConfirmedVolumeMusic);
	SetVolumeSFX(LastConfirmedVolumeSFX);
	SetVolumeVO(LastConfirmedVolumeVO);
	SetVolumeVoice(LastConfirmedVolumeVoice);
	SetPushToTalk(bLastConfirmedPushToTalk);
	SetStreamingMode(bLastConfirmedStreamingMode);

	SetInvertedMouseLook(bLastConfirmedInvertedMouseLook);
	SetMouseSensitivity(LastConfirmedMouseSensitivity);

	SetHoldForCombat(bLastConfirmedHoldForCombat);
	SetHoldToCrouch(bLastConfirmedHoldToCrouch);

	SetControllerEnabled(bLastConfirmedControllerEnabled);
	SetInvertedControllerLook(bLastConfirmedInvertedControllerLook);
	SetControllerVibrationEnabled(bLastConfirmedControllerVibrationEnabled);
	SetHorizontalSensitivity(LastConfirmedHorizontalSensitivity);
	SetVerticalSensitivity(LastConfirmedVerticalSensitivity);
}

void UILLGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	Gamma = GetDefaultGamma();

	SetVolumeSettingDefaults(false);

	SetMouseSettingDefaults();

	SetControllerSettingDefaults();
}

UILLGameUserSettings* UILLGameUserSettings::GetILLGameUserSettings()
{
	return Cast<UILLGameUserSettings>(GEngine->GetGameUserSettings());
}

void UILLGameUserSettings::ApplySettingsToPlayer(AILLPlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
		return;

	// TODO: pjackson: Deprecate these?
	ApplyActionButtonSettingsToPlayer(PlayerController);
	ApplyMouseSettingsToPlayer(PlayerController);
	ApplyControllerSettingsToPlayer(PlayerController);

	PlayerController->UpdatePushToTalkTransmit();
}

void UILLGameUserSettings::ConfirmAllSettings()
{
	ConfirmGamma();
	ConfirmScalabilityQualitySettings();
	ConfirmVideoMode();
	ConfirmVolumeSettings();
	ConfirmMouseSettings();
	ConfirmActionButtonSettings();
	ConfirmControllerSettings();
}

//////////////////////////////////////////////////
// Video settings

FILLSettingsResolutionInfo UILLGameUserSettings::GetCurrentResolutionInfo() const
{
	return FILLSettingsResolutionInfo(ResolutionSizeX, ResolutionSizeY);
}

int32 UILLGameUserSettings::GetCurrentResolutionInfoIndex() const
{
	for (int32 Index = 0, Size(ResolutionList.Num()); Index < Size; ++Index)
	{
		if (ResolutionList[Index].Width == ResolutionSizeX && ResolutionList[Index].Height == ResolutionSizeY)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UILLGameUserSettings::SetGamma(float NewGamma)
{
	GEngine->DisplayGamma = Gamma = FMath::Clamp<float>(NewGamma, .5f, 5.f);
}

float UILLGameUserSettings::GetGamma() const
{
	return GEngine->DisplayGamma;
}

float UILLGameUserSettings::GetDefaultGamma()
{
	const UILLGameUserSettings* const DefaultThis = StaticClass()->GetDefaultObject<UILLGameUserSettings>();
	return DefaultThis->Gamma;
}

void UILLGameUserSettings::SetScalabilityQualityDefaults()
{
	ScalabilityQuality.SetDefaults();
}

void UILLGameUserSettings::ConfirmScalabilityQualitySettings()
{
	LastConfirmedScalabilityQuality = ScalabilityQuality;
}

//////////////////////////////////////////////////
// Audio settings

float UILLGameUserSettings::GetSoundClassVolume(USoundClass* SoundClass) const
{
	if (SoundClass)
	{
		return SoundClass->Properties.Volume;
	}

	return 0.0f;
}

void UILLGameUserSettings::SetSoundClassVolume(USoundClass* SoundClass, float Volume)
{
	if (SoundClass)
	{
		SoundClass->Properties.Volume = Volume;
	}
}

void UILLGameUserSettings::SetPushToTalk(const bool bInUsePushToTalk)
{
	bUsePushToTalk = bInUsePushToTalk;
}

void UILLGameUserSettings::SetStreamingMode(const bool bInUseStreamingMode)
{
	bStreamingMode = bInUseStreamingMode;
}

void UILLGameUserSettings::SetVolumeSettingDefaults(const bool bApplyNow/* = true*/)
{
	auto SetVolumeSettingToDefault = [&](USoundClass* SoundClass, float& Current, float& Last, const float FallbackDefault)
	{
		// Update the current and last confirmed to default
		Current = SoundClass ? SoundClass->GetClass()->GetDefaultObject<USoundClass>()->Properties.Volume : FallbackDefault;

		if (bApplyNow)
		{
			// Update volume now
			SetSoundClassVolume(SoundClass, Current);
			Last = Current;
		}
	};

	const UILLGameUserSettings* const DefaultThis = StaticClass()->GetDefaultObject<UILLGameUserSettings>();
	SetVolumeSettingToDefault(MasterSoundClass, VolumeMaster, LastConfirmedVolumeMaster, DefaultThis->VolumeMaster);
	SetVolumeSettingToDefault(MenuSoundClass, VolumeMenu, LastConfirmedVolumeMenu, DefaultThis->VolumeMenu);
	SetVolumeSettingToDefault(MusicSoundClass, VolumeMusic, LastConfirmedVolumeMusic, DefaultThis->VolumeMusic);
	SetVolumeSettingToDefault(SFXSoundClass, VolumeSFX, LastConfirmedVolumeSFX, DefaultThis->VolumeSFX);
	SetVolumeSettingToDefault(VOSoundClass, VolumeVO, LastConfirmedVolumeVO, DefaultThis->VolumeVO);
	SetVolumeSettingToDefault(VoiceSoundClass, VolumeVoice, LastConfirmedVolumeVoice, DefaultThis->VolumeVoice);
	bUsePushToTalk = bLastConfirmedPushToTalk = DefaultThis->bUsePushToTalk;
	bStreamingMode = bLastConfirmedStreamingMode = DefaultThis->bStreamingMode;
}

void UILLGameUserSettings::ConfirmVolumeSettings()
{
	LastConfirmedVolumeMaster = VolumeMaster;
	LastConfirmedVolumeMenu = VolumeMenu;
	LastConfirmedVolumeMusic = VolumeMusic;
	LastConfirmedVolumeSFX = VolumeSFX;
	LastConfirmedVolumeVO = VolumeVO;
	LastConfirmedVolumeVoice = VolumeVoice;
	bLastConfirmedPushToTalk = bUsePushToTalk;
	bLastConfirmedStreamingMode = bStreamingMode;
}

//////////////////////////////////////////////////
// Input settings: Keyboard / Controller Action Buttons

void UILLGameUserSettings::ConfirmActionButtonSettings()
{
	bLastConfirmedHoldToCrouch = bHoldToCrouch;
	bLastConfirmedHoldForCombat = bHoldForCombat;
}

void UILLGameUserSettings::SetHoldToCrouch(bool bNewHoldToCrouch)
{
	bHoldToCrouch = bNewHoldToCrouch;
}

void UILLGameUserSettings::SetHoldForCombat(bool bNewHoldForCombat)
{
	bHoldForCombat = bNewHoldForCombat;
}

void UILLGameUserSettings::ApplyActionButtonSettingsToPlayer(AILLPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->bCrouchToggle = !bHoldToCrouch;
	}
}

void UILLGameUserSettings::SetActionButtonSettingDefaults()
{
	const UILLGameUserSettings* const DefaultThis = StaticClass()->GetDefaultObject<UILLGameUserSettings>();

	SetHoldToCrouch(DefaultThis->bHoldToCrouch);
	SetHoldForCombat(DefaultThis->bHoldForCombat);
}

//////////////////////////////////////////////////
// Input settings: Mouse

void UILLGameUserSettings::SetInvertedMouseLook(bool bNewInvertedMouseLook)
{
	bInvertedMouseLook = bNewInvertedMouseLook;
}

void UILLGameUserSettings::SetMouseSensitivity(float NewSensitivity)
{
	MouseSensitivity = FMath::Clamp<float>(NewSensitivity, 0.01f, 0.2f);
}

void UILLGameUserSettings::ApplyMouseSettingsToPlayer(AILLPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->SetInvertedLook(bInvertedMouseLook, EKeys::MouseY);
		PlayerController->SetMouseSensitivity(MouseSensitivity);
	}
}

void UILLGameUserSettings::SetMouseSettingDefaults()
{
	const UILLGameUserSettings* const DefaultThis = StaticClass()->GetDefaultObject<UILLGameUserSettings>();

	SetInvertedMouseLook(DefaultThis->bInvertedMouseLook);
	SetMouseSensitivity(DefaultThis->MouseSensitivity);
}

void UILLGameUserSettings::ConfirmMouseSettings()
{
	bLastConfirmedInvertedMouseLook = bInvertedMouseLook;
	LastConfirmedMouseSensitivity = MouseSensitivity;
}

//////////////////////////////////////////////////
// Input settings: Controller

void UILLGameUserSettings::SetInvertedControllerLook(bool InvertedControllerLook)
{
	bInvertedControllerLook = InvertedControllerLook;
}

void UILLGameUserSettings::SetControllerEnabled(bool ControllerEnabled)
{
	bControllerEnabled = ControllerEnabled;
}

void UILLGameUserSettings::SetControllerVibrationEnabled(bool ControllerVibrationEnabled)
{
	bControllerVibrationEnabled = ControllerVibrationEnabled;
}

void UILLGameUserSettings::SetHorizontalSensitivity(float _HorizontalSensitivity)
{
	HorizontalSensitivity = FMath::Clamp<float>(_HorizontalSensitivity, 0.1f, 10.0f);
}

void UILLGameUserSettings::SetVerticalSensitivity(float _VerticalSensitivity)
{
	VerticalSensitivity = FMath::Clamp<float>(_VerticalSensitivity, 0.1f, 10.0f);
}

void UILLGameUserSettings::ApplyControllerSettingsToPlayer(AILLPlayerController* PlayerController)
{
	if (PlayerController)
	{
		// TODO: pjackson: bControllerEnabled
		// ILLPlayerController already handles bControllerVibrationEnabled in ClientPlayForceFeedback
		PlayerController->SetInvertedLook(bInvertedControllerLook, EKeys::Gamepad_RightY);
		PlayerController->SetControllerHorizontalSensitivity(HorizontalSensitivity);
		PlayerController->SetControllerVerticalSensitivity(VerticalSensitivity);
	}
}

void UILLGameUserSettings::SetControllerSettingDefaults()
{
	const UILLGameUserSettings* const DefaultThis = StaticClass()->GetDefaultObject<UILLGameUserSettings>();

	bControllerEnabled = DefaultThis->bControllerEnabled;
	bInvertedControllerLook = DefaultThis->bInvertedControllerLook;
	bControllerVibrationEnabled = DefaultThis->bControllerVibrationEnabled;
	HorizontalSensitivity = DefaultThis->HorizontalSensitivity;
	VerticalSensitivity = DefaultThis->VerticalSensitivity;
}

void UILLGameUserSettings::ConfirmControllerSettings()
{
	bLastConfirmedControllerEnabled = bControllerEnabled;
	bLastConfirmedInvertedControllerLook = bInvertedControllerLook;
	bLastConfirmedControllerVibrationEnabled = bControllerVibrationEnabled;
	LastConfirmedHorizontalSensitivity = HorizontalSensitivity;
	LastConfirmedVerticalSensitivity = VerticalSensitivity;
}
