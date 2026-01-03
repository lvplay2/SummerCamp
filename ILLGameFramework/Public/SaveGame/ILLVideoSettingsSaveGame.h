// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"
#include "ILLVideoSettingsSaveGame.generated.h"

/**
 * @enum EILLWindowMode
 * MUST MATCH EWindowMode! Which does not work with Blueprint.
 */
UENUM(BlueprintType)
enum class EILLWindowMode : uint8
{
	Fullscreen,
	WindowedFullscreen,
	Windowed,
};

/**
 * @class UILLVideoSettingsSaveGame
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLVideoSettingsSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	virtual void SaveGameLoaded(AILLPlayerController* PlayerController) override;
	virtual void SaveGameNewlyCreated(AILLPlayerController* PlayerController) override;
	// End UILLPlayerSettingsSaveGame interface

	/** Changes the scalability settings to the level Value. */
	UFUNCTION(BlueprintCallable, Category="PlayerSettings|Video")
	virtual void SetOverallScalabilityLevel(int32 Value);

	/** @return Overall scalability level, evaluated from all of the scalability settings. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Video")
	virtual int32 GetOverallScalabilityLevel() const;

protected:
	/** @return these settings converted into scalability settings. */
	virtual Scalability::FQualityLevels BuildScalabilitySettings() const;

	/** Helper function for populating ResolutionList and ResolutionStringList. */
	virtual void CacheResolutionList(AILLPlayerController* PlayerController);

	/** Helper function to set ResolutionIndex to the system resolution. */
	virtual void UpdateResolutionIndexFromSystem();

public:
	// Resolution options
	FScreenResolutionArray ResolutionList;

	// Resolution options in string form
	UPROPERTY(BlueprintReadOnly, Category="PlayerSettings|Video")
	TArray<FString> ResolutionStringList;

	// Gamma
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	float Gamma;

	// Index into ResolutionStringList for the current resolution
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 ResolutionIndex;

	// Fullscreen mode to use
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	EILLWindowMode FullscreenMode;

	// Enable vertical sync?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	bool bVerticalSync;

	// Resolution scale scalability setting
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	float ResolutionScale;

	// Scalability anti-aliasing quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 AntiAliasingQuality;

	// Scalability effects quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 EffectsQuality;

	// Scalability foliage quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 FoliageQuality;

	// Scalability post-process quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 PostProcessQuality;

	// Scalability shadow quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 ShadowQuality;

	// Scalability texture quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 TextureQuality;

	// Scalability view distance quality
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Video")
	int32 ViewDistanceQuality;
};
