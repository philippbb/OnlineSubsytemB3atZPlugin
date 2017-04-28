// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OnlineBeaconHostObject.h"
#include "TestBeaconHost.generated.h"

class AB3atZOnlineBeaconClient;

/**
 * A beacon host used for taking reservations for an existing game session
 */
UCLASS(transient, notplaceable, config=Engine)
class ONLINESUBSYSTEMB3ATZUTILS_API ADirectTestBeaconHost : public AB3atZOnlineBeaconHostObject
{
	GENERATED_UCLASS_BODY()

	//~ Begin AB3atZOnlineBeaconHost Interface 
	virtual AB3atZOnlineBeaconClient* SpawnBeaconActor(class UNetConnection* ClientConnection) override;
	virtual void OnClientConnected(class AB3atZOnlineBeaconClient* NewClientActor, class UNetConnection* ClientConnection) override;
	//~ End AB3atZOnlineBeaconHost Interface 

	virtual bool Init();
};
