// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorWardrobe.h"

#if WITH_EDITOR
# include "MessageLog.h"
# include "UObjectToken.h"
#endif

#include "SCClothingData.h"
#include "SCGameInstance.h"
#include "SCPlayerState.h"

DEFINE_LOG_CATEGORY(LogCounselorClothing);

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarSkipEntitlementCheck(TEXT("sc.ClothingSkipEntitlementCheck"), 0,
	TEXT("If turned on, entitlement check will be skipped for clothing options (everything will be unlocked)"));

TAutoConsoleVariable<int32> CVarClothingOutfitPreference(TEXT("sc.ClothingOutfitPreference"), -1,
	TEXT("If set above 0, will prefer a specific clothing index"));
#endif

int32 USCCounselorWardrobe::LastAssignedHandle(INDEX_NONE);

void USCCounselorWardrobe::Init(USCGameInstance* OwningInstance)
{
	OwningGameInstance = OwningInstance;

	// Load all clothing for all characters
	const FString FilePath(TEXT("/Game/Blueprints/ClothingData"));
	CounselorClothingLibrary = UObjectLibrary::CreateLibrary(USCClothingData::StaticClass(), true, GIsEditor);
	CounselorClothingLibrary->AddToRoot();
	CounselorClothingLibrary->LoadBlueprintsFromPath(FilePath);

	TArray<UClass*> AllClothingClasses;
	CounselorClothingLibrary->GetObjects(AllClothingClasses);

	// Find our blueprint outfit data assets
	for (UClass* Class : AllClothingClasses)
	{
		// We don't care about objects that aren't saved
		if (Class->GetFlags() & EObjectFlags::RF_Transient)
			continue;

		const TSubclassOf<USCClothingData> ClothingClass = Class;
		AllCounselorClothingOptions.AddUnique(ClothingClass);
	}

	UE_LOG(LogCounselorClothing, Log, TEXT("Counselor clothing initialized, found %d outfits"), AllCounselorClothingOptions.Num());
}

TArray<TSubclassOf<USCClothingData>> USCCounselorWardrobe::GetClothingOptions(const TSoftClassPtr<ASCCounselorCharacter> CounselorClass) const
{
	// Filter based on CounselorClass
	TArray<TSubclassOf<USCClothingData>> CounselorClothing;
	for (const TSubclassOf<USCClothingData>& Option : AllCounselorClothingOptions)
	{
		if (Option.GetDefaultObject()->Counselor == CounselorClass)
		{
			CounselorClothing.Add(Option);
		}
	}

	// Sort our outfits so they show up in a consistent order
	CounselorClothing.Sort([](const TSubclassOf<USCClothingData>& LeftData, const TSubclassOf<USCClothingData>& RightData)
	{
		return LeftData.GetDefaultObject()->SortIndex < RightData.GetDefaultObject()->SortIndex;
	});

	return CounselorClothing;
}

FSCCounselorOutfit USCCounselorWardrobe::GetRandomOutfit(const TSoftClassPtr<ASCCounselorCharacter> CounselorClass, const AILLPlayerController* PlayerController) const
{
	FSCCounselorOutfit NewOutfit;
	TArray<TSubclassOf<USCClothingData>>&& ClothingOptions = GetClothingOptions(CounselorClass);

	if (ClothingOptions.Num())
	{
		// Get unlocked subset
		TArray<TSubclassOf<USCClothingData>> UnlockedClothingOptions;
		for (const TSubclassOf<USCClothingData>& Option : ClothingOptions)
		{
			const USCClothingData* Data = Option.GetDefaultObject();
			if (!IsOutfitLockedFor(Data, PlayerController))
			{
				UnlockedClothingOptions.Add(Option);
			}
		}

		// No possible outfits (weird)
		if (UnlockedClothingOptions.Num() == 0)
		{
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FUObjectToken::Create(CounselorClass.Get()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" has no unlocked clothing options! Can't pick a random outfit from nothing!"))));
#endif
			UE_LOG(LogCounselorClothing, Error, TEXT("USCCounselorWardrobe::GetRandomOutfit: %s has no unlocked clothing options! Can't pick a random outfit from nothing!"), *GetNameSafe(CounselorClass.Get()));
			return NewOutfit;
		}

		// Pick a random outfit
		int32 ClothingIndex = FMath::RandRange(0, UnlockedClothingOptions.Num() - 1);
#if WITH_EDITOR
		// Override
		const int32 CvarIndex = CVarClothingOutfitPreference.GetValueOnGameThread();
		if (CvarIndex >= 0 && CvarIndex < UnlockedClothingOptions.Num())
			ClothingIndex = CvarIndex;
#endif

		NewOutfit.ClothingClass = UnlockedClothingOptions[ClothingIndex];

		// Find our slot variants
		const USCClothingData* OutfitData = NewOutfit.ClothingClass.GetDefaultObject();
		for (const FSCClothingSlot& Slot : OutfitData->ClothingSlots)
		{
			FSCClothingMaterialPair NewPair;
			NewPair.SlotIndex = Slot.SlotIndex;

			// Get unlocked subset
			TArray<TSubclassOf<USCClothingSlotMaterialOption>> UnlockedMaterialOptions;
			for (TSubclassOf<USCClothingSlotMaterialOption> Option : Slot.MaterialOptions)
			{
				if (!IsMaterialOptionLockedFor(Option, PlayerController))
				{
					UnlockedMaterialOptions.Add(Option);
				}
			}

			// If no variants are available, we'll stick with the default material, which probably ALSO isn't unlocked, but.... somebody configured this outfit poorly then. And that's on them, not on me. Maybe switch to an obviously incorrect material? But that could get missed... boy what a bad screw up!
			if (UnlockedMaterialOptions.Num())
			{
				// Pick a random variant
				NewPair.SelectedSwatch = UnlockedMaterialOptions[FMath::RandRange(0, UnlockedMaterialOptions.Num() - 1)];

				NewOutfit.MaterialPairs.Add(NewPair);
			}
		}
	}
	else
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FUObjectToken::Create(CounselorClass.Get()))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(" has no valid clothing options! Can't pick a random outfit from nothing!"))));
#endif
		UE_LOG(LogCounselorClothing, Error, TEXT("USCCounselorWardrobe::GetRandomOutfit: %s has no valid clothing options! Can't pick a random outfit from nothing!"), *GetNameSafe(CounselorClass.Get()));
	}

	GetSlotSelectionIndicies(NewOutfit);

	// What a fetching outfit!
	return NewOutfit;
}

TArray<int32> USCCounselorWardrobe::GetSlotSelectionIndicies(const FSCCounselorOutfit& Outfit) const
{
	TArray<int32> Indicies;
	const USCClothingData* OutfitData = Outfit.ClothingClass.GetDefaultObject();
	if (!OutfitData)
		return Indicies;

	TArray<FSCClothingMaterialPair> MaterialPairs = Outfit.MaterialPairs;
	// Sort pairs by the order of the slots they're used in
	MaterialPairs.Sort([&OutfitData](const FSCClothingMaterialPair& lh, const FSCClothingMaterialPair& rh) { return lh.SlotIndex < rh.SlotIndex; });

	// Now that we're in proper slot order, get the index of the material options
	for (const FSCClothingMaterialPair& Pair : MaterialPairs)
	{
		for (const FSCClothingSlot& Slot : OutfitData->ClothingSlots)
		{
			bool bFoundMaterial = false;
			for (int32 iMaterialOption(0); iMaterialOption < Slot.MaterialOptions.Num(); ++iMaterialOption)
			{
				if (USCClothingSlotMaterialOption* Swatch = Pair.SelectedSwatch.GetDefaultObject())
				{
					if (Slot.MaterialOptions[iMaterialOption] != nullptr)
					{
						if (USCClothingSlotMaterialOption* SlotSwatch = Cast<USCClothingSlotMaterialOption>(Slot.MaterialOptions[iMaterialOption]->GetDefaultObject()))
						{
							if (SlotSwatch->MaterialInstance == Swatch->MaterialInstance)
							{
								bFoundMaterial = true;
								Indicies.Add(iMaterialOption);
								break;
							}
						}
					}
				}
			}

			if (bFoundMaterial)
				break;
		}
	}

	return Indicies;
}

int32 USCCounselorWardrobe::StartLoadingOutfit(const FSCCounselorOutfit& Outfit, const FSCClothingLoaded& OutfitLoaded, TArray<TAssetPtr<UObject>> AdditionalDependancies /*= TArray<TAssetPtr<UObject>>()*/)
{
	return StartLoadingOutfit_Internal(Outfit, OutfitLoaded, INDEX_NONE, AdditionalDependancies);
}

void USCCounselorWardrobe::CancelLoadingOutfit(int32& OutfitHandle)
{
	const int32 OutfitIndex = FindLoadingDataIndex(OutfitHandle);
	if (OutfitIndex != INDEX_NONE)
	{
		// Cannot cancel actual async loading, but we can prevent the callback and invalidate the handle
		LoadingOutfits.RemoveAt(OutfitIndex);
	}

	UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::CancelLoadingOutfit: Outfit loading canceled for handle %d"), OutfitHandle);

	OutfitHandle = INDEX_NONE;
}

void USCCounselorWardrobe::UnloadOutfit(const FSCCounselorOutfit& Outfit)
{
	// Helper lambda for checking and then unloading an asset
	const auto UnloadAssetPtr = [&](const TAssetPtr<UObject>& AssetPtr)
	{
		if (AssetPtr.IsNull())
			return;

		OwningGameInstance->StreamableManager.Unload(AssetPtr.ToSoftObjectPath());
	};

	UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::UnloadOutfit: Starting unload of outfit %s"), *GetNameSafe(*Outfit.ClothingClass));

	bool bLoadingOutfit = false;
	for (const FSCLoadingOutfitData& LoadData : LoadingOutfits)
	{
		if (LoadData.Outfit.ClothingClass == Outfit.ClothingClass)
		{
			bLoadingOutfit = true;
		}
	}

	// Don't try and unload an asset we're trying to load
	if (!bLoadingOutfit)
	{
		const USCClothingData* OutfitData = Outfit.ClothingClass.GetDefaultObject();
		UnloadAssetPtr(OutfitData->ClothingMesh);
		UnloadAssetPtr(OutfitData->SweaterOutfitMesh);
		UnloadAssetPtr(OutfitData->SkinAlphaMask);

		for (const TAssetPtr<USkeletalMesh>& LimbMesh : OutfitData->DismembermentMeshes)
			UnloadAssetPtr(LimbMesh);
	}
	else
	{
		UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::UnloadOutfit: - Skipping unloading outfit %s due it having a load request pending."), *GetNameSafe(*Outfit.ClothingClass));
	}

	// Materials are checked seperately
	for (const FSCClothingMaterialPair& ClothingPair : Outfit.MaterialPairs)
	{
		bool bLoadingSwatch = false;
		for (const FSCLoadingOutfitData& LoadData : LoadingOutfits)
		{
			for (const FSCClothingMaterialPair& SwatchPair : LoadData.Outfit.MaterialPairs)
			{
				if (SwatchPair.SelectedSwatch == ClothingPair.SelectedSwatch)
				{
					bLoadingSwatch = true;
					break;
				}
			}

			// Don't need to look at other loading outfits
			if (bLoadingSwatch)
				break;
		}

		// We're loading this swatch, don't also unload it
		if (bLoadingSwatch)
		{
			UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::UnloadOutfit: - Skipping unloading swatch %s due it having a load request pending."), *GetNameSafe(*ClothingPair.SelectedSwatch));
			continue;
		}

		if (const USCClothingSlotMaterialOption* SlotOption = ClothingPair.SelectedSwatch.GetDefaultObject())
		{
			UnloadAssetPtr(SlotOption->MaterialInstance);
		}
	}
}

bool USCCounselorWardrobe::IsOutfitFullyLoaded(const FSCCounselorOutfit& Outfit, const int32 OutfitHandle /*= INDEX_NONE*/)
{
	const int32 OutfitIndex = FindLoadingDataIndex(OutfitHandle);
	FSCLoadingOutfitData* LoadingData = OutfitIndex != INDEX_NONE ? &LoadingOutfits[OutfitIndex] : nullptr;

	// Helper lambda for checking a TAssetPtr is valid and loaded
	const auto IsLoaded = [&](const TAssetPtr<UObject>& AssetPtr) -> bool
	{
		if (!AssetPtr.IsNull())
		{
			if (!AssetPtr.Get())
				return false;

			// Might not have an outfit handle to append assets to
			if (LoadingData)
				LoadingData->LoadedAssets.AddUnique(AssetPtr.Get());
		}

		return true;
	};

	const USCClothingData* OutfitData = Outfit.ClothingClass.GetDefaultObject();
	bool bFullLoaded = true;

	// Check all our TAssetPtrs have been loaded
	if (!IsLoaded(OutfitData->ClothingMesh))
		bFullLoaded = false;

	if (!IsLoaded(OutfitData->SweaterOutfitMesh))
		bFullLoaded = false;

	if (!IsLoaded(OutfitData->SkinAlphaMask))
		bFullLoaded = false;

	for (const TAssetPtr<USkeletalMesh>& LimbMesh : OutfitData->DismembermentMeshes)
	{
		if (!IsLoaded(LimbMesh))
			bFullLoaded = false;
	}

	for (const FSCClothingMaterialPair& ClothingPair : Outfit.MaterialPairs)
	{
		if (const USCClothingSlotMaterialOption* SlotOption = ClothingPair.SelectedSwatch.GetDefaultObject())
		{
			if (!IsLoaded(SlotOption->MaterialInstance))
				bFullLoaded = false;
		}
	}

	return bFullLoaded;
}

bool USCCounselorWardrobe::IsOutfitLockedFor(const USCClothingData* Outfit, const AILLPlayerController* PlayerController) const
{
	// Gotta have a player and an outfit!
	if (!PlayerController || !Outfit)
		return true;

	// Level check
	if (Outfit->UnlockLevel > 0)
	{
		ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PlayerController->PlayerState);
		if (!PlayerState || PlayerState->GetPlayerLevel() < Outfit->UnlockLevel)
			return true;
	}

	// Entitlement check
	if (!Outfit->UnlockEntitlement.IsEmpty())
	{
#if !UE_BUILD_SHIPPING
		// Optionally skip entitlement checks
		if (CVarSkipEntitlementCheck.GetValueOnGameThread() || CVarClothingOutfitPreference.GetValueOnGameThread() >= 0)
			return false;
#endif
		return !PlayerController->DoesUserOwnEntitlement(Outfit->UnlockEntitlement);
	}

	// Alright, we're all good here.
	return false;
}

void USCCounselorWardrobe::IsOutfitLockedFor(const USCClothingData* Outfit, const AILLPlayerController* PlayerController, bool& bIsLocked, bool& bOutShouldHide) const
{
	// BP function, useful for UI
	bIsLocked = IsOutfitLockedFor(Outfit, PlayerController);
	bOutShouldHide = bIsLocked && Outfit->bHiddenWhenLocked;
}

bool USCCounselorWardrobe::IsMaterialOptionLockedFor(TSubclassOf<USCClothingSlotMaterialOption> Option, const AILLPlayerController* PlayerController) const
{
	// Gotta have a player!
	if (!PlayerController)
		return true;

	if (Option == nullptr)
		return true;

	if (USCClothingSlotMaterialOption* DefaultOption = Cast<USCClothingSlotMaterialOption>(Option->GetDefaultObject()))
	{
		// Level check
		if (DefaultOption->UnlockLevel > 0)
		{
			ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PlayerController->PlayerState);
			if (!PlayerState || PlayerState->GetPlayerLevel() < DefaultOption->UnlockLevel)
				return true;
		}

		// Entitlement check
		if (!DefaultOption->UnlockEntitlement.IsEmpty())
		{
#if !UE_BUILD_SHIPPING
			// Optionally skip entitlement checks
			if (CVarSkipEntitlementCheck.GetValueOnGameThread())
				return false;
#endif
			return !PlayerController->DoesUserOwnEntitlement(DefaultOption->UnlockEntitlement);
		}

		// Alright, we're all good here.
		return false;
	}

	// No default object... what are you trying to wear?!
	return true;
}

void USCCounselorWardrobe::IsMaterialOptionLockedFor(TSubclassOf<USCClothingSlotMaterialOption> Option, const AILLPlayerController* PlayerController, bool& bIsLocked, bool& bOutShouldHide) const
{
	bool bHiddenWhenLocked = false;
	if (USCClothingSlotMaterialOption* DefaultOption = Cast<USCClothingSlotMaterialOption>(Option->GetDefaultObject()))
	{
		bHiddenWhenLocked = DefaultOption->bHiddenWhenLocked;
	}

	// BP function, useful for UI
	bIsLocked = IsMaterialOptionLockedFor(Option, PlayerController);
	bOutShouldHide = bIsLocked && bHiddenWhenLocked;
}

int32 USCCounselorWardrobe::StartLoadingOutfit_Internal(const FSCCounselorOutfit& Outfit, const FSCClothingLoaded& OutfitLoaded, const int32 OverrideHandle, TArray<TAssetPtr<UObject>> AdditionalDependancies)
{
	// That's not an outfit!
	if (!Outfit.ClothingClass)
	{
		UE_LOG(LogCounselorClothing, Warning, TEXT("USCCounselorWardrobe::StartLoadingOutfit_Internal: Tried to load a null outfit"));
		return INDEX_NONE;
	}

	UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::StartLoadingOutfit_Internal: Starting Async request to load outfit %s"), *GetNameSafe(Outfit.ClothingClass));

	FSCLoadingOutfitData LoadingData;

	// If we're handed an override handle we're going to build onto this loading data. INDEX_NONE means we're making a fresh handle
	const int32 DataIndex = FindLoadingDataIndex(OverrideHandle);
	if (DataIndex != INDEX_NONE)
	{
		LoadingData = LoadingOutfits[DataIndex];

		// TODO: Clean out old load data?
	}

	TArray<FStringAssetReference> AssetsToLoad;

	// Stream in an asset, if need be
	const auto CheckAssetPtr = [&](const TAssetPtr<UObject>& AssetPtr)
	{
		if (!AssetPtr.IsNull() && !AssetPtr.IsValid())
		{
			// We need to load something, assign our handle
			if (LoadingData.Handle == INDEX_NONE)
			{
				// Increment and handle overflow
				if (++LastAssignedHandle < 0)
					LastAssignedHandle = 0;

				// Fill out our data and start to track this outfit
				LoadingData.Handle = LastAssignedHandle;
				LoadingData.Outfit = Outfit;
				LoadingData.Callback = OutfitLoaded;
			}

			++LoadingData.AsyncRequests;

			AssetsToLoad.Add(AssetPtr.ToSoftObjectPath());
		}
	};

	// Check all that beautiful data
	const USCClothingData* OutfitData = Outfit.ClothingClass.GetDefaultObject();
	CheckAssetPtr(OutfitData->ClothingMesh);
	CheckAssetPtr(OutfitData->SweaterOutfitMesh);
	CheckAssetPtr(OutfitData->SkinAlphaMask);
	for (const TAssetPtr<USkeletalMesh>& LimbMesh : OutfitData->DismembermentMeshes)
		CheckAssetPtr(LimbMesh);

	for (const FSCClothingMaterialPair& ClothingPair : Outfit.MaterialPairs)
	{
		if (const USCClothingSlotMaterialOption* SlotOption = ClothingPair.SelectedSwatch.GetDefaultObject())
		{
			CheckAssetPtr(SlotOption->MaterialInstance);
		}
	}

	for (TAssetPtr<UObject> Asset : AdditionalDependancies)
		CheckAssetPtr(Asset);

	// Make our callback
	FStreamableDelegate Delegate;
	Delegate.BindUObject(this, &ThisClass::DeferredOutfitLoadCheck, LoadingData.Handle);
	OwningGameInstance->StreamableManager.RequestAsyncLoad(AssetsToLoad, Delegate);

	// Didn't need to async load anything, just callback now
	if (LoadingData.Handle == INDEX_NONE)
	{
		UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::StartLoadingOutfit_Internal: - Outfit %s already fully loaded, executing callback immediately"), *GetNameSafe(Outfit.ClothingClass));
		OutfitLoaded.ExecuteIfBound();
	}
	else
	{
		LoadingOutfits.Add(LoadingData);
		UE_LOG(LogCounselorClothing, Verbose, TEXT("USCCounselorWardrobe::StartLoadingOutfit_Internal: - Sent async request %d for %d asset%s, on handle %d"), LoadingData.AsyncRequests, AssetsToLoad.Num(), AssetsToLoad.Num() > 1 ? TEXT("s") : TEXT(""), LoadingData.Handle);
	}

	return LoadingData.Handle;
}

int32 USCCounselorWardrobe::FindLoadingDataIndex(const int32 OutfitHandle) const
{
	// Invalid handle
	if (OutfitHandle <= INDEX_NONE)
		return INDEX_NONE;

	// Find our handle in our loading list
	for (int32 iIndex(0); iIndex < LoadingOutfits.Num(); ++iIndex)
	{
		if (LoadingOutfits[iIndex].Handle == OutfitHandle)
			return iIndex;
	}

	// Guess we're not loading this outfit
	return INDEX_NONE;
}

void USCCounselorWardrobe::DeferredOutfitLoadCheck(const int32 OutfitHandle)
{
	// This outfit loading isn't being tracked
	const int32 OutfitIndex = FindLoadingDataIndex(OutfitHandle);
	if (OutfitIndex == INDEX_NONE)
	{
		UE_LOG(LogCounselorClothing, Warning, TEXT("USCCounselorWardrobe::DeferredOutfitLoadCheck: Outfit handle %d could not be found requesting a load, may have been canceled. (%d outfits currently loading)"), OutfitHandle, LoadingOutfits.Num());
		return;
	}

	// Artificial scope so we can invalidate the LoadingData ref safely
	{
		// Get outfit specific data
		FSCLoadingOutfitData& LoadingData = LoadingOutfits[OutfitIndex];

		++LoadingData.AsyncCallbacks;
		UE_LOG(LogCounselorClothing, Verbose, TEXT("USCCounselorWardrobe::DeferredOutfitLoadCheck: Outfit handle %d callback %d/%d. (%d outfits currently loading)"), OutfitHandle, LoadingData.AsyncCallbacks, LoadingData.AsyncRequests, LoadingOutfits.Num());

		// Outfit was not fully loaded, try again
		if (!IsOutfitFullyLoaded(LoadingData.Outfit, OutfitHandle))
		{
			UE_LOG(LogCounselorClothing, Verbose, TEXT("USCCounselorWardrobe::DeferredOutfitLoadCheck: - Outfit handle %d not fully loaded, requesting further loading"), OutfitHandle);
			StartLoadingOutfit_Internal(LoadingData.Outfit, LoadingData.Callback, OutfitHandle, TArray<TAssetPtr<UObject>>());
			return;
		}
	}

	// Loading is complete, remove the outfit from the list and hit our callback. This invalidates our handle too.
	FSCLoadingOutfitData LoadedData = LoadingOutfits[OutfitIndex]; // Making a copy intentionally
	LoadingOutfits.RemoveAt(OutfitIndex);

	UE_LOG(LogCounselorClothing, Log, TEXT("USCCounselorWardrobe::DeferredOutfitLoadCheck: Outfit %s async loading completed for handle %d. %d / %d total assets loaded in %d callbacks, %d outfits still loading"), *GetNameSafe(LoadedData.Outfit.ClothingClass), OutfitHandle, LoadedData.LoadedAssets.Num(), LoadedData.AsyncRequests, LoadedData.AsyncCallbacks, LoadingOutfits.Num());

	// If we got here, everything is loaded!
	LoadedData.Callback.ExecuteIfBound();
}
