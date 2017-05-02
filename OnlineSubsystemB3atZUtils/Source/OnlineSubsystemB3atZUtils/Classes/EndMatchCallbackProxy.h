// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineTurnBasedInterface.h"
#include "EndMatchCallbackProxy.generated.h"

class APlayerController;
class IB3atZTurnBasedMatchInterface;
class UB3atZTurnBasedMatchInterface;

UCLASS(MinimalAPI)
class UB3atZEndMatchCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	virtual ~UB3atZEndMatchCallbackProxy();

	// Called when the match ends successfully
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	// Called when ending the match fails
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

	// End a match that is in progress while it is the current player's turn
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|TurnBased")
	static UB3atZEndMatchCallbackProxy* EndMatch(UObject* WorldContextObject, class APlayerController* PlayerController, TScriptInterface<IB3atZTurnBasedMatchInterface> MatchActor, FString MatchID, EB3atZMPMatchOutcome::Outcome LocalPlayerOutcome, EB3atZMPMatchOutcome::Outcome OtherPlayersOutcome);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

	UB3atZTurnBasedMatchInterface* GetTurnBasedMatchInterfaceObject() { return TurnBasedMatchInterface; }

	void EndMatchDelegate(FString MatchID, bool Succeeded);

private:
	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

	// TurnBasedMatchInterface object, used to set the match data after a match is found
	UB3atZTurnBasedMatchInterface* TurnBasedMatchInterface;

	// ID of the match to end
	FString MatchID;
	
	// Match outcome for the current player (win/loss/tie)
	EB3atZMPMatchOutcome::Outcome LocalPlayerOutcome;
	
	// Match outcome for all other players (win/loss/tie)
	EB3atZMPMatchOutcome::Outcome OtherPlayersOutcome;
};
