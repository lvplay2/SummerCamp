// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCharacterBodyPartComponent.h"
#include "SCClothingData.generated.h"

class ASCCounselorCharacter;
class ASCPlayerState;

class USCClothingData;
class USCClothingSlotMaterialOption;

/**
 * @struct FSCClothingMaterialPair
 * @brief Pair of material index and instance to swap out on the character
 */
USTRUCT(BlueprintType)
struct FSCClothingMaterialPair
{
	GENERATED_BODY()

public:
	// Material index for the slot we're swapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	int32 SlotIndex;

	// Material instance to swap the slot to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	TSubclassOf<USCClothingSlotMaterialOption> SelectedSwatch;
};

/**
 * @struct FSCCounselorOutfit
 * @brief Contains a ClothingClass to get the outfit mesh and alpha mask from as well
 *  as a list of material pairs for the chosen tint/pattern for the outfit
 */
USTRUCT(BlueprintType)
struct FSCCounselorOutfit
{
	GENERATED_BODY()

public:
	// Clothing asset for this selected outfit (used to get the skeletal mesh and alpha mask)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clothing")
	TSubclassOf<USCClothingData> ClothingClass;

	// List of materials we're swapping out, and where. Should always be one per slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clothing")
	TArray<FSCClothingMaterialPair> MaterialPairs;

	FORCEINLINE bool operator ==(const FSCCounselorOutfit& Other) const
	{
		// Needs to be the same outfit
		if (ClothingClass != Other.ClothingClass)
			return false;

		// Same number of material swaps (unlikely this will fail)
		if (MaterialPairs.Num() != Other.MaterialPairs.Num())
			return false;

		// Search for out of order index matchups (the character doesn't care the order)
		for (const FSCClothingMaterialPair& Pair : MaterialPairs)
		{
			bool bFoundMatch = false;
			for (const FSCClothingMaterialPair& OtherPair : Other.MaterialPairs)
			{
				if (Pair.SlotIndex == OtherPair.SlotIndex)
				{
					if (Pair.SelectedSwatch != OtherPair.SelectedSwatch)
						return false;

					bFoundMatch = true;
					break;
				}
			}

			// No matching index
			if (!bFoundMatch)
				return false;
		}

		// Everything matches up!
		return true;
	}

	FORCEINLINE bool operator !=(const FSCCounselorOutfit& Other) const
	{
		return !operator ==(Other);
	}
};

// ----------- Config Classes ------------

/**
 * @class USCClothingSlotMaterialOption
 * @brief A tint/pattern option for a given outfit slot with optional unlock requirements
 */
UCLASS(Blueprintable, NotPlaceable)
class SUMMERCAMP_API USCClothingSlotMaterialOption
: public UObject
{
	GENERATED_BODY()

public:
	// Material for this option (tint/pattern/etc.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<UMaterialInstance> MaterialInstance;

	// Icon for this tint/pattern in the UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<UTexture2D> SwatchIcon;

	// DLC entitlement required to use this material (empty for none), generally used by outfits
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString UnlockEntitlement;

	// Level requirement to use this material (0 for no requirement)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 UnlockLevel;

	// If true, this outfit will not be shown at all unless you have the DLC/level requirements
	// If false, maybe re-direct to the DLC store page? Or tell them what level they need to be.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHiddenWhenLocked;
};

/**
 * @struct FSCClothingSlot
 * @brief Outfit slot such as shirt, pants or shoes. Contains a list of tint/patterns options for this slot
 */
USTRUCT(BlueprintType)
struct FSCClothingSlot
{
	GENERATED_BODY()

public:
	FSCClothingSlot()
	: SlotIndex(INDEX_NONE)
	{
	}

	// Name of our slot (shirt, pants, hat, etc.) displayed to the user, will be localized
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText SlotName;

	// Unique number for this slot, used to sort slots and map materials to material indicies
	// NOTE: This may not corrispond to actual index in ClothingSlots! You must search for a match.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 SlotIndex;

	// String used to find the material in our skeletal mesh or dismemberment mesh to use for this slot
	// Can be the same as SlotName, just won't be localized
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString SlotMaterialName;

	// Icon for this slot in the UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<UTexture2D> SlotIcon;

	// List of all the material swap options for this slot (tints/patterns/etc.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<USCClothingSlotMaterialOption>> MaterialOptions;
};

/**
 * @class FSCClothingDataAsset
 * @brief Full outfit information with a list of all the slots possible and optional unlock requirements
 */
UCLASS(Blueprintable, NotPlaceable)
class SUMMERCAMP_API USCClothingData
: public UObject
{
	GENERATED_BODY()

public:
	USCClothingData(const FObjectInitializer& ObjectInitializer);

	// Name of our outfit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText OutfitName;

	// Description of outfit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText OutfitDescription;

	// Order this outfit should show up in the clothing menu
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 SortIndex;

	// Position of the flashlight on the character model
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FTransform FlashlightTransform;

	// Position of the walkie talkie on the character model
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FTransform WalkieTalkieTransform;

	// Counselor allowed to wear this outfit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<ASCCounselorCharacter> Counselor;

	// Actual mesh of the outfit to apply to the character model
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<USkeletalMesh> ClothingMesh;

	// Outfit to apply when wearing the sweater (only for the ladies)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<USkeletalMesh> SweaterOutfitMesh;

	// List of all dismemeberment meshes for the given outfit
	UPROPERTY(EditDefaultsOnly)
	TAssetPtr<USkeletalMesh> DismembermentMeshes[(uint8)ELimb::MAX];

	// Alpha mask to hide the skin when wearing this outfit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<UTexture2D> SkinAlphaMask;

	// Icon to display in the UI when selecting this outfit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TAssetPtr<UTexture2D> OutfitIcon;

	// The animation to ply on the character in the menus
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* MenuAnimation;

	// DLC entitlement required to use this outfit (leave empty for no requirement)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString UnlockEntitlement;

	// Level requirement to use this outfit (0 for no requirement), generally used by material options
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 UnlockLevel;

	// If true, this outfit will not be shown at all unless you have the DLC/level requirements
	// If false, maybe re-direct to the DLC store page? Or tell them what level they need to be.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHiddenWhenLocked;

	// List of all our slots for this clothing (shirt, pants, hat, etc.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FSCClothingSlot> ClothingSlots;

	/** Work around for blueprint not being able to read static arrays */
	UFUNCTION(BlueprintPure, Category="Clothing")
	TAssetPtr<USkeletalMesh> GetDismembermentMesh(const ELimb Limb) const { return DismembermentMeshes[(uint8)Limb]; }

	/**
	 * Finds the material index on the given skeletal mesh that matches the slot index (may be INDEX_NONE!)
	 * @param Mesh - Mesh to search for a matching material on
	 * @param SlotIndex - Slot to use to search for material with
	 * @return Material index on the skeletal mesh that is probably totally cool to replace, INDEX_NONE if none found
	 */
	UFUNCTION(BlueprintPure, Category="Clothing")
	int32 GetMaterialIndexFromSlotIndex(const USkeletalMesh* Mesh, const int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Clothing")
	bool IsClothingSwatchValidForOutfit(TSubclassOf<USCClothingSlotMaterialOption> Swatch) const;
};
