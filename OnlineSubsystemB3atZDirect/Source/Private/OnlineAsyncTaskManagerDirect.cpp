// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskManagerDirect.h"

void FOnlineAsyncTaskManagerDirect::OnlineTick()
{
	check(DirectSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}

