// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameFrameworkClasses.h"

#include "ILLBuildDefines.h"

DEFINE_LOG_CATEGORY(LogIGF);

static uint32 GetLocalNetworkVersionOverride()
{
	// This is only called once then cached by the engine
	// Slightly different than FNetworkVersion::GetLocalNetworkVersion for now, but will eventually need to support cross-play
	FNetworkVersion::GameNetworkProtocolVersion = ILLBUILD_BUILD_NUMBER;
	FString VersionString = FString::Printf(TEXT("%s %s EngineNetVer: %d, GameNetVer: %d"),
		FApp::GetProjectName(),
		*FNetworkVersion::ProjectVersion,
		FNetworkVersion::GetEngineNetworkProtocolVersion(),
		FNetworkVersion::GetGameNetworkProtocolVersion());

	const uint32 Checksum = FCrc::StrCrc32(*VersionString.ToLower());

	UE_LOG(LogNetVersion, Log, TEXT("%s (Checksum: %u)"), *VersionString, Checksum);

	return Checksum;
}

class FILLGameFrameworkModule
: public IILLGameFrameworkModule
{
public:
	// Begin IModuleInterface interface
	virtual void StartupModule() override
	{
		// Override how network versions are calculated
		FNetworkVersion::GetLocalNetworkVersionOverride.BindStatic(&GetLocalNetworkVersionOverride);
	}
	virtual bool IsGameModule() const override { return true; }
	// End IModuleInterface interface
};

IMPLEMENT_GAME_MODULE(FILLGameFrameworkModule, ILLGameFramework);
