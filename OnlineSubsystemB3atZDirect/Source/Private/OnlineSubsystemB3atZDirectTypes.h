// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemB3atZTypes.h"
#include "IPAddress.h"

class FOnlineSubsystemB3atZDirect;

/** 
 * Implementation of session information
 */
class FOnlineSessionInfoDirect : public FOnlineSessionInfoB3atZ
{
protected:
	
	/** Hidden on purpose */
	FOnlineSessionInfoDirect(const FOnlineSessionInfoDirect& Src)
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoDirect& operator=(const FOnlineSessionInfoDirect& Src)
	{
		return *this;
	}

PACKAGE_SCOPE:

	/** Constructor */
	FOnlineSessionInfoDirect();

	/** 
	 * Initialize a Direct session info with the address of this machine
	 * and an id for the session
	 */
	void Init(const FOnlineSubsystemB3atZDirect& Subsystem);

	/** The ip & port that the host is listening on (valid for LAN/GameServer) */
	TSharedPtr<class FInternetAddr> HostAddr;
	/** Unique Id for this session */
	FB3atZUniqueNetIdString SessionId;

public:

	virtual ~FOnlineSessionInfoDirect() {}

 	bool operator==(const FOnlineSessionInfoDirect& Other) const
 	{
 		return false;
 	}

	virtual const uint8* GetBytes() const override
	{
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64) + sizeof(TSharedPtr<class FInternetAddr>);
	}

	virtual bool IsValid() const override
	{
		// LAN case
		return HostAddr.IsValid() && HostAddr->IsValid();
	}

	virtual FString ToString() const override
	{
		return SessionId.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("HostIP: %s SessionId: %s"), 
			HostAddr.IsValid() ? *HostAddr->ToString(true) : TEXT("INVALID"), 
			*SessionId.ToDebugString());
	}

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return SessionId;
	}
};
