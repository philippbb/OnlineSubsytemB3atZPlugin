// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "FindB3atZSessionsCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UFindB3atZSessionsCallbackProxy

UFindSessionsCallbackProxyB3atZ::UFindSessionsCallbackProxyB3atZ(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Delegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnCompleted))
	, bUseLAN(false)
{
}

UFindSessionsCallbackProxyB3atZ* UFindSessionsCallbackProxyB3atZ::FindB3atZSessions(UObject* WorldContextObject, class APlayerController* PlayerController, int MaxResults, bool bUseLAN, FString HostSessionAddr, FString HostSessionPort)
{
	int32 IP = FCString::Atoi(*HostSessionAddr);
	int32 Port = FCString::Atoi(*HostSessionPort);

	UFindSessionsCallbackProxyB3atZ* Proxy = NewObject<UFindSessionsCallbackProxyB3atZ>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->bUseLAN = bUseLAN;
	Proxy->MaxResults = MaxResults;
	Proxy->HostSessionAddr = IP;
	Proxy->HostSessionPort = Port;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UFindSessionsCallbackProxyB3atZ::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("FindSessions"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			DelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(Delegate);
			
			SearchObject = MakeShareable(new FOnlineSessionSearchB3atZ);
			SearchObject->MaxSearchResults = MaxResults;
			SearchObject->bIsLanQuery = bUseLAN;
			SearchObject->HostSessionAddr = HostSessionAddr;
			SearchObject->HostSessionPort = HostSessionPort;
			SearchObject->QuerySettings.Set(SEARCH_PRESENCE, true, EB3atZOnlineComparisonOp::Equals);

			Sessions->FindSessions(*Helper.UserID, SearchObject.ToSharedRef());

			// OnQueryCompleted will get called, nothing more to do now
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	TArray<FBlueprintSessionResultB3atZ> Results;
	OnFailure.Broadcast(Results);
}

void UFindSessionsCallbackProxyB3atZ::OnCompleted(bool bSuccess)
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("FindSessionsCallback"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(DelegateHandle);
		}
	}

	TArray<FBlueprintSessionResultB3atZ> Results;

	if (bSuccess && SearchObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessionCP OnCompleted bSuccess and SearchObjectValid true"));

		int FoundSessionNr = 0;
		for (auto& Result : SearchObject->SearchResults)
		{
			FBlueprintSessionResultB3atZ BPResult;
			BPResult.OnlineResult = Result;
			Results.Add(BPResult);
			++FoundSessionNr;

		}

		UE_LOG(LogTemp, Warning, TEXT("FindSessionCP OnCompleted bSuccess and SearchObjectValid true number of sessions found is %d"), FoundSessionNr);

		

		OnSuccess.Broadcast(Results);
	}
	else
	{
		OnFailure.Broadcast(Results);
	}
}

int32 UFindSessionsCallbackProxyB3atZ::GetPingInMs(const FBlueprintSessionResultB3atZ& Result)
{
	return Result.OnlineResult.PingInMs;
}

FString UFindSessionsCallbackProxyB3atZ::GetServerName(const FBlueprintSessionResultB3atZ& Result)
{
	return Result.OnlineResult.Session.OwningUserName;
}

int32 UFindSessionsCallbackProxyB3atZ::GetCurrentPlayers(const FBlueprintSessionResultB3atZ& Result)
{
	return Result.OnlineResult.Session.SessionSettings.NumPublicConnections - Result.OnlineResult.Session.NumOpenPublicConnections;
}

int32 UFindSessionsCallbackProxyB3atZ::GetMaxPlayers(const FBlueprintSessionResultB3atZ& Result)
{
	return Result.OnlineResult.Session.SessionSettings.NumPublicConnections;
}
