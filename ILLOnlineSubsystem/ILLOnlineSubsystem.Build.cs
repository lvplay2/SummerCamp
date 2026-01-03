// Copyright (C) 2015-2018 IllFonic, LLC.

using UnrealBuildTool;
using System.IO;

public class ILLOnlineSubsystem : ModuleRules
{
	public ILLOnlineSubsystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Sockets",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"Json"
			}
		);
	}
}
