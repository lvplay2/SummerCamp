// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"

#include "SCClothingData.h"
#include "SCCounselorCharacter.h"

#include "AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeTest, "SummerCamp.Wardrobe.ClothingValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FWardrobeTest::RunTest(const FString& Parameters)
{
	FStreamableManager StreamingMan;

	// Load all clothing for all characters
	const FString FilePath(TEXT("/Game/Blueprints/ClothingData"));
	UObjectLibrary* Wardrobe = UObjectLibrary::CreateLibrary(USCClothingData::StaticClass(), true, GIsEditor);
	Wardrobe->LoadBlueprintsFromPath(FilePath);

	UObjectLibrary* Swatches = UObjectLibrary::CreateLibrary(USCClothingSlotMaterialOption::StaticClass(), true, GIsEditor);
	Swatches->LoadBlueprintsFromPath(FilePath);

	TArray<UClass*> AllClothingClasses;
	Wardrobe->GetObjects(AllClothingClasses);

	// Find our blueprint outfit data assets
	for (UClass* Class : AllClothingClasses)
	{
		// We don't care about objects that aren't saved
		if (Class->GetFlags() & EObjectFlags::RF_Transient)
			continue;

		USCClothingData* const ClothingData = Class->GetDefaultObject<USCClothingData>();
		TestTrue(FString::Printf(TEXT("Outfit %s has no outfit name!"), *ClothingData->GetName()), !ClothingData->OutfitName.IsEmptyOrWhitespace());
		TestTrue(FString::Printf(TEXT("Outfit %s has no outfit description!"), *ClothingData->GetName()), !ClothingData->OutfitDescription.IsEmptyOrWhitespace());
		TestTrue(FString::Printf(TEXT("Outfit %s has no counselor class set!"), *ClothingData->GetName()), !ClothingData->Counselor.IsNull());
		TestTrue(FString::Printf(TEXT("Outfit %s has no clothing mesh!"), *ClothingData->GetName()), ClothingData->ClothingMesh.ToSoftObjectPath().IsValid());
		if (!ClothingData->Counselor.IsNull())
		{
			ClothingData->Counselor.LoadSynchronous();
			if (ClothingData->Counselor.Get()->GetDefaultObject<ASCCounselorCharacter>()->GetIsFemale())
			{
				TestTrue(FString::Printf(TEXT("Outfit %s has no sweater mesh (and is worn by a female character)!"), *ClothingData->GetName()), ClothingData->SweaterOutfitMesh.ToSoftObjectPath().IsValid());
			}
			ClothingData->Counselor.ResetWeakPtr();
			StreamingMan.Unload(ClothingData->Counselor.ToSoftObjectPath());
		}
		int32 DismembermentMeshCount = 0;
		for (const TAssetPtr<USkeletalMesh>& MeshAssetPtr : ClothingData->DismembermentMeshes)
		{
			if (ClothingData->SkinAlphaMask.ToSoftObjectPath().IsValid())
				++DismembermentMeshCount;
		}
		TestTrue(FString::Printf(TEXT("Outfit %s has no dismemberment meshes!"), *ClothingData->GetName()), DismembermentMeshCount > 0);
		TestTrue(FString::Printf(TEXT("Outfit %s has no skin alpha mask!"), *ClothingData->GetName()), ClothingData->SkinAlphaMask.ToSoftObjectPath().IsValid());
		TestTrue(FString::Printf(TEXT("Outfit %s has no outfit icon!"), *ClothingData->GetName()), ClothingData->OutfitIcon.ToSoftObjectPath().IsValid());
		TestTrue(FString::Printf(TEXT("Outfit %s has no menu animation!"), *ClothingData->GetName()), ClothingData->MenuAnimation != nullptr);
		TestTrue(FString::Printf(TEXT("Outfit %s has no clothing slots!"), *ClothingData->GetName()), ClothingData->ClothingSlots.Num() > 0);
		for (const FSCClothingSlot& Slot : ClothingData->ClothingSlots)
		{
			TestTrue(FString::Printf(TEXT("Outfit %s slot %s has no slot name!"), *ClothingData->GetName(), *Slot.SlotName.ToString()), !Slot.SlotName.IsEmptyOrWhitespace());
			TestTrue(FString::Printf(TEXT("Outfit %s slot %s has no slot index!"), *ClothingData->GetName(), *Slot.SlotName.ToString()), Slot.SlotIndex != INDEX_NONE);
			TestTrue(FString::Printf(TEXT("Outfit %s slot %s has no slot material name!"), *ClothingData->GetName(), *Slot.SlotName.ToString()), !Slot.SlotMaterialName.IsEmpty());
			TestTrue(FString::Printf(TEXT("Outfit %s slot %s has no slot icon!"), *ClothingData->GetName(), *Slot.SlotName.ToString()), Slot.SlotIcon.ToSoftObjectPath().IsValid());
			TestTrue(FString::Printf(TEXT("Outfit %s slot %s has no material options!"), *ClothingData->GetName(), *Slot.SlotName.ToString()), Slot.MaterialOptions.Num() > 0);
			for (const TSubclassOf<USCClothingSlotMaterialOption>& MaterialOptionClass : Slot.MaterialOptions)
			{
				const USCClothingSlotMaterialOption* const Option = MaterialOptionClass.GetDefaultObject();
				if (Option)
				{
					TestTrue(FString::Printf(TEXT("Slot option %s has no material instance!"), *MaterialOptionClass->GetName()), Option->MaterialInstance.ToSoftObjectPath().IsValid());
					TestTrue(FString::Printf(TEXT("Slot option %s has no swatch icon!"), *MaterialOptionClass->GetName()), Option->SwatchIcon.ToSoftObjectPath().IsValid());
				}
				else
				{
					AddError(FString::Printf(TEXT("Outfit %s slot %s has a NULL material!"), *ClothingData->GetName(), *Slot.SlotName.ToString()));
				}
			}
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
