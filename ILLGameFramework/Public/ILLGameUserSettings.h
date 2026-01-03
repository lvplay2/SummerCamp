// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "ILLGameUserSettings.generated.h"

// Forward declarations
class USoundClass;

/**
 * @struct FILLSettingsResolutionInfo
 */
USTRUCT(BlueprintType)
struct FILLSettingsResolutionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Width;

	UPROPERTY(BlueprintReadOnly)
	int32 Height;

	FILLSettingsResolutionInfo() : Width(0), Height(0) {}
	FILLSettingsResolutionInfo(uint32 _Width, uint32 _Height) : Width(_Width), Height(_Height) {}

	FString ToString() const;
	operator FIntPoint() const;
};

/**
 * @class UILLUserSettingsBlueprintLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLUserSettingsBlueprintLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Converts a ResolutionInfo value to a string, in the form 'Width x Height' */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "ToString (ResolutionInfo)", CompactNodeTitle = "->", BlueprintAutocast), Category="Utilities|String")
	static FString ToString(const FILLSettingsResolutionInfo& Info);

	/** Converts a ResolutionInfo value to a IntPoint struct */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "ToIntPoint (ResolutionInfo)", CompactNodeTitle = "->", BlueprintAutocast), Category="Utilities|IntPoint")
	static FIntPoint ToIntPoint(const FILLSettingsResolutionInfo& Info);
};

/**
 * @class UILLGameUserSettings
 */
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class ILLGAMEFRAMEWORK_API UILLGameUserSettings
: public UGameUserSettings
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void PostInitProperties() override;
	// End UObject interface

	// Begin UGameUserSettings interface
	virtual void LoadSettings(bool bForceReload = false) override;
	virtual void ApplyNonResolutionSettings() override;
	virtual bool IsDirty() const;
	virtual void ResetToCurrentSettings() override;
	virtual void SetToDefaults() override;
	// End UGameUserSettings interface

	/** @return game local machine settings. */
	UFUNCTION(BlueprintCallable, Category="Settings")
	static UILLGameUserSettings* GetILLGameUserSettings();

	/** Applies all settings to PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Settings")
	virtual void ApplySettingsToPlayer(AILLPlayerController* PlayerController);

	/** Calls to all Confirm functions in this class. */
	virtual void ConfirmAllSettings();

	//////////////////////////////////////////////////
	// Video settings
public:
	/** Returns the current resolution as a ResolutionInfo. */
	UFUNCTION(BlueprintPure, Category="Settings|Video")
	FILLSettingsResolutionInfo GetCurrentResolutionInfo() const;

	/** Returns the index of the current resolution in the ResolutionList. */
	UFUNCTION(BlueprintPure, Category="Settings|Video")
	int32 GetCurrentResolutionInfoIndex() const;

	/** Set the global gamma of the screen (0.5..5.0, Default is 2.2) */
	UFUNCTION(BlueprintCallable, Category="Settings|Video")
	void SetGamma(float NewGamma);

	/** Returns the global gamma of the screen (0.5..5.0, Default is 2.2) */
	UFUNCTION(BlueprintPure, Category="Settings|Video")
	float GetGamma() const;

	/** Checks if the gamma user setting is different from current system setting */
	UFUNCTION(BlueprintPure, Category="Settings|Video")
	bool IsGammaDirty() const { return !FMath::IsNearlyEqual(LastConfirmedGamma, Gamma); }
	
	/** Changes controller settings to default. */
	UFUNCTION(BlueprintPure, Category="Settings|Video")
	static float GetDefaultGamma();

	/** Changes scalability settings to their defaults. */
	UFUNCTION(BlueprintCallable, Category="Settings|Video")
	void SetScalabilityQualityDefaults();

	/** @return true if the scalability quality (which includes resolution scale) settings have been changed. */
	UFUNCTION(BlueprintPure, Category="Settings|Video")
	bool IsScalabilityQualityDirty() const { return (LastConfirmedScalabilityQuality != ScalabilityQuality); }

protected:
	/** Confirms Gamma. */
	void ConfirmGamma() { LastConfirmedGamma = Gamma; }

	/** Confirms scalability quality settings. */
	void ConfirmScalabilityQualitySettings();

	// Full resolution list
	UPROPERTY(BlueprintReadOnly, Category="Settings|Video")
	TArray<FILLSettingsResolutionInfo> ResolutionList;

	// Global gamma for the screen
	UPROPERTY(Config)
	float Gamma;
	float LastConfirmedGamma;

	// Last confirmed screen resolution quality settings from UGameUserSettings
	Scalability::FQualityLevels LastConfirmedScalabilityQuality;
	
	//////////////////////////////////////////////////
	// Audio settings
public:
	/** @return Current volume of SoundClass. */
	float GetSoundClassVolume(USoundClass* SoundClass) const;

	/** Changes the volume of SoundClass to NewVolume. */
	void SetSoundClassVolume(USoundClass* SoundClass, float NewVolume);

	/** @return Volume of MasterSoundClass. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	float GetVolumeMaster() const { return GetSoundClassVolume(MasterSoundClass); }

	/** @return true if the VolumeMaster user setting is different from the last saved. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsVolumeMasterDirty() const { return !FMath::IsNearlyEqual(LastConfirmedVolumeMaster, VolumeMaster); }

	/** Set the volume of MasterSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetVolumeMaster(float Volume)
	{
		SetSoundClassVolume(MasterSoundClass, Volume);
		VolumeMaster = Volume;
	}

	/** @return Volume of MenuSoundClass. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	float GetVolumeMenu() const { return GetSoundClassVolume(MenuSoundClass); }

	/** @return true if the VolumeMenu user setting is different from the last saved. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsVolumeMenuDirty() const { return !FMath::IsNearlyEqual(LastConfirmedVolumeMenu, VolumeMenu); }

	/** Set the volume of MenuSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetVolumeMenu(float Volume)
	{
		SetSoundClassVolume(MenuSoundClass, Volume);
		VolumeMenu = Volume;
	}

	/** @return Volume of MusicSoundClass. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	float GetVolumeMusic() const { return GetSoundClassVolume(MusicSoundClass); }

	/** @return true if the VolumeMusic user setting is different from the last saved. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsVolumeMusicDirty() const { return !FMath::IsNearlyEqual(LastConfirmedVolumeMusic, VolumeMusic); }

	/** Set the volume of MusicSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetVolumeMusic(float Volume)
	{
		SetSoundClassVolume(MusicSoundClass, Volume);
		VolumeMusic = Volume;
	}

	/** @return Volume of SFXSoundClass. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	float GetVolumeSFX() const { return GetSoundClassVolume(SFXSoundClass); }

	/** @return true if the VolumeSFX user setting is different from the last saved. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsVolumeSFXDirty() const { return !FMath::IsNearlyEqual(LastConfirmedVolumeSFX, VolumeSFX); }

	/** Set the volume of the SFXSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetVolumeSFX(float Volume)
	{
		SetSoundClassVolume(SFXSoundClass, Volume);
		VolumeSFX = Volume;
	}

	/** @return Volume of VOSoundClass. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	float GetVolumeVO() const { return GetSoundClassVolume(VOSoundClass); }
	
	/** @return true if the VolumeVO user setting is different from the last saved. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsVolumeVODirty() const { return !FMath::IsNearlyEqual(LastConfirmedVolumeVO, VolumeVO); }

	/** Set the volume of the VOSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetVolumeVO(float Volume)
	{
		SetSoundClassVolume(VOSoundClass, Volume);
		VolumeVO = Volume;
	}

	/** @return Volume of VoiceSoundClass. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	float GetVolumeVoice() const { return GetSoundClassVolume(VoiceSoundClass); }
	
	/** @return true if the VolumeVoice user setting is different from the last saved. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsVolumeVoiceDirty() const { return !FMath::IsNearlyEqual(LastConfirmedVolumeVoice, VolumeVoice); }

	/** Set the volume of the VoiceSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetVolumeVoice(float Volume)
	{
		SetSoundClassVolume(VoiceSoundClass, Volume);
		VolumeVoice = Volume;
	}

	/** @return true if we want to use push to talk. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsUsingPushToTalk() const { return bUsePushToTalk; }

	/** @return true if the push to talk setting is dirty. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsPushToTalkDirty() const { return (bUsePushToTalk != bLastConfirmedPushToTalk); }

	/** Set the volume of the VoiceSoundClass. */
	UFUNCTION(BlueprintCallable, Category="Settings|Audio")
	void SetPushToTalk(const bool bInUsePushToTalk);

	/** @return true if we want to use streaming mode. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsUsingStreamingMode() const { return bStreamingMode; }

	/** @return true if the streaming mode setting is dirty. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool IsStreamingModeDirty() const { return (bStreamingMode != bLastConfirmedStreamingMode); }

	/** Turns on Streaming Mode. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetStreamingMode(const bool bInUseStreamingMode);

	/** @return true if the volume user settings is different from current system setting. */
	UFUNCTION(BlueprintPure, Category="Settings|Audio")
	bool AnyVolumeSettingDirty() const { return (IsVolumeMasterDirty() || IsVolumeMenuDirty() || IsVolumeMusicDirty() || IsVolumeSFXDirty() || IsVolumeVODirty() || IsVolumeVoiceDirty() || IsPushToTalkDirty() || IsStreamingModeDirty()); } // TODO: pjackson: Rename to AnyAudioSettingsDirty

	/** Changes volume settings to default. Applies to sound classes immediately. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetVolumeSettingDefaults(const bool bApplyNow = true); // TODO: pjackson: Rename to SetAudioSettingDefaults

protected:
	/** Confirms volume settings. */
	void ConfirmVolumeSettings();

	// Volume for the "Master" sound class
	UPROPERTY(Transient)
	USoundClass* MasterSoundClass;
	UPROPERTY(Config)
	float VolumeMaster;
	float LastConfirmedVolumeMaster;

	// Volume for the "Menu" sound class
	UPROPERTY(Transient)
	USoundClass* MenuSoundClass;
	UPROPERTY(Config)
	float VolumeMenu;
	float LastConfirmedVolumeMenu;

	// Volume for the "Music" sound class
	UPROPERTY(Transient)
	USoundClass* MusicSoundClass;
	UPROPERTY(Config)
	float VolumeMusic;
	float LastConfirmedVolumeMusic;

	// Volume for the "SFX" sound class
	UPROPERTY(Transient)
	USoundClass* SFXSoundClass;
	UPROPERTY(Config)
	float VolumeSFX;
	float LastConfirmedVolumeSFX;

	// Volume for the "VO" sound class
	UPROPERTY(Transient)
	USoundClass* VOSoundClass;
	UPROPERTY(Config)
	float VolumeVO;
	float LastConfirmedVolumeVO;

	// Volume for the "Voice" sound class
	UPROPERTY(Transient)
	USoundClass* VoiceSoundClass;
	UPROPERTY(Config)
	float VolumeVoice;
	float LastConfirmedVolumeVoice;

	// Push to talk option
	UPROPERTY(Config)
	bool bUsePushToTalk;
	bool bLastConfirmedPushToTalk;

	// Streaming Mode Options
	UPROPERTY(Config)
	bool bStreamingMode;
	bool bLastConfirmedStreamingMode;

	//////////////////////////////////////////////////
	// Input settings: Keyboard / Controller Action Buttons.
public:
	/** Sets hold to crouch to a new bool value. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Buttons")
	void SetHoldToCrouch(bool bNewHoldToCrouch);

	/** Gets hold to crouch is bool. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Buttons")
	bool GetHoldToCrouch() { return bHoldToCrouch; }

	/** Checks to see if hold to crouch is dirty. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Buttons")
	bool IsHoldToCrouchDirty() const { return bHoldToCrouch != bLastConfirmedHoldToCrouch; }

	/** This can either be ADS or Combat mode (like a lock on). */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Buttons")
	void SetHoldForCombat(bool bNewHoldForCombat);

	/** Gets the hold for combat bool. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Buttons")
	bool GetHoldForCombat() { return bHoldForCombat; }

	/** Checks to see if hold for combat is dirty */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Buttons")
	bool IsHoldForCombatDirty() const { return bHoldForCombat != bLastConfirmedHoldForCombat; }

	/** @return true if any mouse input settings are dirty. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Buttons")
	bool AnyActionButtonSettingsDirty() const { return (IsHoldToCrouchDirty() || IsHoldForCombatDirty()); }

	/** Applies action button settings to PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Buttons")
	void ApplyActionButtonSettingsToPlayer(AILLPlayerController* PlayerController);

	/** Changes action button settings to default. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Buttons")
	void SetActionButtonSettingDefaults();

protected:
	/** Confirms action button settings. */
	void ConfirmActionButtonSettings();

	// For holding to crouch.
	UPROPERTY(Config)
	bool bHoldToCrouch;
	bool bLastConfirmedHoldToCrouch;

	// For holding to crouch.
	UPROPERTY(Config)
	bool bHoldForCombat;
	bool bLastConfirmedHoldForCombat;

	//////////////////////////////////////////////////
	// Input settings: Mouse
public:
	/** Sets inverted controls */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Mouse")
	void SetInvertedMouseLook(bool bNewInvertedMouseLook);

	/** Returns the inverted look bool */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Mouse")
	bool GetInvertedMouseLook() const { return bInvertedMouseLook; }

	/** Checks if the inverted look setting is different from current system setting */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Mouse")
	bool IsInvertedMouseLookDirty() const { return (bInvertedMouseLook != bLastConfirmedInvertedMouseLook); }

	/** Changes MouseSensitivity to NewSensitivity and optionall updates PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Mouse")
	void SetMouseSensitivity(float NewSensitivity);

	/** @return Current MouseSensitivity setting. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Mouse")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	/** @return true if MouseSensitivity is has been changed from the LastConfirmedMouseSensitivity.  */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Mouse")
	bool IsMouseSensitivityDirty() const { return !FMath::IsNearlyEqual(LastConfirmedMouseSensitivity, MouseSensitivity); }

	/** @return true if any mouse input settings are dirty. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Mouse")
	bool AnyMouseSettingsDirty() const { return (IsInvertedMouseLookDirty() || IsMouseSensitivityDirty()); }

	/** Applies mouse settings to PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Mouse")
	void ApplyMouseSettingsToPlayer(AILLPlayerController* PlayerController);

	/** Changes mouse settings to default. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Mouse")
	void SetMouseSettingDefaults();

protected:
	/** Confirms mouse settings. */
	void ConfirmMouseSettings();

	// For inverting the look controls
	UPROPERTY(Config)
	bool bInvertedMouseLook;
	bool bLastConfirmedInvertedMouseLook;

	UPROPERTY(Config)
	float MouseSensitivity;
	float LastConfirmedMouseSensitivity;
	
	//////////////////////////////////////////////////
	// Input settings: Controller
public:
	/** Sets inverted controls */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetInvertedControllerLook(bool InvertedControllerLook);

	/** Returns the inverted look bool */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool GetInvertedControllerLook() const { return bInvertedControllerLook; }

	/** Checks if the inverted look setting is different from current system setting */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool IsInvertedControllerLookDirty() const { return bInvertedControllerLook != bLastConfirmedInvertedControllerLook; }

	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetControllerEnabled(bool ControllerEnabled);

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool GetControllerEnabled() const { return bControllerEnabled; }

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool IsControllerEnabledDirty() const { return bControllerEnabled != bLastConfirmedControllerEnabled; }

	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetControllerVibrationEnabled(bool ControllerVibrationEnabled);

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool GetControllerVibrationEnabled() const { return bControllerVibrationEnabled; }

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool IsControllerVibrationEnabledDirty() const { return bControllerVibrationEnabled != bLastConfirmedControllerVibrationEnabled; }

	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetHorizontalSensitivity(float _HorizontalSensitivity);

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	float GetHorizontalSensitivity() const { return HorizontalSensitivity; }

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool IsHorizontalSensitivityDirty() const { return !FMath::IsNearlyEqual(LastConfirmedHorizontalSensitivity, HorizontalSensitivity); }

	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetVerticalSensitivity(float _VerticalSensitivity);

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	float GetVerticalSensitivity() const { return VerticalSensitivity; }

	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool IsVerticalSensitivityDirty() const { return !FMath::IsNearlyEqual(LastConfirmedVerticalSensitivity, VerticalSensitivity); }

	/** @return true if any controller input settings are dirty. */
	UFUNCTION(BlueprintPure, Category="Settings|Input|Controller")
	bool AnyControllerSettingsDirty() const { return (IsInvertedControllerLookDirty() || IsControllerEnabledDirty() || IsControllerVibrationEnabledDirty() || IsHorizontalSensitivityDirty() || IsVerticalSensitivityDirty()); }

	/** Applies controller settings to PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void ApplyControllerSettingsToPlayer(AILLPlayerController* PlayerController);

	/** Changes controller settings to default. */
	UFUNCTION(BlueprintCallable, Category="Settings|Input|Controller")
	void SetControllerSettingDefaults();

protected:
	/** Confirms controller settings. */
	void ConfirmControllerSettings();

	UPROPERTY(Config)
	bool bControllerEnabled;
	bool bLastConfirmedControllerEnabled;

	UPROPERTY(Config)
	bool bControllerVibrationEnabled;
	bool bLastConfirmedControllerVibrationEnabled;

	UPROPERTY(Config)
	bool bInvertedControllerLook;
	bool bLastConfirmedInvertedControllerLook;

	UPROPERTY(Config)
	float HorizontalSensitivity;
	float LastConfirmedHorizontalSensitivity;

	UPROPERTY(Config)
	float VerticalSensitivity;
	float LastConfirmedVerticalSensitivity;
};
