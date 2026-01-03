// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSwatchButton.h"
#include "SCGameInstance.h"
#include "SCCounselorWardrobe.h"

USCSwatchButton::USCSwatchButton(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCSwatchButton::InitializeButton(TSubclassOf<USCClothingSlotMaterialOption> _SwatchClass, bool Unlocked/*=true*/, bool ShowStore/*=false*/)
{
	SwatchClass = _SwatchClass;
	bLocked = !Unlocked;
	bShowStore = ShowStore;

	if (SwatchClass != nullptr)
	{
		if (const USCClothingSlotMaterialOption* const SD = Cast<USCClothingSlotMaterialOption>(SwatchClass->GetDefaultObject()))
		{
			if (SD->SwatchIcon.IsNull())
				return;

			FStringAssetReference ButtonImage = SD->SwatchIcon.ToSoftObjectPath();

			FStreamableManager AssetLoader;
			AssetLoader.RequestAsyncLoad(ButtonImage, FStreamableDelegate(), FStreamableManager::DefaultAsyncLoadPriority, true);

			UnlockLevel = SD->UnlockLevel;
		}
	}
}

void USCSwatchButton::RemoveFromParent()
{
	if (SwatchClass != nullptr)
	{
		if (const USCClothingSlotMaterialOption* const SD = Cast<USCClothingSlotMaterialOption>(SwatchClass->GetDefaultObject()))
		{
			FStringAssetReference ButtonImage = SD->SwatchIcon.ToSoftObjectPath();

			FStreamableManager AssetLoader;
			AssetLoader.Unload(ButtonImage);
		}
	}

	Super::RemoveFromParent();
}

UTexture2D* USCSwatchButton::GetButtonImage() const
{
	if (SwatchClass != nullptr)
	{
		if (const USCClothingSlotMaterialOption* const SD = Cast<USCClothingSlotMaterialOption>(SwatchClass->GetDefaultObject()))
		{
			return SD->SwatchIcon.Get();
		}
	}

	return nullptr;
}


void USCSwatchButton::OnClicked()
{
	if (bLocked)
	{
		if (bShowStore)
			OnClickedLockedCallback.ExecuteIfBound();

		return;
	}

	SetSelected(true);
	OnClickedCallback.ExecuteIfBound(this);
}

void USCSwatchButton::OnHighlighted()
{
	bHighlighted = true;
	OnHighlightedCallback.ExecuteIfBound(this);
}

void USCSwatchButton::OnUnHighlighted()
{
	bHighlighted = false;
}

FString USCSwatchButton::GetHighlightText() const
{
	if (bLocked)
	{
		if (UnlockLevel > 0)
			return FString::Printf(TEXT("Unlocks at level %d."), UnlockLevel);
		else
			return FString(TEXT("Select to open store page."));
	}

	return TEXT("");
}

ESlateVisibility USCSwatchButton::GetSelectedVisibility() const
{
	return bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility USCSwatchButton::GetLockedVisibility() const
{
	return (bLocked && !bHighlighted) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}
