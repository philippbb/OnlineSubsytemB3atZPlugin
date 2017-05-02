// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "EndTurnCallbackProxy.h"
#include "Serialization/BitWriter.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "Interfaces/OnlineTurnBasedInterface.h"
#include "OnlineSubsystemB3atZ.h"
#include "GameFramework/PlayerController.h"
#include "Net/RepLayout.h"

//////////////////////////////////////////////////////////////////////////
// UTurnBasedMatchEndTurnCallbackProxy

UEndTurnCallbackProxyB3atZ::UEndTurnCallbackProxyB3atZ(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UEndTurnCallbackProxyB3atZ::~UEndTurnCallbackProxyB3atZ()
{
}

UEndTurnCallbackProxyB3atZ* UEndTurnCallbackProxyB3atZ::EndTurn(UObject* WorldContextObject, class APlayerController* PlayerController, FString MatchID, TScriptInterface<IB3atZTurnBasedMatchInterface> TurnBasedMatchInterface)
{
	UEndTurnCallbackProxyB3atZ* Proxy = NewObject<UEndTurnCallbackProxyB3atZ>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MatchID = MatchID;
	Proxy->TurnBasedMatchInterface = (UB3atZTurnBasedMatchInterface*)TurnBasedMatchInterface.GetObject();
	return Proxy;
}

void UEndTurnCallbackProxyB3atZ::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("ConnectToService"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineTurnBasedPtr TurnBasedInterface = Helper.OnlineSub->GetTurnBasedInterface();
		if (TurnBasedInterface.IsValid())
		{
			if (TurnBasedMatchInterface != nullptr)
			{
				FRepLayout RepLayout;
				RepLayout.InitFromObjectClass(TurnBasedMatchInterface->GetClass());
				FBitWriter Writer(TurnBasedInterface->GetMatchDataSize());
				RepLayout.SerializeObjectReplicatedProperties(TurnBasedMatchInterface, Writer);

				FUploadMatchDataSignature UploadMatchDataSignature;
				UploadMatchDataSignature.BindUObject(this, &UEndTurnCallbackProxyB3atZ::UploadMatchDataDelegate);

				FTurnBasedMatchPtr TurnBasedMatchPtr = TurnBasedInterface->GetMatchWithID(MatchID);
				if (TurnBasedMatchPtr.IsValid())
				{
					TurnBasedMatchPtr->EndTurnWithMatchData(*Writer.GetBuffer(), 0, UploadMatchDataSignature);
					return;
				}

				FString Message = FString::Printf(TEXT("Match ID %s not found"), *MatchID);
				FFrame::KismetExecutionMessage(*Message, ELogVerbosity::Warning);
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("No match data passed in to End Turn."), ELogVerbosity::Warning);
			}
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Turn based games not supported by online subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UEndTurnCallbackProxyB3atZ::UploadMatchDataDelegate(FString InMatchID, bool Succeeded)
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
