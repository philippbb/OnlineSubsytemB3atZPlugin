// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineEngineInterfaceImpl.h"
#include "OnlineSubsystemB3atZ.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemB3atZUtils.h"

UB3atZOnlineEngineInterfaceImpl::UB3atZOnlineEngineInterfaceImpl(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UB3atZOnlineEngineInterfaceImpl::IsLoaded(FName OnlineIdentifier)
{
	return IOnlineSubsystemB3atZ::IsLoaded(OnlineIdentifier);
}

FName UB3atZOnlineEngineInterfaceImpl::GetOnlineIdentifier(FWorldContext& WorldContext)
{
	IOnlineSubsystemB3atZUtils* Utils = Online::GetUtils();
	if (Utils)
	{
		return Utils->GetOnlineIdentifier(WorldContext);
	}
	return NAME_None;
}

FName UB3atZOnlineEngineInterfaceImpl::GetOnlineIdentifier(UWorld* World)
{
	IOnlineSubsystemB3atZUtils* Utils = Online::GetUtils();
	if (Utils)
	{
		return Utils->GetOnlineIdentifier(World);
	}

	return NAME_None;
}

bool UB3atZOnlineEngineInterfaceImpl::DoesInstanceExist(FName OnlineIdentifier)
{
	return IOnlineSubsystemB3atZ::DoesInstanceExist(OnlineIdentifier);
}

void UB3atZOnlineEngineInterfaceImpl::ShutdownOnlineSubsystem(FName OnlineIdentifier)
{
	IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::Get(OnlineIdentifier);
	if (OnlineSub)
	{
		OnlineSub->Shutdown();
	}
}

void UB3atZOnlineEngineInterfaceImpl::DestroyOnlineSubsystem(FName OnlineIdentifier)
{
	IOnlineSubsystemB3atZ::Destroy(OnlineIdentifier);
}

TSharedPtr<const FUniqueNetId> UB3atZOnlineEngineInterfaceImpl::CreateUniquePlayerId(const FString& Str)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterface CreateUniquePlayerId"));

	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface();
	if (IdentityInt.IsValid())
	{
		UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterface CreateUniquePlayerId IdInterface Valid"));
		return IdentityInt->CreateUniquePlayerId(Str);
	}
	return nullptr;
}

TSharedPtr<const FUniqueNetId> UB3atZOnlineEngineInterfaceImpl::GetUniquePlayerId(UWorld* World, int32 LocalUserNum)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterfaceImpl GetUniquePlayerID"));

	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(World);
	if (IdentityInt.IsValid())
	{
		UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterfaceImpl GetUniquePlayerId IdInterface is valid"));
		TSharedPtr<const FUniqueNetId> UniqueId = IdentityInt->GetUniquePlayerId(LocalUserNum);
		return UniqueId;
	}
	return nullptr;
}

FString UB3atZOnlineEngineInterfaceImpl::GetPlayerNickname(UWorld* World, const FUniqueNetId& UniqueId)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterfaceImpl GetPlayerNickname"));

	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(World);
	if (IdentityInt.IsValid())
	{
		UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterfaceImpl GetPlayerNickname IdentityInt is Valid"));
		return IdentityInt->GetPlayerNickname(UniqueId);
	}

	static FString InvalidName(TEXT("InvalidOSSUser"));
	return InvalidName;
}

bool UB3atZOnlineEngineInterfaceImpl::GetPlayerPlatformNickname(UWorld* World, int32 LocalUserNum, FString& OutNickname)
{
	IOnlineSubsystemB3atZ* PlatformSubsystem = IOnlineSubsystemB3atZ::GetByPlatform(false);
	if (PlatformSubsystem)
	{
		IOnlineIdentityPtr OnlineIdentityInt = PlatformSubsystem->GetIdentityInterface();
		if (OnlineIdentityInt.IsValid())
		{
			OutNickname = OnlineIdentityInt->GetPlayerNickname(LocalUserNum);
			if (!OutNickname.IsEmpty())
			{
				return true;
			}
		}
	}
	return false;
}

bool UB3atZOnlineEngineInterfaceImpl::AutoLogin(UWorld* World, int32 LocalUserNum, const FOnlineAutoLoginComplete& InCompletionDelegate)
{
	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(World);
	if (IdentityInt.IsValid())
	{
		FName OnlineIdentifier = GetOnlineIdentifier(World);

		OnLoginCompleteDelegateHandle = IdentityInt->AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnAutoLoginComplete, OnlineIdentifier, InCompletionDelegate));
		if (IdentityInt->AutoLogin(LocalUserNum))
		{
			// Async login started
			return true;
		}
	}

	// Not waiting for async login
	return false;
}

void UB3atZOnlineEngineInterfaceImpl::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error, FName OnlineIdentifier, FOnlineAutoLoginComplete InCompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OnlineEngineInterface OnAutoLoginComplete"));

	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(OnlineIdentifier);
	if (IdentityInt.IsValid())
	{
		IdentityInt->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);
	}

	InCompletionDelegate.ExecuteIfBound(LocalUserNum, bWasSuccessful, Error);
}

bool UB3atZOnlineEngineInterfaceImpl::IsLoggedIn(UWorld* World, int32 LocalUserNum)
{
	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(World);
	if (IdentityInt.IsValid())
	{
		return (IdentityInt->GetLoginStatus(LocalUserNum) == ELoginStatusB3atZ::LoggedIn);
	}

	return false;
}

void UB3atZOnlineEngineInterfaceImpl::StartSession(UWorld* World, FName SessionName, FOnlineSessionStartComplete& InCompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII StartSession"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
		if (Session && (Session->SessionState == EB3atZOnlineSessionState::Pending || Session->SessionState == EB3atZOnlineSessionState::Ended))
		{
			FName OnlineIdentifier = GetOnlineIdentifier(World);

			FDelegateHandle StartSessionCompleteHandle = SessionInt->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete, OnlineIdentifier, InCompletionDelegate));
			OnStartSessionCompleteDelegateHandles.Add(OnlineIdentifier, StartSessionCompleteHandle);

			SessionInt->StartSession(SessionName);
		}
		else
		{
			InCompletionDelegate.ExecuteIfBound(SessionName, false);
		}
	}
	else
	{
		InCompletionDelegate.ExecuteIfBound(SessionName, false);
	}
}

void UB3atZOnlineEngineInterfaceImpl::OnStartSessionComplete(FName SessionName, bool bWasSuccessful, FName OnlineIdentifier, FOnlineSessionStartComplete CompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII StartSession OnStartSessionComplete"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(OnlineIdentifier);
	if (SessionInt.IsValid())
	{
		// Cleanup the login delegate before calling create below
		FDelegateHandle* DelegateHandle = OnStartSessionCompleteDelegateHandles.Find(OnlineIdentifier);
		if (DelegateHandle)
		{
			SessionInt->ClearOnStartSessionCompleteDelegate_Handle(*DelegateHandle);
		}
	}

	CompletionDelegate.ExecuteIfBound(SessionName, bWasSuccessful);
}

void UB3atZOnlineEngineInterfaceImpl::EndSession(UWorld* World, FName SessionName, FOnlineSessionEndComplete& InCompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII EndSession"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		FName OnlineIdentifier = GetOnlineIdentifier(World);

		FDelegateHandle EndSessionCompleteHandle = SessionInt->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnEndSessionComplete, OnlineIdentifier, InCompletionDelegate));
		OnEndSessionCompleteDelegateHandles.Add(OnlineIdentifier, EndSessionCompleteHandle);

		SessionInt->EndSession(SessionName);
	}
	else
	{
		InCompletionDelegate.ExecuteIfBound(SessionName, false);
	}
}

void UB3atZOnlineEngineInterfaceImpl::OnEndSessionComplete(FName SessionName, bool bWasSuccessful, FName OnlineIdentifier, FOnlineSessionEndComplete CompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII OnEndSessionComplete"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(OnlineIdentifier);
	if (SessionInt.IsValid())
	{
		FDelegateHandle* DelegateHandle = OnEndSessionCompleteDelegateHandles.Find(OnlineIdentifier);
		if (DelegateHandle)
		{
			SessionInt->ClearOnEndSessionCompleteDelegate_Handle(*DelegateHandle);
		}
	}

	CompletionDelegate.ExecuteIfBound(SessionName, bWasSuccessful);
}

bool UB3atZOnlineEngineInterfaceImpl::DoesSessionExist(UWorld* World, FName SessionName)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII DoesSessionExist"));

	FOnlineSessionSettingsBeatZ* SessionSettings = nullptr;
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		SessionSettings = SessionInt->GetSessionSettings(SessionName);
	}

	return SessionSettings != nullptr;
}

bool UB3atZOnlineEngineInterfaceImpl::GetSessionJoinability(UWorld* World, FName SessionName, FJoinabilitySettings& OutSettings)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII Get Session Joinability"));

	bool bValidData = false;

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		FOnlineSessionSettingsBeatZ* SessionSettings = SessionInt->GetSessionSettings(SessionName);
		if (SessionSettings)
		{
			OutSettings.SessionName = SessionName;
			OutSettings.bPublicSearchable = SessionSettings->bShouldAdvertise;
			OutSettings.bAllowInvites = SessionSettings->bAllowInvites;
			OutSettings.bJoinViaPresence = SessionSettings->bAllowJoinViaPresence;
			OutSettings.bJoinViaPresenceFriendsOnly = SessionSettings->bAllowJoinViaPresenceFriendsOnly;
			bValidData = true;
		}
	}

	return bValidData;
}

void UB3atZOnlineEngineInterfaceImpl::UpdateSessionJoinability(UWorld* World, FName SessionName, bool bPublicSearchable, bool bAllowInvites, bool bJoinViaPresence, bool bJoinViaPresenceFriendsOnly)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII UpdateSessionJoinability"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		FOnlineSessionSettingsBeatZ* SessionSettings = SessionInt->GetSessionSettings(SessionName);
		if (SessionSettings != nullptr)
		{
			SessionSettings->bShouldAdvertise = bPublicSearchable;
			SessionSettings->bAllowInvites = bAllowInvites;
			SessionSettings->bAllowJoinViaPresence = bJoinViaPresence && !bJoinViaPresenceFriendsOnly;
			SessionSettings->bAllowJoinViaPresenceFriendsOnly = bJoinViaPresenceFriendsOnly;
			SessionInt->UpdateSession(SessionName, *SessionSettings, true);
		}
	}
}

void UB3atZOnlineEngineInterfaceImpl::RegisterPlayer(UWorld* World, FName SessionName, const FUniqueNetId& UniqueId, bool bWasInvited)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII Register Player"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid() && UniqueId.IsValid())
	{
		SessionInt->RegisterPlayer(SessionName, UniqueId, bWasInvited);
	}
}

void UB3atZOnlineEngineInterfaceImpl::UnregisterPlayer(UWorld* World, FName SessionName, const FUniqueNetId& UniqueId)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII UnregisterPlayer"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid())
	{
		SessionInt->UnregisterPlayer(SessionName, UniqueId);
	}
}

bool UB3atZOnlineEngineInterfaceImpl::GetResolvedConnectString(UWorld* World, FName SessionName, FString& URL)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII GetResolvedConnectString"));

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (SessionInt.IsValid() && SessionInt->GetResolvedConnectString(SessionName, URL))
	{
		return true;
	}

	return false;
}

TSharedPtr<FVoicePacket> UB3atZOnlineEngineInterfaceImpl::GetLocalPacket(UWorld* World, uint8 LocalUserNum)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		TSharedPtr<FVoicePacket> LocalPacket = VoiceInt->GetLocalPacket(LocalUserNum);
		return LocalPacket;
	}

	return nullptr;
}

TSharedPtr<FVoicePacket> UB3atZOnlineEngineInterfaceImpl::SerializeRemotePacket(UWorld* World, FArchive& Ar)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		return VoiceInt->SerializeRemotePacket(Ar);
	}
	return nullptr;
}

void UB3atZOnlineEngineInterfaceImpl::StartNetworkedVoice(UWorld* World, uint8 LocalUserNum)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		VoiceInt->StartNetworkedVoice(LocalUserNum);
	}
}

void UB3atZOnlineEngineInterfaceImpl::StopNetworkedVoice(UWorld* World, uint8 LocalUserNum)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		VoiceInt->StopNetworkedVoice(LocalUserNum);
	}
}

void UB3atZOnlineEngineInterfaceImpl::ClearVoicePackets(UWorld* World)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		VoiceInt->ClearVoicePackets();
	}
}

bool UB3atZOnlineEngineInterfaceImpl::MuteRemoteTalker(UWorld* World, uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		return VoiceInt->MuteRemoteTalker(LocalUserNum, PlayerId, bIsSystemWide);
	}
	return false;
}

bool UB3atZOnlineEngineInterfaceImpl::UnmuteRemoteTalker(UWorld* World, uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		return VoiceInt->UnmuteRemoteTalker(LocalUserNum, PlayerId, bIsSystemWide);
	}
	return false;
}

int32 UB3atZOnlineEngineInterfaceImpl::GetNumLocalTalkers(UWorld* World)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		return VoiceInt->GetNumLocalTalkers();
	}

	return 0;
}

void UB3atZOnlineEngineInterfaceImpl::ShowLeaderboardUI(UWorld* World, const FString& CategoryName)
{
	IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface();
	if(ExternalUI.IsValid())
	{
		ExternalUI->ShowLeaderboardUI(CategoryName);
	}
}

void UB3atZOnlineEngineInterfaceImpl::ShowAchievementsUI(UWorld* World, int32 LocalUserNum)
{
	IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface();
	if (ExternalUI.IsValid())
	{
		ExternalUI->ShowAchievementsUI(LocalUserNum);
	}
}

void UB3atZOnlineEngineInterfaceImpl::BindToExternalUIOpening(const FOnlineExternalUIChanged& Delegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII BindToExternalUIOpening"));

	IOnlineSubsystemB3atZ* SubSystem = IOnlineSubsystemB3atZ::IsLoaded() ? IOnlineSubsystemB3atZ::Get() : nullptr;
	if (SubSystem != nullptr)
	{
		UE_LOG(LogInit, VeryVerbose, TEXT("OEII BindToExternalUIOpening Subsystem ptr valid"));

		IOnlineExternalUIPtr ExternalUI = SubSystem->GetExternalUIInterface();
		if (ExternalUI.IsValid())
		{
			FOnExternalUIChangeDelegate OnExternalUIChangeDelegate;
			OnExternalUIChangeDelegate.BindUObject(this, &ThisClass::OnExternalUIChange, Delegate);

			ExternalUI->AddOnExternalUIChangeDelegate_Handle(OnExternalUIChangeDelegate);
		}
	}

	IOnlineSubsystemB3atZ* SubSystemConsole = IOnlineSubsystemB3atZ::GetByPlatform();
	if (SubSystemConsole != nullptr &&
		SubSystem != SubSystemConsole)
	{
		IOnlineExternalUIPtr ExternalUI = SubSystemConsole->GetExternalUIInterface();
		if (ExternalUI.IsValid())
		{
			FOnExternalUIChangeDelegate OnExternalUIChangeDelegate;
			OnExternalUIChangeDelegate.BindUObject(this, &ThisClass::OnExternalUIChange, Delegate);

			ExternalUI->AddOnExternalUIChangeDelegate_Handle(OnExternalUIChangeDelegate);
		}
	}
}

void UB3atZOnlineEngineInterfaceImpl::OnExternalUIChange(bool bInIsOpening, FOnlineExternalUIChanged Delegate)
{
	Delegate.ExecuteIfBound(bInIsOpening);
}

void UB3atZOnlineEngineInterfaceImpl::DumpSessionState(UWorld* World)
{
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(GetWorld());
	if (SessionInt.IsValid())
	{
		SessionInt->DumpSessionState();
	}
}

void UB3atZOnlineEngineInterfaceImpl::DumpVoiceState(UWorld* World)
{
	IOnlineB3atZVoicePtr VoiceInt = Online::GetB3atZVoiceInterface(World);
	if (VoiceInt.IsValid())
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\n%s"), *VoiceInt->GetVoiceDebugState());
	}
}

//void UB3atZOnlineEngineInterfaceImpl::DumpChatState(UWorld* World)
//{
//	IB3atZOnlineChatPtr ChatInt = Online::GetChatInterface(World);
//	if (ChatInt.IsValid())
//	{
//		ChatInt->DumpChatState();
//	}
//}

#if WITH_EDITOR
bool UB3atZOnlineEngineInterfaceImpl::SupportsOnlinePIE()
{
	return Online::GetUtils()->SupportsOnlinePIE();
}

void UB3atZOnlineEngineInterfaceImpl::SetShouldTryOnlinePIE(bool bShouldTry)
{
	Online::GetUtils()->SetShouldTryOnlinePIE(bShouldTry);
}

int32 UB3atZOnlineEngineInterfaceImpl::GetNumPIELogins()
{
	return Online::GetUtils()->GetNumPIELogins();
}

void UB3atZOnlineEngineInterfaceImpl::SetForceDedicated(FName OnlineIdentifier, bool bForce)
{
	IOnlineSubsystemB3atZ* OnlineSub = IOnlineSubsystemB3atZ::Get(OnlineIdentifier);
	if (OnlineSub)
	{
		OnlineSub->SetForceDedicated(bForce);
	}
}

void UB3atZOnlineEngineInterfaceImpl::LoginPIEInstance(FName OnlineIdentifier, int32 LocalUserNum, int32 PIELoginNum, FOnPIELoginComplete& CompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII LoginPIEInstance"));

	FString ErrorStr;
	if (SupportsOnlinePIE())
	{
		TArray<FOnlineAccountCredentials> PIELogins;
		Online::GetUtils()->GetPIELogins(PIELogins);
		if (PIELogins.IsValidIndex(PIELoginNum))
		{
			IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(OnlineIdentifier);
			if (IdentityInt.IsValid())
			{
				FDelegateHandle DelegateHandle = IdentityInt->AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnPIELoginComplete, OnlineIdentifier, CompletionDelegate));
				OnLoginPIECompleteDelegateHandlesForPIEInstances.Add(OnlineIdentifier, DelegateHandle);
				IdentityInt->Login(LocalUserNum, PIELogins[PIELoginNum]);
			}
			else
			{
				ErrorStr = TEXT("No identify interface to login");
			}
		}
		else
		{ 
			ErrorStr = TEXT("Invalid credentials for PIE login");
		}
	}
	else
	{
		ErrorStr = TEXT("PIE login not supported");
	}

	if (!ErrorStr.IsEmpty())
	{
		CompletionDelegate.ExecuteIfBound(LocalUserNum, false, ErrorStr);
	}
}

void UB3atZOnlineEngineInterfaceImpl::OnPIELoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error, FName OnlineIdentifier, FOnlineAutoLoginComplete InCompletionDelegate)
{
	UE_LOG(LogInit, VeryVerbose, TEXT("OEII OnPIELoginComplete"));

	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(OnlineIdentifier);

	// Cleanup the login delegate before calling create below
	FDelegateHandle* DelegateHandle = OnLoginPIECompleteDelegateHandlesForPIEInstances.Find(OnlineIdentifier);
	if (DelegateHandle)
	{
		IdentityInt->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, *DelegateHandle);
		OnLoginPIECompleteDelegateHandlesForPIEInstances.Remove(OnlineIdentifier);
	}

	InCompletionDelegate.ExecuteIfBound(LocalUserNum, bWasSuccessful, Error);
}

#endif
