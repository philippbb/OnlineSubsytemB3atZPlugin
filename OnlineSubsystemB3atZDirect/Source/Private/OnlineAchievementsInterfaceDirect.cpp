// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineAchievementsInterfaceDirect.h"
#include "OnlineSubsystemB3atZ.h"

FOnlineAchievementsDirect::FOnlineAchievementsDirect(class FOnlineSubsystemB3atZDirect* InSubsystem)
	:	DirectSubsystem(InSubsystem)
{
	check(DirectSubsystem);
}

bool FOnlineAchievementsDirect::ReadAchievementsFromConfig()
{
	if (Achievements.Num() > 0)
	{
		return true;
	}

	DirectAchievementsConfig Config;
	return Config.ReadAchievements(Achievements);
}

void FOnlineAchievementsDirect::WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		WriteObject->WriteState = EB3atZOnlineAsyncTaskState::Failed;
		Delegate.ExecuteIfBound(PlayerId, false);
		return;
	}

	FB3atZUniqueNetIdString DirectId(PlayerId);
	const TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(DirectId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		WriteObject->WriteState = EB3atZOnlineAsyncTaskState::Failed;
		Delegate.ExecuteIfBound(PlayerId, false);
		return;
	}

	// treat each achievement as unlocked
	const int32 AchNum = PlayerAch->Num();
	for (FStatPropertyArray::TConstIterator It(WriteObject->Properties); It; ++It)
	{
		const FString AchievementId = It.Key().ToString();
		for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
		{
			if ((*PlayerAch)[ AchIdx ].Id == AchievementId)
			{
				TriggerOnAchievementUnlockedDelegates(PlayerId, AchievementId);
				break;
			}
		}
	}

	WriteObject->WriteState = EB3atZOnlineAsyncTaskState::Done;
	Delegate.ExecuteIfBound(PlayerId, true);
};

void FOnlineAchievementsDirect::QueryAchievements( const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate )
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		Delegate.ExecuteIfBound(PlayerId, false);
		return;
	}

	FB3atZUniqueNetIdString DirectId(PlayerId);
	if (!PlayerAchievements.Find(DirectId))
	{
		// copy for a new player
		TArray<FOnlineAchievement> AchievementsForPlayer;
		const int32 AchNum = Achievements.Num();

		for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
		{
			AchievementsForPlayer.Add( Achievements[ AchIdx ] );
		}

		PlayerAchievements.Add(DirectId, AchievementsForPlayer);
	}

	Delegate.ExecuteIfBound(PlayerId, true);
}

void FOnlineAchievementsDirect::QueryAchievementDescriptions( const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate )
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		Delegate.ExecuteIfBound(PlayerId, false);
		return;
	}

	if (AchievementDescriptions.Num() == 0)
	{
		const int32 AchNum = Achievements.Num();
		for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
		{
			AchievementDescriptions.Add(Achievements[AchIdx].Id, Achievements[AchIdx]);
		}

		check(AchievementDescriptions.Num() > 0);
	}

	Delegate.ExecuteIfBound(PlayerId, true);
}

EB3atZOnlineCachedResult::Type FOnlineAchievementsDirect::GetCachedAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		return EB3atZOnlineCachedResult::NotFound;
	}

	FB3atZUniqueNetIdString DirectId(PlayerId);
	const TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(DirectId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		return EB3atZOnlineCachedResult::NotFound;
	}

	const int32 AchNum = PlayerAch->Num();
	for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
	{
		if ((*PlayerAch)[ AchIdx ].Id == AchievementId)
		{
			OutAchievement = (*PlayerAch)[ AchIdx ];
			return EB3atZOnlineCachedResult::Success;
		}
	}

	// no such achievement
	return EB3atZOnlineCachedResult::NotFound;
};

EB3atZOnlineCachedResult::Type FOnlineAchievementsDirect::GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement> & OutAchievements)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		return EB3atZOnlineCachedResult::NotFound;
	}

	FB3atZUniqueNetIdString DirectId(PlayerId);
	const TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(DirectId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		return EB3atZOnlineCachedResult::NotFound;
	}

	OutAchievements = *PlayerAch;
	return EB3atZOnlineCachedResult::Success;
};

EB3atZOnlineCachedResult::Type FOnlineAchievementsDirect::GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		return EB3atZOnlineCachedResult::NotFound;
	}

	if (AchievementDescriptions.Num() == 0 )
	{
		// don't have descs
		return EB3atZOnlineCachedResult::NotFound;
	}

	FOnlineAchievementDesc * AchDesc = AchievementDescriptions.Find(AchievementId);
	if (NULL == AchDesc)
	{
		// no such achievement
		return EB3atZOnlineCachedResult::NotFound;
	}

	OutAchievementDesc = *AchDesc;
	return EB3atZOnlineCachedResult::Success;
};

#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsDirect::ResetAchievements(const FUniqueNetId& PlayerId)
{
	if (!ReadAchievementsFromConfig())
	{
		// we don't have achievements
		UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("No achievements have been configured"));
		return false;
	}

	FB3atZUniqueNetIdString DirectId(PlayerId);
	TArray<FOnlineAchievement> * PlayerAch = PlayerAchievements.Find(DirectId);
	if (NULL == PlayerAch)
	{
		// achievements haven't been read for a player
		UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("Could not find achievements for player %s"), *PlayerId.ToString());
		return false;
	}

	// treat each achievement as unlocked
	const int32 AchNum = PlayerAch->Num();
	for (int32 AchIdx = 0; AchIdx < AchNum; ++AchIdx)
	{
		(*PlayerAch)[ AchIdx ].Progress = 0.0;
	}

	return true;
};
#endif // !UE_BUILD_SHIPPING
