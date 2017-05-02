// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "OnlineSubsystemB3atZ.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"
#include "NboSerializer.h"
#include "Online.h"
#include "Misc/NetworkVersion.h"

DEFINE_LOG_CATEGORY(LogB3atZOnline);
DEFINE_LOG_CATEGORY(LogB3atZOnlineGame);
DEFINE_LOG_CATEGORY(LogB3atZOnlineChat);

#if STATS
ONLINESUBSYSTEMB3ATZ_API DEFINE_STAT(STAT_B3atZOnline_Async);
ONLINESUBSYSTEMB3ATZ_API DEFINE_STAT(STAT_B3atZOnline_AsyncTasks);
ONLINESUBSYSTEMB3ATZ_API DEFINE_STAT(STAT_B3atZSession_Interface);
ONLINESUBSYSTEMB3ATZ_API DEFINE_STAT(STAT_B3atZVoice_Interface);
#endif

int32 GetBuildUniqueId()
{
	static bool bStaticCheck = false;
	static bool bUseBuildIdOverride = false;
	static int32 BuildIdOverride = 0;
	if (!bStaticCheck)
	{
		if (FParse::Value(FCommandLine::Get(), TEXT("BuildIdOverride="), BuildIdOverride) && BuildIdOverride != 0)
		{
			bUseBuildIdOverride = true;
		}
		else
		{
			if (!GConfig->GetBool(TEXT("OnlineSubsystemB3atZ"), TEXT("bUseBuildIdOverride"), bUseBuildIdOverride, GEngineIni))
			{
				UE_LOG_ONLINEB3ATZ(Warning, TEXT("Missing bUseBuildIdOverride= in [OnlineSubsystemB3atZ] of DefaultEngine.ini"));
			}

			if (!GConfig->GetInt(TEXT("OnlineSubsystemB3atZ"), TEXT("BuildIdOverride"), BuildIdOverride, GEngineIni))
			{
				UE_LOG_ONLINEB3ATZ(Warning, TEXT("Missing BuildIdOverride= in [OnlineSubsystemB3atZ] of DefaultEngine.ini"));
			}
		}

		bStaticCheck = true;
	}

	const uint32 NetworkVersion = FNetworkVersion::GetLocalNetworkVersion();

	int32 BuildId = 0;
	if (bUseBuildIdOverride == false)
	{
		/** Engine package CRC doesn't change, can't be used as the version - BZ */
		FNboSerializeToBuffer Buffer(64);
		// Serialize to a NBO buffer for consistent CRCs across platforms
		Buffer << NetworkVersion;
		// Now calculate the CRC
		uint32 Crc = FCrc::MemCrc32((uint8*)Buffer, Buffer.GetByteCount());

		// make sure it's positive when it's cast back to an int
		BuildId = static_cast<int32>(Crc & 0x7fffffff);
	}
	else
	{
		BuildId = BuildIdOverride;
	}

	UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("GetBuildUniqueId: Network CL %u LocalNetworkVersion %u bUseBuildIdOverride %d BuildIdOverride %d BuildId %d"),
		FNetworkVersion::GetNetworkCompatibleChangelist(),
		NetworkVersion,
		bUseBuildIdOverride,
		BuildIdOverride,
		BuildId);

	return BuildId;
}

bool IsPlayerInSessionImpl(IOnlineSession* SessionInt, FName SessionName, const FUniqueNetId& UniqueId)
{
	UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("OSB IsPlayerInSession"));

	bool bFound = false;
	FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
	if (Session != NULL)
	{
		const bool bIsSessionOwner = *Session->OwningUserId == UniqueId;

		FB3atZUniqueNetIdMatcher PlayerMatch(UniqueId);
		if (bIsSessionOwner || 
			Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) != INDEX_NONE)
		{
			UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("OSB IsPlayerInSession yes found"));
			bFound = true;
		}
	}
	return bFound;
}


#if !UE_BUILD_SHIPPING

static void ResetAchievements()
{
	auto IdentityInterface = Online::GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG_ONLINEB3ATZ(Warning, TEXT("ResetAchievements command: couldn't get the identity interface"));
		return;
	}
	
	TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(0);
	if(!UserId.IsValid())
	{
		UE_LOG_ONLINEB3ATZ(Warning, TEXT("ResetAchievements command: invalid UserId"));
		return;
	}

	auto AchievementsInterface = Online::GetAchievementsInterface();
	if (!AchievementsInterface.IsValid())
	{
		UE_LOG_ONLINEB3ATZ(Warning, TEXT("ResetAchievements command: couldn't get the achievements interface"));
		return;
	}

	AchievementsInterface->ResetAchievements(*UserId);
}

FAutoConsoleCommand CmdResetAchievements(
	TEXT("online.ResetAchievements"),
	TEXT("Reset achievements for the currently logged in user."),
	FConsoleCommandDelegate::CreateStatic(ResetAchievements)
	);

#endif
