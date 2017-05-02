// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

using UnrealBuildTool;

public class B3atZOnlineBlueprintSupport : ModuleRules
{
	public B3atZOnlineBlueprintSupport(TargetInfo Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
        PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject", 
				"Engine",
				"BlueprintGraph",
				"OnlineSubsystemB3atZ",
				"OnlineSubsystemB3atZUtils",
				"Sockets",
				"Json"
			}
		);
	}
}
