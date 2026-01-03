// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "ILLGameFramework.h"
#include "ILLActionMapWidget.h"

#include "ILLPlayerController.h"
#include "ILLPlayerInput.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLActionMapWidget"

UILLActionMapWidget::UILLActionMapWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bUseSpaceText = false;
	bUseCtrlText = false;
	bUSeShiftText = false;
}

void UILLActionMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (AILLPlayerController* Controller = Cast<AILLPlayerController>(GetOwningPlayer()))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(Controller->PlayerInput))
		{
			if (bUsingGamepad != Input->IsUsingGamepad())
			{
				bUsingGamepad = Input->IsUsingGamepad();

			}
			
			// Make sure we update our cached data if it doesn't exist for some raisin.
			if (MappedGamepadImage == nullptr || MappedPCImage == nullptr)
					CacheKeyInfo();
		}
	}

	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UILLActionMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheKeyInfo();
}

void UILLActionMapWidget::SetActionMappings(FName NewPrimaryAction, FName NewSecondaryAction)
{
	PrimaryAction = NewPrimaryAction;
	SecondaryAction = NewSecondaryAction;

	CacheKeyInfo();
}

UTexture2D* UILLActionMapWidget::GetImage() const
{
	return bUsingGamepad ? MappedGamepadImage : MappedPCImage;
}

ESlateVisibility UILLActionMapWidget::GetImageVisibility() const
{
	return GetImage() == nullptr ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
}

FText UILLActionMapWidget::GetKeyText() const
{
	return KeyText;
}

ESlateVisibility UILLActionMapWidget::GetTextVisibility() const
{
	return !bUsingGamepad && !PCImageOverride && (bUseKeyText || MappedPCImage == nullptr) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

void UILLActionMapWidget::CacheKeyInfo()
{
	// Find the MappedPCImage
	if (PCImageOverride)
	{
		MappedPCImage = PCImageOverride;
	}
	else
	{
		const FKey PCKey = GetActionKey(false);
		if (PCKey != EKeys::AnyKey)
			MappedPCImage = GetImageForKey(PCKey);
	}

	// Find the MappedGamepadImage
	const FKey GamepadKey = GetActionKey(true);
	if (GamepadKey != EKeys::AnyKey)
		MappedGamepadImage = GetImageForKey(GamepadKey);
}

FKey UILLActionMapWidget::GetActionKey(const bool bIsGamepad)
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* Controller = World->GetFirstPlayerController())
		{
			if (const UPlayerInput* Settings = Controller->PlayerInput)
			{
				// Search for a matching action mapping
				for (FInputActionKeyMapping Mapping : Settings->ActionMappings)
				{
					if (Mapping.Key.IsGamepadKey() == bIsGamepad)
					{
						if (PrimaryAction == Mapping.ActionName || SecondaryAction == Mapping.ActionName)
						{
							return Mapping.Key;
						}
					}
				}

				// Search for a matching axis mapping
				for (FInputAxisKeyMapping Mapping : Settings->AxisMappings)
				{
					if (Mapping.Key.IsGamepadKey() == bIsGamepad)
					{
						if (PrimaryAction == Mapping.AxisName || SecondaryAction == Mapping.AxisName)
						{
							return Mapping.Key;
						}
					}
				}
			}
		}
	}

	return EKeys::AnyKey;
}

#define ISKEY(OtherKey) Key == EKeys::OtherKey

UTexture2D* UILLActionMapWidget::GetImageForKey(const FKey& Key)
{
	if (Key.IsGamepadKey())
	{
#if PLATFORM_PS4
		const FGamepadImages& GamepadImages = PS4GamepadImages;
#else
		const FGamepadImages& GamepadImages = XboxOneGamepadImages;
#endif

		if (ISKEY(Gamepad_FaceButton_Bottom))
		{
			return GamepadImages.FaceButton_Bottom;
		}
		else if (ISKEY(Gamepad_FaceButton_Right))
		{
			return GamepadImages.FaceButton_Right;
		}
		else if (ISKEY(Gamepad_FaceButton_Left))
		{
			return GamepadImages.FaceButton_Left;
		}
		else if (ISKEY(Gamepad_FaceButton_Top))
		{
			return GamepadImages.FaceButton_Top;
		}
		else if (ISKEY(Gamepad_DPad_Up))
		{
			return GamepadImages.DPad_Up;
		}
		else if (ISKEY(Gamepad_DPad_Down))
		{
			return GamepadImages.DPad_Down;
		}
		else if (ISKEY(Gamepad_DPad_Right))
		{
			return GamepadImages.DPad_Right;
		}
		else if (ISKEY(Gamepad_DPad_Left))
		{
			return GamepadImages.DPad_Left;
		}
		else if (ISKEY(Gamepad_LeftX) || ISKEY(Gamepad_LeftY) || ISKEY(Gamepad_LeftThumbstick))
		{
			return GamepadImages.Thumbstick_Left;
		}
		else if(ISKEY(Gamepad_RightX) || ISKEY(Gamepad_RightY) || ISKEY(Gamepad_RightThumbstick))
		{
			return GamepadImages.Thumbstick_Right;
		}
		else if (ISKEY(Gamepad_LeftTriggerAxis) || ISKEY(Gamepad_LeftTrigger))
		{
			return GamepadImages.Trigger_Left;
		}
		else if (ISKEY(Gamepad_RightTriggerAxis) || ISKEY(Gamepad_RightTrigger))
		{
			return GamepadImages.Trigger_Right;
		}
		else if (ISKEY(Gamepad_LeftShoulder))
		{
			return GamepadImages.Shoulder_Left;
		}
		else if (ISKEY(Gamepad_RightShoulder))
		{
			return GamepadImages.Shoulder_Right;
		}
		else if (ISKEY(Gamepad_Special_Left))
		{
			return GamepadImages.Special_Left;
		}
		else if (ISKEY(Gamepad_Special_Right))
		{
			return GamepadImages.Special_Right;
		}
	}
	else if (Key.IsMouseButton())
	{
		KeyText = Key.GetDisplayName();
		if (ISKEY(LeftMouseButton))
		{
			return PCImages.MouseLeft;
		}
		else if (ISKEY(RightMouseButton))
		{
			return PCImages.MouseRight;
		}
		else if (ISKEY(MiddleMouseButton))
		{
			return PCImages.MouseMiddle;
		}
		else if (ISKEY(MouseScrollUp))
		{
			return PCImages.MouseScrollUp;
		}
		else if (ISKEY(MouseScrollDown))
		{
			return PCImages.MouseScrollDown;
		}
	}
	else
	{
		KeyText = Key.GetDisplayName();
		if (ISKEY(Tab))
		{
			bUseKeyText = false;
			return PCImages.Tab;
		}
		else if (ISKEY(CapsLock))
		{
			bUseKeyText = false;
			return PCImages.CapsLock;
		}
		else if (ISKEY(LeftShift))
		{
			bUseKeyText = bUSeShiftText;
			KeyText = FText::FromString(TEXT("L-Shift"));
			return PCImages.LShift;
		}
		else if (ISKEY(RightShift))
		{
			bUseKeyText = bUSeShiftText;
			KeyText = FText::FromString(TEXT("R-Shift"));
			return PCImages.RShift;
		}
		else if (ISKEY(LeftControl))
		{
			bUseKeyText = bUseCtrlText;
			KeyText = FText::FromString(TEXT("L-Ctrl"));
			return PCImages.LCtrl;
		}
		else if (ISKEY(RightControl))
		{
			bUseKeyText = bUseCtrlText;
			KeyText = FText::FromString(TEXT("R-Ctrl"));
			return PCImages.RCtrl;
		}
		else if (ISKEY(SpaceBar))
		{
			bUseKeyText = bUseSpaceText;
			KeyText = FText::FromString(TEXT("Space"));
			return PCImages.Spacebar;
		}
		else if (ISKEY(BackSpace))
		{
			bUseKeyText = false;
			return PCImages.Backspace;
		}
		else if (ISKEY(Enter))
		{
			bUseKeyText = false;
			return PCImages.Enter;
		}
		else if (ISKEY(Up))
		{
			bUseKeyText = false;
			return PCImages.ArrowUp;
		}
		else if (ISKEY(Right))
		{
			bUseKeyText = false;
			return PCImages.ArrowRight;
		}
		else if (ISKEY(Down))
		{
			bUseKeyText = false;
			return PCImages.ArrowDown;
		}
		else if (ISKEY(Left))
		{
			bUseKeyText = false;
			return PCImages.ArrowLeft;
		}
		
		bUseKeyText = true;

		// TODO: pjackson: Localize?
		if (ISKEY(Delete))
			KeyText = FText::FromString(TEXT("Del"));
		else if (ISKEY(Insert))
			KeyText = FText::FromString(TEXT("Ins"));
		else if (ISKEY(Multiply))
			KeyText = FText::FromString(TEXT("*"));
		else if (ISKEY(Add))
			KeyText = FText::FromString(TEXT("+"));
		else if (ISKEY(Subtract) || ISKEY(Hyphen))
			KeyText = FText::FromString(TEXT("-"));
		else if (ISKEY(Decimal) || ISKEY(Period))
			KeyText = FText::FromString(TEXT("."));
		else if (ISKEY(Divide) || ISKEY(Slash))
			KeyText = FText::FromString(TEXT("/"));
		else if (ISKEY(Zero) || ISKEY(NumPadZero))
			KeyText = FText::FromString(TEXT("0"));
		else if (ISKEY(One) || ISKEY(NumPadOne))
			KeyText = FText::FromString(TEXT("1"));
		else if (ISKEY(Two) || ISKEY(NumPadTwo))
			KeyText = FText::FromString(TEXT("2"));
		else if (ISKEY(Three) || ISKEY(NumPadThree))
			KeyText = FText::FromString(TEXT("3"));
		else if (ISKEY(Four) || ISKEY(NumPadFour))
			KeyText = FText::FromString(TEXT("4"));
		else if (ISKEY(Five) || ISKEY(NumPadFive))
			KeyText = FText::FromString(TEXT("5"));
		else if (ISKEY(Six) || ISKEY(NumPadSix))
			KeyText = FText::FromString(TEXT("6"));
		else if (ISKEY(Seven) || ISKEY(NumPadSeven))
			KeyText = FText::FromString(TEXT("7"));
		else if (ISKEY(Eight) || ISKEY(NumPadEight))
			KeyText = FText::FromString(TEXT("8"));
		else if (ISKEY(Nine) || ISKEY(NumPadNine))
			KeyText = FText::FromString(TEXT("9"));
		else if (ISKEY(Semicolon))
			KeyText = FText::FromString(TEXT(";"));
		else if (ISKEY(Equals))
			KeyText = FText::FromString(TEXT("="));
		else if (ISKEY(Comma))
			KeyText = FText::FromString(TEXT(","));
		else if (ISKEY(LeftBracket))
			KeyText = FText::FromString(TEXT("["));
		else if (ISKEY(RightBracket))
			KeyText = FText::FromString(TEXT("]"));
		else if (ISKEY(Backslash))
			KeyText = FText::FromString(TEXT("\\"));
		else if (ISKEY(Apostrophe))
			KeyText = FText::FromString(TEXT("'"));

		return PCImages.Key;
	}

	return nullptr;
}

#undef ISKEY

#undef LOCTEXT_NAMESPACE
