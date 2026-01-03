// Copyright (C) 2015-2018 IllFonic, LLC.

using UnrealBuildTool;

public class ILLGameFramework : ModuleRules
{
	public ILLGameFramework(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"ILLGameFramework/Private",
				"ILLGameFramework/Private/AI",
				"ILLGameFramework/Private/AI/BehaviorTree",
				"ILLGameFramework/Private/Animation",
				"ILLGameFramework/Private/Backend",
				"ILLGameFramework/Private/Beacon",
				"ILLGameFramework/Private/Camera",
				"ILLGameFramework/Private/Components",
				"ILLGameFramework/Private/HTTP",
				"ILLGameFramework/Private/Inventory",
				"ILLGameFramework/Private/Messages",
				"ILLGameFramework/Private/Misc",
				"ILLGameFramework/Private/Online",
				"ILLGameFramework/Private/Player",
				"ILLGameFramework/Private/Messages",
				"ILLGameFramework/Private/SaveGame",
				"ILLGameFramework/Private/UI",
				"ILLGameFramework/Private/Weapons",
			}
		);

		PublicIncludePaths.AddRange(
			new string[] {
				"ILLGameFramework/Public",
				"ILLGameFramework/Public/AI",
				"ILLGameFramework/Public/AI/BehaviorTree",
				"ILLGameFramework/Public/Animation",
				"ILLGameFramework/Public/Backend",
				"ILLGameFramework/Public/Beacon",
				"ILLGameFramework/Public/Camera",
				"ILLGameFramework/Public/Components",
				"ILLGameFramework/Public/HTTP",
				"ILLGameFramework/Public/Interfaces",
				"ILLGameFramework/Public/Inventory",
				"ILLGameFramework/Public/Misc",
				"ILLGameFramework/Public/Online",
				"ILLGameFramework/Public/Player",
				"ILLGameFramework/Public/SaveGame",
				"ILLGameFramework/Public/UI",
				"ILLGameFramework/Public/Weapons",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"AIModule",
				"AssetRegistry",
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"GameplayTasks",
				"HTTP",
				"Icmp",
				"InputCore",
				"Json",
				"JsonUtilities",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"PlatformCrypto",
				"RHI",
				"Slate",
				"SlateCore",
				"Sockets",
				"UMG",
				"ILLOnlineSubsystem"
			}
		);

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"PlatformCrypto"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Launch",
				"RenderCore"
			}
		);

		if (Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"PlatformCryptoBCrypt",
				}
			);
		}
		else
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"PlatformCryptoOpenSSL",
				}
			);
		}

		// Online Subsystem selection
		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Mac)
		{
			DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
		}
		else if (Target.Platform == UnrealTargetPlatform.PS4)
		{
			DynamicallyLoadedModuleNames.Add("OnlineSubsystemPS4");
		}
		else if (Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			DynamicallyLoadedModuleNames.Add("OnlineSubsystemLive");
		}

		// GameLift SDK support
		string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", ".."));
		string SDKDirectory = System.IO.Path.Combine(BaseDirectory, "Plugins", "GameLiftServerSDK");
		if (System.IO.Directory.Exists(SDKDirectory))
		{
			// Automatically add the dependency
			Definitions.Add("HAS_GAMELIFTSERVERSDK_PLUGIN=1");
			PublicDependencyModuleNames.Add("GameLiftServerSDK");
		}
		else
		{
			Definitions.Add("HAS_GAMELIFTSERVERSDK_PLUGIN=0");
		}
	}
}
