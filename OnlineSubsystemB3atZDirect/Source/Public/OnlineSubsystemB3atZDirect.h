// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemB3atZImpl.h"
#include "OnlineSubsystemB3atZDirectPackage.h"

class FOnlineAchievementsDirect;
class FOnlineIdentityDirect;
class FOnlineLeaderboardsDirect;
class FOnlineSessionDirect;
class FOnlineB3atZVoiceImpl;

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionDirect, ESPMode::ThreadSafe> FOnlineSessionDirectPtr;
typedef TSharedPtr<class FOnlineProfileDirect, ESPMode::ThreadSafe> FOnlineProfileDirectPtr;
typedef TSharedPtr<class FOnlineFriendsDirect, ESPMode::ThreadSafe> FOnlineFriendsDirectPtr;
typedef TSharedPtr<class FOnlineUserCloudDirect, ESPMode::ThreadSafe> FOnlineUserCloudDirectPtr;
typedef TSharedPtr<class FOnlineLeaderboardsDirect, ESPMode::ThreadSafe> FOnlineLeaderboardsDirectPtr;
typedef TSharedPtr<class FOnlineB3atZVoiceImpl, ESPMode::ThreadSafe> FOnlineB3atZVoiceImplPtr;
typedef TSharedPtr<class FOnlineExternalUIDirect, ESPMode::ThreadSafe> FOnlineExternalUIDirectPtr;
typedef TSharedPtr<class FOnlineIdentityDirect, ESPMode::ThreadSafe> FOnlineIdentityDirectPtr;
typedef TSharedPtr<class FOnlineAchievementsDirect, ESPMode::ThreadSafe> FOnlineAchievementsDirectPtr;

/**
 *	OnlineSubsystemB3atZDirect - Implementation of the online subsystem for Direct services
 */
class ONLINESUBSYSTEMB3ATZDIRECT_API FOnlineSubsystemB3atZDirect : 
	public FOnlineSubsystemB3atZImpl
{

public:

	virtual ~FOnlineSubsystemB3atZDirect()
	{
	}

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineB3atZVoicePtr GetB3atZVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;	
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStorePtr GetStoreInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override { return nullptr; }
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IB3atZOnlineChatPtr GetChatInterface() const override;
    virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	
	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	// FTickerObjectBase
	
	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemB3atZDirect

	/**
	 * Is the Direct API available for use
	 * @return true if Direct functionality is available, false otherwise
	 */
	bool IsEnabled();

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemB3atZDirect(FName InInstanceName) :
		FOnlineSubsystemB3atZImpl(InInstanceName),
		SessionInterface(nullptr),
		VoiceInterface(nullptr),
		bVoiceInterfaceInitialized(false),
		LeaderboardsInterface(nullptr),
		IdentityInterface(nullptr),
		AchievementsInterface(nullptr),
		OnlineAsyncTaskThreadRunnable(nullptr),
		OnlineAsyncTaskThread(nullptr)
	{}

	FOnlineSubsystemB3atZDirect() :
		SessionInterface(nullptr),
		VoiceInterface(nullptr),
		bVoiceInterfaceInitialized(false),
		LeaderboardsInterface(nullptr),
		IdentityInterface(nullptr),
		AchievementsInterface(nullptr),
		OnlineAsyncTaskThreadRunnable(nullptr),
		OnlineAsyncTaskThread(nullptr)
	{}

private:

	/** Interface to the session services */
	FOnlineSessionDirectPtr SessionInterface;

	/** Interface for voice communication */
	mutable FOnlineB3atZVoiceImplPtr VoiceInterface;

	/** Interface for voice communication */
	mutable bool bVoiceInterfaceInitialized;

	/** Interface to the leaderboard services */
	FOnlineLeaderboardsDirectPtr LeaderboardsInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityDirectPtr IdentityInterface;

	/** Interface for achievements */
	FOnlineAchievementsDirectPtr AchievementsInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerDirect* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;
};

typedef TSharedPtr<FOnlineSubsystemB3atZDirect, ESPMode::ThreadSafe> FOnlineSubsystemDirectPtr;

