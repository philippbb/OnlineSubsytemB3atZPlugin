// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LeaderboardQueryCallbackProxy.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystemB3atZ.h"
#include "Engine/World.h"

//////////////////////////////////////////////////////////////////////////
// UDirectLeaderboardQueryCallbackProxy

UDirectLeaderboardQueryCallbackProxy::UDirectLeaderboardQueryCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDirectLeaderboardQueryCallbackProxy::TriggerQuery(APlayerController* PlayerController, FName InStatName, EOnlineKeyValuePairDataType::Type StatType)
{
	bFailedToEvenSubmit = true;

	WorldPtr = (PlayerController != NULL) ? PlayerController->GetWorld() : NULL;
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
					bFailedToEvenSubmit = false;

					StatName = InStatName;
					ReadObject = MakeShareable(new FOnlineLeaderboardRead());
					ReadObject->LeaderboardName = StatName;
					ReadObject->SortedColumn = StatName;
					new (ReadObject->ColumnMetadata) FColumnMetaData(StatName, StatType);

					// Register the completion callback
					LeaderboardReadCompleteDelegate       = FOnLeaderboardReadCompleteDelegate::CreateUObject(this, &UDirectLeaderboardQueryCallbackProxy::OnStatsRead);
					LeaderboardReadCompleteDelegateHandle = Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegate);

					TArray< TSharedRef<const FUniqueNetId> > ListOfIDs;
					ListOfIDs.Add(UserID.ToSharedRef());

					FOnlineLeaderboardReadRef ReadObjectRef = ReadObject.ToSharedRef();
					Leaderboards->ReadLeaderboards(ListOfIDs, ReadObjectRef);
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardQueryCallbackProxy::TriggerQuery - Leaderboards not supported by Online Subsystem"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardQueryCallbackProxy::TriggerQuery - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardQueryCallbackProxy::TriggerQuery - Cannot map local player to unique net ID"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("UDirectLeaderboardQueryCallbackProxy::TriggerQuery - Invalid player state"), ELogVerbosity::Warning);
	}

	if (bFailedToEvenSubmit && (PlayerController != NULL))
	{
		OnStatsRead(false);
	}
}

void UDirectLeaderboardQueryCallbackProxy::OnStatsRead(bool bWasSuccessful)
{
	RemoveDelegate();

	bool bFoundValue = false;
	int32 Value = 0;

	if (bWasSuccessful)
	{
		if (ReadObject.IsValid())
		{
			for (int Idx = 0; Idx < ReadObject->Rows.Num(); ++Idx)
			{
				FVariantData* Variant = ReadObject->Rows[Idx].Columns.Find(StatName);

				if (Variant != nullptr)
				{
					bFoundValue = true;
					Variant->GetValue(Value);
				}
			}
		}
	}

	if (bFoundValue)
	{
		bSavedWasSuccessful = true;
		SavedValue = Value;
	}
	else
	{
		bSavedWasSuccessful = false;
		SavedValue = 0;
	}

	if (UWorld* World = WorldPtr.Get())
	{
		// Use a dummy timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(OnStatsRead_DelayedTimerHandle, this, &UDirectLeaderboardQueryCallbackProxy::OnStatsRead_Delayed, 0.001f, false);
	}
	ReadObject = NULL;
}

void UDirectLeaderboardQueryCallbackProxy::OnStatsRead_Delayed()
{
	if (bSavedWasSuccessful)
	{
		OnSuccess.Broadcast(SavedValue);
	}
	else
	{
		OnFailure.Broadcast(0);
	}
}

void UDirectLeaderboardQueryCallbackProxy::RemoveDelegate()
{
	if (!bFailedToEvenSubmit)
	{
		if (IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
			if (Leaderboards.IsValid())
			{
				Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
			}
		}
	}
}

void UDirectLeaderboardQueryCallbackProxy::BeginDestroy()
{
	ReadObject = NULL;
	RemoveDelegate();

	Super::BeginDestroy();
}

UDirectLeaderboardQueryCallbackProxy* UDirectLeaderboardQueryCallbackProxy::CreateProxyObjectForIntQuery(class APlayerController* PlayerController, FName StatName)
{
	UDirectLeaderboardQueryCallbackProxy* Proxy = NewObject<UDirectLeaderboardQueryCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->TriggerQuery(PlayerController, StatName, EOnlineKeyValuePairDataType::Int32);
	return Proxy;
}
