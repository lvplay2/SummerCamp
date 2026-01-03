// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCClothingData.h"
#include "SCCounselorWardrobe.generated.h"

class AILLPlayerController;

class ASCCounselorCharacter;

class USCClothingData;
class USCGameInstance;

DECLARE_DYNAMIC_DELEGATE(FSCClothingLoaded);

DECLARE_LOG_CATEGORY_EXTERN(LogCounselorClothing, Warning, All);

/**
 * @struct FSCLoadingOutfitData
 */
USTRUCT()
struct FSCLoadingOutfitData
{
	GENERATED_BODY()

public:
	FSCLoadingOutfitData()
	: Handle(INDEX_NONE)
	, AsyncRequests(0)
	, AsyncCallbacks(0)
	{
	}

	// Handle set by StartLoadingOutfit, used for canceling
	UPROPERTY()
	int32 Handle;

	// Outfit we're loading
	UPROPERTY()
	FSCCounselorOutfit Outfit;

	// List of strongly referenced assets so they don't get unloaded while we're waiting for other assets to load
	UPROPERTY()
	TArray<UObject*> LoadedAssets;

	// Number of assets we requested for async loading
	UPROPERTY()
	int32 AsyncRequests;

	// Number of times our callback was hit
	UPROPERTY()
	int32 AsyncCallbacks;

	// Callback to make when the outfit is loaded
	UPROPERTY()
	FSCClothingLoaded Callback;
};

/**
 * @class USCCounselorWardrobe
 * @brief Manager class containing all clothing options/variants for all counselors.
 */
UCLASS()
class SUMMERCAMP_API USCCounselorWardrobe
: public UObject
{
	GENERATED_BODY()

public:
	/** Sets up the wardrobe */
	void Init(USCGameInstance* OwningInstance);

	/**
	 * Gets a list of all possible clothing options (locked and unlocked) for the passed in counselor class.
	 * The first call loads all clothing options from blueprints, subsequent calls will only parse for matching character class.
	 * @param CounselorClass - Class to limit outfit choices
	 * @return List of all possible clothing options (locked and unlocked) for the passed in counselor class. Locked/hidden checks will need to be performed for validity.
	 */
	UFUNCTION(BlueprintPure, Category = "Clothing")
	TArray<TSubclassOf<USCClothingData>> GetClothingOptions(const TSoftClassPtr<ASCCounselorCharacter> CounselorClass) const;

	/**
	 * Gets a random valid outfit with random valid variants from a list of all outfits for this counselor
	 * @param CounselorClass - Class to limit outfit choices
	 * @param PlayerController - Used to check level / entitlement unlocks
	 * @return A valid random outfit for this player to use
	 */
	UFUNCTION(BlueprintPure, Category = "Clothing")
	FSCCounselorOutfit GetRandomOutfit(const TSoftClassPtr<ASCCounselorCharacter> CounselorClass, const AILLPlayerController* PlayerController) const;


	TArray<int32> GetSlotSelectionIndicies(const FSCCounselorOutfit& Outfit) const;

	/**
	 * Begins an async load for all the assets needed for the passed in outfit. Provides a callback when the outfit is loaded and a handle to cancel the load if needed.
	 * @param Outfit - Outfit to load
	 * @param OutfitLoaded - Callback when the outfit is fully loaded
	 * @return Loading handle used to cancel a load, if need be
	 */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	int32 StartLoadingOutfit(const FSCCounselorOutfit& Outfit, const FSCClothingLoaded& OutfitLoaded, TArray<TAssetPtr<UObject>> AdditionalDependancies);

	/**
	 * Cancels the loading of a given outfit (actually just prevents the callback from triggering)
	 * @param OutfitHandle - Handle from StartLoadingOutfit for the outfit we no long care about loading, will be invalidated after the cancel is complete
	 */
	UFUNCTION(BlueprintCallable, Category = "Clothing")
	void CancelLoadingOutfit(int32& OutfitHandle);

	/**
	 * Unloads the passed in outfits assets, provided we're not also trying to load any of them in for another outfit
	 */
	void UnloadOutfit(const FSCCounselorOutfit& Outfit);

	/**
	 * Checks that all the assets in the passed in outfit are loaded and ready for use
	 * @param Outfit - Outfit to check
	 * @param OutfitHandle - If passed in, will add the loaded assets to the LoadedAssets list
	 * @return If true, all AssetPtrs are valid and ready for use
	 */
	bool IsOutfitFullyLoaded(const FSCCounselorOutfit& Outfit, const int32 OutfitHandle = INDEX_NONE);

	/** Native function for quickly checking if this outfit is locked or not */
	bool IsOutfitLockedFor(const USCClothingData* Outfit, const AILLPlayerController* PlayerController) const;

	/**
	 * Checks that the player state passes the unlock requirements
	 * @param PlayerController - Used to check level / entitlement unlocks
	 * @param bIsLocked - True if the item cannot be used by the passed in player state (ignores counselor requirements)
	 * @param bOutShouldHide - True if the item should not show up at all (not unlocked and bHiddenWhenLocked is true)
	 */
	UFUNCTION(BlueprintPure, Category="Clothing")
	void IsOutfitLockedFor(const USCClothingData* Outfit, const AILLPlayerController* PlayerController, bool& bIsLocked, bool& bOutShouldHide) const;

	/** Native function for quickly checking if option is locked or not */
	bool IsMaterialOptionLockedFor(TSubclassOf<USCClothingSlotMaterialOption> Option, const AILLPlayerController* PlayerController) const;

	/**
	 * Checks that the player state passes the unlock requirements
	 * @param Option - Material option to check for unlock requirements
	 * @param PlayerController - Used to check level / entitlement unlocks
	 * @param bIsLocked - True if the item cannot be used by the passed in player state (ignores counselor requirements)
	 * @param bOutShouldHide - True if the item should not show up at all (not unlocked and bHiddenWhenLocked is true)
	 */
	UFUNCTION(BlueprintPure, Category="Clothing")
	void IsMaterialOptionLockedFor(TSubclassOf<USCClothingSlotMaterialOption> Option, const AILLPlayerController* PlayerController, bool& bIsLocked, bool& bOutShouldHide) const;

protected:
	// Game instance that created this wardrobe
	UPROPERTY(Transient)
	USCGameInstance* OwningGameInstance;

	// Library for loading all our clothing data
	UPROPERTY(Transient)
	UObjectLibrary* CounselorClothingLibrary;

	// List of all clothing for all counselors, regardless of class or unlock status
	UPROPERTY(Transient)
	TArray<TSubclassOf<USCClothingData>> AllCounselorClothingOptions;

	// Handle for our async loading of clothing assets (hopefully we don't need more than 8 million at a time)
	static int32 LastAssignedHandle;

	// List of all the loading outfits we're waiting on
	UPROPERTY(Transient)
	TArray<FSCLoadingOutfitData> LoadingOutfits;

	/**
	 * Starts async loading of a passed in outfit
	 * @param Outfit - Outfit to load
	 * @param OutfitLoaded - Callback to use when loading is complete
	 * @param OverrideHandle - Optional handle to allow re-using a FSCLoadingOutfitData when loading additional assets that were missed in previous passes (INDEX_NONE for new requests)
	 * @return Handle for the load request
	 */
	int32 StartLoadingOutfit_Internal(const FSCCounselorOutfit& Outfit, const FSCClothingLoaded& OutfitLoaded, const int32 OverrideHandle, TArray<TAssetPtr<UObject>> AdditionalDependancies);

	/**
	 * Utility function to get the index into LoadingOutfits for the passed in handle
	 * @param OutfitHandle - Handle to check for the loading outfit
	 * @return The index into LoadingOutfits (may be INDEX_NONE)
	 */
	int32 FindLoadingDataIndex(const int32 OutfitHandle) const;

	/**
	 * Checks all assets have been loaded for the outfit handle and will execute the callback when we're done loading
	 * @param OutfitHandle - Handle to check for the loading outfit
	 */
	UFUNCTION()
	void DeferredOutfitLoadCheck(const int32 OutfitHandle);
};
