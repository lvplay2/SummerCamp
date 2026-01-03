// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"

#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"

class FSCGameModule
	: public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		// Make sure the AssetRegistry module is loaded
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSCGameModule, SummerCamp, "SummerCamp");

DEFINE_LOG_CATEGORY(LogSC);
DEFINE_LOG_CATEGORY(LogSCWeapon);
