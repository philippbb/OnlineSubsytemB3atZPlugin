// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemB3atZDirectModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemB3atZDirect.h"

IMPLEMENT_MODULE(FOnlineSubsystemDirectModule, OnlineSubsystemB3atZDirect);

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryDirect : public IB3atZOnlineFactory
{
public:

	FOnlineFactoryDirect() {}
	virtual ~FOnlineFactoryDirect() {}

	virtual IOnlineSubsystemB3atZPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemDirectPtr OnlineSub = MakeShareable(new FOnlineSubsystemB3atZDirect(InstanceName));
		if (OnlineSub->IsEnabled())
		{
			if(!OnlineSub->Init())
			{
				UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("Direct API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINEB3ATZ(VeryVerbose, TEXT("Direct API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemDirectModule::StartupModule()
{
	DirectFactory = new FOnlineFactoryDirect();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemB3atZModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemB3atZModule>("OnlineSubsystemB3atZ");
	OSS.RegisterPlatformService(DIRECT_SUBSYSTEM, DirectFactory);

	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OSMD Startup finished"));
}

void FOnlineSubsystemDirectModule::ShutdownModule()
{
	FOnlineSubsystemB3atZModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemB3atZModule>("OnlineSubsystemB3atZ");
	OSS.UnregisterPlatformService(DIRECT_SUBSYSTEM);
	
	delete DirectFactory;
	DirectFactory = NULL;
}
