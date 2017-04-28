// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "InAppPurchaseRestoreCallbackProxy.h"
#include "Async/TaskGraphInterfaces.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystemB3atZ.h"
#include "Engine/World.h"

//////////////////////////////////////////////////////////////////////////
// UDirectInAppPurchaseRestoreCallbackProxy

UDirectInAppPurchaseRestoreCallbackProxy::UDirectInAppPurchaseRestoreCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WorldPtr = nullptr;
}


void UDirectInAppPurchaseRestoreCallbackProxy::Trigger(const TArray<FB3atZInAppPurchaseProductRequest>& ConsumableProductFlags, APlayerController* PlayerController)
{
	bFailedToEvenSubmit = true;

	WorldPtr = (PlayerController != nullptr) ? PlayerController->GetWorld() : nullptr;
	if (APlayerState* PlayerState = (PlayerController != nullptr) ? PlayerController->PlayerState : nullptr)
	{
		if (IOnlineSubsystemB3atZ* const OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineStorePtr StoreInterface = OnlineSub->GetStoreInterface();
			if (StoreInterface.IsValid())
			{
				bFailedToEvenSubmit = false;

				// Register the completion callback
				InAppPurchaseRestoreCompleteDelegate = FOnInAppPurchaseCompleteDelegate::CreateUObject(this, &UDirectInAppPurchaseRestoreCallbackProxy::OnInAppPurchaseRestoreComplete);
				InAppPurchaseRestoreCompleteDelegateHandle = StoreInterface->AddOnInAppPurchaseRestoreCompleteDelegate_Handle(InAppPurchaseRestoreCompleteDelegate);

				// Set-up, and trigger the transaction through the store interface
				ReadObject = MakeShareable(new FOnlineInAppPurchaseRestoreRead());
				FOnlineInAppPurchaseRestoreReadRef ReadObjectRef = ReadObject.ToSharedRef();
				StoreInterface->RestorePurchases(ConsumableProductFlags, ReadObjectRef);
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("UDirectInAppPurchaseRestoreCallbackProxy::Trigger - In-App Purchases are not supported by Online Subsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("UDirectInAppPurchaseRestoreCallbackProxy::Trigger - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("UDirectInAppPurchaseRestoreCallbackProxy::Trigger - Invalid player state"), ELogVerbosity::Warning);
	}

	if (bFailedToEvenSubmit && (PlayerController != NULL))
	{
		OnInAppPurchaseRestoreComplete(EB3atZInAppPurchaseState::Failed);
	}
}


void UDirectInAppPurchaseRestoreCallbackProxy::OnInAppPurchaseRestoreComplete(EB3atZInAppPurchaseState::Type CompletionState)
{
	RemoveDelegate();
	SavedPurchaseState = CompletionState;
	if (CompletionState == EB3atZInAppPurchaseState::Restored)
	{
		SavedProductInformation = ReadObject->ProvidedRestoreInformation;
	}
    
	if (UWorld* World = WorldPtr.Get())
	{
		DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.DelayInAppPurchaseRestoreComplete"), STAT_FSimpleDelegateGraphTask_DelayInAppPurchaseRestoreComplete, STATGROUP_TaskGraphTasks);

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateLambda([=](){

				OnInAppPurchaseRestoreComplete_Delayed();

			}),
			GET_STATID(STAT_FSimpleDelegateGraphTask_DelayInAppPurchaseRestoreComplete), 
			nullptr, 
			ENamedThreads::GameThread
		);
    }

	ReadObject = NULL;
}


void UDirectInAppPurchaseRestoreCallbackProxy::OnInAppPurchaseRestoreComplete_Delayed()
{    
	if (SavedPurchaseState == EB3atZInAppPurchaseState::Restored)
	{
		OnSuccess.Broadcast(SavedPurchaseState, SavedProductInformation);
	}
	else
	{
		OnFailure.Broadcast(SavedPurchaseState, SavedProductInformation);
	}
}


void UDirectInAppPurchaseRestoreCallbackProxy::RemoveDelegate()
{
	if (!bFailedToEvenSubmit)
	{
		if (IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineStorePtr InAppPurchases = OnlineSub->GetStoreInterface();
			if (InAppPurchases.IsValid())
			{
				InAppPurchases->ClearOnInAppPurchaseRestoreCompleteDelegate_Handle(InAppPurchaseRestoreCompleteDelegateHandle);
			}
		}
	}
}


void UDirectInAppPurchaseRestoreCallbackProxy::BeginDestroy()
{
	ReadObject = NULL;
	RemoveDelegate();

	Super::BeginDestroy();
}


UDirectInAppPurchaseRestoreCallbackProxy* UDirectInAppPurchaseRestoreCallbackProxy::CreateProxyObjectForInAppPurchaseRestore(const TArray<FB3atZInAppPurchaseProductRequest>& ConsumableProductFlags, class APlayerController* PlayerController)
{
	UDirectInAppPurchaseRestoreCallbackProxy* Proxy = NewObject<UDirectInAppPurchaseRestoreCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->Trigger(ConsumableProductFlags, PlayerController);
	return Proxy;
}
