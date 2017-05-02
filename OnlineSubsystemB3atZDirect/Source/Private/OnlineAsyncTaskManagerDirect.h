// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

/**
 *	Direct version of the async task manager to register the various Direct callbacks with the engine
 */
class FOnlineAsyncTaskManagerDirect : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemB3atZDirect* DirectSubsystem;

public:

	FOnlineAsyncTaskManagerDirect(class FOnlineSubsystemB3atZDirect* InOnlineSubsystem)
		: DirectSubsystem(InOnlineSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerDirect() 
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
};
