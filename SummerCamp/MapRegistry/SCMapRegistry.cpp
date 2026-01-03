// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMapRegistry.h"

#include "SCChallengeData.h"
#include "SCChallengeMapDefinition.h"

extern TSubclassOf<USCMapDefinition> GLongLoadMapDefinition;

USCMapRegistry::USCMapRegistry(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

TSubclassOf<USCMapDefinition> USCMapRegistry::GetCurrentlyLoadingMap()
{
	return GLongLoadMapDefinition;
}

TSubclassOf<USCMapDefinition> USCMapRegistry::FindMapByAssetName(TSubclassOf<USCMapRegistry> MapRegistryClass, const FString& MapAssetName)
{
	const USCMapRegistry* const DefaultRegistry = MapRegistryClass ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr;
	if (!DefaultRegistry)
		return nullptr;

	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetFullMapList(MapRegistryClass);
	for (TSubclassOf<USCMapDefinition> MapDefinition : MapList)
	{
		const USCMapDefinition* const DefaultMap = MapDefinition->GetDefaultObject<USCMapDefinition>();
		const FString DefaultAssetName = FPackageName::GetLongPackageAssetName(DefaultMap->Path.FilePath);
		if (DefaultAssetName == MapAssetName)
		{
			return MapDefinition;
		}
	}

	return nullptr;
}

TArray<TSubclassOf<USCMapDefinition>> USCMapRegistry::GetFullMapList(TSubclassOf<USCMapRegistry> MapRegistryClass, const bool bExcludeEditorMaps/* = false*/)
{
	TArray<TSubclassOf<USCMapDefinition>> Results;

	if (const USCMapRegistry* const DefaultRegistry = MapRegistryClass ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr)
	{
		Results = DefaultRegistry->ShippingMapList;
#if !UE_BUILD_SHIPPING
		Results.Append(DefaultRegistry->AdditionalDevelopmentMaps);
#endif
#if WITH_EDITOR
		if (!bExcludeEditorMaps)
			Results.Append(DefaultRegistry->AdditionalEditorMaps);
#endif
	}

	ValidateMapArray(Results);

	return Results;
}

TSubclassOf<USCMapDefinition> USCMapRegistry::PickPrevMap(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCMapDefinition> CurrentMapClass)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetFullMapList(MapRegistryClass);
	const int32 CurrentIndex = CurrentMapClass ? MapList.Find(CurrentMapClass) : INDEX_NONE;
	if (CurrentIndex == INDEX_NONE)
	{
		return MapList.Last();
	}
	if (CurrentIndex-1 < 0)
	{
		return nullptr;
	}

	return MapList[CurrentIndex-1];
}

TSubclassOf<USCMapDefinition> USCMapRegistry::PickNextMap(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCMapDefinition> CurrentMapClass)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetFullMapList(MapRegistryClass);
	const int32 CurrentIndex = CurrentMapClass ? MapList.Find(CurrentMapClass) : INDEX_NONE;
	if (CurrentIndex == INDEX_NONE)
	{
		return MapList[0];
	}
	if (CurrentIndex+1 >= MapList.Num())
	{
		return nullptr;
	}

	return MapList[CurrentIndex+1];
}

TSubclassOf<USCMapDefinition> USCMapRegistry::PickRandomMap(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCMapDefinition> SkipMapClass/* = nullptr*/)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetFullMapList(MapRegistryClass, true);
	for ( ; ; )
	{
		const int32 RandomIndex = FMath::RandHelper(MapList.Num());
		TSubclassOf<USCMapDefinition> RandomMapClass = MapList[RandomIndex];
		if (SkipMapClass && RandomMapClass == SkipMapClass)
			continue;

		return RandomMapClass;
	}

	return nullptr;
}

TArray<TSubclassOf<USCChallengeData>> USCMapRegistry::GetSortedChallenges(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetMapListByMode(MapRegistryClass, GameModeClass);
	
	TArray<TSubclassOf<USCChallengeData>> Challenges;
	for (TSubclassOf<USCMapDefinition> MapClass : MapList)
	{
		const USCChallengeMapDefinition* const MapDef = MapClass ? Cast<USCChallengeMapDefinition>(MapClass.GetDefaultObject()) : nullptr;
		if (!MapDef)
			continue;

		Challenges.Append(MapDef->GetMapChallenges());
	}

	Challenges.Sort( [] (const TSubclassOf<USCChallengeData>& ClassA, const TSubclassOf<USCChallengeData>& ClassB)
	{
		const USCChallengeData* const DataA = ClassA ? ClassA.GetDefaultObject() : nullptr;
		const USCChallengeData* const DataB = ClassB ? ClassB.GetDefaultObject() : nullptr;

		if (!DataA || !DataB)
		{
			//This is hella bad...
			return false;
		}

		return DataA->GetChallengeIndex() < DataB->GetChallengeIndex();
	});

	return Challenges;
}

TSubclassOf<USCChallengeMapDefinition> USCMapRegistry::GetMapDefinitionFromChallenge(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCChallengeData> ChallengeClass, TSubclassOf<USCModeDefinition> GameModeClass)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetMapListByMode(MapRegistryClass, GameModeClass);

	for (TSubclassOf<USCMapDefinition> MapClass : MapList)
	{
		const USCChallengeMapDefinition* const MapDef = MapClass ? Cast<USCChallengeMapDefinition>(MapClass.GetDefaultObject()) : nullptr;
		if (!MapDef)
			continue;

		if (MapDef->GetMapChallenges().Contains(ChallengeClass))
			return MapDef->GetClass();
	}

	return nullptr;
}

TSubclassOf<USCChallengeData> USCMapRegistry::GetChallengeByID(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, const FString& ChallengeID)
{
	TArray<TSubclassOf<USCChallengeData>>&& ChallengeClasses = GetSortedChallenges(MapRegistryClass, GameModeClass);

	for (TSubclassOf<USCChallengeData> ChallengeClass : ChallengeClasses)
	{
		const USCChallengeData* const ChallengeData = ChallengeClass ? ChallengeClass.GetDefaultObject() : nullptr;
		if (!ChallengeData)
			continue;

		if (ChallengeData->GetChallengeID().Compare(ChallengeID, ESearchCase::IgnoreCase) == 0)
			return ChallengeClass;
	}

	return TSubclassOf<USCChallengeData>(nullptr);
}

TSubclassOf<USCModeDefinition> USCMapRegistry::FindModeByAlias(TSubclassOf<USCMapRegistry> MapRegistryClass, const FString& Alias)
{
	if (const USCMapRegistry* const DefaultRegistry = MapRegistryClass ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr)
	{
		const TArray<TSubclassOf<USCModeDefinition>> ModeList = GetFullModeList(MapRegistryClass);
		for (TSubclassOf<USCModeDefinition> ModeEntry : ModeList)
		{
			const USCModeDefinition* const ModeDefault = ModeEntry->GetDefaultObject<USCModeDefinition>();
			if (ModeDefault && ModeDefault->Alias == Alias)
				return ModeEntry;
		}
	}

	return nullptr;
}

TArray<TSubclassOf<USCModeDefinition>> USCMapRegistry::GetFullModeList(TSubclassOf<USCMapRegistry> MapRegistryClass)
{
	TArray<TSubclassOf<USCModeDefinition>> Results;

	if (const USCMapRegistry* const DefaultRegistry = MapRegistryClass ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr)
	{
		Results = DefaultRegistry->ShippingGameModes;
#if !UE_BUILD_SHIPPING
		Results.Append(DefaultRegistry->AdditionalDevelopmentGameModes);
#endif
	}

	return Results;
}

TArray<TSubclassOf<USCMapDefinition>> USCMapRegistry::GetMapListByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, const bool bExcludeEditorMaps/* = false*/)
{
	TArray<TSubclassOf<USCMapDefinition>> Results;

	if (const USCMapRegistry* const DefaultRegistry = (MapRegistryClass && GameModeClass) ? MapRegistryClass->GetDefaultObject<USCMapRegistry>() : nullptr)
	{
		// Collect all allowed maps for this configuration
		const TArray<TSubclassOf<USCMapDefinition>> FullMapList = GetFullMapList(MapRegistryClass, bExcludeEditorMaps);

		// Now intersect that against the the maps the mode itself supports
		const USCModeDefinition* const DefaultGameMode = GameModeClass->GetDefaultObject<USCModeDefinition>();
		Results.Empty(DefaultGameMode->SupportedMaps.Num());
		for (TSubclassOf<USCMapDefinition> MapEntry : DefaultGameMode->SupportedMaps)
		{
			if (FullMapList.Contains(MapEntry))
				Results.Add(MapEntry);
		}
	}

	ValidateMapArray(Results);

	return Results;
}

TSubclassOf<USCMapDefinition> USCMapRegistry::PickPrevMapByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, TSubclassOf<USCMapDefinition> CurrentMapClass)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetMapListByMode(MapRegistryClass, GameModeClass);
	const int32 CurrentIndex = CurrentMapClass ? MapList.Find(CurrentMapClass) : INDEX_NONE;
	if (CurrentIndex == INDEX_NONE)
	{
		return MapList.Last();
	}
	if (CurrentIndex-1 < 0)
	{
		return nullptr;
	}

	return MapList[CurrentIndex-1];
}

TSubclassOf<USCMapDefinition> USCMapRegistry::PickNextMapByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, TSubclassOf<USCMapDefinition> CurrentMapClass)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetMapListByMode(MapRegistryClass, GameModeClass);
	const int32 CurrentIndex = CurrentMapClass ? MapList.Find(CurrentMapClass) : INDEX_NONE;
	if (CurrentIndex == INDEX_NONE)
	{
		return MapList[0];
	}
	if (CurrentIndex+1 >= MapList.Num())
	{
		return nullptr;
	}

	return MapList[CurrentIndex+1];
}

TSubclassOf<USCMapDefinition> USCMapRegistry::PickRandomMapByMode(TSubclassOf<USCMapRegistry> MapRegistryClass, TSubclassOf<USCModeDefinition> GameModeClass, TSubclassOf<USCMapDefinition> SkipMapClass/* = nullptr*/)
{
	TArray<TSubclassOf<USCMapDefinition>> MapList = USCMapRegistry::GetMapListByMode(MapRegistryClass, GameModeClass, true);
	for ( ; ; )
	{
		const int32 RandomIndex = FMath::RandHelper(MapList.Num());
		TSubclassOf<USCMapDefinition> RandomMapClass = MapList[RandomIndex];
		if (SkipMapClass && RandomMapClass == SkipMapClass)
			continue;

		return RandomMapClass;
	}

	return nullptr;
}

bool USCMapRegistry::ValidateMap(const TSubclassOf<USCMapDefinition>& MapClass)
{
	// Valid class
	const USCMapDefinition* const MapDef = MapClass ? MapClass.GetDefaultObject() : nullptr;
	if (!MapDef)
		return false;

	// Is in package
	FString MapName = MapDef->Path.FilePath;
	if (!GEngine->MakeSureMapNameIsValid(MapName))
	{
		UE_LOG(LogTemp, Warning, TEXT("USCMapRegistry::ValidateMap Could not find map \"%s\""), *MapDef->Path.FilePath);
		return false;
	}

	return true;
}

void USCMapRegistry::ValidateMapArray(TArray<TSubclassOf<USCMapDefinition>>& MapList)
{
	for (int32 iMapClass(0); iMapClass < MapList.Num(); ++iMapClass)
	{
		if (!ValidateMap(MapList[iMapClass]))
			MapList.RemoveAt(iMapClass--);
	}
}
