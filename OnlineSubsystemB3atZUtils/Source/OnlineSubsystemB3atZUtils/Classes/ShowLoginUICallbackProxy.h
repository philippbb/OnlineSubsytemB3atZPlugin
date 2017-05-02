// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/CoreOnline.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ShowLoginUICallbackProxy.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnlineShowLoginUIResult, APlayerController*, PlayerController);

UCLASS(MinimalAPI)
class UDirectShowLoginUICallbackProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UDirectShowLoginUICallbackProxy(const FObjectInitializer& ObjectInitializer);

	// Called when there is a successful query
	UPROPERTY(BlueprintAssignable)
	FOnlineShowLoginUIResult OnSuccess;

	// Called when there is an unsuccessful query
	UPROPERTY(BlueprintAssignable)
	FOnlineShowLoginUIResult OnFailure;

	// Shows the login UI for the currently active online subsystem, if the subsystem supports a login UI.
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "Online")
	static UDirectShowLoginUICallbackProxy* ShowExternalLoginUI(UObject* WorldContextObject, class APlayerController* InPlayerController);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// End of UBlueprintAsyncActionBase interface

private:
	// Internal callback when the login UI closes, calls out to the public success/failure callbacks
	void OnShowLoginUICompleted(TSharedPtr<const FUniqueNetId> UniqueId, int LocalUserNum);

	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;
};
