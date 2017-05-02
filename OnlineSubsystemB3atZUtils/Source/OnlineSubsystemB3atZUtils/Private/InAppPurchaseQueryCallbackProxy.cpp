// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "InAppPurchaseQueryCallbackProxy.h"
#include "Async/TaskGraphInterfaces.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystemB3atZ.h"
#include "Engine/World.h"

//////////////////////////////////////////////////////////////////////////
// UDirectInAppPurchaseQueryCallbackProxy

UDirectInAppPurchaseQueryCallbackProxy::UDirectInAppPurchaseQueryCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDirectInAppPurchaseQueryCallbackProxy::TriggerQuery(APlayerController* PlayerController, const TArray<FString>& ProductIdentifiers)
{
	bFailedToEvenSubmit = true;

	WorldPtr = (PlayerController != nullptr) ? PlayerController->GetWorld() : nullptr;
	if (APlayerState* PlayerState = (PlayerController != NULL) ? PlayerController->PlayerState : nullptr)
	{
		if (IOnlineSubsystemB3atZ* const OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineStorePtr StoreInterface = OnlineSub->GetStoreInterface();
			if (StoreInterface.IsValid())
			{
				bFailedToEvenSubmit = false;

				// Register the completion callback
				InAppPurchaseReadCompleteDelegate       = FOnQueryForAvailablePurchasesCompleteDelegate::CreateUObject(this, &UDirectInAppPurchaseQueryCallbackProxy::OnInAppPurchaseRead);
				InAppPurchaseReadCompleteDelegateHandle = StoreInterface->AddOnQueryForAvailablePurchasesCompleteDelegate_Handle(InAppPurchaseReadCompleteDelegate);


				ReadObject = MakeShareable(new FOnlineProductInformationRead());
				FOnlineProductInformationReadRef ReadObjectRef = ReadObject.ToSharedRef();
				StoreInterface->QueryForAvailablePurchases(ProductIdentifiers, ReadObjectRef);
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("UDirectInAppPurchaseQueryCallbackProxy::TriggerQuery - In App Purchases are not supported by Online Subsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("UDirectInAppPurchaseQueryCallbackProxy::TriggerQuery - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("UDirectInAppPurchaseQueryCallbackProxy::TriggerQuery - Invalid player state"), ELogVerbosity::Warning);
	}

	if (bFailedToEvenSubmit && (PlayerController != NULL))
	{
		OnInAppPurchaseRead(false);
	}
}

void UDirectInAppPurchaseQueryCallbackProxy::OnInAppPurchaseRead(bool bWasSuccessful)
{
	RemoveDelegate();

	if (bWasSuccessful && ReadObject.IsValid())
	{
		SavedProductInformation = ReadObject->ProvidedProductInformation;
		bSavedWasSuccessful = true;
	}
	else
	{
		bSavedWasSuccessful = false;
	}

	if (UWorld* World = WorldPtr.Get())
	{
		DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.DelayInAppPurchaseRead"), STAT_FSimpleDelegateGraphTask_DelayInAppPurchaseRead, STATGROUP_TaskGraphTasks);

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateLambda([=](){

				OnInAppPurchaseRead_Delayed();

			}),
			GET_STATID(STAT_FSimpleDelegateGraphTask_DelayInAppPurchaseRead), 
			nullptr, 
			ENamedThreads::GameThread
		);
	}

	ReadObject = NULL;
}

void UDirectInAppPurchaseQueryCallbackProxy::OnInAppPurchaseRead_Delayed()
{
	if (bSavedWasSuccessful)
	{
		OnSuccess.Broadcast(SavedProductInformation);
	}
	else
	{
		OnFailure.Broadcast(SavedProductInformation);
	}
}

void UDirectInAppPurchaseQueryCallbackProxy::RemoveDelegate()
{
	if (!bFailedToEvenSubmit)
	{
		if (IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr)
		{
			IOnlineStorePtr InAppPurchases = OnlineSub->GetStoreInterface();
			if (InAppPurchases.IsValid())
			{
				InAppPurchases->ClearOnQueryForAvailablePurchasesCompleteDelegate_Handle(InAppPurchaseReadCompleteDelegateHandle);
			}
		}
	}
}

void UDirectInAppPurchaseQueryCallbackProxy::BeginDestroy()
{
	ReadObject = NULL;
	RemoveDelegate();

	Super::BeginDestroy();
}

UDirectInAppPurchaseQueryCallbackProxy* UDirectInAppPurchaseQueryCallbackProxy::CreateProxyObjectForInAppPurchaseQuery(class APlayerController* PlayerController, const TArray<FString>& ProductIdentifiers)
{
	UDirectInAppPurchaseQueryCallbackProxy* Proxy = NewObject<UDirectInAppPurchaseQueryCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->TriggerQuery(PlayerController, ProductIdentifiers);
	return Proxy;
}
