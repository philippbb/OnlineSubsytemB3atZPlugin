// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..
// Code for getting external IP addr from Ue4 community member Rama

#include "CreateB3atZSessionCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSessionSettingsB3atZ.h"
#include "OnlineSubsystemBPCallHelper.h"
#include "GameFramework/PlayerController.h"

#include <iostream>
#include <sstream>
#include <string.h>

//////////////////////////////////////////////////////////////////////////
// UCreateB3atZSessionCallbackProxy

UCreateB3atZSessionCallbackProxy::UCreateB3atZSessionCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CreateCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateCompleted))
	, StartCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartCompleted))
	, NumPublicConnections(1)
{
}

UCreateB3atZSessionCallbackProxy* UCreateB3atZSessionCallbackProxy::CreateB3atZSession(UObject* WorldContextObject, class APlayerController* PlayerController, int32 PublicConnections, bool bUseLAN, FString CustomAddr)
{
	UCreateB3atZSessionCallbackProxy* Proxy = NewObject<UCreateB3atZSessionCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->NumPublicConnections = PublicConnections;
	Proxy->bUseLAN = bUseLAN;
	Proxy->CustomAddr = CustomAddr;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UCreateB3atZSessionCallbackProxy::Activate()
{
	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		UWorld* World = GetWorld();
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);

		if (SessionInt.IsValid())
		{
			FOnlineSessionSettings Settings;
			Settings.NumPublicConnections = NumPublicConnections;
			Settings.bShouldAdvertise = true;
			Settings.bAllowJoinInProgress = true;
			Settings.bIsLANMatch = bUseLAN;
			Settings.bUsesPresence = true;
			Settings.bAllowJoinViaPresence = false;

			SessionInt->CreateSession(0, GameSessionName, Settings);
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FOnlineSubsystemBPCallHelper Helper(TEXT("CreateSession"), GEngine->GetWorldFromContextObject(WorldContextObject));
		Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

		if (Helper.IsValid())
		{
			auto Sessions = Helper.OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				CreateCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegate);

				FOnlineSessionSettings Settings;
				Settings.NumPublicConnections = NumPublicConnections;
				Settings.bShouldAdvertise = true;
				Settings.bAllowJoinInProgress = true;
				Settings.bIsLANMatch = bUseLAN;
				Settings.bUsesPresence = true;
				Settings.bAllowJoinViaPresence = false;

				Sessions->CreateSession(*Helper.UserID, GameSessionName, Settings);

				// OnCreateCompleted will get called, nothing more to do now
				return;
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
			}
		}
	}
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OSBU OSBPCallHelper not valid"));
	// Fail immediately
	OnFailure.Broadcast(SessionIP, SessionHostPort);
}


void UCreateB3atZSessionCallbackProxy::OnCreateCompleted(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions;

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		IOnlineSubsystemB3atZ* const OnlineSub = IOnlineSubsystemB3atZ::Get();
		if (OnlineSub)
		{
			Sessions = OnlineSub->GetSessionInterface();
		}
	}
	else
	{
		FOnlineSubsystemBPCallHelper Helper(TEXT("CreateSessionCallback"), GEngine->GetWorldFromContextObject(WorldContextObject));
		Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

		if (Helper.IsValid())
		{
			Sessions = Helper.OnlineSub->GetSessionInterface();
		}
	}

	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegateHandle);

		if (bWasSuccessful)
		{
			StartCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(StartCompleteDelegate);
			Sessions->StartSession(GameSessionName);

			// OnStartCompleted will get called, nothing more to do now
			return;
		}
	}
	

	SessionIP = "Failed";

	if (!bWasSuccessful)
	{
		OnFailure.Broadcast(SessionIP, SessionHostPort);
	}
}

void UCreateB3atZSessionCallbackProxy::OnStartCompleted(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions;

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		IOnlineSubsystemB3atZ* const OnlineSub = IOnlineSubsystemB3atZ::Get();
		if (OnlineSub)
		{
			Sessions = OnlineSub->GetSessionInterface();
		}
	}
	else
	{
		FOnlineSubsystemBPCallHelper Helper(TEXT("StartSessionCallback"), GEngine->GetWorldFromContextObject(WorldContextObject));
		Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

		if (Helper.IsValid())
		{
			Sessions = Helper.OnlineSub->GetSessionInterface();

		}
	}

	if (Sessions.IsValid())
	{
		Sessions->ClearOnStartSessionCompleteDelegate_Handle(StartCompleteDelegateHandle);
		SessionHostPort = FString::FromInt(Sessions->GetPort());
	}
	

	if (bWasSuccessful)
	{
		if (!bUseLAN)
		{
			UE_LOG(LogB3atZOnline, Verbose, TEXT("CreateSession online successfull getting external IP now"));
			GetMyIP_SendRequest();
		}
		else
		{
			UE_LOG(LogB3atZOnline, Verbose, TEXT("CreateSession LAN successfull"));
			SessionIP = "LAN Local IP";
			SessionHostPort = "Unreal default LAN Port";
			OnSuccess.Broadcast(SessionIP, SessionHostPort);
		}
	}
	else
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("CreateSession NOT successfull"));
		SessionIP = "Failed";
		SessionHostPort = "Failed";
		OnFailure.Broadcast(SessionIP, SessionHostPort);
	}
}


bool UCreateB3atZSessionCallbackProxy::GetMyIP_SendRequest()
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http || !Http->IsHttpEnabled())
	{
		return false;
	}

	FString TargetHost;
	if (CustomAddr.IsEmpty())
	{
		TargetHost	= "http://api.ipify.org";
	}
	else
	{
		TargetHost = CustomAddr;
	}

	TSharedRef <IHttpRequest> Request = Http->CreateRequest();
	Request->SetVerb("Get");
	Request->SetURL(TargetHost);
	Request->SetHeader("UserAgent", "B3atZSession");
	Request->SetHeader("Content-Type", "text/html");

	Request->OnProcessRequestComplete().BindUObject(this, &UCreateB3atZSessionCallbackProxy::HTTPOnResponseReceived);
	if (!Request->ProcessRequest())
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("CreateSession failed processing external IP"));

		SessionIP = "Failed Processing external ip";
		SessionHostPort = "Failed Processing external ip";
		OnFailure.Broadcast(SessionIP, SessionHostPort);

		return false;
	}
	else
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("CreateSession processing external IP waiting for result"));

		return true;
	}
}

void UCreateB3atZSessionCallbackProxy::HTTPOnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessfull)
{
	TArray<FString> IPStrings;

	FString PublicIP = Response->GetContentAsString();

	PublicIP.ParseIntoArray(IPStrings, TEXT("."), true);

	uint32 DecimalIP = 0;

	int32 DecimalInt = 256;

	for (int8 i = 0; i < IPStrings.Num(); i++)
	{
		int8 power = 3 - i;

		uint32 ip = FCString::Atoi(*IPStrings[i]);


		int32 PowerInt = ipow(DecimalInt, power);

		DecimalIP += (ip)*(PowerInt);
	}

	std::stringstream stream;
	uint32 value(DecimalIP);
	stream << value;
	std::string strValue(stream.str());

	FString ConvString = FString(strValue.c_str());

	
	OnSuccess.Broadcast(ConvString, SessionHostPort);
	
	

}


