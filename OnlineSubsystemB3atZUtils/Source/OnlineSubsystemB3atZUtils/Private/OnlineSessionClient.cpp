// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OnlineSessionClient: Online session related implementations 
	(creating/joining/leaving/destroying sessions)
=============================================================================*/

#include "OnlineSessionClient.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystemB3atZUtils.h"
#include "GameFramework/PlayerController.h"

UDirectOnlineSessionClient::UDirectOnlineSessionClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHandlingDisconnect = false;
	bIsFromInvite = false;
}

UGameInstance* UDirectOnlineSessionClient::GetGameInstance() const
{
	return CastChecked<UGameInstance>(GetOuter());
}

UWorld* UDirectOnlineSessionClient::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}

IOnlineSessionPtr UDirectOnlineSessionClient::GetSessionInt()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogB3atZOnline, Warning, TEXT("UDirectOnlineSessionClient::GetSessionInt: Called with NULL world."));
		return nullptr;
	}

	return Online::GetSessionInterface(World);
}

void UDirectOnlineSessionClient::RegisterOnlineDelegates()
{
	OnJoinSessionCompleteDelegate           = FOnJoinSessionCompleteDelegate   ::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
	OnEndForJoinSessionCompleteDelegate     = FOnEndSessionCompleteDelegate    ::CreateUObject(this, &ThisClass::OnEndForJoinSessionComplete);
	OnDestroyForJoinSessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyForJoinSessionComplete);
	OnDestroyForMainMenuCompleteDelegate	= FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyForMainMenuComplete);
	OnSessionUserInviteAcceptedDelegate     = FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted);
	OnPlayTogetherEventReceivedDelegate		= FOnPlayTogetherEventReceivedDelegate::CreateUObject(this, &UOnlineSession::OnPlayTogetherEventReceived);

	if (IOnlineSubsystemB3atZ* const OnlineSubsystem = IOnlineSubsystemB3atZ::Get())
	{
		OnPlayTogetherEventReceivedDelegateHandle = OnlineSubsystem->AddOnPlayTogetherEventReceivedDelegate_Handle(OnPlayTogetherEventReceivedDelegate);
	}

	IOnlineSessionPtr SessionInt = GetSessionInt();
	if (SessionInt.IsValid())
	{
		OnSessionUserInviteAcceptedDelegateHandle = SessionInt->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
	}
}

void UDirectOnlineSessionClient::ClearOnlineDelegates()
{
	IOnlineSessionPtr SessionInt = GetSessionInt();
	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}

	if (IOnlineSubsystemB3atZ* const OnlineSubsystem = IOnlineSubsystemB3atZ::Get())
	{
		OnlineSubsystem->ClearOnPlayTogetherEventReceivedDelegate_Handle(OnPlayTogetherEventReceivedDelegateHandle);
	}
}

void UDirectOnlineSessionClient::OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnSessionUserInviteAccepted LocalUserNum: %d bSuccess: %d"), ControllerId, bWasSuccessful);
	// Don't clear invite accept delegate

	if (bWasSuccessful)
	{
		if (SearchResult.IsValid())
		{
			bIsFromInvite = true;
			JoinSession(GameSessionName, SearchResult);
		}
		else
		{
			UE_LOG(LogB3atZOnline, Warning, TEXT("Invite accept returned no search result."));
		}
	}
}

void UDirectOnlineSessionClient::OnEndForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnEndForJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	IOnlineSessionPtr SessionInt = GetSessionInt();
	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnEndSessionCompleteDelegate_Handle(OnEndForJoinSessionCompleteDelegateHandle);
	}
	DestroyExistingSession_Impl(OnDestroyForJoinSessionCompleteDelegateHandle, SessionName, OnDestroyForJoinSessionCompleteDelegate);
}

void UDirectOnlineSessionClient::EndExistingSession(FName SessionName, FOnEndSessionCompleteDelegate& Delegate)
{
	EndExistingSession_Impl(SessionName, Delegate);
}

FDelegateHandle UDirectOnlineSessionClient::EndExistingSession_Impl(FName SessionName, FOnEndSessionCompleteDelegate& Delegate)
{
	FDelegateHandle Result;

	IOnlineSessionPtr SessionInt = GetSessionInt();

	if (SessionInt.IsValid())
	{
		Result = SessionInt->AddOnEndSessionCompleteDelegate_Handle(Delegate);
		SessionInt->EndSession(SessionName);
	}
	else
	{
		Delegate.ExecuteIfBound(SessionName, true);
	}

	return Result;
}

void UDirectOnlineSessionClient::OnDestroyForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnDestroyForJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr SessionInt = GetSessionInt();

	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyForJoinSessionCompleteDelegateHandle);
	}

	if (bWasSuccessful)
	{
		JoinSession(SessionName, CachedSessionResult);
	}

	bHandlingDisconnect = false;
}

void UDirectOnlineSessionClient::OnDestroyForMainMenuComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnDestroyForMainMenuComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr SessionInt = GetSessionInt();

	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyForMainMenuCompleteDelegateHandle);
	}

	UWorld* World = GetWorld();
	UNetDriver* NetDriver = World ? World->GetNetDriver() : nullptr;
		
	// Call disconnect to force us back to the menu level
	GEngine->HandleDisconnect(World, NetDriver);

	bHandlingDisconnect = false;
}

void UDirectOnlineSessionClient::DestroyExistingSession(FName SessionName, FOnDestroySessionCompleteDelegate& Delegate)
{
	FDelegateHandle UnusedHandle;
	DestroyExistingSession_Impl(UnusedHandle, SessionName, Delegate);
}

void UDirectOnlineSessionClient::DestroyExistingSession_Impl(FDelegateHandle& OutResult, FName SessionName, FOnDestroySessionCompleteDelegate& Delegate)
{
	IOnlineSessionPtr SessionInt = GetSessionInt();

	if (SessionInt.IsValid())
	{
		OutResult = SessionInt->AddOnDestroySessionCompleteDelegate_Handle(Delegate);
		SessionInt->DestroySession(SessionName);
	}
	else
	{
		OutResult.Reset();
		Delegate.ExecuteIfBound(SessionName, true);
	}
}

void UDirectOnlineSessionClient::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), static_cast<int32>(Result));

	IOnlineSessionPtr SessionInt = GetSessionInt();

	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			FString URL;
			if (SessionInt->GetResolvedConnectString(SessionName, URL))
			{
				APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController(GetWorld());
				if (PC)
				{
					if (bIsFromInvite)
					{
						URL += TEXT("?bIsFromInvite");
						bIsFromInvite = false;
					}
					PC->ClientTravel(URL, TRAVEL_Absolute);
				}
			}
			else
			{
				UE_LOG(LogB3atZOnline, Warning, TEXT("Failed to join session %s"), *SessionName.ToString());
			}
		}
	}
}

void UDirectOnlineSessionClient::JoinSession(FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	// Clean up existing sessions if applicable
	IOnlineSessionPtr SessionInt = GetSessionInt();

	if (SessionInt.IsValid())
	{
		EB3atZOnlineSessionState::Type SessionState = SessionInt->GetSessionState(SessionName);
		if (SessionState != EB3atZOnlineSessionState::NoSession)
		{
			CachedSessionResult = SearchResult;
			OnEndForJoinSessionCompleteDelegateHandle = EndExistingSession_Impl(SessionName, OnEndForJoinSessionCompleteDelegate);
		}
		else
		{
			GetGameInstance()->JoinSession(GetGameInstance()->GetFirstGamePlayer(), SearchResult);
		}
	}
}

void UDirectOnlineSessionClient::HandleDisconnect(UWorld *World, UNetDriver *NetDriver)
{
	bool bWasHandled = HandleDisconnectInternal(World, NetDriver);
	
	if (!bWasHandled)
	{
		// This may have been a pending net game that failed, let the engine handle it (dont tear our stuff down)
		// (Would it be better to return true/false based on if we handled the disconnect or not? Let calling code call GEngine stuff
		GEngine->HandleDisconnect(World, NetDriver);
	}
}

bool UDirectOnlineSessionClient::HandleDisconnectInternal(UWorld* World, UNetDriver* NetDriver)
{
	// This was a disconnect for our active world, we will handle it
	if (GetWorld() == World)
	{
		// Prevent multiple calls to this async flow
		if (!bHandlingDisconnect)
		{
			bHandlingDisconnect = true;
			DestroyExistingSession_Impl(OnDestroyForMainMenuCompleteDelegateHandle, GameSessionName, OnDestroyForMainMenuCompleteDelegate);
		}

		return true;
	}

	return false;
}

void UDirectOnlineSessionClient::StartOnlineSession(FName SessionName)
{
	UE_LOG(LogTemp, Warning, TEXT("OSC StartOnlineSession"));

	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (SessionInterface.IsValid())
	{
		FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
		if (Session &&
			(Session->SessionState == EB3atZOnlineSessionState::Pending || Session->SessionState == EB3atZOnlineSessionState::Ended))
		{
			StartSessionCompleteHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &UDirectOnlineSessionClient::OnStartSessionComplete));
			SessionInterface->StartSession(SessionName);
		}
	}
}

void UDirectOnlineSessionClient::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnStartSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
	}
}

void UDirectOnlineSessionClient::EndOnlineSession(FName SessionName)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (SessionInterface.IsValid())
	{
		FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
		if (Session &&
			Session->SessionState == EB3atZOnlineSessionState::InProgress)
		{
			EndSessionCompleteHandle = SessionInterface->AddOnEndSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &UDirectOnlineSessionClient::OnEndSessionComplete));
			SessionInterface->EndSession(SessionName);
		}
	}
}

void UDirectOnlineSessionClient::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnEndSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);

	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnEndSessionCompleteDelegate_Handle(EndSessionCompleteHandle);
	}
}

void UDirectOnlineSessionClient::SetInviteFlags(UWorld* World, const FJoinabilitySettings& Settings)
{
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		FOnlineSessionSettingsBeatZ* GameSettings = SessionInt->GetSessionSettings(Settings.SessionName);
		if (GameSettings != NULL)
		{
			GameSettings->bShouldAdvertise = Settings.bPublicSearchable;
			GameSettings->bAllowInvites = Settings.bAllowInvites;
			GameSettings->bAllowJoinViaPresence = Settings.bJoinViaPresence && !Settings.bJoinViaPresenceFriendsOnly;
			GameSettings->bAllowJoinViaPresenceFriendsOnly = Settings.bJoinViaPresenceFriendsOnly;
			GameSettings->NumPublicConnections = Settings.MaxPlayers;
			SessionInt->UpdateSession(Settings.SessionName, *GameSettings, false);
		}
	}
}
