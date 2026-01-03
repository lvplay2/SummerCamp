// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "ILLActionMapWidget.generated.h"

class UTexture2D;

/**
 * @struct FGamepadImages
 */
USTRUCT(BlueprintType)
struct FGamepadImages
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Face")
	UTexture2D* FaceButton_Top;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face")
	UTexture2D* FaceButton_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face")
	UTexture2D* FaceButton_Bottom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Face")
	UTexture2D* FaceButton_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "D-Pad")
	UTexture2D* DPad_Up;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "D-Pad")
	UTexture2D* DPad_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "D-Pad")
	UTexture2D* DPad_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "D-Pad")
	UTexture2D* DPad_Down;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Special")
	UTexture2D* Special_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Special")
	UTexture2D* Special_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thumbstick")
	UTexture2D* Thumbstick_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thumbstick")
	UTexture2D* Thumbstick_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shoulder")
	UTexture2D* Shoulder_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shoulder")
	UTexture2D* Shoulder_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
	UTexture2D* Trigger_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
	UTexture2D* Trigger_Right;
};

/**
 * @struct FPCImages
 */
USTRUCT(BlueprintType)
struct FPCImages
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* Key;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* Tab;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* CapsLock;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* LShift;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* RShift;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* LCtrl;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* RCtrl;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* Spacebar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* Backspace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* Enter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* ArrowUp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* ArrowRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* ArrowDown;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keyboard")
	UTexture2D* ArrowLeft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	UTexture2D* MouseLeft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	UTexture2D* MouseRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	UTexture2D* MouseMiddle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	UTexture2D* MouseScrollUp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	UTexture2D* MouseScrollDown;
};

/**
 * @class UILLActionMapWidget
 * @brief	Widget that dynamically changes its image based on the passed in Action Map name.
 *			The blueprint widget needs to have an Image component and a Text component.
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLActionMapWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UUserWidget interface
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;
	// End UUserWidget interface

	/** Update the actions this widget is using. */
	UFUNCTION(BlueprintCallable, Category = "ActionMapWidget")
	void SetActionMappings(FName NewPrimaryAction, FName NewSecondaryAction);

	/** @return Image texture to use based on bUsingGamepad. */
	UFUNCTION(BlueprintPure, Category = "ActionMapWidget")
	UTexture2D* GetImage() const;

	/** @return Visibility for GetImage. */
	UFUNCTION(BlueprintPure, Category = "ActionMapWidget")
	ESlateVisibility GetImageVisibility() const;

	/** @return Text to display for the action. */
	UFUNCTION(BlueprintPure, Category = "ActionMapWidget")
	FText GetKeyText() const;

	/** @return Visibility for GetKeyText. Visible when not using a gamepad, and there is no image or bUseKeyText is true. */
	UFUNCTION(BlueprintPure, Category = "ActionMapWidget")
	ESlateVisibility GetTextVisibility() const;

	/** Updates the uncommented private properties. */
	void CacheKeyInfo();

protected:
	// Primary action to map to
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ActionMapWidget")
	FName PrimaryAction;

	// Primary action to map to
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ActionMapWidget")
	FName SecondaryAction;

	// Images used for PS4 gamepad controls
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActionMapWidget")
	FGamepadImages PS4GamepadImages;

	// Images used for XboxOne gamepad controls
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActionMapWidget")
	FGamepadImages XboxOneGamepadImages;

	// Images used for PC controls
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ActionMapWidget", meta = (DisplayName = "PC Images"))
	FPCImages PCImages;

	// Image to use instead of PCImages or the text when not using a gamepad
	UPROPERTY(EditAnywhere, Category = "ActionMapWidget", meta = (DisplayName = "PC Image Override"))
	UTexture2D* PCImageOverride;

	UPROPERTY(EditAnywhere, Category = "ActionMapWidget")
	bool bUseSpaceText;

	UPROPERTY(EditAnywhere, Category = "ActionMapWidget")
	bool bUSeShiftText;

	UPROPERTY(EditAnywhere, Category = "ActionMapWidget")
	bool bUseCtrlText;

    // Moved to public so that it can be updated at runtime @cpederson
	/** Updates the uncommented private properties. */
	//void CacheKeyInfo();
    // End move

	/** @return Key corresponding to the PrimaryAction or SecondaryAction, matching bIsGamepad. */
	FKey GetActionKey(const bool bIsGamepad);

	/** @return Texture in GamepadImages or PCImages corresponding to Key. */
	UTexture2D* GetImageForKey(const FKey& Key);

private:
	// Image to use for non-gamepad
	UPROPERTY(Transient)
	UTexture2D* MappedPCImage;

	// Image to use for gamepad
	UPROPERTY(Transient)
	UTexture2D* MappedGamepadImage;

	// Text to display
	FText KeyText;

	// Show KeyText instead of an image?
	bool bUseKeyText;

	// Is the local player using a gamepad? Cached in NativeTick
	bool bUsingGamepad;
};
