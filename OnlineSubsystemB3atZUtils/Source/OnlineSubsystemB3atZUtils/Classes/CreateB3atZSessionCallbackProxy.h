// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterfaceB3atZ.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "CreateB3atZSessionCallbackProxy.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBlueprintCreateSessionResultDelegate, FString, SessionIP, FString, SessionHostPort);

UCLASS(MinimalAPI)
class UCreateB3atZSessionCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the session was created successfully
	UPROPERTY(BlueprintAssignable)
	FBlueprintCreateSessionResultDelegate OnSuccess;

	// Called when there was an error creating the session
	UPROPERTY(BlueprintAssignable)
	FBlueprintCreateSessionResultDelegate OnFailure;

	// Creates a session with the B3atZ online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "Online|Session")
	static UCreateB3atZSessionCallbackProxy* CreateB3atZSession(UObject* WorldContextObject, class APlayerController* PlayerController, int32 PublicConnections, bool bUseLAN);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

	

	UFUNCTION(BlueprintImplementableEvent, Category = "Utilities|Platform")
		void VictoryPC_GetMyIP_DataReceived(const FString& YourIP);

	UFUNCTION(BlueprintCallable, Category = "Utilities|Platform")
		bool GetMyIP_SendRequest();

	void HTTPOnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessfull);


	int ipow(int base, int exp) {
		int result = 1;
		while (exp) {
			if (exp & 1)
			{
				result *= base;
			}
			exp >>= 1;
			base *= base;
		}
		return result;
	}

private:
	// Internal callback when session creation completes, calls StartSession
	void OnCreateCompleted(FName SessionName, bool bWasSuccessful);

	// Internal callback when session creation completes, calls StartSession
	void OnStartCompleted(FName SessionName, bool bWasSuccessful);

	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The delegate executed by the online subsystem
	FOnCreateSessionCompleteDelegate CreateCompleteDelegate;

	// The delegate executed by the online subsystem
	FOnStartSessionCompleteDelegate StartCompleteDelegate;

	// Handles to the registered delegates above
	FDelegateHandle CreateCompleteDelegateHandle;
	FDelegateHandle StartCompleteDelegateHandle;

	// Number of public connections
	int NumPublicConnections;

	// Whether or not to search LAN
	bool bUseLAN;

	//Session IP Return Value for Blueprint
	FString SessionIP;

	//Session Host Port Return Value for Blueprint
	FString SessionHostPort;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;
};
