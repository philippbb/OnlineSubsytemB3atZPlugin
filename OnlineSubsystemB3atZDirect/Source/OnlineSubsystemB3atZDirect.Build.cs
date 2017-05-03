// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemB3atZDirect : ModuleRules
{
	public OnlineSubsystemB3atZDirect(ReadOnlyTargetRules Target) : base(Target)
    {
		Definitions.Add("ONLINESUBSYSTEMDIRECT_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"Sockets", 
				"OnlineSubsystemB3atZ", 
				"OnlineSubsystemB3atZUtils",
				"Json"
			}
			);
	}
}
