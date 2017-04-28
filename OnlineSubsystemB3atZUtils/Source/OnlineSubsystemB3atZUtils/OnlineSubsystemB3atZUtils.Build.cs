// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnlineSubsystemB3atZUtils : ModuleRules
{
	public OnlineSubsystemB3atZUtils(TargetInfo Target)
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
