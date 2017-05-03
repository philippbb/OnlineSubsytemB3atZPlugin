// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

using UnrealBuildTool;

public class OnlineSubsystemB3atZUtils : ModuleRules
{
	public OnlineSubsystemB3atZUtils(ReadOnlyTargetRules Target) : base(Target)
    {
		Definitions.Add("ONLINESUBSYSTEMB3ATZUTILS_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add("OnlineSubsystemB3atZUtils/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject",
				"Engine", 
				"EngineSettings",
                "ImageCore",
				"Sockets",
				"Voice",
                "PacketHandler",
				"Json",
                "HTTP"
			}
		);

        PublicDependencyModuleNames.Add("OnlineSubsystemB3atZ");
	}
}
