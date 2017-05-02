// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "TestBeaconClient.h"

ADirectTestBeaconClient::ADirectTestBeaconClient(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
}

void ADirectTestBeaconClient::OnFailure()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Verbose, TEXT("Test beacon connection failure, handling connection timeout."));
#endif
	Super::OnFailure();
}

void ADirectTestBeaconClient::ClientPing_Implementation()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Log, TEXT("Ping"));
	ServerPong();
#endif
}

bool ADirectTestBeaconClient::ServerPong_Validate()
{
#if !UE_BUILD_SHIPPING
	return true;
#else
	return false;
#endif
}

void ADirectTestBeaconClient::ServerPong_Implementation()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBeacon, Log, TEXT("Pong"));
	ClientPing();
#endif
}
