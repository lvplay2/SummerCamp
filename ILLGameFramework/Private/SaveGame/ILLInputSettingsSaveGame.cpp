// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLInputSettingsSaveGame.h"

#include "ILLPlayerController.h"

UILLInputSettingsSaveGame::UILLInputSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bToggleCrouch = false;

	MouseSensitivity = .07f;
	bInvertedMouseLook = false;
	bMouseSmoothing = false;

	ControllerHorizontalSensitivity = 2.f;
	ControllerVerticalSensitivity = 2.f;
	bControllerVibrationEnabled = true;
	bInvertedControllerLook = false;
}

void UILLInputSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	if (bAndSave)
	{
		// This is gnarly, but also the only way of doing it! Yay!
		UInputSettings* DefaultInputSettings = const_cast<UInputSettings*>(GetDefault<UInputSettings>());
		DefaultInputSettings->bEnableMouseSmoothing = bMouseSmoothing;

		if (PlayerController)
		{
			PlayerController->SetMouseSensitivity(MouseSensitivity);
			PlayerController->SetInvertedLook(bInvertedMouseLook, EKeys::MouseY);
			PlayerController->SetControllerHorizontalSensitivity(ControllerHorizontalSensitivity);
			PlayerController->SetControllerVerticalSensitivity(ControllerVerticalSensitivity);
			// ILLPlayerController already handles bControllerVibrationEnabled in ClientPlayForceFeedback
			PlayerController->SetInvertedLook(bInvertedControllerLook, EKeys::Gamepad_RightY);
		}
	}
}

bool UILLInputSettingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	UILLInputSettingsSaveGame* Other = CastChecked<UILLInputSettingsSaveGame>(OtherSave);
	return(bToggleCrouch != Other->bToggleCrouch
		|| MouseSensitivity != Other->MouseSensitivity
		|| bInvertedMouseLook != Other->bInvertedMouseLook
		|| bMouseSmoothing != Other->bMouseSmoothing
		|| ControllerHorizontalSensitivity != Other->ControllerHorizontalSensitivity
		|| ControllerVerticalSensitivity != Other->ControllerVerticalSensitivity
		|| bControllerVibrationEnabled != Other->bControllerVibrationEnabled
		|| bInvertedControllerLook != Other->bInvertedControllerLook);
}
