// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineBeaconHost.h"
#include "Misc/CommandLine.h"
#include "UObject/Package.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Engine/World.h"
#include "OnlineBeaconHostObject.h"
#include "Engine/PackageMapClient.h"
#include "Misc/NetworkVersion.h"
#include "Net/DataChannel.h"
#include "B3atZOnlineBeaconClient.h"

AB3atZOnlineBeaconHost::AB3atZOnlineBeaconHost(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	NetDriverName = FName(TEXT("BeaconDriverHost"));
}

void AB3atZOnlineBeaconHost::OnNetCleanup(UNetConnection* Connection)
{
	UE_LOG(LogBeacon, Error, TEXT("Cleaning up a beacon host!"));
	ensure(0);
}

bool AB3atZOnlineBeaconHost::InitHost()
{
	UE_LOG(LogTemp, Warning, TEXT("OnlineBeaconHost InitHost"));


	FURL URL(nullptr, TEXT(""), TRAVEL_Absolute);

	// Allow the command line to override the default port
	int32 PortOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("BeaconPort="), PortOverride) && PortOverride != 0)
	{
		ListenPort = PortOverride;
	}

	URL.Port = ListenPort;
	if(URL.Valid)
	{
		if (InitBase() && NetDriver)
		{
			FString Error;
			if (NetDriver->InitListen(this, URL, false, Error))
			{
				ListenPort = URL.Port;
				NetDriver->SetWorld(GetWorld());
				NetDriver->Notify = this;
				NetDriver->InitialConnectTimeout = BeaconConnectionInitialTimeout;
				NetDriver->ConnectionTimeout = BeaconConnectionTimeout;
				return true;
			}
			else
			{
				// error initializing the network stack...
				UE_LOG(LogBeacon, Log, TEXT("AB3atZOnlineBeaconHost::InitHost failed"));
				OnFailure();
			}
		}
	}

	return false;
}

void AB3atZOnlineBeaconHost::HandleNetworkFailure(UWorld* World, class UNetDriver* InNetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (InNetDriver && InNetDriver->NetDriverName == NetDriverName)
	{
		// Timeouts from clients are ignored
		if (FailureType != ENetworkFailure::ConnectionTimeout)
		{
			Super::HandleNetworkFailure(World, InNetDriver, FailureType, ErrorString);
		}
	}
}

void AB3atZOnlineBeaconHost::NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch)
{
	if(NetDriver->ServerConnection == nullptr)
	{
		bool bCloseConnection = false;

		// We are the server.
#if !(UE_BUILD_SHIPPING && WITH_EDITOR)
		UE_LOG(LogBeacon, Verbose, TEXT("%s[%s] Host received: %s"), *GetName(), Connection ? *Connection->GetName() : TEXT("Invalid"), FNetControlMessageInfo::GetName(MessageType));
#endif
		switch (MessageType)
		{
		case NMT_Hello:
			{
				UE_LOG(LogBeacon, Log, TEXT("Beacon Hello"));
				uint8 IsLittleEndian;

				uint32 RemoteNetworkVersion = 0;
				uint32 LocalNetworkVersion = FNetworkVersion::GetLocalNetworkVersion();

				FNetControlMessage<NMT_Hello>::Receive(Bunch, IsLittleEndian, RemoteNetworkVersion);

				if (!FNetworkVersion::IsNetworkCompatible(LocalNetworkVersion, RemoteNetworkVersion))
				{
					UE_LOG(LogBeacon, Log, TEXT("Client not network compatible %s"), *Connection->GetName());
					FNetControlMessage<NMT_Upgrade>::Send(Connection, LocalNetworkVersion);
					bCloseConnection = true;
				}
				else
				{
					Connection->Challenge = FString::Printf(TEXT("%08X"), FPlatformTime::Cycles());
					FNetControlMessage<NMT_BeaconWelcome>::Send(Connection);
					Connection->FlushNet();
				}
				break;
			}
		case NMT_Netspeed:
			{
				int32 Rate;
				FNetControlMessage<NMT_Netspeed>::Receive(Bunch, Rate);
				Connection->CurrentNetSpeed = FMath::Clamp(Rate, 1800, NetDriver->MaxClientRate);
				UE_LOG(LogBeacon, Log, TEXT("Client netspeed is %i"), Connection->CurrentNetSpeed);
				break;
			}
		case NMT_BeaconJoin:
			{
				FString ErrorMsg;
				FString BeaconType;
				FUniqueNetIdRepl UniqueId;
				FNetControlMessage<NMT_BeaconJoin>::Receive(Bunch, BeaconType, UniqueId);
				UE_LOG(LogBeacon, Log, TEXT("Beacon Join %s %s"), *BeaconType, *UniqueId.ToDebugString());

				if (Connection->ClientWorldPackageName == NAME_None)
				{
					AB3atZOnlineBeaconClient* ClientActor = GetClientActor(Connection);
					if (ClientActor == nullptr)
					{
						UWorld* World = GetWorld();
						Connection->ClientWorldPackageName = World->GetOutermost()->GetFName();

						AB3atZOnlineBeaconClient* NewClientActor = nullptr;
						FOnBeaconSpawned* OnBeaconSpawnedDelegate = OnBeaconSpawnedMapping.Find(BeaconType);
						if (OnBeaconSpawnedDelegate && OnBeaconSpawnedDelegate->IsBound())
						{
							NewClientActor = OnBeaconSpawnedDelegate->Execute(Connection);
						}

						if (NewClientActor && BeaconType == NewClientActor->GetBeaconType())
						{
							NewClientActor->SetConnectionState(EDirectBeaconConnectionState::Pending);

							FNetworkGUID NetGUID = Connection->Driver->GuidCache->AssignNewNetGUID_Server(NewClientActor);
							NewClientActor->SetNetConnection(Connection);
							Connection->PlayerId = UniqueId;
							Connection->OwningActor = NewClientActor;
							NewClientActor->Role = ROLE_Authority;
							NewClientActor->SetReplicates(false);
							check(NetDriverName == NetDriver->NetDriverName);
							NewClientActor->SetNetDriverName(NetDriverName);
							ClientActors.Add(NewClientActor);
							FNetControlMessage<NMT_BeaconAssignGUID>::Send(Connection, NetGUID);
						}
						else
						{
							ErrorMsg = NSLOCTEXT("NetworkErrors", "BeaconSpawnFailureError", "Join failure, Couldn't spawn beacon.").ToString();
						}
					}
					else
					{
						ErrorMsg = NSLOCTEXT("NetworkErrors", "BeaconSpawnExistingActorError", "Join failure, existing beacon actor.").ToString();
					}
				}
				else
				{
					ErrorMsg = NSLOCTEXT("NetworkErrors", "BeaconSpawnClientWorldPackageNameError", "Join failure, existing ClientWorldPackageName.").ToString();
				}

				if (!ErrorMsg.IsEmpty())
				{
					UE_LOG(LogBeacon, Log, TEXT("%s: %s"), *Connection->GetName(), *ErrorMsg);
					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
					bCloseConnection = true;
				}

				break;
			}
		case NMT_BeaconNetGUIDAck:
			{
				FString ErrorMsg;
				FString BeaconType;
				FNetControlMessage<NMT_BeaconNetGUIDAck>::Receive(Bunch, BeaconType);

				AB3atZOnlineBeaconClient* ClientActor = GetClientActor(Connection);
				if (ClientActor && BeaconType == ClientActor->GetBeaconType())
				{
					FOnBeaconConnected* OnBeaconConnectedDelegate = OnBeaconConnectedMapping.Find(BeaconType);
					if (OnBeaconConnectedDelegate)
					{
						ClientActor->SetReplicates(true);
						ClientActor->SetAutonomousProxy(true);
						ClientActor->SetConnectionState(EDirectBeaconConnectionState::Open);
						// Send an RPC to the client to open the actor channel and guarantee RPCs will work
						ClientActor->ClientOnConnected();
						UE_LOG(LogBeacon, Log, TEXT("Handshake complete for %s!"), *ClientActor->GetName());

						OnBeaconConnectedDelegate->ExecuteIfBound(ClientActor, Connection);
					}
					else
					{
						// Failed to connect.
						ErrorMsg = NSLOCTEXT("NetworkErrors", "BeaconSpawnNetGUIDAckError1", "Join failure, no host object at NetGUIDAck.").ToString();
					}
				}
				else
				{
					// Failed to connect.
					ErrorMsg = NSLOCTEXT("NetworkErrors", "BeaconSpawnNetGUIDAckError2", "Join failure, no actor at NetGUIDAck.").ToString();
				}

				if (!ErrorMsg.IsEmpty())
				{
					UE_LOG(LogBeacon, Log, TEXT("%s: %s"), *Connection->GetName(), *ErrorMsg);
					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
					bCloseConnection = true;
				}

				break;
			}
		case NMT_BeaconWelcome:
		case NMT_BeaconAssignGUID:
		default:
			{
				FString ErrorMsg = NSLOCTEXT("NetworkErrors", "BeaconSpawnUnexpectedError", "Join failure, unexpected control message.").ToString();
				UE_LOG(LogBeacon, Log, TEXT("%s: %s"), *Connection->GetName(), *ErrorMsg);
				FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
				bCloseConnection = true;
			}
			break;
		}

		if (bCloseConnection)
		{		
			UE_LOG(LogBeacon, Verbose, TEXT("Closing connection %s: %s"), *Connection->GetName(), *Connection->PlayerId.ToDebugString());
			AB3atZOnlineBeaconClient* ClientActor = GetClientActor(Connection);
			if (ClientActor)
			{
				UE_LOG(LogBeacon, Verbose, TEXT("- BeaconActor: %s %s"), *ClientActor->GetName(), *ClientActor->GetBeaconType());
				AB3atZOnlineBeaconHostObject* BeaconHostObject = GetHost(ClientActor->GetBeaconType());
				if (BeaconHostObject)
				{
					UE_LOG(LogBeacon, Verbose, TEXT("- HostObject: %s"), *BeaconHostObject->GetName());
					BeaconHostObject->NotifyClientDisconnected(ClientActor);
				}

				RemoveClientActor(ClientActor);
			}

			Connection->FlushNet(true);
			Connection->Close();
			UE_LOG(LogBeacon, Verbose, TEXT("--------------------------------"));
		}
	}
}

void AB3atZOnlineBeaconHost::DisconnectClient(AB3atZOnlineBeaconClient* ClientActor)
{
	if (ClientActor && ClientActor->GetConnectionState() != EDirectBeaconConnectionState::Closed && !ClientActor->IsPendingKill())
	{
		ClientActor->SetConnectionState(EDirectBeaconConnectionState::Closed);

		UNetConnection* Connection = ClientActor->GetNetConnection();

		UE_LOG(LogBeacon, Log, TEXT("DisconnectClient for %s. UNetConnection %s UNetDriver %s State %d"), 
			*GetNameSafe(ClientActor), 
			*GetNameSafe(Connection), 
			Connection ? *GetNameSafe(Connection->Driver) : TEXT("null"),
			Connection ? Connection->State : -1);

		// Closing the connection will start the chain of events leading to the removal from lists and destruction of the actor
		if (Connection && Connection->State != USOCK_Closed)
		{
			Connection->FlushNet(true);
			Connection->Close();
		}
	}
}

AB3atZOnlineBeaconClient* AB3atZOnlineBeaconHost::GetClientActor(UNetConnection* Connection)
{
	for (int32 ClientIdx=0; ClientIdx < ClientActors.Num(); ClientIdx++)
	{
		if (ClientActors[ClientIdx]->GetNetConnection() == Connection)
		{
			return ClientActors[ClientIdx];
		}
	}

	return nullptr;
}

void AB3atZOnlineBeaconHost::RemoveClientActor(AB3atZOnlineBeaconClient* ClientActor)
{
	if (ClientActor)
	{
		ClientActors.RemoveSingleSwap(ClientActor);
		if (!ClientActor->IsPendingKillPending())
		{
			ClientActor->Destroy();
		}
	}
}

void AB3atZOnlineBeaconHost::RegisterHost(AB3atZOnlineBeaconHostObject* NewHostObject)
{
	const FString& BeaconType = NewHostObject->GetBeaconType();
	if (GetHost(BeaconType) == NULL)
	{
		NewHostObject->SetOwner(this);
		OnBeaconSpawned(BeaconType).BindUObject(NewHostObject, &AB3atZOnlineBeaconHostObject::SpawnBeaconActor);
		OnBeaconConnected(BeaconType).BindUObject(NewHostObject, &AB3atZOnlineBeaconHostObject::OnClientConnected);
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("Beacon host type %s already exists"), *BeaconType);
	}
}

void AB3atZOnlineBeaconHost::UnregisterHost(const FString& BeaconType)
{
	AB3atZOnlineBeaconHostObject* HostObject = GetHost(BeaconType);
	if (HostObject)
	{
		HostObject->Unregister();
	}
	
	OnBeaconSpawned(BeaconType).Unbind();
	OnBeaconConnected(BeaconType).Unbind();
}

AB3atZOnlineBeaconHostObject* AB3atZOnlineBeaconHost::GetHost(const FString& BeaconType)
{
	for (int32 HostIdx=0; HostIdx < Children.Num(); HostIdx++)
	{
		AB3atZOnlineBeaconHostObject* HostObject = Cast<AB3atZOnlineBeaconHostObject>(Children[HostIdx]);
		if (HostObject && HostObject->GetBeaconType() == BeaconType)
		{
			return HostObject;
		}
	}

	return nullptr;
}

AB3atZOnlineBeaconHost::FOnBeaconSpawned& AB3atZOnlineBeaconHost::OnBeaconSpawned(const FString& BeaconType)
{ 
	FOnBeaconSpawned* BeaconDelegate = OnBeaconSpawnedMapping.Find(BeaconType);
	if (BeaconDelegate == nullptr)
	{
		FOnBeaconSpawned NewDelegate;
		OnBeaconSpawnedMapping.Add(BeaconType, NewDelegate);
		BeaconDelegate = OnBeaconSpawnedMapping.Find(BeaconType);
	}

	return *BeaconDelegate; 
}

AB3atZOnlineBeaconHost::FOnBeaconConnected& AB3atZOnlineBeaconHost::OnBeaconConnected(const FString& BeaconType)
{ 
	FOnBeaconConnected* BeaconDelegate = OnBeaconConnectedMapping.Find(BeaconType);
	if (BeaconDelegate == nullptr)
	{
		FOnBeaconConnected NewDelegate;
		OnBeaconConnectedMapping.Add(BeaconType, NewDelegate);
		BeaconDelegate = OnBeaconConnectedMapping.Find(BeaconType);
	}

	return *BeaconDelegate; 
}
