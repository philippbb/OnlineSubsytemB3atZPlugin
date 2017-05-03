// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

using UnrealBuildTool;

public class OnlineSubsystemB3atZ : ModuleRules
{
	public OnlineSubsystemB3atZ(ReadOnlyTargetRules Target) : base(Target)
    {
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Json",
			}
		);

		PublicIncludePaths.Add("OnlineSubsystemB3atZ/Public/Interfaces");

        PrivateIncludePaths.Add("OnlineSubsystemB3atZ/Private");

        Definitions.Add("ONLINESUBSYSTEMB3ATZ_PACKAGE=1");
		Definitions.Add("DEBUG_LAN_BEACON=0");

		PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject",
				"ImageCore",
				"Sockets",
				"JsonUtilities"
			}
		);


	}

  
}
