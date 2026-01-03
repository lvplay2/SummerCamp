// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"
#include "ILLInputSettingsSaveGame.generated.h"

/**
 * @class UILLInputSettingsSaveGame
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLInputSettingsSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	// End UILLPlayerSettingsSaveGame interface

	//////////////////////////////////////////////////
	// General input settings

	// Hold button to crouch instead of toggling?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	bool bToggleCrouch;

	//////////////////////////////////////////////////
	// Mouse input settings

	// Mouse sensitivity
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	float MouseSensitivity;

	// Invert mouse pitch?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	bool bInvertedMouseLook;

	// Mouse smoothing?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	bool bMouseSmoothing;

	//////////////////////////////////////////////////
	// Controller input settings

	// Horizontal controller sensitivity
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	float ControllerHorizontalSensitivity;

	// Vertical controller sensitivity
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	float ControllerVerticalSensitivity;

	// Enable controller vibration?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	bool bControllerVibrationEnabled;

	// Invert controller look?
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Input")
	bool bInvertedControllerLook;
};
