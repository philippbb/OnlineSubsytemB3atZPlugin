// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AchievementQueryCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemB3atZ.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UB3atZAchievementQueryCallbackProxy

UB3atZAchievementQueryCallbackProxy::UB3atZAchievementQueryCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WorldContextObject(nullptr)
{
}

UB3atZAchievementQueryCallbackProxy* UB3atZAchievementQueryCallbackProxy::CacheAchievements(UObject* WorldContextObject, class APlayerController* PlayerController)
{
	UB3atZAchievementQueryCallbackProxy* Proxy = NewObject<UB3atZAchievementQueryCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->bFetchDescriptions = false;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

UB3atZAchievementQueryCallbackProxy* UB3atZAchievementQueryCallbackProxy::CacheAchievementDescriptions(UObject* WorldContextObject, class APlayerController* PlayerController)
{
	UB3atZAchievementQueryCallbackProxy* Proxy = NewObject<UB3atZAchievementQueryCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->bFetchDescriptions = true;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UB3atZAchievementQueryCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("CacheAchievements or CacheAchievementDescriptions"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineAchievementsPtr Achievements = Helper.OnlineSub->GetAchievementsInterface();
		if (Achievements.IsValid())
		{
			FOnQueryAchievementsCompleteDelegate QueryFinishedDelegate = FOnQueryAchievementsCompleteDelegate::CreateUObject(this, &ThisClass::OnQueryCompleted);
			
			if (bFetchDescriptions)
			{
				Achievements->QueryAchievementDescriptions(*Helper.UserID, QueryFinishedDelegate);
			}
			else
			{
				Achievements->QueryAchievements(*Helper.UserID, QueryFinishedDelegate);
			}

			// OnQueryCompleted will get called, nothing more to do now
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Achievements not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UB3atZAchievementQueryCallbackProxy::OnQueryCompleted(const FUniqueNetId& UserID, bool bSuccess)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}
