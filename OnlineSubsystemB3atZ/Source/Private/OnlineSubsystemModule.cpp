// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemModule.h"
#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemB3atZImpl.h"

IMPLEMENT_MODULE( FOnlineSubsystemB3atZModule, OnlineSubsystemB3atZ );

/** Helper function to turn the friendly subsystem name into the module name */
static inline FName GetOnlineModuleName(const FString& SubsystemName)
{
	FString ModuleBase(TEXT("OnlineSubsystemB3atZ"));

	FName ModuleName;
	if (!SubsystemName.Contains(ModuleBase, ESearchCase::CaseSensitive))
	{
		ModuleName = FName(*(ModuleBase + SubsystemName));
	}
	else
	{
		ModuleName = FName(*SubsystemName);
	}

	return ModuleName;
}

/**
 * Helper function that loads a given platform service module if it isn't already loaded
 *
 * @param SubsystemName Name of the requested platform service to load
 * @return The module interface of the requested platform service, NULL if the service doesn't exist
 */
static TSharedPtr<IModuleInterface> LoadSubsystemModule(const FString& SubsystemName)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR
	// Early out if we are overriding the module load
	bool bAttemptLoadModule = !FParse::Param(FCommandLine::Get(), *FString::Printf(TEXT("no%s"), *SubsystemName));
	if (bAttemptLoadModule)
#endif
	{
		FName ModuleName;
		FModuleManager& ModuleManager = FModuleManager::Get();

		ModuleName = GetOnlineModuleName(SubsystemName);
		if (!ModuleManager.IsModuleLoaded(ModuleName))
		{
			// Attempt to load the module
			ModuleManager.LoadModule(ModuleName);
		}

		return ModuleManager.GetModule(ModuleName);
	}

	return NULL;
}

void FOnlineSubsystemB3atZModule::StartupModule()
{
	// These should not be LoadModuleChecked because these modules might not exist
	// Load dependent modules to ensure they will still exist during ShutdownModule.
	// We will alwawys load these modules at the cost of extra modules loaded for the few OSS (like Null) that don't use it.
	if (FModuleManager::Get().ModuleExists(TEXT("HTTP")))
	{
		FModuleManager::Get().LoadModule(TEXT("HTTP"));
	}
	if (FModuleManager::Get().ModuleExists(TEXT("XMPP")))
	{
		FModuleManager::Get().LoadModule(TEXT("XMPP"));
	}

	LoadDefaultSubsystem();
	// Also load the console/platform specific OSS which might not necessarily be the default OSS instance
	IOnlineSubsystemB3atZ::GetByPlatform();
}

void FOnlineSubsystemB3atZModule::PreUnloadCallback()
{
	PreUnloadOnlineSubsystem();
}

void FOnlineSubsystemB3atZModule::ShutdownModule()
{
	ShutdownOnlineSubsystem();
}

void FOnlineSubsystemB3atZModule::LoadDefaultSubsystem()
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule loaddefaultsubsystem"));
	FString InterfaceString;

	//ToDo: Maybe add ini file ability
	//InterfaceString = "Direct";
	
	// look up the OSS name from the .ini. 
	// first, look in a per-platform key (DefaultPlatformService_INIPLATNAME)
	FString BaseKeyName = TEXT("DefaultPlatformService");
	FString PlatformKeyName = BaseKeyName + TEXT("_") + FPlatformProperties::IniPlatformName();

	// Load the platform defined "default" online services module
	if (GConfig->GetString(TEXT("OnlineSubsystemB3atZ"), *PlatformKeyName, InterfaceString, GEngineIni) == false ||
		InterfaceString.Len() == 0)
	{
		GConfig->GetString(TEXT("OnlineSubsystemB3atZ"), *BaseKeyName, InterfaceString, GEngineIni);
	}

	if (InterfaceString.Len() > 0)
	{
		FName InterfaceName = FName(*InterfaceString);
		// A module loaded with its factory method set for creation and a default instance of the online subsystem is required
		if (LoadSubsystemModule(InterfaceString).IsValid() &&
			OnlineFactories.Contains(InterfaceName) &&
			GetOnlineSubsystem(InterfaceName) != NULL)
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule LoadDefaultSubsystem is loaded because string len > 0"));
			DefaultPlatformService = InterfaceName;
		}
		else
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule Unable to load default OnlineSubsystemB3atZ module %s, using Direct interface"), *InterfaceString);
			InterfaceString = TEXT("Direct");
			InterfaceName = FName(*InterfaceString);

			// A module loaded with its factory method set for creation and a default instance of the online subsystem is required
			if (LoadSubsystemModule(InterfaceString).IsValid() &&
				OnlineFactories.Contains(InterfaceName) &&
				GetOnlineSubsystem(InterfaceName) != NULL)
			{
				UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule LoadDefaultSubsystem loaded string len > 0 else 222"));
				DefaultPlatformService = InterfaceName;
			}
		}
	}
	else
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("No default platform service specified for OnlineSubsystemB3atZ"));
	}
}

void FOnlineSubsystemB3atZModule::ReloadDefaultSubsystem()
{
	DestroyOnlineSubsystem(DefaultPlatformService);
	LoadDefaultSubsystem();
}

void FOnlineSubsystemB3atZModule::PreUnloadOnlineSubsystem()
{
	// Shutdown all online subsystem instances
	for (TMap<FName, IOnlineSubsystemB3atZPtr>::TIterator It(OnlineSubsystems); It; ++It)
	{
		It.Value()->PreUnload();
	}
}

void FOnlineSubsystemB3atZModule::ShutdownOnlineSubsystem()
{
	FModuleManager& ModuleManager = FModuleManager::Get();

	// Shutdown all online subsystem instances
	for (TMap<FName, IOnlineSubsystemB3atZPtr>::TIterator It(OnlineSubsystems); It; ++It)
	{
		It.Value()->Shutdown();
	}
	OnlineSubsystems.Empty();

	// Unload all the supporting factories
	for (TMap<FName, IB3atZOnlineFactory*>::TIterator It(OnlineFactories); It; ++It)
	{
		//UE_LOG(LogB3atZOnline, Display, TEXT("Unloading online subsystem: %s"), *It.Key().ToString());

		// Unloading the module will do proper cleanup
		FName ModuleName = GetOnlineModuleName(It.Key().ToString());

		const bool bIsShutdown = true;
		ModuleManager.UnloadModule(ModuleName, bIsShutdown);
	} 
	//ensure(OnlineFactories.Num() == 0);
}

void FOnlineSubsystemB3atZModule::RegisterPlatformService(const FName FactoryName, IB3atZOnlineFactory* Factory)
{
	if (!OnlineFactories.Contains(FactoryName))
	{
		OnlineFactories.Add(FactoryName, Factory);
	}
}

void FOnlineSubsystemB3atZModule::UnregisterPlatformService(const FName FactoryName)
{
	if (OnlineFactories.Contains(FactoryName))
	{
		OnlineFactories.Remove(FactoryName);
	}
}

FName FOnlineSubsystemB3atZModule::ParseOnlineSubsystemName(const FName& FullName, FName& SubsystemName, FName& InstanceName) const
{
#if !(UE_GAME || UE_SERVER)
	SubsystemName = DefaultPlatformService;
	InstanceName = FOnlineSubsystemB3atZImpl::DefaultInstanceName;

	if (FullName != NAME_None)
	{
		FString FullNameStr = FullName.ToString();

		int32 DelimIdx = INDEX_NONE;
		static const TCHAR InstanceDelim = ':';
		if (FullNameStr.FindChar(InstanceDelim, DelimIdx))
		{
			if (DelimIdx > 0)
			{
				SubsystemName = FName(*FullNameStr.Left(DelimIdx));
			}

			if ((DelimIdx + 1) < FullNameStr.Len())
			{
				InstanceName = FName(*FullNameStr.RightChop(DelimIdx + 1));
			}
		}
		else
		{
			SubsystemName = FName(*FullNameStr);
		}
	}

	return FName(*FString::Printf(TEXT("%s:%s"), *SubsystemName.ToString(), *InstanceName.ToString()));
#else	
	
	SubsystemName = FullName == NAME_None ? DefaultPlatformService : FullName;
	InstanceName = FOnlineSubsystemB3atZImpl::DefaultInstanceName;

#if !UE_BUILD_SHIPPING
	int32 DelimIdx = INDEX_NONE;
	static const TCHAR InstanceDelim = ':';
	ensure(!FullName.ToString().FindChar(InstanceDelim, DelimIdx) && DelimIdx == INDEX_NONE);
#endif
	return SubsystemName;
#endif // !(UE_GAME || UE_SERVER)
}

IOnlineSubsystemB3atZ* FOnlineSubsystemB3atZModule::GetOnlineSubsystem(const FName InSubsystemName)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem incoming name is %s"), *InSubsystemName.ToString());
	FName SubsystemName, InstanceName;
	FName KeyName = ParseOnlineSubsystemName(InSubsystemName, SubsystemName, InstanceName);

	IOnlineSubsystemB3atZPtr* OnlineSubsystemB3atZ = NULL;
	if (SubsystemName != NAME_None)
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem key name is %s"), *KeyName.ToString());

		OnlineSubsystemB3atZ = OnlineSubsystems.Find(KeyName);
		if (OnlineSubsystemB3atZ == NULL)
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem ptr is null"));
			IB3atZOnlineFactory** OSSFactory = OnlineFactories.Find(SubsystemName);
			if (OSSFactory == NULL)
			{
				UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem Factory ptr is null"));
				// Attempt to load the requested factory
				TSharedPtr<IModuleInterface> NewModule = LoadSubsystemModule(SubsystemName.ToString());
				if (NewModule.IsValid())
				{
					UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem new module is valid"));
					// If the module loaded successfully this should be non-NULL
					OSSFactory = OnlineFactories.Find(SubsystemName);
				}
			}

			if (OSSFactory != NULL)
			{
				UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem oss factory not null"));
				IOnlineSubsystemB3atZPtr NewSubsystemInstance = (*OSSFactory)->CreateSubsystem(InstanceName);
				if (NewSubsystemInstance.IsValid())
				{
					UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemModule GetOnlineSubsystem instance valid"));
					OnlineSubsystems.Add(KeyName, NewSubsystemInstance);
					OnlineSubsystemB3atZ = OnlineSubsystems.Find(KeyName);
				}
				else
				{
						bool* bNotedPreviously = OnlineSubsystemFailureNotes.Find(KeyName);
						if (!bNotedPreviously || !(*bNotedPreviously))
						{
							//UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Unable to create OnlineSubsystemB3atZ module %s"), *SubsystemName.ToString());
							OnlineSubsystemFailureNotes.Add(KeyName, true);
						}
					}
				}
			}
	}

	return (OnlineSubsystemB3atZ == NULL) ? NULL : (*OnlineSubsystemB3atZ).Get();
}

void FOnlineSubsystemB3atZModule::DestroyOnlineSubsystem(const FName InSubsystemName)
{
	FName SubsystemName, InstanceName;
	FName KeyName = ParseOnlineSubsystemName(InSubsystemName, SubsystemName, InstanceName);

	if (SubsystemName != NAME_None)
	{
		IOnlineSubsystemB3atZPtr OnlineSubsystemB3atZ;
		OnlineSubsystems.RemoveAndCopyValue(KeyName, OnlineSubsystemB3atZ);
		if (OnlineSubsystemB3atZ.IsValid())
		{
			OnlineSubsystemB3atZ->Shutdown();
			OnlineSubsystemFailureNotes.Remove(KeyName);
		}
		else
		{
			//UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("OnlineSubsystemB3atZ instance %s not found, unable to destroy."), *KeyName.ToString());
		}
	}
}

bool FOnlineSubsystemB3atZModule::DoesInstanceExist(const FName InSubsystemName) const
{
	bool bIsLoaded = false;

	FName SubsystemName, InstanceName;
	FName KeyName = ParseOnlineSubsystemName(InSubsystemName, SubsystemName, InstanceName);
	if (SubsystemName != NAME_None)
	{
		const IOnlineSubsystemB3atZPtr* OnlineSubsystemB3atZ = OnlineSubsystems.Find(KeyName);
		return OnlineSubsystemB3atZ && OnlineSubsystemB3atZ->IsValid() ? true : false;
	}

	return false;
}

bool FOnlineSubsystemB3atZModule::IsOnlineSubsystemLoaded(const FName InSubsystemName) const
{
	bool bIsLoaded = false;

	FName SubsystemName, InstanceName;
	ParseOnlineSubsystemName(InSubsystemName, SubsystemName, InstanceName);

	if (SubsystemName != NAME_None)
	{
		if (FModuleManager::Get().IsModuleLoaded(GetOnlineModuleName(SubsystemName.ToString())))
		{
			bIsLoaded = true;
		}
	}
	return bIsLoaded;
}

