// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

using UnrealBuildTool;
using System.Collections.Generic;

public class SummerCampServerTarget : TargetRules
{
	public SummerCampServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		bUsesSteam = true;

		ExtraModuleNames.Add("ILLGameFramework");
		ExtraModuleNames.Add("ILLOnlineSubsystem");
		ExtraModuleNames.Add("SummerCamp");
		ExtraModuleNames.Add("SCLoadingScreen");
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Mac)
		{
			// Turn on shipping logging
			OutCPPEnvironmentConfiguration.Definitions.Add("USE_LOGGING_IN_SHIPPING=1");
			bUseLoggingInShipping = true;

			// .map files are needed for Arxan
			bCreateMapFile = true;
		}
	}
}
