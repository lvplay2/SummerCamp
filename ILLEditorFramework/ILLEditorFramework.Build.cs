// Copyright (C) 2015-2018 IllFonic, LLC.

using UnrealBuildTool;

public class ILLEditorFramework : ModuleRules
{
	public ILLEditorFramework(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"ILLEditorFramework/Private",
				"ILLEditorFramework/Private/Backend",
                "ILLEditorFramework/Private/ComponentVisualizers",
			}
		);

		PublicIncludePaths.AddRange(
			new string[] {
				"ILLEditorFramework/Public",
				"ILLEditorFramework/Public/Interfaces",
                "ILLEditorFramework/Public/ComponentVisualizers",
            }
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"AssetRegistry",
				"AIModule",
				"RHI",
				"InputCore",
				"UMG",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ILLGameFramework",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"BlueprintGraph",
				"InputCore",
				"Slate",
				"SlateCore",
			}
		);
	}
}
