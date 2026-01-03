// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"

#include "SCMapRegistry.h"

#include "AutomationTest.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

// If this test fails, the build fails. This is intentional!
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSCCookedMaps, "SummerCamp.CookedMaps", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

void CheckMapList(FSCCookedMaps* UnitTest, const USCMapRegistry* Registry, const TArray<TSubclassOf<USCMapDefinition>>& MapList, const TArray<FString>& CheckList, TArray<FString>& AddList, const bool bDevList)
{
	for (const TSubclassOf<USCMapDefinition>& MapDefClass : MapList)
	{
		if (MapDefClass->GetFlags() & EObjectFlags::RF_Transient)
			continue;

		const USCMapDefinition* const MapDef = MapDefClass->GetDefaultObject<USCMapDefinition>();
		AddList.Add(MapDef->Path.FilePath);

		if (!CheckList.Contains(MapDef->Path.FilePath))
		{
			UnitTest->AddError(FString::Printf(TEXT("Map %s in map registry %s isn't in the project's cooked %s maps"),
				*MapDef->Path.FilePath, *Registry->GetClass()->GetPathName(), bDevList ? TEXT("developer") : TEXT("shipping")));
		}
	}
}

bool FSCCookedMaps::RunTest(const FString& Parameters)
{
	// Get our blueprint map list
	const FString FilePath(TEXT("/Game/Blueprints/MapRegistry"));
	UObjectLibrary* Cartographer = UObjectLibrary::CreateLibrary(USCMapRegistry::StaticClass(), true, GIsEditor);
	Cartographer->LoadBlueprintsFromPath(FilePath);
	TArray<UClass*> AllMapRegistries;
	Cartographer->GetObjects(AllMapRegistries);

	// Get our project settings map lists
	// Shipping
	TArray<FString> CookedMapList;
	const UProjectPackagingSettings* const PackagingSettings = UProjectPackagingSettings::StaticClass()->GetDefaultObject<UProjectPackagingSettings>();
	for (const auto& MapToCook : PackagingSettings->MapsToCook)
	{
		CookedMapList.Add(MapToCook.FilePath);
	}

	// Development
	TArray<FString> CookedDevMapList;
	for (const auto& MapToCook : PackagingSettings->DevelopmentMapsToCook)
	{
		CookedDevMapList.Add(MapToCook.FilePath);
	}

	TArray<FString> AllShippingMapsInRegistries;
	TArray<FString> AllDevMapsInRegistries;

	// Check that all the maps in the registries are in project settings to be cooked
	for (UClass* Class : AllMapRegistries)
	{
		// We don't care about objects that aren't saved
		if (Class->GetFlags() & EObjectFlags::RF_Transient)
			continue;

		const USCMapRegistry* const Registry = Class->GetDefaultObject<USCMapRegistry>();
		CheckMapList(this, Registry, Registry->ShippingMapList, CookedMapList, AllShippingMapsInRegistries, /*bDevList=*/ false);
		CheckMapList(this, Registry, Registry->AdditionalDevelopmentMaps, CookedDevMapList, AllDevMapsInRegistries, /*bDevList=*/ true);
		// We don't care about Registry->AdditionalEditorMaps
	}

	// NOTE: The following tests are only set as warnings since not all maps should be available to pick for playing on.
	// Maps like the lobby or main menu

	// Check all the shipping maps are in any map registry
	for (const FString& MapPath : CookedMapList)
	{
		if (!AllShippingMapsInRegistries.Contains(MapPath))
		{
			AddWarning(FString::Printf(TEXT("Map %s is being cooked for shipping, but isn't in any registry and won't be available from the menus"), *MapPath));
		}
	}

	// Check all the developer maps are in any map registry
	for (const FString& MapPath : CookedDevMapList)
	{
		if (!AllDevMapsInRegistries.Contains(MapPath))
		{
			AddWarning(FString::Printf(TEXT("Map %s is being cooked for development, but isn't in any registry and won't be available from the menus"), *MapPath));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
