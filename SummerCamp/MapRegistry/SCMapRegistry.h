// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/DataAsset.h"
#include "SCMapDefinition.h"
#include "SCModeDefinition.h"
#include "SCMapRegistry.generated.h"

class USCChallengeData;

/**
 * @class USCMapRegistry
 */
UCLASS(Abstract, MinimalAPI, const, Blueprintable, BlueprintType)
class USCMapRegistry 
: public UDataAsset
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Maps
public:
	// List of all available maps for Shipping builds
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Maps")
	TArray<TSubclassOf<USCMapDefinition>> ShippingMapList;

	// List of additional available maps for non-Shipping builds
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Maps")
	TArray<TSubclassOf<USCMapDefinition>> AdditionalDevelopmentMaps;

	// List of additional available maps for use in editor builds
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Maps")
	TArray<TSubclassOf<USCMapDefinition>> AdditionalEditorMaps;

	/** @return Map definition for the level we are traveling to. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TSubclassOf<USCMapDefinition> GetCurrentlyLoadingMap();

	/** @return Map definition with a matching MapAssetName. */
	static TSubclassOf<USCMapDefinition> FindMapByAssetName(TSubclassOf<USCMapRegistry> MapRegistryClass, const FString& MapAssetName);

	/** @return ShippingMapList + Additional*Maps based on build configuration. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TArray<TSubclassOf<USCMapDefinition>> GetFullMapList(TSubclassOf<USCMapRegistry> MapRegistryClass, const bool bExcludeEditorMaps = false);

	/** @return Previous map in GetFullMapList after CurrentMapClass. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TSubclassOf<USCMapDefinition> PickPrevMap(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCMapDefinition> CurrentMapClass);

	/** @return Next map in GetFullMapList after CurrentMapClass. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TSubclassOf<USCMapDefinition> PickNextMap(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCMapDefinition> CurrentMapClass);

	/** @return Randomly selected map definition from GetFullMapList. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TSubclassOf<USCMapDefinition> PickRandomMap(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCMapDefinition> SkipMapClass = nullptr);

	/** @return a sorted list of all challenges from single player maps. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TArray<TSubclassOf<USCChallengeData>> GetSortedChallenges(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass);

	/** @return the map definition that contains the given Challenge */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TSubclassOf<USCChallengeMapDefinition> GetMapDefinitionFromChallenge(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCChallengeData> ChallengeClass, TSubclassOf<USCModeDefinition> GameModeClass);

	/** @return The challenge matching the passed in unqiue ID */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Maps")
	static TSubclassOf<USCChallengeData> GetChallengeByID(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, const FString& ChallengeID);

	//////////////////////////////////////////////////
	// Game modes
public:
	// Game mode to use in QuickPlay
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Modes")
	TSubclassOf<USCModeDefinition> QuickPlayGameMode;

	// List of all available game modes for Shipping builds
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Modes")
	TArray<TSubclassOf<USCModeDefinition>> ShippingGameModes;

	// List of additional available game modes for non-Shipping builds
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Modes")
	TArray<TSubclassOf<USCModeDefinition>> AdditionalDevelopmentGameModes;

	/** @return Mode definition with a matching Alias. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Modes")
	static TSubclassOf<USCModeDefinition> FindModeByAlias(TSubclassOf<USCMapRegistry> MapRegistryClass, const FString& Alias);

	/** @return ShippingGameModes + Additional*Modes based on build configuration. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Modes")
	static TArray<TSubclassOf<USCModeDefinition>> GetFullModeList(TSubclassOf<USCMapRegistry> MapRegistryClass);

	/** @return Map list that supports GameModeClass. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Modes")
	static TArray<TSubclassOf<USCMapDefinition>> GetMapListByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, const bool bExcludeEditorMaps = false);

	/** @return Previous map in GetMapListByMode after CurrentMapClass. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Modes")
	static TSubclassOf<USCMapDefinition> PickPrevMapByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, TSubclassOf<USCMapDefinition> CurrentMapClass);

	/** @return Next map in GetMapListByMode after CurrentMapClass. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Modes")
	static TSubclassOf<USCMapDefinition> PickNextMapByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, TSubclassOf<USCMapDefinition> CurrentMapClass);

	/** @return Randomly selected map definition from GetMapListByMode. */
	UFUNCTION(BlueprintPure, Category="MapRegistry|Modes")
	static TSubclassOf<USCMapDefinition> PickRandomMapByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, TSubclassOf<USCMapDefinition> SkipMapClass = nullptr);

	//////////////////////////////////////////////////
	// Utility
protected:
	/** Checks that an individual map is in the packaged build */
	static bool ValidateMap(const TSubclassOf<USCMapDefinition>& MapClass);

	/** Calls ValidateMap on all maps in the passed in array. Any invalid entries are removed */
	static void ValidateMapArray(TArray<TSubclassOf<USCMapDefinition>>& MapList);
};
