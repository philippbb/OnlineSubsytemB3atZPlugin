// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "QuitMatchCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"

UB3atZQuitMatchCallbackProxy::UB3atZQuitMatchCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UB3atZQuitMatchCallbackProxy::~UB3atZQuitMatchCallbackProxy()
{
}

UB3atZQuitMatchCallbackProxy* UB3atZQuitMatchCallbackProxy::QuitMatch(UObject* WorldContextObject, APlayerController* PlayerController, FString MatchID, EB3atZMPMatchOutcome::Outcome Outcome, int32 TurnTimeoutInSeconds)
{
	UB3atZQuitMatchCallbackProxy* Proxy = NewObject<UB3atZQuitMatchCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->WorldContextObject = WorldContextObject;
    Proxy->MatchID = MatchID;
	Proxy->Outcome = Outcome;
	Proxy->TurnTimeoutInSeconds = TurnTimeoutInSeconds;
	return Proxy;
}

void UB3atZQuitMatchCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("ConnectToService"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineTurnBasedPtr OnlineTurnBasedPtr = Helper.OnlineSub->GetTurnBasedInterface();
		if (OnlineTurnBasedPtr.IsValid())
		{
			FTurnBasedMatchPtr Match = OnlineTurnBasedPtr->GetMatchWithID(MatchID);
			if (Match.IsValid())
			{
				FQuitMatchSignature QuitMatchSignature;
				QuitMatchSignature.BindUObject(this, &UB3atZQuitMatchCallbackProxy::QuitMatchDelegate);
				Match->QuitMatch(Outcome, TurnTimeoutInSeconds, QuitMatchSignature);
                return;
			}
			else
			{
				FString Message = FString::Printf(TEXT("Match ID %s not found"), *MatchID);
				FFrame::KismetExecutionMessage(*Message, ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Turn Based Matches not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UB3atZQuitMatchCallbackProxy::QuitMatchDelegate(FString InMatchID, bool Succeeded)
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
