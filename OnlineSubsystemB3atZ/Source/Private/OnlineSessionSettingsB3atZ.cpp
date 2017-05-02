// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "OnlineSessionSettingsB3atZ.h"
#include "OnlineSubsystemB3atZ.h"

void DumpNamedSession(const FNamedOnlineSession* NamedSession)
{
	if (NamedSession != NULL)
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("dumping NamedSession: "));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	SessionName: %s"), *NamedSession->SessionName.ToString());	
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	HostingPlayerNum: %d"), NamedSession->HostingPlayerNum);
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	SessionState: %s"), EB3atZOnlineSessionState::ToString(NamedSession->SessionState));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	RegisteredPlayers: "));
		if (NamedSession->RegisteredPlayers.Num())
		{
			for (int32 UserIdx=0; UserIdx < NamedSession->RegisteredPlayers.Num(); UserIdx++)
			{
				UE_LOG(LogB3atZOnline, Verbose, TEXT("	    %d: %s"), UserIdx, *NamedSession->RegisteredPlayers[UserIdx]->ToDebugString());
			}
		}
		else
		{
			UE_LOG(LogB3atZOnline, Verbose, TEXT("	    0 registered players"));
		}

		DumpSession(NamedSession);
	}
}

void DumpSession(const FOnlineSession* Session)
{
	if (Session != NULL)
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("dumping Session: "));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	OwningPlayerName: %s"), *Session->OwningUserName);	
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	OwningPlayerId: %s"), Session->OwningUserId.IsValid() ? *Session->OwningUserId->ToDebugString() : TEXT("") );
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	NumOpenPrivateConnections: %d"), Session->NumOpenPrivateConnections);	
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	NumOpenPublicConnections: %d"), Session->NumOpenPublicConnections);	
		UE_LOG(LogB3atZOnline, Verbose, TEXT("	SessionInfo: %s"), Session->SessionInfo.IsValid() ? *Session->SessionInfo->ToDebugString() : TEXT("NULL"));
		DumpSessionSettings(&Session->SessionSettings);
	}
}

void DumpSessionSettings(const FOnlineSessionSettings* SessionSettings)
{
	if (SessionSettings != NULL)
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("dumping SessionSettings: "));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tNumPublicConnections: %d"), SessionSettings->NumPublicConnections);
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tNumPrivateConnections: %d"), SessionSettings->NumPrivateConnections);
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbIsLanMatch: %s"), SessionSettings->bIsLANMatch ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbIsDedicated: %s"), SessionSettings->bIsDedicated ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbUsesStats: %s"), SessionSettings->bUsesStats ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbShouldAdvertise: %s"), SessionSettings->bShouldAdvertise ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbAllowJoinInProgress: %s"), SessionSettings->bAllowJoinInProgress ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbAllowInvites: %s"), SessionSettings->bAllowInvites ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbUsesPresence: %s"), SessionSettings->bUsesPresence ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbAllowJoinViaPresence: %s"), SessionSettings->bAllowJoinViaPresence ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tbAllowJoinViaPresenceFriendsOnly: %s"), SessionSettings->bAllowJoinViaPresenceFriendsOnly ? TEXT("true") : TEXT("false"));
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tBuildUniqueId: 0x%08x"), SessionSettings->BuildUniqueId);
		UE_LOG(LogB3atZOnline, Verbose, TEXT("\tSettings:"));
		for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
		{
			FName Key = It.Key();
			const FOnlineSessionSetting& Setting = It.Value();
			UE_LOG(LogB3atZOnline, Verbose, TEXT("\t\t%s=%s"), *Key.ToString(), *Setting.ToString());
		}
	}
}

template<typename ValueType>
void FOnlineSessionSettings::Set(FName Key, const ValueType& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID)
{
	FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data.SetValue(Value);
		Setting->AdvertisementType = InType;
		Setting->ID = InID;
	}
	else
	{
		Settings.Add(Key, FOnlineSessionSetting(Value, InType, InID));
	}
}

/** Explicit instantiation of supported types to Set template above */
#if !UE_BUILD_DOCS
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const int32& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const float& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const uint64& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const double& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const FString& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const bool& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const TArray<uint8>& Value, EB3atZOnlineDataAdvertisementType::Type InType, int32 InID);
#endif

template<typename ValueType> 
void FOnlineSessionSettings::Set(FName Key, const ValueType& Value, EB3atZOnlineDataAdvertisementType::Type InType)
{
	FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data.SetValue(Value);
		Setting->AdvertisementType = InType;
	}
	else
	{
		Settings.Add(Key, FOnlineSessionSetting(Value, InType));
	}
}

/** Explicit instantiation of supported types to Set template above */
#if !UE_BUILD_DOCS
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const int32& Value, EB3atZOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const float& Value, EB3atZOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const uint64& Value, EB3atZOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const double& Value, EB3atZOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const FString& Value, EB3atZOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const bool& Value, EB3atZOnlineDataAdvertisementType::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSessionSettings::Set(FName Key, const TArray<uint8>& Value, EB3atZOnlineDataAdvertisementType::Type InType);
#endif

void FOnlineSessionSettings::Set(FName Key, const FOnlineSessionSetting& SrcSetting)
{
	FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data = SrcSetting.Data;
		Setting->AdvertisementType = SrcSetting.AdvertisementType;
	}
	else
	{
		Settings.Add(Key, SrcSetting);
	}
}

template<typename ValueType> 
bool FOnlineSessionSettings::Get(FName Key, ValueType& Value) const
{
	const FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		Setting->Data.GetValue(Value);
		return true;
	}

	return false;
}

/** Explicit instantiation of supported types to Get template above */
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, int32& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, float& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, uint64& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, double& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, bool& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, FString& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSessionSettings::Get(FName Key, TArray<uint8>& Value) const;

bool FOnlineSessionSettings::Remove(FName Key)
{
	return Settings.Remove(Key) > 0;
}

EB3atZOnlineDataAdvertisementType::Type FOnlineSessionSettings::GetAdvertisementType(FName Key) const
{
	const FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		return Setting->AdvertisementType;
	}

	UE_LOG(LogB3atZOnline, Warning, TEXT("Unable to find key for advertisement type request: %s"), *Key.ToString());
	return EB3atZOnlineDataAdvertisementType::DontAdvertise;
}

int32 FOnlineSessionSettings::GetID(FName Key) const
{
	const FOnlineSessionSetting* Setting = Settings.Find(Key);
	if (Setting)
	{
		return Setting->ID;
	}

	UE_LOG(LogB3atZOnline, Warning, TEXT("Unable to find key for ID request: %s"), *Key.ToString());
	return -1;
}

template<typename ValueType>
void FOnlineSearchSettings::Set(FName Key, const ValueType& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID)
{
	FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		SearchParam->Data.SetValue(Value);
		SearchParam->ComparisonOp = InType;
		SearchParam->ID = InID;
	}
	else
	{
		SearchParams.Add(Key, FOnlineSessionSearchParam(Value, InType, InID));
	}
}

/** Explicit instantiation of supported types to Set template above */
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<int32>(FName Key, const int32& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<float>(FName Key, const float& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<uint64>(FName Key, const uint64& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<double>(FName Key, const double& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<FString>(FName Key, const FString& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set< TArray<uint8> >(FName Key, const TArray<uint8>& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<bool>(FName Key, const bool& Value, EB3atZOnlineComparisonOp::Type InType, int32 InID);

template<typename ValueType> 
void FOnlineSearchSettings::Set(FName Key, const ValueType& Value, EB3atZOnlineComparisonOp::Type InType)
{
	FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		SearchParam->Data.SetValue(Value);
		SearchParam->ComparisonOp = InType;
	}
	else
	{
		SearchParams.Add(Key, FOnlineSessionSearchParam(Value, InType));
	}
}

/** Explicit instantiation of supported types to Set template above */
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<int32>(FName Key, const int32& Value, EB3atZOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<float>(FName Key, const float& Value, EB3atZOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<uint64>(FName Key, const uint64& Value, EB3atZOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<double>(FName Key, const double& Value, EB3atZOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<FString>(FName Key, const FString& Value, EB3atZOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set< TArray<uint8> >(FName Key, const TArray<uint8>& Value, EB3atZOnlineComparisonOp::Type InType);
template ONLINESUBSYSTEMB3ATZ_API void FOnlineSearchSettings::Set<bool>(FName Key, const bool& Value, EB3atZOnlineComparisonOp::Type InType);

template<typename ValueType> 
bool FOnlineSearchSettings::Get(FName Key, ValueType& Value) const
{
	const FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		SearchParam->Data.GetValue(Value);
		return true;
	}

	return false;
}

/** Explicit instantiation of supported types to Get template above */
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get<int32>(FName Key, int32& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get<float>(FName Key, float& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get<uint64>(FName Key, uint64& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get<double>(FName Key, double& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get<FString>(FName Key, FString& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get< TArray<uint8> >(FName Key, TArray<uint8>& Value) const;
template ONLINESUBSYSTEMB3ATZ_API bool FOnlineSearchSettings::Get<bool>(FName Key, bool& Value) const;

EB3atZOnlineComparisonOp::Type FOnlineSearchSettings::GetComparisonOp(FName Key) const
{
	const FOnlineSessionSearchParam* SearchParam = SearchParams.Find(Key);
	if (SearchParam)
	{
		return SearchParam->ComparisonOp;
	}

	UE_LOG(LogB3atZOnline, Warning, TEXT("Unable to find key for comparison op request: %s"), *Key.ToString());
	return EB3atZOnlineComparisonOp::Equals;
}

