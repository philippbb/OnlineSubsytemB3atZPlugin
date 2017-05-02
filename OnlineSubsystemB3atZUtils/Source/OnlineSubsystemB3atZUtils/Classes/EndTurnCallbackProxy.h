// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "Interfaces/TurnBasedMatchInterface.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "EndTurnCallbackProxy.generated.h"

class APlayerController;

UCLASS(MinimalAPI)
class UEndTurnCallbackProxyB3atZ : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	virtual ~UEndTurnCallbackProxyB3atZ();

	// Called when there is a successful query
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	// Called when there is an unsuccessful query
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

	// Ends the turn for the current player
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|TurnBased")
	static UEndTurnCallbackProxyB3atZ* EndTurn(UObject* WorldContextObject, class APlayerController* PlayerController, FString MatchID, TScriptInterface<IB3atZTurnBasedMatchInterface> TurnBasedMatchInterface);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

    void UploadMatchDataDelegate(FString MatchID, bool Succeeded);

private:
	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

	// The MatchID of the active match
	FString MatchID;

	// Object that is inheriting from UB3atZTurnBasedMatchInterface. Replicated properties will be serialize into the platform specific match data object.
	UB3atZTurnBasedMatchInterface* TurnBasedMatchInterface;
};
