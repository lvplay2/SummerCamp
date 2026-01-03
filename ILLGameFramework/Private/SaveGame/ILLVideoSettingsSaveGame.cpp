// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLVideoSettingsSaveGame.h"

#include "ILLGameInstance.h"
#include "ILLGameUserSettings.h"
#include "ILLPlayerController.h"

UILLVideoSettingsSaveGame::UILLVideoSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SaveType = EILLPlayerSettingsSaveType::Platform;

	Gamma = 2.2f;
	ResolutionIndex = INDEX_NONE;
	FullscreenMode = EILLWindowMode::WindowedFullscreen;
	bVerticalSync = false;

#if PLATFORM_DESKTOP
	if (GConfig && GEngine && GEngine->IsInitialized())
	{
		// Pull scalability defaults from the config
		const TCHAR* Section = TEXT("ScalabilityGroups");
		GConfig->GetFloat(Section, TEXT("sg.ResolutionQuality"), ResolutionScale, GGameUserSettingsIni);
		ResolutionScale *= 1.f/100.f; // Convert 0-100 to 0-1
		GConfig->GetInt(Section, TEXT("sg.AntiAliasingQuality"), AntiAliasingQuality, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("sg.EffectsQuality"), EffectsQuality, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("sg.FoliageQuality"), FoliageQuality, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("sg.PostProcessQuality"), PostProcessQuality, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("sg.ShadowQuality"), ShadowQuality, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("sg.TextureQuality"), TextureQuality, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("sg.ViewDistanceQuality"), ViewDistanceQuality, GGameUserSettingsIni);
	}
#endif // PLATFORM_DESKTOP
}

void UILLVideoSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	// Update gamma
	Gamma = FMath::Clamp<float>(Gamma, .5f, 5.f);
	GEngine->DisplayGamma = Gamma;

#if PLATFORM_DESKTOP
	// Update vertical sync
	if (auto VSyncCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))
	{
		VSyncCVar->Set(bVerticalSync, ECVF_SetByGameSetting);
	}
#endif // PLATFORM_DESKTOP

	if (bAndSave)
	{
		if (UGameEngine* GameEngine = Cast<UGameEngine>(GEngine))
		{
#if PLATFORM_DESKTOP
			// Make sure the resolution list is setup
			CacheResolutionList(PlayerController);

			// Update scalability settings
			Scalability::SetQualityLevels(BuildScalabilitySettings());

			// Update resolution settings
			int32 ResolutionX = 0;
			int32 ResolutionY = 0;
			if (!ResolutionList.IsValidIndex(ResolutionIndex))
			{
				UpdateResolutionIndexFromSystem();
			}
			if (ResolutionList.IsValidIndex(ResolutionIndex))
			{
				ResolutionX = ResolutionList[ResolutionIndex].Width;
				ResolutionY = ResolutionList[ResolutionIndex].Height;
			}

			// Map the fullscreen mode and allow the game engine to override resolution settings
			EILLWindowMode NewFullscreenMode = FullscreenMode;
			{
				EWindowMode::Type MappedType = static_cast<EWindowMode::Type>(NewFullscreenMode);
				UGameEngine::ConditionallyOverrideSettings(ResolutionX, ResolutionY, MappedType);
				NewFullscreenMode = static_cast<EILLWindowMode>(MappedType);
			}

			// Request the resolution change
			FSystemResolution::RequestResolutionChange(ResolutionX, ResolutionY, static_cast<EWindowMode::Type>(NewFullscreenMode));

			/*if (NewFullscreenMode == EILLWindowMode::Fullscreen || NewFullscreenMode == EILLWindowMode::WindowedFullscreen)
			{
				if (auto FullScreenModeCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode")))
				{
					FullScreenModeCVar->Set(NewFullscreenMode == EILLWindowMode::Fullscreen ? 0 : 1, ECVF_SetByGameSetting);
				}
			}*/
#endif // PLATFORM_DESKTOP

			// Save as system defaults
			// This will make these settings apply on startup
			if (UILLGameUserSettings* UserSettings = UILLGameUserSettings::GetILLGameUserSettings())
			{
#if PLATFORM_DESKTOP
				UserSettings->SetVSyncEnabled(bVerticalSync);
				UserSettings->SetScreenResolution(FIntPoint(ResolutionX, ResolutionY));
				UserSettings->SetFullscreenMode(EWindowMode::Type(NewFullscreenMode));
#endif // PLATFORM_DESKTOP
				UserSettings->SetGamma(Gamma);
				UserSettings->SaveSettings();
			}
		}
	}

	// Flush CVar changes
	IConsoleManager::Get().CallAllConsoleVariableSinks();

	Super::ApplyPlayerSettings(PlayerController, bAndSave);
}

bool UILLVideoSettingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	UILLVideoSettingsSaveGame* Other = CastChecked<UILLVideoSettingsSaveGame>(OtherSave);
	return(Gamma != Other->Gamma
#if PLATFORM_DESKTOP
		|| ResolutionIndex != Other->ResolutionIndex
		|| FullscreenMode != Other->FullscreenMode
		|| bVerticalSync != Other->bVerticalSync
		|| ResolutionScale != Other->ResolutionScale
		|| AntiAliasingQuality != Other->AntiAliasingQuality
		|| EffectsQuality != Other->EffectsQuality
		|| FoliageQuality != Other->FoliageQuality
		|| PostProcessQuality != Other->PostProcessQuality
		|| ShadowQuality != Other->ShadowQuality
		|| TextureQuality != Other->TextureQuality
		|| ViewDistanceQuality != Other->ViewDistanceQuality
#endif // PLATFORM_DESKTOP
		);
}

void UILLVideoSettingsSaveGame::SaveGameLoaded(AILLPlayerController* PlayerController)
{
	Super::SaveGameLoaded(PlayerController);

	CacheResolutionList(PlayerController);

#if PLATFORM_DESKTOP
	// Take the system resolution when our ResolutionIndex is invalid
	// *Should* handle resolution list changes due to video card or system configuration changes
	if (!ResolutionList.IsValidIndex(ResolutionIndex))
	{
		UpdateResolutionIndexFromSystem();
	}
#endif // PLATFORM_DESKTOP
}

void UILLVideoSettingsSaveGame::SaveGameNewlyCreated(AILLPlayerController* PlayerController)
{
	Super::SaveGameNewlyCreated(PlayerController);

	CacheResolutionList(PlayerController);
}

int32 UILLVideoSettingsSaveGame::GetOverallScalabilityLevel() const
{
	return BuildScalabilitySettings().GetSingleQualityLevel();
}

void UILLVideoSettingsSaveGame::SetOverallScalabilityLevel(int32 Value)
{
	// Build scalability settings from a single quality level
	Scalability::FQualityLevels ScalabilityQuality(BuildScalabilitySettings());
	ScalabilityQuality.SetFromSingleQualityLevel(Value);

	// Convert back
//	ResolutionScale = ScalabilityQuality.ResolutionQuality / 100.f; // Convert 0-100 to 0-1
	AntiAliasingQuality = ScalabilityQuality.AntiAliasingQuality;
	EffectsQuality = ScalabilityQuality.EffectsQuality;
	FoliageQuality = ScalabilityQuality.FoliageQuality;
	PostProcessQuality = ScalabilityQuality.PostProcessQuality;
	ShadowQuality = ScalabilityQuality.ShadowQuality;
	TextureQuality = ScalabilityQuality.TextureQuality;
	ViewDistanceQuality = ScalabilityQuality.ViewDistanceQuality;
}

Scalability::FQualityLevels UILLVideoSettingsSaveGame::BuildScalabilitySettings() const
{
	Scalability::FQualityLevels Result;
	Result.ResolutionQuality = FMath::RoundToFloat(ResolutionScale * 100.f); // Convert 0-1 to 0-100
	Result.AntiAliasingQuality = AntiAliasingQuality;
	Result.EffectsQuality = EffectsQuality;
	Result.FoliageQuality = FoliageQuality;
	Result.PostProcessQuality = PostProcessQuality;
	Result.ShadowQuality = ShadowQuality;
	Result.TextureQuality = TextureQuality;
	Result.ViewDistanceQuality = ViewDistanceQuality;
	return Result;
}

void UILLVideoSettingsSaveGame::CacheResolutionList(AILLPlayerController* PlayerController)
{
#if PLATFORM_DESKTOP
	if (ResolutionList.Num() == 0)
	{
		// Copy the cached resolution off and build a string version
		UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(PlayerController->GetGameInstance());
		ResolutionList = GameInstance->ResolutionList;
		for (const FScreenResolutionRHI& Resolution : ResolutionList)
			ResolutionStringList.Add(FString::Printf(TEXT("%4d x %4d"), Resolution.Width, Resolution.Height));
	}
#endif // PLATFORM_DESKTOP
}

void UILLVideoSettingsSaveGame::UpdateResolutionIndexFromSystem()
{
	for (int32 ResIndex = 0; ResIndex < ResolutionList.Num(); ++ResIndex)
	{
		const FScreenResolutionRHI& Resolution = ResolutionList[ResIndex];
		if (GSystemResolution.ResX == Resolution.Width && GSystemResolution.ResY == Resolution.Height)
		{
			ResolutionIndex = ResIndex;
			break;
		}
	}
}
