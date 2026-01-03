// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGVCPlayerController.h"

#include "ILLPlayerInput.h"

#include "SCInputSettingsSaveGame.h"
#include "SCLocalPlayer.h"
#include "Virtual_Cabin/SCGVCCharacter.h"
#include "Virtual_Cabin/SCGVCHUD.h"

void ASCGVCPlayerController::Possess(APawn* VCPawn)
{
	Super::Possess(VCPawn);
}

void ASCGVCPlayerController::InitInputSystem()
{
	if (!PlayerInput)
	{
		//PlayerInput = NewObject<UPlayerInput>(this);
		PlayerInput = NewObject<UILLPlayerInput>(this);
	}

	if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerInput))
	{
		// Listen for gamepad use changes
		Input->OnUsingGamepadChanged.AddDynamic(this, &ThisClass::OnUsingGamepadChanged);

		// Fake an update now
		OnUsingGamepadChanged(Input->IsUsingGamepad());
	}

	SetupInputComponent();
	
	CurrentMouseCursor = DefaultMouseCursor;
	CurrentClickTraceChannel = DefaultClickTraceChannel;
	
	/*UWorld* World = GetWorld();
	check(World);
	World->PersistentLevel->PushPendingAutoReceiveInput(this);*/

	if (Player != nullptr)
	{
		USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(GetLocalPlayer());
		if (USCInputSettingsSaveGame* InputSettings = LocalPlayer->GetLoadedSettingsSave<USCInputSettingsSaveGame>())
		{
			SetMouseSensitivity(InputSettings->MouseSensitivity);
			SetInvertedLook(InputSettings->bInvertedMouseLook, EKeys::MouseY);

			SetControllerHorizontalSensitivity(InputSettings->ControllerHorizontalSensitivity);
			SetControllerVerticalSensitivity(InputSettings->ControllerVerticalSensitivity);
			SetInvertedLook(InputSettings->bInvertedControllerLook, EKeys::Gamepad_RightY);
		}
	}
}

void ASCGVCPlayerController::HideInventory()
{
	if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(GetHUD()))
	{
		FInputModeGameOnly InputMode;

		HUD->HideInventory();

		SetInputMode(InputMode);
	}
}

void ASCGVCPlayerController::ShowInventory()
{
	if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(GetHUD()))
	{
		FInputModeGameOnly InputMode;
		HUD->ShowInventory();
		SetInputMode(InputMode);
	}
}

void ASCGVCPlayerController::HideDebugMenu()
{
	if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(GetHUD()))
	{

		FInputModeGameOnly InputMode;

		HUD->HideDebugMenu();

		SetInputMode(InputMode);
	}
}

void ASCGVCPlayerController::ShowDebugMenu()
{
	if (ASCGVCHUD* HUD = Cast<ASCGVCHUD>(GetHUD()))
	{

		FInputModeGameOnly InputMode;

		HUD->ShowDebugMenu();

		SetInputMode(InputMode);
	}
}
