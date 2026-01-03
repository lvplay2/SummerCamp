// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCOutfitButton.h"
#include "SCGameInstance.h"
#include "SCCounselorWardrobe.h"
#include "SCCounselorWardrobeWidget.h"

USCOutfitButton::USCOutfitButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USCOutfitButton::InitializeButton(TSubclassOf<USCClothingData> _OutfitClass, USCCounselorWardrobeWidget* OwningMenu, bool Unlocked/*=true*/, bool ShowStore/*=false*/)
{
	OutfitClass = _OutfitClass;

	bLocked = !Unlocked;
	bShowStore = ShowStore;

	ParentMenu = OwningMenu;

	if (OutfitClass != nullptr)
	{
		if (const USCClothingData* const CD = Cast<USCClothingData>(OutfitClass->GetDefaultObject()))
		{
			if (CD->OutfitIcon.IsNull())
				return;

			FStringAssetReference ButtonImage = CD->OutfitIcon.ToSoftObjectPath();

			FStreamableManager AssetLoader;
			AssetLoader.RequestAsyncLoad(ButtonImage, FStreamableDelegate(), FStreamableManager::DefaultAsyncLoadPriority, true);

			DescriptionText = CD->OutfitDescription.ToString();
			UnlockLevel = CD->UnlockLevel;
		}
	}
}

void USCOutfitButton::RemoveFromParent()
{
	if (OutfitClass != nullptr)
	{
		if (const USCClothingData* const CD = Cast<USCClothingData>(OutfitClass->GetDefaultObject()))
		{
			FStringAssetReference ButtonImage = CD->OutfitIcon.ToSoftObjectPath();

			FStreamableManager AssetLoader;
			AssetLoader.Unload(ButtonImage);
		}
	}

	Super::RemoveFromParent();
}

UTexture2D* USCOutfitButton::GetButtonImage() const
{
	if (OutfitClass != nullptr)
	{
		if (const USCClothingData* const CD = Cast<USCClothingData>(OutfitClass->GetDefaultObject()))
		{
			return CD->OutfitIcon.Get();
		}
	}

	return nullptr;
}


void USCOutfitButton::OnClicked()
{
	if (ParentMenu && !ParentMenu->AllowedToSetOutfit())
		return;

	if (bLocked)
	{
		if (bShowStore)
			OnClickedLockedCallback.ExecuteIfBound();

		return;
	}

	SetSelected(true);
	OnClickedCallback.ExecuteIfBound(this);
}

void USCOutfitButton::OnHighlighted()
{
	bHighlighted = true;
	OnHighlightedCallback.ExecuteIfBound(this);
}

void USCOutfitButton::OnUnHighlighted()
{
	bHighlighted = false;
}

FString USCOutfitButton::GetHighlightText() const
{
	if (bLocked)
	{
		if (UnlockLevel > 0)
			return FString::Printf(TEXT("Unlocks at level %d."), UnlockLevel);
		else
			return FString(TEXT("Select to open store page."));
	}

	if (bHighlighted)
	{
		return DescriptionText;
	}

	return TEXT("");
}

ESlateVisibility USCOutfitButton::GetSelectedVisibility() const
{
	return bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility USCOutfitButton::GetLockedVisibility() const
{
	return (bLocked && !bHighlighted) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}