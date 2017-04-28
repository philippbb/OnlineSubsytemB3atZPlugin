// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "Interfaces/OnlineTurnBasedInterface.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "FindTurnBasedMatchCallbackProxy.generated.h"

class APlayerController;
class IB3atZTurnBasedMatchInterface;
class UB3atZFindTurnBasedMatchCallbackProxy;
class UB3atZTurnBasedMatchInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnlineTurnBasedMatchResult, FString, MatchID);

class FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate : public FTurnBasedMatchmakerDelegate, public TSharedFromThis<FTurnBasedMatchmakerDelegate>
{
public:

	FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate();
	virtual ~FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate();

	virtual void OnMatchmakerCancelled() override;
	virtual void OnMatchmakerFailed() override;
	virtual void OnMatchFound(FTurnBasedMatchRef Match) override;

	void SetFindTurnBasedMatchCallbackProxy(UB3atZFindTurnBasedMatchCallbackProxy* InFindTurnBasedMatchCallbackProxy) { FindTurnBasedMatchCallbackProxy = InFindTurnBasedMatchCallbackProxy; }
	void SetTurnBasedInterface(IOnlineTurnBasedPtr InTurnBasedInterface) { TurnBasedInterface = InTurnBasedInterface; }
private:
	UB3atZFindTurnBasedMatchCallbackProxy* FindTurnBasedMatchCallbackProxy;
	IOnlineTurnBasedPtr TurnBasedInterface;
};

UCLASS(MinimalAPI)
class UB3atZFindTurnBasedMatchCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

    virtual ~UB3atZFindTurnBasedMatchCallbackProxy();
    
	// Called when matchmaking succeeded.
	UPROPERTY(BlueprintAssignable)
	FOnlineTurnBasedMatchResult OnSuccess;

	// Called when matchmaking failed
	UPROPERTY(BlueprintAssignable)
	FOnlineTurnBasedMatchResult OnFailure;

	// Use the platform matchmaking service (like Game Center) to find a match.
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online|TurnBased")
	static UB3atZFindTurnBasedMatchCallbackProxy* FindTurnBasedMatch(UObject* WorldContextObject, class APlayerController* PlayerController, TScriptInterface<IB3atZTurnBasedMatchInterface> MatchActor, int32 MinPlayers, int32 MaxPlayers, int32 PlayerGroup, bool ShowExistingMatches);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

	UB3atZTurnBasedMatchInterface* GetTurnBasedMatchInterfaceObject() { return TurnBasedMatchInterface; }

private:
	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

	// TurnBasedMatchInterface object, used to set the match data after a match is found
	UB3atZTurnBasedMatchInterface* TurnBasedMatchInterface;

	// Minimum number of players needed for the match if a match is created
	uint32 MinPlayers;

	// Maximum number of players needed for the match if a match is created
	uint32 MaxPlayers;

	// Another matchmaking parameter that must be the same for players to matchmake together - for example, this could be the game mode (1 for deathmatch, 2 for capture the flag, etc.)
	uint32 PlayerGroup;

	// Show matches that the player is already a part of in the matchmaking interface
	bool ShowExistingMatches;

	TSharedPtr<FB3atZtFindTurnBasedMatchCallbackProxyMatchmakerDelegate, ESPMode::ThreadSafe> Delegate;
};
