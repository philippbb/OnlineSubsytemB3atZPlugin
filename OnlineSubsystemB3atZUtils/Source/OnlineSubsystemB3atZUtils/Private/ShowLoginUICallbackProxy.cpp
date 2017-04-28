// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ShowLoginUICallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "Interfaces/OnlineExternalUIInterface.h"

UDirectShowLoginUICallbackProxy::UDirectShowLoginUICallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WorldContextObject(nullptr)
{
}

UDirectShowLoginUICallbackProxy* UDirectShowLoginUICallbackProxy::ShowExternalLoginUI(UObject* WorldContextObject, class APlayerController* InPlayerController)
{
	UDirectShowLoginUICallbackProxy* Proxy = NewObject<UDirectShowLoginUICallbackProxy>();
	Proxy->PlayerControllerWeakPtr = InPlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UDirectShowLoginUICallbackProxy::Activate()
{
	if (!PlayerControllerWeakPtr.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("A player controller must be provided in order to show the external login UI."), ELogVerbosity::Warning);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}

	FOnlineSubsystemBPCallHelper Helper(TEXT("ShowLoginUI"), GEngine->GetWorldFromContextObject(WorldContextObject));

	if (Helper.OnlineSub == nullptr)
	{
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}
		
	IOnlineExternalUIPtr OnlineExternalUI = Helper.OnlineSub->GetExternalUIInterface();
	if (!OnlineExternalUI.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("External UI not supported by the current online subsystem"), ELogVerbosity::Warning);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}

	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerControllerWeakPtr->Player);

	if (LocalPlayer == nullptr)
	{
		FFrame::KismetExecutionMessage(TEXT("Can only show login UI for local players"), ELogVerbosity::Warning);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}
		
	const bool bWaitForDelegate = OnlineExternalUI->ShowLoginUI(LocalPlayer->GetControllerId(), false,
		FOnLoginUIClosedDelegate::CreateUObject(this, &UDirectShowLoginUICallbackProxy::OnShowLoginUICompleted));

	if (!bWaitForDelegate)
	{
		FFrame::KismetExecutionMessage(TEXT("The online subsystem couldn't show its login UI"), ELogVerbosity::Log);
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
	}
}

void UDirectShowLoginUICallbackProxy::OnShowLoginUICompleted(TSharedPtr<const FUniqueNetId> UniqueId, int LocalPlayerNum)
{
	// Update the cached unique ID for the local player and the player state.
	APlayerController* PlayerController = PlayerControllerWeakPtr.Get();
	
	if (PlayerController != nullptr)
	{
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (LocalPlayer != nullptr)
		{
			LocalPlayer->SetCachedUniqueNetId(UniqueId);
		}
		
		if (PlayerController->PlayerState != nullptr)
		{
			PlayerController->PlayerState->SetUniqueId(UniqueId);
		}
	}

	if (UniqueId.IsValid())
	{
		OnSuccess.Broadcast(PlayerControllerWeakPtr.Get());
	}
	else
	{
		OnFailure.Broadcast(PlayerControllerWeakPtr.Get());
	}
}
