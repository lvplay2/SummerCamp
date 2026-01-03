// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCOutfitSlotButton.h"

USCOutfitSlotButton::USCOutfitSlotButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USCOutfitSlotButton::InitSlot(const FSCClothingSlot& SlotData)
{
	ClothingSlotData = SlotData;

	if (ClothingSlotData.SlotIcon.IsNull())
		return;

	FStringAssetReference ImageString = ClothingSlotData.SlotIcon.ToSoftObjectPath();

	FStreamableManager AssetLoader;
	AssetLoader.RequestAsyncLoad(ImageString, FStreamableDelegate(), FStreamableManager::DefaultAsyncLoadPriority, true);
}

void USCOutfitSlotButton::RemoveFromParent()
{
	FStringAssetReference ImageString = ClothingSlotData.SlotIcon.ToSoftObjectPath();

	FStreamableManager AssetLoader;
	AssetLoader.Unload(ImageString);

	Super::RemoveFromParent();
}

UTexture2D* USCOutfitSlotButton::GetSlotIcon() const
{
	return ClothingSlotData.SlotIcon.Get();
}

void USCOutfitSlotButton::OnClicked()
{
	OnSlotSelectedCallback.Broadcast(ClothingSlotData);
}