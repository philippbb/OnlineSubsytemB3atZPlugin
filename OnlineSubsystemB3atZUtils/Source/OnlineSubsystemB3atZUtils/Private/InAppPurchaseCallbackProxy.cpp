// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "InAppPurchaseCallbackProxy.h"
#include "Async/TaskGraphInterfaces.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystemB3atZ.h"
#include "Engine/World.h"

//////////////////////////////////////////////////////////////////////////
// UB3atZInAppPurchaseCallbackProxy

UB3atZInAppPurchaseCallbackProxy::UB3atZInAppPurchaseCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PurchaseRequest = nullptr;
	WorldPtr = nullptr;
	SavedPurchaseState = EB3atZInAppPurchaseState::Unknown;
}


void UB3atZInAppPurchaseCallbackProxy::Trigger(APlayerController* PlayerController, const FB3atZInAppPurchaseProductRequest& ProductRequest)
{
	bFailedToEvenSubmit = true;
	EB3atZInAppPurchaseState::Type TempState = EB3atZInAppPurchaseState::Unknown;

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
				InAppPurchaseCompleteDelegate = FOnInAppPurchaseCompleteDelegate::CreateUObject(this, &UB3atZInAppPurchaseCallbackProxy::OnInAppPurchaseComplete);
				InAppPurchaseCompleteDelegateHandle = StoreInterface->AddOnInAppPurchaseCompleteDelegate_Handle(InAppPurchaseCompleteDelegate);

				// Set-up, and trigger the transaction through the store interface
				PurchaseRequest = MakeShareable(new FOnlineInAppPurchaseTransaction());
				FOnlineInAppPurchaseTransactionRef PurchaseRequestRef = PurchaseRequest.ToSharedRef();
				StoreInterface->BeginPurchase(ProductRequest, PurchaseRequestRef);
			}
			else
			{
				TempState = EB3atZInAppPurchaseState::NotAllowed;
				FFrame::KismetExecutionMessage(TEXT("UB3atZInAppPurchaseCallbackProxy::Trigger - In-App Purchases are not supported by Online Subsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			TempState = EB3atZInAppPurchaseState::Invalid;
			FFrame::KismetExecutionMessage(TEXT("UB3atZInAppPurchaseCallbackProxy::Trigger - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
		}
	}
	else
	{
		TempState = EB3atZInAppPurchaseState::Invalid;
		FFrame::KismetExecutionMessage(TEXT("UB3atZInAppPurchaseCallbackProxy::Trigger - Invalid player state"), ELogVerbosity::Warning);
	}

	if (bFailedToEvenSubmit && (PlayerController != NULL))
	{
		OnInAppPurchaseComplete(TempState);
	}
}


void UB3atZInAppPurchaseCallbackProxy::OnInAppPurchaseComplete(EB3atZInAppPurchaseState::Type CompletionState)
{
	RemoveDelegate();
	SavedPurchaseState = CompletionState;
    
	if (UWorld* World = WorldPtr.Get())
	{
		DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.DelayInAppPurchaseComplete"), STAT_FSimpleDelegateGraphTask_DelayInAppPurchaseComplete, STATGROUP_TaskGraphTasks);

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateLambda([=](){

				OnInAppPurchaseComplete_Delayed();

			}),
			GET_STATID(STAT_FSimpleDelegateGraphTask_DelayInAppPurchaseComplete), 
			nullptr, 
			ENamedThreads::GameThread
		);
    }
    else
    {
        PurchaseRequest = nullptr;
    }
}


void UB3atZInAppPurchaseCallbackProxy::OnInAppPurchaseComplete_Delayed()
{
    /** Cached product details of the purchased product */
    FB3atZInAppPurchaseProductInfo ProductInformation; 

    if (SavedPurchaseState == EB3atZInAppPurchaseState::Success && PurchaseRequest.IsValid())
    {
        ProductInformation = PurchaseRequest->ProvidedProductInformation;
    }
    
	if (SavedPurchaseState == EB3atZInAppPurchaseState::Success)
	{
		OnSuccess.Broadcast(SavedPurchaseState, ProductInformation);
	}
	else
	{
		OnFailure.Broadcast(SavedPurchaseState, ProductInformation);
	}
    
    PurchaseRequest = nullptr;
}


void UB3atZInAppPurchaseCallbackProxy::RemoveDelegate()
{
	if (!bFailedToEvenSubmit)
	{
		if (IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineStorePtr InAppPurchases = OnlineSub->GetStoreInterface();
			if (InAppPurchases.IsValid())
			{
				InAppPurchases->ClearOnInAppPurchaseCompleteDelegate_Handle(InAppPurchaseCompleteDelegateHandle);
			}
		}
	}
}


void UB3atZInAppPurchaseCallbackProxy::BeginDestroy()
{
	PurchaseRequest = nullptr;
	RemoveDelegate();

	Super::BeginDestroy();
}


UB3atZInAppPurchaseCallbackProxy* UB3atZInAppPurchaseCallbackProxy::CreateProxyObjectForInAppPurchase(class APlayerController* PlayerController, const FB3atZInAppPurchaseProductRequest& ProductRequest)
{
	UB3atZInAppPurchaseCallbackProxy* Proxy = NewObject<UB3atZInAppPurchaseCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->Trigger(PlayerController, ProductRequest);
	return Proxy;
}
