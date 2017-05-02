// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "AchievementBlueprintLibrary.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemB3atZ.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystemBPCallHelper.h"

UDirectAchievementBlueprintLibrary::UDirectAchievementBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDirectAchievementBlueprintLibrary::GetCachedAchievementProgress(UObject* WorldContextObject, APlayerController* PlayerController, FName AchievementID, /*out*/ bool& bFoundID, /*out*/ float& Progress)
{
	bFoundID = false;
	Progress = 0.0f;

	FOnlineSubsystemBPCallHelper Helper(TEXT("GetCachedAchievementProgress"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.IsValid())
	{
		IOnlineAchievementsPtr Achievements = Helper.OnlineSub->GetAchievementsInterface();
		if (Achievements.IsValid())
		{
			FOnlineAchievement AchievementStatus;
			if (Achievements->GetCachedAchievement(*Helper.UserID, AchievementID.ToString(), AchievementStatus) == EB3atZOnlineCachedResult::Success)
			{
				bFoundID = true;
				Progress = AchievementStatus.Progress;
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Achievements not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}
}

void UDirectAchievementBlueprintLibrary::GetCachedAchievementDescription(UObject* WorldContextObject, APlayerController* PlayerController, FName AchievementID, /*out*/ bool& bFoundID, /*out*/ FText& Title, /*out*/ FText& LockedDescription, /*out*/ FText& UnlockedDescription, /*out*/ bool& bHidden)
{
	bFoundID = false;
	Title = FText::GetEmpty();
	LockedDescription = FText::GetEmpty();
	UnlockedDescription = FText::GetEmpty();
	bHidden = false;

	FOnlineSubsystemBPCallHelper Helper(TEXT("GetCachedAchievementDescription"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerController);

	if (Helper.IsValid())
	{
		IOnlineAchievementsPtr Achievements = Helper.OnlineSub->GetAchievementsInterface();
		if (Achievements.IsValid())
		{
			FOnlineAchievementDesc AchievementDescription;
			if (Achievements->GetCachedAchievementDescription(AchievementID.ToString(), AchievementDescription) == EB3atZOnlineCachedResult::Success)
			{
				bFoundID = true;
				Title = AchievementDescription.Title;
				LockedDescription = AchievementDescription.LockedDesc;
				UnlockedDescription = AchievementDescription.UnlockedDesc;
				bHidden = AchievementDescription.bIsHidden;
				//@TODO: UnlockTime - FDateTime is not exposed to Blueprints right now, see TTP 315420
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Achievements not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}
}
