// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "JoinB3atZSessionCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UJoinB3atZSessionCallbackProxy

UJoinB3atZSessionCallbackProxy::UJoinB3atZSessionCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Delegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCompleted))
{
}

UJoinB3atZSessionCallbackProxy* UJoinB3atZSessionCallbackProxy::JoinB3atZSession(UObject* WorldContextObject, class APlayerController* PlayerController, const FBlueprintSessionResultB3atZ& SearchResult)
{
	UJoinB3atZSessionCallbackProxy* Proxy = NewObject<UJoinB3atZSessionCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->OnlineSearchResult = SearchResult.OnlineResult;
	//Proxy->MatchingHostIP = MatchingHostIP;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UJoinB3atZSessionCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("JoinSession"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			DelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(Delegate);
			Sessions->JoinSession(*Helper.UserID, GameSessionName, OnlineSearchResult);

			// OnCompleted will get called, nothing more to do now
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UJoinB3atZSessionCallbackProxy::OnCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("JoinSessionCallback"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(DelegateHandle);

			if (Result == EOnJoinSessionCompleteResult::Success)
			{
				// Client travel to the server
				FString ConnectString;
				if (Sessions->GetResolvedConnectString(GameSessionName, ConnectString) && PlayerControllerWeakPtr.IsValid())
				{
					UE_LOG(LogB3atZOnline, Log, TEXT("Join session: traveling to %s"), *ConnectString);
					PlayerControllerWeakPtr->ClientTravel(ConnectString, TRAVEL_Absolute);
					OnSuccess.Broadcast();
					return;
				}
			}
		}
	}

	OnFailure.Broadcast();
}
