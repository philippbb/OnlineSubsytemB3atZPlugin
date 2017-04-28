// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsModule.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemB3atZUtils.h"
#include "OnlinePIESettingsB3atZ.h"

IMPLEMENT_MODULE(FOnlineSubsystemB3atZUtilsModule, OnlineSubsystemB3atZUtils);

/**
 * Concrete implementation of IOnlineSubsystemB3atZUtils interface 
 */
class FOnlineSubsystemUtils : public IOnlineSubsystemB3atZUtils
{
public:

	FOnlineSubsystemUtils() 
		: bShouldTryOnlinePIE(true)
	{}

	virtual ~FOnlineSubsystemUtils() {}

	FName GetOnlineIdentifier(const FWorldContext& WorldContext) override
	{
#if WITH_EDITOR
		return FName(*FString::Printf(TEXT(":%s"), *WorldContext.ContextHandle.ToString()));
#else
		return NAME_None;
#endif
	}

	FName GetOnlineIdentifier(UWorld* World, const FName Subsystem = NAME_None) override
	{
#if WITH_EDITOR
		if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
		{
			return FName(
				*FString::Printf(TEXT("%s:%s"), !Subsystem.IsNone() ? *Subsystem.ToString() : TEXT(""), *WorldContext->ContextHandle.ToString()));
		}

		return NAME_None;
#else
		return Subsystem;
#endif
	}

#if WITH_EDITOR
	virtual bool SupportsOnlinePIE() const override
	{
		const UB3atZOnlinePIESettingsB3atZ* OnlinePIESettings = GetDefault<UB3atZOnlinePIESettingsB3atZ>();
		if (OnlinePIESettings->bOnlinePIEEnabled && GetNumPIELogins() > 0)
		{
			// If we can't get the identity interface then things are either not configured right or disabled
			IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface();
			return IdentityInt.IsValid();
		}

		return false;
	}

	virtual void SetShouldTryOnlinePIE(bool bShouldTry) override
	{
		if (bShouldTryOnlinePIE != bShouldTry)
		{
			bShouldTryOnlinePIE = bShouldTry;

			// This will swap it back to the null subsystem if needed
			IOnlineSubsystemB3atZ::ReloadDefaultSubsystem();
		}
	}

	virtual bool IsOnlinePIEEnabled() const override
	{
		const UB3atZOnlinePIESettingsB3atZ* OnlinePIESettings = GetDefault<UB3atZOnlinePIESettingsB3atZ>();
		return bShouldTryOnlinePIE && OnlinePIESettings->bOnlinePIEEnabled;
	}

	virtual int32 GetNumPIELogins() const override
	{
		int32 NumValidLogins = 0;
		const UB3atZOnlinePIESettingsB3atZ* OnlinePIESettings = GetDefault<UB3atZOnlinePIESettingsB3atZ>();
		for (const FPIELoginSettingsInternalB3atZ& Login : OnlinePIESettings->Logins)
		{
			if (Login.IsValid())
			{
				NumValidLogins++;
			}
		}
	
		return NumValidLogins;
	}

	virtual void GetPIELogins(TArray<FOnlineAccountCredentials>& Logins) override
	{
		const UB3atZOnlinePIESettingsB3atZ* OnlinePIESettings = GetDefault<UB3atZOnlinePIESettingsB3atZ>();
		if (OnlinePIESettings->Logins.Num() > 0)
		{
			FOnlineAccountCredentials TempLogin;

			Logins.Empty(OnlinePIESettings->Logins.Num());
			for (const FPIELoginSettingsInternalB3atZ& Login : OnlinePIESettings->Logins)
			{
				if (Login.IsValid())
				{
					new (Logins)FOnlineAccountCredentials(Login.Type, Login.Id, Login.Token);
				}
			}
		}
	}

#endif // WITH_EDITOR

private:

	// If false it will not try to do online PIE at all
	bool bShouldTryOnlinePIE;
};

void FOnlineSubsystemB3atZUtilsModule::StartupModule()
{
	SubsystemUtils = new FOnlineSubsystemUtils();
}

void FOnlineSubsystemB3atZUtilsModule::ShutdownModule()
{
	delete SubsystemUtils;
	SubsystemUtils = nullptr;
}
