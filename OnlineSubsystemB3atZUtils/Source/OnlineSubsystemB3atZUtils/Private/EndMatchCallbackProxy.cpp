// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "EndMatchCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystemB3atZ.h"
#include "Interfaces/TurnBasedMatchInterface.h"

//////////////////////////////////////////////////////////////////////////
// UB3atZEndMatchCallbackProxy

UB3atZEndMatchCallbackProxy::UB3atZEndMatchCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WorldContextObject(nullptr)
	, TurnBasedMatchInterface(nullptr)
{
}

UB3atZEndMatchCallbackProxy::~UB3atZEndMatchCallbackProxy()
{
}

UB3atZEndMatchCallbackProxy* UB3atZEndMatchCallbackProxy::EndMatch(UObject* WorldContextObject, class APlayerController* PlayerController, TScriptInterface<IB3atZTurnBasedMatchInterface> MatchActor, FString MatchID, EB3atZMPMatchOutcome::Outcome LocalPlayerOutcome, EB3atZMPMatchOutcome::Outcome OtherPlayersOutcome)
{
	UB3atZEndMatchCallbackProxy* Proxy = NewObject<UB3atZEndMatchCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MatchID = MatchID;
	Proxy->LocalPlayerOutcome = LocalPlayerOutcome;
	Proxy->OtherPlayersOutcome = OtherPlayersOutcome;
	return Proxy;
}

void UB3atZEndMatchCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("ConnectToService"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineTurnBasedPtr TurnBasedInterface = Helper.OnlineSub->GetTurnBasedInterface();
		if (TurnBasedInterface.IsValid())
		{
			FTurnBasedMatchPtr Match = TurnBasedInterface->GetMatchWithID(MatchID);
			if (Match.IsValid())
			{
				FEndMatchSignature EndMatchDelegate;
				EndMatchDelegate.BindUObject(this, &UB3atZEndMatchCallbackProxy::EndMatchDelegate);
				Match->EndMatch(EndMatchDelegate, LocalPlayerOutcome, OtherPlayersOutcome);
				return;
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Turn based games not supported by online subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UB3atZEndMatchCallbackProxy::EndMatchDelegate(FString InMatchID, bool Succeeded)
{
	if (Succeeded)
	{ 
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}
