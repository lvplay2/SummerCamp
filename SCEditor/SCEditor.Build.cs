// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

using UnrealBuildTool;

public class SCEditor : ModuleRules
{
	public SCEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"SCEditor/Private",
			}
		);

		PublicIncludePaths.AddRange(
			new string[] {
				"SCEditor/Public",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"EditorStyle",
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
				"ILLEditorFramework",
				"SummerCamp",
				"PropertyEditor",
				"DetailCustomizations",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"BlueprintGraph",
				"CoreUObject",
				"EditorStyle",
				"Engine",
				"InputCore",
				"MessageLog",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"PropertyEditor",
				"DetailCustomizations",
			}
		);
	}
}
