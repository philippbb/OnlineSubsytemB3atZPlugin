// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "FindTurnBasedMatchCallbackProxy.h"
#include "Serialization/BitReader.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"
#include "Net/RepLayout.h"
#include "OnlineSubsystemB3atZ.h"
#include "Interfaces/TurnBasedMatchInterface.h"

//////////////////////////////////////////////////////////////////////////
// UB3atZFindTurnBasedMatchCallbackProxy

UB3atZFindTurnBasedMatchCallbackProxy::UB3atZFindTurnBasedMatchCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, WorldContextObject(nullptr)
, Delegate(new FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate)
{
}

UB3atZFindTurnBasedMatchCallbackProxy::~UB3atZFindTurnBasedMatchCallbackProxy()
{
}

UB3atZFindTurnBasedMatchCallbackProxy* UB3atZFindTurnBasedMatchCallbackProxy::FindTurnBasedMatch(UObject* WorldContextObject, class APlayerController* PlayerController, TScriptInterface<IB3atZTurnBasedMatchInterface> MatchActor, int32 MinPlayers, int32 MaxPlayers, int32 PlayerGroup, bool ShowExistingMatches)
{
	UB3atZFindTurnBasedMatchCallbackProxy* Proxy = NewObject<UB3atZFindTurnBasedMatchCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MinPlayers = MinPlayers;
	Proxy->MaxPlayers = MaxPlayers;
	Proxy->PlayerGroup = PlayerGroup;
	Proxy->ShowExistingMatches = ShowExistingMatches;
	Proxy->TurnBasedMatchInterface = (UB3atZTurnBasedMatchInterface*)MatchActor.GetObject();
	return Proxy;
}

void UB3atZFindTurnBasedMatchCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("ConnectToService"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineTurnBasedPtr TurnBasedInterface = Helper.OnlineSub->GetTurnBasedInterface();
		if (TurnBasedInterface.IsValid())
		{
			Delegate->SetFindTurnBasedMatchCallbackProxy(this);
			Delegate->SetTurnBasedInterface(TurnBasedInterface);
			TurnBasedInterface->SetMatchmakerDelegate(Delegate);
			FTurnBasedMatchRequest MatchRequest(MinPlayers, MaxPlayers, PlayerGroup, ShowExistingMatches);
			TurnBasedInterface->ShowMatchmaker(MatchRequest);

			// Results will be handled in the FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate object
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Turn based games not supported by online subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast(FString());
}

FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate::FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate()
: FTurnBasedMatchmakerDelegate()
, FindTurnBasedMatchCallbackProxy(nullptr)
, TurnBasedInterface(nullptr)
{
}

FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate::~FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate()
{
}

void FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate::OnMatchmakerCancelled()
{
	if (FindTurnBasedMatchCallbackProxy)
	{
		FindTurnBasedMatchCallbackProxy->OnFailure.Broadcast(FString());
	}
}

void FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate::OnMatchmakerFailed()
{
	if (FindTurnBasedMatchCallbackProxy)
	{
		FindTurnBasedMatchCallbackProxy->OnFailure.Broadcast(FString());
	}
}

void FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate::OnMatchFound(FTurnBasedMatchRef Match)
{
    //(LogB3atZOnline Verbose, TEXT("Turn-based match found: %s"), *Match->GetMatchID());
	TArray<uint8> MatchData;
	
	if (Match->GetMatchData(MatchData) && FindTurnBasedMatchCallbackProxy)
	{
		FRepLayout RepLayout;
		RepLayout.InitFromObjectClass(FindTurnBasedMatchCallbackProxy->GetTurnBasedMatchInterfaceObject()->GetClass());
		FBitReader Reader(MatchData.GetData(), TurnBasedInterface->GetMatchDataSize());
		RepLayout.SerializeObjectReplicatedProperties(FindTurnBasedMatchCallbackProxy->GetTurnBasedMatchInterfaceObject(), Reader);
	}

	if (FindTurnBasedMatchCallbackProxy)
	{
		FindTurnBasedMatchCallbackProxy->OnSuccess.Broadcast(Match->GetMatchID());
	}
}
