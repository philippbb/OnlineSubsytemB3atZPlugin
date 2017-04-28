// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineBeaconHostObject.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "OnlineBeaconHost.h"
#include "B3atZOnlineBeaconClient.h"

AB3atZOnlineBeaconHostObject::AB3atZOnlineBeaconHostObject(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	BeaconTypeName(TEXT("UNDEFINED"))
{
	PrimaryActorTick.bCanEverTick = true;
}

AB3atZOnlineBeaconClient* AB3atZOnlineBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	if (ClientBeaconActorClass)
	{
		FActorSpawnParameters SpawnInfo;
		AB3atZOnlineBeaconClient* BeaconActor = GetWorld()->SpawnActor<AB3atZOnlineBeaconClient>(ClientBeaconActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
		if (BeaconActor)
		{
			BeaconActor->SetBeaconOwner(this);
		}

		return BeaconActor;
	}
	
	UE_LOG(LogBeacon, Warning, TEXT("Invalid client beacon actor class of type %s"), *GetBeaconType());
	return nullptr;
}

void AB3atZOnlineBeaconHostObject::OnClientConnected(AB3atZOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	UE_LOG(LogBeacon, Verbose, TEXT("OnClientConnected %s from (%s)"),
		NewClientActor ? *NewClientActor->GetName() : TEXT("NULL"),
		NewClientActor ? *NewClientActor->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));

	ClientActors.Add(NewClientActor);
}

void AB3atZOnlineBeaconHostObject::DisconnectClient(AB3atZOnlineBeaconClient* ClientActor)
{
	AB3atZOnlineBeaconHost* BeaconHost = Cast<AB3atZOnlineBeaconHost>(GetOwner());
	if (BeaconHost)
	{
		BeaconHost->DisconnectClient(ClientActor);
	}
}

void AB3atZOnlineBeaconHostObject::NotifyClientDisconnected(AB3atZOnlineBeaconClient* LeavingClientActor)
{
	UE_LOG(LogBeacon, Verbose, TEXT("NotifyClientDisconnected %s"),
		LeavingClientActor ? *LeavingClientActor->GetName() : TEXT("NULL"));

	// Remove from local list of clients
	if (LeavingClientActor)
	{
		ClientActors.RemoveSingleSwap(LeavingClientActor);
	}

	// Remove from global list of clients
	AB3atZOnlineBeaconHost* BeaconHost = Cast<AB3atZOnlineBeaconHost>(GetOwner());
	if (BeaconHost)
	{
		BeaconHost->RemoveClientActor(LeavingClientActor);
	}
}

void AB3atZOnlineBeaconHostObject::Unregister()
{
	// Kill all the client connections associated with this host object
	for (AB3atZOnlineBeaconClient* ClientActor : ClientActors)
	{
		DisconnectClient(ClientActor);
	}

	SetOwner(nullptr);
}

FName AB3atZOnlineBeaconHostObject::GetNetDriverName() const
{
	AActor* BeaconHost = GetOwner();
	if (BeaconHost)
	{
		return BeaconHost->GetNetDriverName();
	}

	return NAME_None;
}

EBeaconState::Type AB3atZOnlineBeaconHostObject::GetBeaconState() const
{
	AB3atZOnlineBeaconHost* BeaconHost = Cast<AB3atZOnlineBeaconHost>(GetOwner());
	if (BeaconHost)
	{
		return BeaconHost->GetBeaconState();
	}

	return EBeaconState::DenyRequests;
}
