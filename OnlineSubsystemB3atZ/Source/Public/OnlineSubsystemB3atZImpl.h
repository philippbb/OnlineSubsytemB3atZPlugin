// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "OnlineSubsystemB3atZ.h"
#include "Containers/Queue.h"
#include "OnlineSubsystemPackage.h"

struct FOnlineError;

DECLARE_DELEGATE(FNextTickDelegate);

namespace OSSConsoleVariables
{
	extern ONLINESUBSYSTEMB3ATZ_API TAutoConsoleVariable<int32> CVarVoiceLoopback;
}

/**
 *	FOnlineSubsystemB3atZImpl - common functionality to share across online platforms, not intended for direct use
 */
class ONLINESUBSYSTEMB3ATZ_API FOnlineSubsystemB3atZImpl 
	: public IOnlineSubsystemB3atZ
{
private:

	/**
	 * Exec function handling for Exec() call
	 */
	bool HandleFriendExecCommands(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar);
	bool HandleSessionExecCommands(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar);
	bool HandlePurchaseExecCommands(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar);
	
	/** Delegate fired when exec cheat related to receipts completes */
	void OnQueryReceiptsComplete(const FOnlineError& Result, TSharedPtr<const FUniqueNetId> UserId);
	
	/** Dump purchase receipts for a given user id */
	void DumpReceipts(const FUniqueNetId& UserId);

protected:

	/** Hidden on purpose */
	FOnlineSubsystemB3atZImpl();
	FOnlineSubsystemB3atZImpl(FName InInstanceName);

	/** Instance name (disambiguates PIE instances for example) */
	FName InstanceName;

	/** Whether or not the online subsystem is in forced dedicated server mode */
	bool bForceDedicated;

	/** Holds all currently named interfaces */
	mutable class UB3atZNamedInterfaces* NamedInterfaces;

	/** Load in any named interfaces specified by the ini configuration */
	void InitNamedInterfaces();

	/** Delegate fired when named interfaces are cleaned up at exit */
	void OnNamedInterfaceCleanup();

	/** Queue to hold callbacks scheduled for next tick using ExecuteNextTick */
	TQueue<FNextTickDelegate, EQueueMode::Mpsc> NextTickQueue;

	/** Buffer to hold callbacks for the current tick (so it's safe to call ExecuteNextTick within a tick callback) */
	TArray<FNextTickDelegate> CurrentTickBuffer;

	/** Start Ticker */
	void StartTicker();

	/** Stop Ticker */
	void StopTicker();
	
	/** Delegate for callbacks to Tick */
	FDelegateHandle TickHandle;

public:
	
	virtual ~FOnlineSubsystemB3atZImpl();

	// IOnlineSubsystemB3atZ
	virtual void PreUnload() override;
	virtual bool Shutdown() override;
	virtual bool IsServer() const override;
	virtual bool IsDedicated() const override{ return bForceDedicated || IsRunningDedicatedServer(); }
	virtual void SetForceDedicated(bool bForce) override { bForceDedicated = bForce; }
	virtual class UObject* GetNamedInterface(FName InterfaceName) override;
	virtual void SetNamedInterface(FName InterfaceName, class UObject* NewInterface) override;
	virtual bool IsLocalPlayer(const FUniqueNetId& UniqueId) const override;
	virtual void SetUsingMultiplayerFeatures(const FUniqueNetId& UniqueId, bool bUsingMP) override {};
	virtual EOnlineEnvironmentB3atZ::Type GetOnlineEnvironment() const override { return EOnlineEnvironmentB3atZ::Unknown; }
	virtual IMessageSanitizerPtr GetMessageSanitizer(int32 LocalUserNum, FString& OutAuthTypeToExclude) const override;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/**
	 * Tick function
	 *
	 * @param DeltaTime	time passed since the last call.
	 * @return true if should continue ticking
	 */
	virtual bool Tick(float DeltaTime);

	/**
	 * @return the name of the online subsystem instance
	 */
	virtual FName GetInstanceName() const override { return InstanceName; }

	/**
	 * Queue a delegate to be executed on the next tick
	 */
	void ExecuteDelegateNextTick(const FNextTickDelegate& Callback);

	/**
	 * Templated helper for calling ExecuteDelegateNextTick with a lambda function
	 */
	template<typename LAMBDA_TYPE>
	FORCEINLINE void ExecuteNextTick(LAMBDA_TYPE&& Callback)
	{
		ExecuteDelegateNextTick(FNextTickDelegate::CreateLambda(Callback));
	}

	/** Name given to default OSS instances (disambiguates for PIE) */
	static const FName DefaultInstanceName;
};

