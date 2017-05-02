// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "B3atZOnlineBeaconClient.h"
#include "TestBeaconClient.generated.h"

/**
 * A beacon client used for making reservations with an existing game session
 */
UCLASS(transient, notplaceable, config=Engine)
class ONLINESUBSYSTEMB3ATZUTILS_API ADirectTestBeaconClient : public AB3atZOnlineBeaconClient
{
	GENERATED_UCLASS_BODY()

	//~ Begin AB3atZOnlineBeaconClient Interface
	virtual void OnFailure() override;
	//~ End AB3atZOnlineBeaconClient Interface

	/** Send a ping RPC to the client */
	UFUNCTION(client, reliable)
	virtual void ClientPing();

	/** Send a pong RPC to the host */
	UFUNCTION(server, reliable, WithValidation)
	virtual void ServerPong();
};
