// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "LogoutCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "OnlineSubsystemB3atZ.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemBPCallHelper.h"

UDirectLogoutCallbackProxy::UDirectLogoutCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WorldContextObject(nullptr)
{
}

UDirectLogoutCallbackProxy* UDirectLogoutCallbackProxy::Logout(UObject* WorldContextObject, class APlayerController* PlayerController)
{
	UDirectLogoutCallbackProxy* Proxy = NewObject<UDirectLogoutCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UDirectLogoutCallbackProxy::Activate()
{
	if (!PlayerControllerWeakPtr.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("A player controller must be provided in order to show the external login UI."), ELogVerbosity::Warning);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}

	FOnlineSubsystemBPCallHelper Helper(TEXT("Logout"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.OnlineSub == nullptr)
	{
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}
		
	IOnlineIdentityPtr OnlineIdentity = Helper.OnlineSub->GetIdentityInterface();
	if (!OnlineIdentity.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Logout: identity functionality not supported by the current online subsystem"), ELogVerbosity::Warning);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}

	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerControllerWeakPtr->Player);

	if (LocalPlayer == nullptr)
	{
		FFrame::KismetExecutionMessage(TEXT("Only local players can log out"), ELogVerbosity::Warning);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}
	
	const int32 ControllerId = LocalPlayer->GetControllerId();

	if (!OnlineIdentity->OnLogoutCompleteDelegates[ControllerId].IsBoundToObject(this))
	{
		OnLogoutCompleteDelegateHandle = OnlineIdentity->AddOnLogoutCompleteDelegate_Handle(ControllerId,
			FOnLogoutCompleteDelegate::CreateUObject(this, &UDirectLogoutCallbackProxy::OnLogoutCompleted));
		OnlineIdentity->Logout(ControllerId);
	}
	else
	{
		// already trying to logout!
	}
}

void UDirectLogoutCallbackProxy::OnLogoutCompleted(int LocalPlayerNum, bool bWasSuccessful)
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("Logout"), GEngine->GetWorldFromContextObject(WorldContextObject));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineIdentityPtr OnlineIdentity = Helper.OnlineSub->GetIdentityInterface();
		if (OnlineIdentity.IsValid())
		{
			OnlineIdentity->ClearOnLogoutCompleteDelegate_Handle(LocalPlayerNum, OnLogoutCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast(PlayerControllerWeakPtr.Get());
	}
	else
	{
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
	}
}
