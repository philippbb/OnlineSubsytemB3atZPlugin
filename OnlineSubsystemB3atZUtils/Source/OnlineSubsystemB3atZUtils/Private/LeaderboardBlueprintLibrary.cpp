// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "LeaderboardBlueprintLibrary.h"
#include "UObject/CoreOnline.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineStats.h"
#include "Interfaces/OnlineLeaderboardInterface.h"

//////////////////////////////////////////////////////////////////////////
// UDirectLeaderboardBlueprintLibrary

UDirectLeaderboardBlueprintLibrary::UDirectLeaderboardBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UDirectLeaderboardBlueprintLibrary::WriteLeaderboardObject(APlayerController* PlayerController, class FOnlineLeaderboardWrite& WriteObject)
{
	if (APlayerState* PlayerState = (PlayerController != NULL) ? PlayerController->PlayerState : NULL)
	{
		TSharedPtr<const FUniqueNetId> UserId = PlayerState->UniqueId.GetUniqueNetId();
		if (UserId.IsValid())
		{
			if (IOnlineSubsystemB3atZ* const OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
			{
				IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
				if (Leaderboards.IsValid())
				{
					// the call will copy the user id and write object to its own memory
					bool bResult = Leaderboards->WriteLeaderboards(PlayerState->SessionName, *UserId, WriteObject);

					// Flush the leaderboard immediately for now
					bool bFlushResult = Leaderboards->FlushLeaderboards(PlayerState->SessionName);

					return bResult && bFlushResult;
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Leaderboards not supported by Online Subsystem"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Cannot map local player to unique net ID"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("WriteLeaderboardObject - Invalid player state"), ELogVerbosity::Warning);
	}

	return false;
}

bool UDirectLeaderboardBlueprintLibrary::WriteLeaderboardInteger(APlayerController* PlayerController, FName StatName, int32 StatValue)
{
	FOnlineLeaderboardWrite WriteObject;
	WriteObject.LeaderboardNames.Add(StatName);
	WriteObject.RatedStat = StatName;
	WriteObject.DisplayFormat = EB3atZLeaderboardFormat::Number;
	WriteObject.SortMethod = EB3atZLeaderboardSort::Descending;
	WriteObject.UpdateMethod = EB3atZLeaderboardUpdateMethod::KeepBest;
	WriteObject.SetIntStat(StatName, StatValue);

	return WriteLeaderboardObject(PlayerController, WriteObject);
}
