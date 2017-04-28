// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LeaderboardFlushCallbackProxy.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystemB3atZ.h"

//////////////////////////////////////////////////////////////////////////
// UDirectLeaderboardFlushCallbackProxy

UDirectLeaderboardFlushCallbackProxy::UDirectLeaderboardFlushCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDirectLeaderboardFlushCallbackProxy::TriggerFlush(APlayerController* PlayerController, FName InSessionName)
{
	bFailedToEvenSubmit = true;

	if (APlayerState* PlayerState = (PlayerController != NULL) ? PlayerController->PlayerState : NULL)
	{
		TSharedPtr<const FUniqueNetId> UserID = PlayerState->UniqueId.GetUniqueNetId();
		if (UserID.IsValid())
		{
			if (IOnlineSubsystemB3atZ* const OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
			{
				IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
				if (Leaderboards.IsValid())
				{
					// Register the completion callback
					LeaderboardFlushCompleteDelegate       = FOnLeaderboardFlushCompleteDelegate::CreateUObject(this, &UDirectLeaderboardFlushCallbackProxy::OnFlushCompleted);
					LeaderboardFlushCompleteDelegateHandle = Leaderboards->AddOnLeaderboardFlushCompleteDelegate_Handle(LeaderboardFlushCompleteDelegate);

					// Flush the leaderboard
					Leaderboards->FlushLeaderboards(InSessionName);
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardFlushCallbackProxy::TriggerFlush - Leaderboards not supported by Online Subsystem"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardFlushCallbackProxy::TriggerFlush - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardFlushCallbackProxy::TriggerFlush - Cannot map local player to unique net ID"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardQueryCallbackProxy::TriggerFlush - Invalid player state"), ELogVerbosity::Warning);
	}

	if (bFailedToEvenSubmit)
	{
		OnFlushCompleted(InSessionName, false);
	}
}


void UDirectLeaderboardFlushCallbackProxy::OnFlushCompleted(FName SessionName, bool bWasSuccessful)
{
	RemoveDelegate();

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast(SessionName);
	}
	else
	{
		OnFailure.Broadcast(SessionName);
	}
}

void UDirectLeaderboardFlushCallbackProxy::RemoveDelegate()
{
	if (!bFailedToEvenSubmit)
	{
		if (IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
			if (Leaderboards.IsValid())
			{
				Leaderboards->ClearOnLeaderboardFlushCompleteDelegate_Handle(LeaderboardFlushCompleteDelegateHandle);
			}
		}
	}
}

void UDirectLeaderboardFlushCallbackProxy::BeginDestroy()
{
	RemoveDelegate();

	Super::BeginDestroy();
}

UDirectLeaderboardFlushCallbackProxy* UDirectLeaderboardFlushCallbackProxy::CreateProxyObjectForFlush(class APlayerController* PlayerController, FName SessionName)
{
	UDirectLeaderboardFlushCallbackProxy* Proxy = NewObject<UDirectLeaderboardFlushCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->TriggerFlush(PlayerController, SessionName);
	return Proxy;
}
