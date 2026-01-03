// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

using UnrealBuildTool;

public class SCLoadingScreen : ModuleRules
{
	public SCLoadingScreen(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.Add("SCLoadingScreen");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"ILLGameFramework",
				"MoviePlayer",
				"Slate",
				"SlateCore",
				"UMG",
			}
		);
	}
}
