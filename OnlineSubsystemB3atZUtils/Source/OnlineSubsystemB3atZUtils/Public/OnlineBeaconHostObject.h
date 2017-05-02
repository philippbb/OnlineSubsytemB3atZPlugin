// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/Actor.h"
#include "B3atZOnlineBeacon.h"
#include "OnlineBeaconHostObject.generated.h"

class AB3atZOnlineBeaconClient;
class UNetConnection;

/**
 * Base class for any unique beacon connectivity, paired with an AB3atZOnlineBeaconClient implementation 
 *
 * By defining a beacon type and implementing the ability to spawn unique AB3atZOnlineBeaconClients, any two instances of the engine
 * can communicate with each other without officially connecting through normal Unreal networking
 */
UCLASS(transient, config=Engine, notplaceable)
class ONLINESUBSYSTEMB3ATZUTILS_API AB3atZOnlineBeaconHostObject : public AActor
{
	GENERATED_UCLASS_BODY()

	/** @return the name of the net driver associated with this object */
	FName GetNetDriverName() const;

	/** Get the state of the beacon (accepting/rejecting connections) */
	EBeaconState::Type GetBeaconState() const;

	/** Get the type of beacon supported by this host */
	const FString& GetBeaconType() const { return BeaconTypeName; }

	/** Simple accessor for client beacon actor class */
	TSubclassOf<AB3atZOnlineBeaconClient> GetClientBeaconActorClass() const { return ClientBeaconActorClass; }

	/**
	 * Each beacon host must be able to spawn the appropriate client beacon actor to communicate with the initiating client
	 *
	 * @return new client beacon actor that this beacon host knows how to communicate with
	 */
	virtual AB3atZOnlineBeaconClient* SpawnBeaconActor(UNetConnection* ClientConnection);

	/**
	 * Delegate triggered when a new client connection is made
	 *
	 * @param NewClientActor new client beacon actor
	 * @param ClientConnection new connection established
	 */
	virtual void OnClientConnected(AB3atZOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection);

	/**
	 * Disconnect a given client from the host
	 *
	 * @param ClientActor the beacon client to disconnect
	 */
	virtual void DisconnectClient(AB3atZOnlineBeaconClient* ClientActor);

	/**
	 * Notification that a client has been disconnected from the host in some way (timeout, client initiated, etc)
	 *
	 * @param LeavingClientActor actor that has disconnected
	 */
	virtual void NotifyClientDisconnected(AB3atZOnlineBeaconClient* LeavingClientActor);

	/**
	 * Called when this class is unregistered by the beacon host 
	 * Do any necessary cleanup.
	 */
	virtual void Unregister();

protected:

	/** Custom name for this beacon */
	UPROPERTY(Transient)
	FString BeaconTypeName;

	/** Class reference for spawning client beacon actor */
	UPROPERTY()
	TSubclassOf<AB3atZOnlineBeaconClient> ClientBeaconActorClass;

	/** List of all client beacon actors with active connections */
	UPROPERTY()
	TArray<AB3atZOnlineBeaconClient*> ClientActors;
};
