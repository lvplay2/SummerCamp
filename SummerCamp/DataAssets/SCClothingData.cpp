// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCClothingData.h"

#include "SCPlayerState.h"

USCClothingData::USCClothingData(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FlashlightTransform.SetScale3D(FVector::ZeroVector);
	WalkieTalkieTransform.SetScale3D(FVector::ZeroVector);
}

int32 USCClothingData::GetMaterialIndexFromSlotIndex(const USkeletalMesh* Mesh, const int32 SlotIndex) const
{
	if (!Mesh || SlotIndex == INDEX_NONE)
		return INDEX_NONE;

	FString MaterialName;
	for (const FSCClothingSlot& Slot : ClothingSlots)
	{
		if (Slot.SlotIndex == SlotIndex)
		{
			MaterialName = Slot.SlotMaterialName;
			break;
		}
	}

	// Didn't find one, or the material name wasn't filled out for the slot
	if (MaterialName.IsEmpty())
		return INDEX_NONE;

	for (int32 iMat(0); iMat < Mesh->Materials.Num(); ++iMat)
	{
		const FSkeletalMaterial& SkeletalMat = Mesh->Materials[iMat];
		if (!SkeletalMat.MaterialInterface)
			continue;

		if (SkeletalMat.MaterialInterface->GetName().Contains(MaterialName))
		{
			return iMat;
		}
	}

	return INDEX_NONE;
}

bool USCClothingData::IsClothingSwatchValidForOutfit(TSubclassOf<USCClothingSlotMaterialOption> Swatch) const
{
	for (const FSCClothingSlot& Slot : ClothingSlots)
	{
		for (TSubclassOf<USCClothingSlotMaterialOption> SlotSwatch : Slot.MaterialOptions)
		{
			if (SlotSwatch == Swatch)
				return true;
		}
	}

	return false;
}