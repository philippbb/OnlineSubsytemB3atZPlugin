// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TestBeaconHost.h"
#include "TestBeaconClient.h"

ADirectTestBeaconHost::ADirectTestBeaconHost(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	ClientBeaconActorClass = ADirectTestBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

bool ADirectTestBeaconHost::Init()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Verbose, TEXT("Init"));
#endif
	return true;
}

void ADirectTestBeaconHost::OnClientConnected(AB3atZOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
#if !UE_BUILD_SHIPPING
	Super::OnClientConnected(NewClientActor, ClientConnection);

	ADirectTestBeaconClient* BeaconClient = Cast<ADirectTestBeaconClient>(NewClientActor);
	if (BeaconClient != NULL)
	{
		BeaconClient->ClientPing();
	}
#endif
}

AB3atZOnlineBeaconClient* ADirectTestBeaconHost::SpawnBeaconActor(UNetConnection* ClientConnection)
{	
#if !UE_BUILD_SHIPPING
	return Super::SpawnBeaconActor(ClientConnection);
#else
	return NULL;
#endif
}
