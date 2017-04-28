// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "../OnlineSubsystemB3atZUtils/Public/B3atZPartyBeaconClient.h"
#include "../OnlineSubsystemB3atZUtils/Public/OnlineSubsystemB3atZUtils.h"
#include "B3atZPartyBeaconHost.h"
//#include "Plugins/Online/OnlineSubsystemB3atZUtils/Source/OnlineSubsystemB3atZUtils/Public/B3atZPartyBeaconClient.h"

#if !UE_BUILD_SHIPPING
namespace BeaconConsoleVariables
{
	/** Time to delay delegates firing a reservation request response */
	TAutoConsoleVariable<float> CVarDelayReservationResponse(
		TEXT("beacon.DelayReservationResponse"),
		0.0f,
		TEXT("Delay time between received response and notification\n")
		TEXT("Time in secs"),
		ECVF_Default);

	/** Time to delay delegates firing a cancel reservation request response */
	TAutoConsoleVariable<float> CVarDelayCancellationResponse(
		TEXT("beacon.DelayCancellationResponse"),
		0.0f,
		TEXT("Delay time between received cancel response and notification\n")
		TEXT("Time in secs"),
		ECVF_Default);

	/** Time to delay delegates firing a reservation update response */
	TAutoConsoleVariable<float> CVarDelayUpdateResponse(
		TEXT("beacon.DelayUpdateResponse"),
		0.0f,
		TEXT("Delay time between received update response and notification\n")
		TEXT("Time in secs"),
		ECVF_Default);

	/** Time to delay delegates firing a reservation full response */
	TAutoConsoleVariable<float> CVarDelayFullResponse(
		TEXT("beacon.DelayFullResponse"),
		0.0f,
		TEXT("Delay time between received full response and notification\n")
		TEXT("Time in secs"),
		ECVF_Default);
}
#endif

/** Max time to wait for a response from the server for CancelReservation */
#define CANCEL_FAILSAFE 5.0f

AB3atZPartyBeaconClient::AB3atZPartyBeaconClient(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	RequestType(EDirectClientRequestType::NonePending),
	bPendingReservationSent(false),
	bCancelReservation(false)
{
}

void AB3atZPartyBeaconClient::BeginDestroy()
{
	ClearTimers();
	Super::BeginDestroy();
}

void AB3atZPartyBeaconClient::ClearTimers()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (PendingResponseTimerHandle.IsValid())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("ClearTimers: Pending reservation response cleared."));
		}

		if (PendingCancelResponseTimerHandle.IsValid())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("ClearTimers: Pending cancel response cleared."));
		}

		if (PendingReservationUpdateTimerHandle.IsValid())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("ClearTimers: Pending reservation update cleared."));
		}

		if (PendingReservationFullTimerHandle.IsValid())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("ClearTimers: Pending reservation full cleared."));
		}

		if (CancelRPCFailsafe.IsValid())
		{
			UE_LOG(LogBeacon, Verbose, TEXT("ClearTimers: Cancel failsafe cleared."));
		}

		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(PendingResponseTimerHandle);
		TM.ClearTimer(PendingCancelResponseTimerHandle);
		TM.ClearTimer(PendingReservationUpdateTimerHandle);
		TM.ClearTimer(PendingReservationFullTimerHandle);
		TM.ClearTimer(CancelRPCFailsafe);

		PendingResponseTimerHandle.Invalidate();
		PendingCancelResponseTimerHandle.Invalidate();
		PendingReservationUpdateTimerHandle.Invalidate();
		PendingReservationFullTimerHandle.Invalidate();
		CancelRPCFailsafe.Invalidate();
	}
}

bool AB3atZPartyBeaconClient::RequestReservation(const FString& ConnectInfoStr, const FString& InSessionId, const FUniqueNetIdRepl& RequestingPartyLeader, const TArray<FB3atZPlayerReservation>& PartyMembers)
{
	bool bSuccess = false;

	FURL ConnectURL(NULL, *ConnectInfoStr, TRAVEL_Absolute);
	if (InitClient(ConnectURL))
	{
		DestSessionId = InSessionId;
		PendingReservation.PartyLeader = RequestingPartyLeader;
		PendingReservation.PartyMembers = PartyMembers;
		bPendingReservationSent = false;
		RequestType = EDirectClientRequestType::ExistingSessionReservation;
		bSuccess = true;
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("RequestReservation: Failure to init client beacon with %s."), *ConnectURL.ToString());
		RequestType = EDirectClientRequestType::NonePending;
	}

	if (!bSuccess)
	{
		OnFailure();
	}

	return bSuccess;
}

bool AB3atZPartyBeaconClient::RequestReservation(const FOnlineSessionSearchResult& DesiredHost, const FUniqueNetIdRepl& RequestingPartyLeader, const TArray<FB3atZPlayerReservation>& PartyMembers)
{
	UE_LOG_ONLINEB3ATZ(Warning, TEXT("BPBC Request Reservation"));
	bool bSuccess = false;

	if (DesiredHost.IsValid())
	{
		UWorld* World = GetWorld();

		IOnlineSubsystemB3atZ* OnlineSub = Online::GetSubsystem(World);
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInt = OnlineSub->GetSessionInterface();
			if (SessionInt.IsValid())
			{
				FString ConnectInfo;
				if (SessionInt->GetResolvedConnectString(DesiredHost, BeaconPort, ConnectInfo))
				{
					FString SessionId = DesiredHost.Session.SessionInfo->GetSessionId().ToString();
					return RequestReservation(ConnectInfo, SessionId, RequestingPartyLeader, PartyMembers);
				}
			}
		}
	}

	if (!bSuccess)
	{
		OnFailure();
	}

	return bSuccess;
}

bool AB3atZPartyBeaconClient::RequestReservationUpdate(const FUniqueNetIdRepl& RequestingPartyLeader, const TArray<FB3atZPlayerReservation>& PlayersToAdd)
{
	bool bWasStarted = false;

	EDirectBeaconConnectionState MyConnectionState = GetConnectionState();
	if (ensure(MyConnectionState == EDirectBeaconConnectionState::Open))
	{
		RequestType = EDirectClientRequestType::ReservationUpdate;
		PendingReservation.PartyLeader = RequestingPartyLeader;
		PendingReservation.PartyMembers = PlayersToAdd;
		ServerUpdateReservationRequest(DestSessionId, PendingReservation);
		bPendingReservationSent = true;
		bWasStarted = true;
	}

	return bWasStarted;
}

bool AB3atZPartyBeaconClient::RequestReservationUpdate(const FString& ConnectInfoStr, const FString& InSessionId, const FUniqueNetIdRepl& RequestingPartyLeader, const TArray<FB3atZPlayerReservation>& PlayersToAdd)
{
	bool bWasStarted = false;

	EDirectBeaconConnectionState MyConnectionState = GetConnectionState();
	if (MyConnectionState != EDirectBeaconConnectionState::Open)
	{
		// create a new pending reservation for these players in the same way as a new reservation request
		bWasStarted = RequestReservation(ConnectInfoStr, InSessionId, RequestingPartyLeader, PlayersToAdd);
		if (bWasStarted)
		{
			// Treat this reservation as an update to an existing reservation on the host
			RequestType = EDirectClientRequestType::ReservationUpdate;
		}
	}
	else if (MyConnectionState == EDirectBeaconConnectionState::Open)
	{
		RequestReservationUpdate(RequestingPartyLeader, PlayersToAdd);
	}

	return bWasStarted;
}

bool AB3atZPartyBeaconClient::RequestReservationUpdate(const FOnlineSessionSearchResult& DesiredHost, const FUniqueNetIdRepl& RequestingPartyLeader, const TArray<FB3atZPlayerReservation>& PlayersToAdd)
{
	bool bWasStarted = false;

	EDirectBeaconConnectionState MyConnectionState = GetConnectionState();
	if (MyConnectionState != EDirectBeaconConnectionState::Open)
	{
		// create a new pending reservation for these players in the same way as a new reservation request
		bWasStarted = RequestReservation(DesiredHost, RequestingPartyLeader, PlayersToAdd);
		if (bWasStarted)
		{
			// Treat this reservation as an update to an existing reservation on the host
			RequestType = EDirectClientRequestType::ReservationUpdate;
		}
	}
	else if (MyConnectionState == EDirectBeaconConnectionState::Open)
	{
		RequestReservationUpdate(RequestingPartyLeader, PlayersToAdd);
	}

	return bWasStarted;
}

void AB3atZPartyBeaconClient::CancelReservation()
{
	if (ensure(PendingReservation.PartyLeader.IsValid()))
	{
		bCancelReservation = true;

		// Clear out any pending response handling, only the cancel matters
		ClearTimers();

		if (bPendingReservationSent)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Sending cancel reservation request."));
			ServerCancelReservationRequest(PendingReservation.PartyLeader);

			// In case the server is loading or unresponsive (ie no host beacon)
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUObject(this, &ThisClass::OnCancelledFailsafe);
			
			UWorld* World = GetWorld();
			check(World);
			World->GetTimerManager().SetTimer(CancelRPCFailsafe, TimerDelegate, CANCEL_FAILSAFE, false);
		}
		else
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Reservation request never sent, no need to send cancelation request."));
			OnCancelledComplete();
		}
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Unable to cancel reservation request with invalid party leader."));
		OnCancelledComplete();
	}
}

void AB3atZPartyBeaconClient::OnConnected()
{
	if (!bCancelReservation)
	{
		if (RequestType == EDirectClientRequestType::ExistingSessionReservation)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Party beacon connection established, sending join reservation request."));
			ServerReservationRequest(DestSessionId, PendingReservation);
			bPendingReservationSent = true;
		}
		else if (RequestType == EDirectClientRequestType::ReservationUpdate)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Party beacon connection established, sending reservation update request."));
			ServerUpdateReservationRequest(DestSessionId, PendingReservation);
			bPendingReservationSent = true;
		}
		else
		{
			UE_LOG(LogBeacon, Warning, TEXT("Failed to handle reservation request type %s"), ToString(RequestType));
			OnFailure();
		}
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Reservation request previously canceled, nothing sent."));
		OnCancelledComplete();
	}
}

void AB3atZPartyBeaconClient::OnCancelledFailsafe()
{
	ClientCancelReservationResponse_Implementation(EB3atZPartyReservationResult::ReservationRequestCanceled);
}
 
void AB3atZPartyBeaconClient::OnCancelledComplete()
{
	ReservationRequestComplete.ExecuteIfBound(EB3atZPartyReservationResult::ReservationRequestCanceled);
	RequestType = EDirectClientRequestType::NonePending;
	bCancelReservation = false;
}

void AB3atZPartyBeaconClient::OnFailure()
{
	ClearTimers();
	RequestType = EDirectClientRequestType::NonePending;
	Super::OnFailure();
}

bool AB3atZPartyBeaconClient::ServerReservationRequest_Validate(const FString& SessionId, const FB3atZPartyReservation& Reservation)
{
	return !SessionId.IsEmpty() && Reservation.PartyLeader.IsValid() && Reservation.PartyMembers.Num() > 0;
}

void AB3atZPartyBeaconClient::ServerReservationRequest_Implementation(const FString& SessionId, const FB3atZPartyReservation& Reservation)
{
	ADirectPartyBeaconHost* BeaconHost = Cast<ADirectPartyBeaconHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		PendingReservation = Reservation;
		RequestType = EDirectClientRequestType::ExistingSessionReservation;
		BeaconHost->ProcessReservationRequest(this, SessionId, Reservation);
	}
}

bool AB3atZPartyBeaconClient::ServerUpdateReservationRequest_Validate(const FString& SessionId, const FB3atZPartyReservation& ReservationUpdate)
{
	return !SessionId.IsEmpty() && ReservationUpdate.PartyLeader.IsValid() && ReservationUpdate.PartyMembers.Num() > 0;
}

void AB3atZPartyBeaconClient::ServerUpdateReservationRequest_Implementation(const FString& SessionId, const FB3atZPartyReservation& ReservationUpdate)
{
	ADirectPartyBeaconHost* BeaconHost = Cast<ADirectPartyBeaconHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		PendingReservation = ReservationUpdate;
		RequestType = EDirectClientRequestType::ReservationUpdate;
		BeaconHost->ProcessReservationUpdateRequest(this, SessionId, ReservationUpdate);
	}
}

bool AB3atZPartyBeaconClient::ServerCancelReservationRequest_Validate(const FUniqueNetIdRepl& PartyLeader)
{
	return true;
}

void AB3atZPartyBeaconClient::ServerCancelReservationRequest_Implementation(const FUniqueNetIdRepl& PartyLeader)
{
	ADirectPartyBeaconHost* BeaconHost = Cast<ADirectPartyBeaconHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		bCancelReservation = true;
		BeaconHost->ProcessCancelReservationRequest(this, PartyLeader);
	}
}

void AB3atZPartyBeaconClient::ClientReservationResponse_Implementation(EB3atZPartyReservationResult::Type ReservationResponse)
{
	if (!bCancelReservation)
	{
#if !UE_BUILD_SHIPPING
		const float Rate = BeaconConsoleVariables::CVarDelayReservationResponse.GetValueOnGameThread();
#else
		const float Rate = 0.0f;
#endif
		if (Rate > 0.0f)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Party beacon response received %s, waiting %fs to notify"), EB3atZPartyReservationResult::ToString(ReservationResponse), Rate);

			FTimerDelegate TimerDelegate;
			TimerDelegate.BindLambda([this, ReservationResponse]()
			{
				ProcessReservationResponse(ReservationResponse);
			});

			PendingResponseTimerHandle = DelayResponse(TimerDelegate, Rate);
		}
		else
		{
			ProcessReservationResponse(ReservationResponse);
		}
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Party beacon response received %s, ignored due to cancel in progress"), EB3atZPartyReservationResult::ToString(ReservationResponse));
		// Cancel RPC or failsafe timer will trigger the cancel
	}
}

void AB3atZPartyBeaconClient::ProcessReservationResponse(EB3atZPartyReservationResult::Type ReservationResponse)
{
	if (!bCancelReservation)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Party beacon response received %s"), EB3atZPartyReservationResult::ToString(ReservationResponse));
		ReservationRequestComplete.ExecuteIfBound(ReservationResponse);
		RequestType = EDirectClientRequestType::NonePending;
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Party beacon response received %s, ignored due to cancel in progress"), EB3atZPartyReservationResult::ToString(ReservationResponse));
		// Cancel RPC or failsafe timer will trigger the cancel
	}
}

void AB3atZPartyBeaconClient::ClientCancelReservationResponse_Implementation(EB3atZPartyReservationResult::Type ReservationResponse)
{
	ensure(bCancelReservation);

	// Clear out any pending response handling (including failsafe timer)
	ClearTimers();
#if !UE_BUILD_SHIPPING	
	const float Rate = BeaconConsoleVariables::CVarDelayCancellationResponse.GetValueOnGameThread();
#else
	const float Rate = 0.0f;
#endif
	if (Rate > 0.0f)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Party beacon cancellation response received %s, waiting %fs to notify"), EB3atZPartyReservationResult::ToString(ReservationResponse), Rate);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([this, ReservationResponse]()
		{
			ProcessCancelReservationResponse(ReservationResponse);
		});

		PendingCancelResponseTimerHandle = DelayResponse(TimerDelegate, Rate);
	}
	else
	{
		ProcessCancelReservationResponse(ReservationResponse);
	}
}

void AB3atZPartyBeaconClient::ProcessCancelReservationResponse(EB3atZPartyReservationResult::Type ReservationResponse)
{
	ensure(ReservationResponse == EB3atZPartyReservationResult::ReservationRequestCanceled || ReservationResponse == EB3atZPartyReservationResult::ReservationNotFound);
	ensure(bCancelReservation);
	OnCancelledComplete();
}

void AB3atZPartyBeaconClient::ClientSendReservationUpdates_Implementation(int32 NumRemainingReservations)
{
	if (!bCancelReservation)
	{
#if !UE_BUILD_SHIPPING
		const float Rate = BeaconConsoleVariables::CVarDelayUpdateResponse.GetValueOnGameThread();
#else
		const float Rate = 0.0f;
#endif
		if (Rate > 0.0f)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Party beacon reservations remaining %d, waiting %fs to notify"), NumRemainingReservations, Rate);

			FTimerDelegate TimerDelegate;
			TimerDelegate.BindLambda([this, NumRemainingReservations]()
			{
				ProcessReservationUpdate(NumRemainingReservations);
			});

			PendingReservationUpdateTimerHandle = DelayResponse(TimerDelegate, Rate);
		}
		else
		{
			ProcessReservationUpdate(NumRemainingReservations);
		}
	}
}

void AB3atZPartyBeaconClient::ProcessReservationUpdate(int32 NumRemainingReservations)
{
	UE_LOG(LogBeacon, Verbose, TEXT("Party beacon reservations remaining %d"), NumRemainingReservations);
	ReservationCountUpdate.ExecuteIfBound(NumRemainingReservations);
}

void AB3atZPartyBeaconClient::ClientSendReservationFull_Implementation()
{
	if (!bCancelReservation)
	{
#if !UE_BUILD_SHIPPING
		const float Rate = BeaconConsoleVariables::CVarDelayFullResponse.GetValueOnGameThread();
#else
		const float Rate = 0.0f;
#endif
		if (Rate > 0.0f)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Party beacon reservations full, waiting %fs to notify"), Rate);

			FTimerDelegate TimerDelegate;
			TimerDelegate.BindLambda([this]()
			{
				ProcessReservationFull();
			});

			PendingReservationFullTimerHandle = DelayResponse(TimerDelegate, Rate);
		}
		else
		{
			ProcessReservationFull();
		}
	}
}

void AB3atZPartyBeaconClient::ProcessReservationFull()
{
	UE_LOG(LogBeacon, Verbose, TEXT("Party beacon reservations full"));
	ReservationFull.ExecuteIfBound();
}

FTimerHandle AB3atZPartyBeaconClient::DelayResponse(FTimerDelegate& Delegate, float Delay)
{
	FTimerHandle TimerHandle;

	UWorld* World = GetWorld();
	if (ensure(World != nullptr))
	{
		World->GetTimerManager().SetTimer(TimerHandle, Delegate, Delay, false);
	}

	return TimerHandle;
}
