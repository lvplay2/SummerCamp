// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

using UnrealBuildTool;

public class SummerCamp : ModuleRules
{
	public SummerCamp(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"SummerCamp",
				"SummerCamp/AI",
				"SummerCamp/AI/BehaviorTree",
				"SummerCamp/AI/Components",
				"SummerCamp/Animation",
				"SummerCamp/Audio",
				"SummerCamp/Backend",
				"SummerCamp/Beacon",
				"SummerCamp/Camera",
				"SummerCamp/Components",
				"SummerCamp/Context",
				"SummerCamp/Damage",
				"SummerCamp/DataAssets",
				"SummerCamp/Effects",
				"SummerCamp/Foliage",
				"SummerCamp/HTTP",
				"SummerCamp/Interactable",
				"SummerCamp/Inventory",
				"SummerCamp/Lights",
				"SummerCamp/MapRegistry",
				"SummerCamp/Messages",
				"SummerCamp/Online",
				"SummerCamp/Player",
				"SummerCamp/PowerPlant",
				"SummerCamp/Projectiles",
				"SummerCamp/SaveGame",
				"SummerCamp/SinglePlayer",
				"SummerCamp/Spawners",
				"SummerCamp/SpecialMoves",
				"SummerCamp/Triggers",
				"SummerCamp/Vehicles",
				"SummerCamp/UI",
				"SummerCamp/UI/Dialogs",
				"SummerCamp/UI/HUD",
				"SummerCamp/UI/Interact",
				"SummerCamp/UI/Loading",
				"SummerCamp/UI/Lobby",
				"SummerCamp/UI/Menu",
				"SummerCamp/UI/Menu/FrontEnd",
				"SummerCamp/UI/Menu/InGame",
				"SummerCamp/UI/MiniMap",
				"SummerCamp/Weapons",

				"Engine/Source/ThirdParty/PhysX/PhysX-3.3/include",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"AIModule",
				"ApexDestruction",
				"AssetRegistry",
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"Foliage",
				"GameplayTasks",
				"ILLGameFramework",
				"InputCore",
				"Json",
				"JsonUtilities",
				"Landscape",
				"LevelSequence",
				"MoviePlayer",
				"MediaAssets",
				"MovieScene",
				"OnlineSubsystem",
				"OnlineSubSystemUtils",
				"PhysX",
				"PhysXVehicles",
				"Slate",
				"SlateCore",
				"UMG",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"InputCore",
				"SCLoadingScreen",
				"Slate",
				"SlateCore",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Mac)
		{
			if (Target.bBuildEditor)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}
		}

		// EasyAntiCheat SDK support
		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Mac)
		{
			Definitions.Add("WITH_EAC=1");
			PrivateDependencyModuleNames.Add("EasyAntiCheatClient");
			PrivateDependencyModuleNames.Add("EasyAntiCheatServer");
		}
		else
		{
			Definitions.Add("WITH_EAC=0");
		}
	}
}
