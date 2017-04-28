// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "Interfaces/OnlineStoreInterface.h"
#include "InAppPurchaseRestoreCallbackProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInAppPurchaseRestoreResult, EB3atZInAppPurchaseState::Type, CompletionStatus, const TArray<FB3atZInAppPurchaseRestoreInfo>&, InAppRestorePurchaseInformation);

UCLASS(MinimalAPI)
class UDirectInAppPurchaseRestoreCallbackProxy : public UObject
{
	GENERATED_UCLASS_BODY()

	// Called when there is a successful In-App Purchase transaction
	UPROPERTY(BlueprintAssignable)
	FInAppPurchaseRestoreResult OnSuccess;

	// Called when there is an unsuccessful In-App Purchase transaction
	UPROPERTY(BlueprintAssignable)
	FInAppPurchaseRestoreResult OnFailure;

	// Kicks off a transaction for the provided product identifier
	UFUNCTION(BlueprintCallable, meta = (DisplayName="Restore In-App Purchases"), Category="Online|InAppPurchase")
	static UDirectInAppPurchaseRestoreCallbackProxy* CreateProxyObjectForInAppPurchaseRestore(const TArray<FB3atZInAppPurchaseProductRequest>& ConsumableProductFlags, class APlayerController* PlayerController);

public:

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End UObject Interface

private:

	/** Called by the InAppPurchase system when the transaction has finished */
	void OnInAppPurchaseRestoreComplete_Delayed();
	void OnInAppPurchaseRestoreComplete(EB3atZInAppPurchaseState::Type CompletionState);

	/** Unregisters our delegate from the In-App Purchase system */
	void RemoveDelegate();

	/** Triggers the In-App Purchase Restore Transaction for the specifed user */
	void Trigger(const TArray<FB3atZInAppPurchaseProductRequest>& ConsumableProductFlags, class APlayerController* PlayerController);

private:

	/** Delegate called when a InAppPurchase has been successfully restored */
	FOnInAppPurchaseRestoreCompleteDelegate InAppPurchaseRestoreCompleteDelegate;

	/** Handle to the registered InAppPurchaseCompleteDelegate */
	FDelegateHandle InAppPurchaseRestoreCompleteDelegateHandle;

	/** The InAppPurchaseRestore read request */
	FOnlineInAppPurchaseRestoreReadPtr ReadObject;

	/** Did we fail immediately? */
	bool bFailedToEvenSubmit;

	/** Pointer to the world, needed to delay the results slightly */
	TWeakObjectPtr<UWorld> WorldPtr;

	/** Did the purchase succeed? */
	EB3atZInAppPurchaseState::Type SavedPurchaseState;
	TArray<FB3atZInAppPurchaseRestoreInfo> SavedProductInformation;
};
