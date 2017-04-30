// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemB3atZDirect.h"
#include "HAL/RunnableThread.h"
#include "OnlineAsyncTaskManagerDirect.h"

#include "OnlineSessionInterfaceDirect.h"
#include "OnlineLeaderboardInterfaceDirect.h"
#include "OnlineIdentityDirect.h"
#include "VoiceInterfaceImpl.h"
#include "OnlineAchievementsInterfaceDirect.h"

IOnlineSessionPtr FOnlineSubsystemB3atZDirect::GetSessionInterface() const
{
	return SessionInterface;
}

IOnlineFriendsPtr FOnlineSubsystemB3atZDirect::GetFriendsInterface() const
{
	return nullptr;
}

IOnlinePartyPtr FOnlineSubsystemB3atZDirect::GetPartyInterface() const
{
	return nullptr;
}

IOnlineGroupsPtr FOnlineSubsystemB3atZDirect::GetGroupsInterface() const
{
	return nullptr;
}

IOnlineSharedCloudPtr FOnlineSubsystemB3atZDirect::GetSharedCloudInterface() const
{
	return nullptr;
}

IOnlineUserCloudPtr FOnlineSubsystemB3atZDirect::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FOnlineSubsystemB3atZDirect::GetEntitlementsInterface() const
{
	return nullptr;
};

IOnlineLeaderboardsPtr FOnlineSubsystemB3atZDirect::GetLeaderboardsInterface() const
{
	return LeaderboardsInterface;
}

IOnlineB3atZVoicePtr FOnlineSubsystemB3atZDirect::GetB3atZVoiceInterface() const
{
	if (VoiceInterface.IsValid() && !bVoiceInterfaceInitialized)
	{	
		if (!VoiceInterface->Init())
		{
			VoiceInterface = nullptr;
		}

		bVoiceInterfaceInitialized = true;
	}

	return VoiceInterface;
}

IOnlineExternalUIPtr FOnlineSubsystemB3atZDirect::GetExternalUIInterface() const
{
	return nullptr;
}

IOnlineTimePtr FOnlineSubsystemB3atZDirect::GetTimeInterface() const
{
	return nullptr;
}

IOnlineIdentityPtr FOnlineSubsystemB3atZDirect::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineTitleFilePtr FOnlineSubsystemB3atZDirect::GetTitleFileInterface() const
{
	return nullptr;
}

IOnlineStorePtr FOnlineSubsystemB3atZDirect::GetStoreInterface() const
{
	return nullptr;
}

IOnlineEventsPtr FOnlineSubsystemB3atZDirect::GetEventsInterface() const
{
	return nullptr;
}

IOnlineAchievementsPtr FOnlineSubsystemB3atZDirect::GetAchievementsInterface() const
{
	return AchievementsInterface;
}

IOnlineSharingPtr FOnlineSubsystemB3atZDirect::GetSharingInterface() const
{
	return nullptr;
}

IOnlineUserPtr FOnlineSubsystemB3atZDirect::GetUserInterface() const
{
	return nullptr;
}

IOnlineMessagePtr FOnlineSubsystemB3atZDirect::GetMessageInterface() const
{
	return nullptr;
}

IOnlinePresencePtr FOnlineSubsystemB3atZDirect::GetPresenceInterface() const
{
	return nullptr;
}

IB3atZOnlineChatPtr FOnlineSubsystemB3atZDirect::GetChatInterface() const
{
	return nullptr;
}

IOnlineTurnBasedPtr FOnlineSubsystemB3atZDirect::GetTurnBasedInterface() const
{
    return nullptr;
}

bool FOnlineSubsystemB3atZDirect::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemB3atZImpl::Tick(DeltaTime))
	{
		return false;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

 	if (SessionInterface.IsValid())
 	{
 		SessionInterface->Tick(DeltaTime);
 	}

	
	if (VoiceInterface.IsValid() && bVoiceInterfaceInitialized)
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OSSD Tick VoiceInterface valid and initialized "));
		VoiceInterface->Tick(DeltaTime);
	}

	return true;
}

bool FOnlineSubsystemB3atZDirect::Init()
{
	const bool bDirectInit = true;
	
	if (bDirectInit)
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OSD Init"));

		// Create the online async task thread
		OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerDirect(this);
		check(OnlineAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadDirect %s"), *InstanceName.ToString()), 128 * 1024, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

 		SessionInterface = MakeShareable(new FOnlineSessionDirect(this));
		LeaderboardsInterface = MakeShareable(new FOnlineLeaderboardsDirect(this));
		IdentityInterface = MakeShareable(new FOnlineIdentityDirect(this));
		AchievementsInterface = MakeShareable(new FOnlineAchievementsDirect(this));
		VoiceInterface = MakeShareable(new FOnlineB3atZVoiceImpl(this));

		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OSD Init"));
	}
	else
	{
		Shutdown();
	}

	return bDirectInit;
}

bool FOnlineSubsystemB3atZDirect::Shutdown()
{
	UE_LOG_ONLINEB3ATZ(Display, TEXT("FOnlineSubsystemB3atZDirect::Shutdown()"));

	FOnlineSubsystemB3atZImpl::Shutdown();

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}

	if (VoiceInterface.IsValid() && bVoiceInterfaceInitialized)
	{
		VoiceInterface->Shutdown();
	}
	
 	#define DESTRUCT_INTERFACE(Interface) \
 	if (Interface.IsValid()) \
 	{ \
 		ensure(Interface.IsUnique()); \
 		Interface = nullptr; \
 	}
 
 	// Destruct the interfaces
	DESTRUCT_INTERFACE(VoiceInterface);
	DESTRUCT_INTERFACE(AchievementsInterface);
	DESTRUCT_INTERFACE(IdentityInterface);
	DESTRUCT_INTERFACE(LeaderboardsInterface);
 	DESTRUCT_INTERFACE(SessionInterface);
	
	#undef DESTRUCT_INTERFACE
	
	return true;
}

FString FOnlineSubsystemB3atZDirect::GetAppId() const
{
	return TEXT("");
}

bool FOnlineSubsystemB3atZDirect::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FOnlineSubsystemB3atZImpl::Exec(InWorld, Cmd, Ar))
	{
		return true;
	}
	return false;
}

bool FOnlineSubsystemB3atZDirect::IsEnabled()
{
	return true;
}
