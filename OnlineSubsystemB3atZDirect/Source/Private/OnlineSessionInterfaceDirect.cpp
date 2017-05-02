// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "OnlineSessionInterfaceDirect.h"
#include "Misc/Guid.h"
#include "OnlineSubsystemB3atZ.h"
#include "OnlineSubsystemB3atZUtils.h"
#include "OnlineSubsystemB3atZDirect.h"
#include "OnlineSubsystemB3atZDirectTypes.h"
#include "OnlineAsyncTaskManager.h"
#include "SocketSubsystem.h"
#include "NboSerializerDirect.h"
#include "Engine/EngineBaseTypes.h"




FOnlineSessionInfoDirect::FOnlineSessionInfoDirect() :
	HostAddr(NULL),
	SessionId(TEXT("INVALID"))
{
}

void FOnlineSessionInfoDirect::Init(const FOnlineSubsystemB3atZDirect& Subsystem)
{
	// Read the IP from the system
	bool bCanBindAll;
	HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

	// The below is a workaround for systems that set hostname to a distinct address from 127.0.0.1 on a loopback interface.
	// See e.g. https://www.debian.org/doc/manuals/debian-reference/ch05.en.html#_the_hostname_resolution
	// and http://serverfault.com/questions/363095/why-does-my-hostname-appear-with-the-address-127-0-1-1-rather-than-127-0-0-1-in
	// Since we bind to 0.0.0.0, we won't answer on 127.0.1.1, so we need to advertise ourselves as 127.0.0.1 for any other loopback address we may have.
	uint32 HostIp = 0;
	HostAddr->GetIp(HostIp); // will return in host order
	// if this address is on loopback interface, advertise it as 127.0.0.1
	if ((HostIp & 0xff000000) == 0x7f000000)
	{
		HostAddr->SetIp(0x7f000001);	// 127.0.0.1
	}

	
	// Now set the port that was configured
	HostAddr->SetPort(GetPortFromNetDriver(Subsystem.GetInstanceName()));

	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect Init HostAddr is %s "), *HostAddr->ToString(true));

	FGuid OwnerGuid;
	FPlatformMisc::CreateGuid(OwnerGuid);
	SessionId = FB3atZUniqueNetIdString(OwnerGuid.ToString());
}

/**
 *	Async task for ending a Direct online session
 */
class FOnlineAsyncTaskDirectEndSession : public FOnlineAsyncTaskBasic<FOnlineSubsystemB3atZDirect>
{
private:
	/** Name of session ending */
	FName SessionName;

public:
	FOnlineAsyncTaskDirectEndSession(class FOnlineSubsystemB3atZDirect* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskBasic(InSubsystem),
		SessionName(InSessionName)
	{
	}

	~FOnlineAsyncTaskDirectEndSession()
	{
	}

	/**
	 *	Get a human readable description of task
	 */
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FOnlineAsyncTaskDirectEndSession bWasSuccessful: %d SessionName: %s"), bWasSuccessful, *SessionName.ToString());
	}

	/**
	 * Give the async task time to do its work
	 * Can only be called on the async task manager thread
	 */
	virtual void Tick() override
	{
		bIsComplete = true;
		bWasSuccessful = true;
	}

	/**
	 * Give the async task a chance to marshal its data back to the game thread
	 * Can only be called on the game thread by the async task manager
	 */
	virtual void Finalize() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
		if (Session)
		{
			Session->SessionState = EB3atZOnlineSessionState::Ended;
		}
	}

	/**
	 *	Async task is given a chance to trigger it's delegates
	 */
	virtual void TriggerDelegates() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			SessionInt->TriggerOnEndSessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}
};

/**
 *	Async task for destroying a Direct online session
 */
class FOnlineAsyncTaskDirectDestroySession : public FOnlineAsyncTaskBasic<FOnlineSubsystemB3atZDirect>
{
private:
	/** Name of session ending */
	FName SessionName;

public:
	FOnlineAsyncTaskDirectDestroySession(class FOnlineSubsystemB3atZDirect* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskBasic(InSubsystem),
		SessionName(InSessionName)
	{
	}

	~FOnlineAsyncTaskDirectDestroySession()
	{
	}

	/**
	 *	Get a human readable description of task
	 */
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FOnlineAsyncTaskDirectDestroySession bWasSuccessful: %d SessionName: %s"), bWasSuccessful, *SessionName.ToString());
	}

	/**
	 * Give the async task time to do its work
	 * Can only be called on the async task manager thread
	 */
	virtual void Tick() override
	{
		bIsComplete = true;
		bWasSuccessful = true;
	}

	/**
	 * Give the async task a chance to marshal its data back to the game thread
	 * Can only be called on the game thread by the async task manager
	 */
	virtual void Finalize() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
			if (Session)
			{
				SessionInt->RemoveNamedSession(SessionName);
			}
		}
	}

	/**
	 *	Async task is given a chance to trigger it's delegates
	 */
	virtual void TriggerDelegates() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			SessionInt->TriggerOnDestroySessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}
};

bool FOnlineSessionDirect::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	uint32 Result = E_FAIL;

	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID CreateSession"));

	// Check for an existing session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session == NULL)
	{
		// Create a new session and deep copy the game settings
		Session = AddNamedSession(SessionName, NewSessionSettings);
		check(Session);
		Session->SessionState = EB3atZOnlineSessionState::Creating;
		Session->NumOpenPrivateConnections = NewSessionSettings.NumPrivateConnections;
		Session->NumOpenPublicConnections = NewSessionSettings.NumPublicConnections;	// always start with full public connections, local player will register later

		Session->HostingPlayerNum = HostingPlayerNum;

		check(DirectSubsystem);
		IOnlineIdentityPtr Identity = DirectSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->OwningUserId = Identity->GetUniquePlayerId(HostingPlayerNum);
			Session->OwningUserName = Identity->GetPlayerNickname(HostingPlayerNum);
		}

		// if did not get a valid one, use just something
		if (!Session->OwningUserId.IsValid())
		{
			Session->OwningUserId = MakeShareable(new FB3atZUniqueNetIdString(FString::Printf(TEXT("%d"), HostingPlayerNum)));
			Session->OwningUserName = FString(TEXT("DirectUser"));
		}
		
		// Unique identifier of this build for compatibility
		Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

		// Setup the host session info
		FOnlineSessionInfoDirect* NewSessionInfo = new FOnlineSessionInfoDirect();
		NewSessionInfo->Init(*DirectSubsystem);
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		Result = UpdateLANStatus();

		if (Result != ERROR_IO_PENDING)
		{
			// Set the game state as pending (not started)
			Session->SessionState = EB3atZOnlineSessionState::Pending;

			if (Result != ERROR_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Cannot create session '%s': session already exists."), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		TriggerOnCreateSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}
	
	return Result == ERROR_IO_PENDING || Result == ERROR_SUCCESS;
}

bool FOnlineSessionDirect::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	// todo: use proper	HostingPlayerId
	return CreateSession(0, SessionName, NewSessionSettings);
}

bool FOnlineSessionDirect::NeedsToAdvertise()
{
	FScopeLock ScopeLock(&SessionLock);

	bool bResult = false;
	for (int32 SessionIdx=0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		FNamedOnlineSession& Session = Sessions[SessionIdx];
		if (NeedsToAdvertise(Session))
		{
			bResult = true;
			break;
		}
	}

	return bResult;
}

bool FOnlineSessionDirect::NeedsToAdvertise( FNamedOnlineSession& Session )
{
	// In Direct, we have to imitate missing online service functionality, so we advertise:
	// a) LAN match with open public connections (same as usually)
	// b) Not started public LAN session (same as usually)
	// d) Joinable presence-enabled session that would be advertised with in an online service
	// (all of that only if we're server)
	return Session.SessionSettings.bShouldAdvertise && IsHost(Session) &&
		(
			(
			  !Session.SessionSettings.bIsLANMatch && 			  
			  (Session.SessionState != EB3atZOnlineSessionState::InProgress || (Session.SessionSettings.bAllowJoinInProgress && Session.NumOpenPublicConnections > 0))
			) 
			||
			(
				Session.SessionSettings.bAllowJoinViaPresence || Session.SessionSettings.bAllowJoinViaPresenceFriendsOnly
			)
		);		
}

uint32 FOnlineSessionDirect::UpdateLANStatus()
{
	uint32 Result = ERROR_SUCCESS;

	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID UpdateLANStatus"));

	if ( NeedsToAdvertise() )
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID UpdateLANStatus advertisment needed"));
		// set up  session
		if (B3atZSessionManager.GetBeaconState() == EB3atZBeaconState::NotUsingB3atZBeacon)
		{
			//Use the Port for the Host's netdriver from cfg file and check above ports for valid listen port
			B3atZSessionManager.IsLANMatch = false;
			FURL DefaultURL;
			DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);

			UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID UpdateLANStatus Default URL Port is %u"), DefaultURL.Port);
			HostSessionPort = DefaultURL.Port + 1;
			FOnValidQueryPacketDelegate QueryPacketDelegate = FOnValidQueryPacketDelegate::CreateRaw(this, &FOnlineSessionDirect::OnValidQueryPacketReceived);
			FOnPortChangedDelegate PortChangedDelegate = FOnPortChangedDelegate::CreateRaw(this, &FOnlineSessionDirect::OnSessionListenPortChanged);
			if (!B3atZSessionManager.Host(QueryPacketDelegate, HostSessionPort))
			{
				Result = E_FAIL;

				B3atZSessionManager.StopB3atZSession();
			}
		}
	}
	else
	{
		if (B3atZSessionManager.GetBeaconState() != EB3atZBeaconState::Searching)
		{
			
			if ((Sessions.Num() > 0) && Sessions[0].SessionSettings.bIsLANMatch)
			{
				B3atZSessionManager.IsLANMatch = true;

				FOnValidQueryPacketDelegate QueryPacketDelegate = FOnValidQueryPacketDelegate::CreateRaw(this, &FOnlineSessionDirect::OnValidQueryPacketReceived);
				FOnPortChangedDelegate PortChangedDelegate = FOnPortChangedDelegate::CreateRaw(this, &FOnlineSessionDirect::OnSessionListenPortChanged);
				//TODO: if its a LAN Connection just send port 1 for now, maybe change this...
				int32 TempPort = -1;
				if (!B3atZSessionManager.Host(QueryPacketDelegate, TempPort))
				{
					Result = E_FAIL;

					B3atZSessionManager.StopB3atZSession();
				}

			}
			//TODO: Check if this can really be left out
			//else
			//{
			//	Result = E_FAIL;
			//	// Tear down the LAN beacon, we are not searching, nor building a session on lan/public
			//	LANSessionManager.StopLANSession();
			//}
		}
	}

	return Result;
}

void FOnlineSessionDirect::OnSessionListenPortChanged(int32 Port)
{
	HostSessionPort = Port;
}

bool FOnlineSessionDirect::StartSession(FName SessionName)
{
	uint32 Result = E_FAIL;
	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't start a match multiple times
		if (Session->SessionState == EB3atZOnlineSessionState::Pending ||
			Session->SessionState == EB3atZOnlineSessionState::Ended)
		{
			// If this lan match has join in progress disabled, shut down the beacon
			Result = UpdateLANStatus();
			Session->SessionState = EB3atZOnlineSessionState::InProgress;
		}
		else
		{
			UE_LOG_ONLINEB3ATZ(Verbose,	TEXT("Can't start an online session (%s) in state %s"),
				*SessionName.ToString(),
				EB3atZOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Can't start an online game for session (%s) that hasn't been created"), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		// Just trigger the delegate
		TriggerOnStartSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

int32 FOnlineSessionDirect::GetPort()
{
	return HostSessionPort;
}

bool FOnlineSessionDirect::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData)
{
	bool bWasSuccessful = true;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// @TODO ONLINE update LAN settings
		Session->SessionSettings = UpdatedSessionSettings;
		TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);
	}

	return bWasSuccessful;
}

bool FOnlineSessionDirect::EndSession(FName SessionName)
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID EndSession"));

	uint32 Result = E_FAIL;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't end a match that isn't in progress
		if (Session->SessionState == EB3atZOnlineSessionState::InProgress)
		{
			Session->SessionState = EB3atZOnlineSessionState::Ended;

			// If the session should be advertised and the lan beacon was destroyed, recreate
			Result = UpdateLANStatus();
		}
		else
		{
			UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Can't end session (%s) in state %s"),
				*SessionName.ToString(),
				EB3atZOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Can't end an online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		if (Session)
		{
			Session->SessionState = EB3atZOnlineSessionState::Ended;
		}

		TriggerOnEndSessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionDirect::DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID Destroy Session"));

	uint32 Result = E_FAIL;
	// Find the session in question
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// The session info is no longer needed
		RemoveNamedSession(Session->SessionName);

		Result = UpdateLANStatus();
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Can't destroy a null online session (%s)"), *SessionName.ToString());
	}

	if (Result != ERROR_IO_PENDING)
	{
		CompletionDelegate.ExecuteIfBound(SessionName, (Result == ERROR_SUCCESS) ? true : false);
		TriggerOnDestroySessionCompleteDelegates(SessionName, (Result == ERROR_SUCCESS) ? true : false);
	}

	return Result == ERROR_SUCCESS || Result == ERROR_IO_PENDING;
}

bool FOnlineSessionDirect::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionDirect::StartMatchmaking(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearchB3atZ>& SearchSettings)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("StartMatchmaking is not supported on this platform. Use FindSessions or FindSessionById."));
	TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionDirect::CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionDirect::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionDirect::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearchB3atZ>& SearchSettings)
{
	uint32 Return = E_FAIL;

	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EB3atZOnlineAsyncTaskState::InProgress)
	{
		// Free up previous results
		SearchSettings->SearchResults.Empty();

		// Copy the search pointer so we can keep it around
		CurrentSessionSearch = SearchSettings;

		// remember the time at which we started search, as this will be used for a "good enough" ping estimation
		SessionSearchStartInSeconds = FPlatformTime::Seconds();

		if (!SearchSettings->bIsLanQuery)
		{
			B3atZSessionManager.IsLANMatch = false;

			B3atZSessionManager.HostSessionAddr = CurrentSessionSearch->HostSessionAddr;
			B3atZSessionManager.HostSessionPort = CurrentSessionSearch->HostSessionPort;

			FURL DefaultURL;
			DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);

			B3atZSessionManager.ClientSessionPort = DefaultURL.Port + 2;
		}
		else
		{
			B3atZSessionManager.IsLANMatch = true;
		}

		// Check if its a LAN query
		Return = FindLANSession();

		if (Return == ERROR_IO_PENDING)
		{
			SearchSettings->SearchState = EB3atZOnlineAsyncTaskState::InProgress;
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Ignoring game search request while one is pending"));
		Return = ERROR_IO_PENDING;
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionDirect::FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearchB3atZ>& SearchSettings)
{
	// This function doesn't use the SearchingPlayerNum parameter, so passing in anything is fine.
	return FindSessions(0, SearchSettings);
}

bool FOnlineSessionDirect::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegates)
{
	FOnlineSessionSearchResult EmptyResult;
	CompletionDelegates.ExecuteIfBound(0, false, EmptyResult);
	return true;
}

uint32 FOnlineSessionDirect::FindLANSession()
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID FindLANSession"));

	uint32 Return = ERROR_IO_PENDING;

	// Recreate the unique identifier for this client
	GenerateNonceB3atZ((uint8*)&B3atZSessionManager.B3atZNonce, 8);

	FOnValidResponsePacketDelegate ResponseDelegate = FOnValidResponsePacketDelegate::CreateRaw(this, &FOnlineSessionDirect::OnValidResponsePacketReceived);
	FOnSearchingTimeoutDelegate TimeoutDelegate = FOnSearchingTimeoutDelegate::CreateRaw(this, &FOnlineSessionDirect::OnLANSearchTimeout);

	FNboSerializeToBufferDirect Packet(LAN_BEACON_MAX_PACKET_SIZE);
	B3atZSessionManager.CreateClientQueryPacket(Packet, B3atZSessionManager.B3atZNonce);
	if (B3atZSessionManager.Search(Packet, ResponseDelegate, TimeoutDelegate) == false)
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID FindLANSession Search in LANSessionManager failed"));

		Return = E_FAIL;

		FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EB3atZOnlineAsyncTaskState::Failed;
		
		// Just trigger the delegate as having failed
		TriggerOnFindSessionsCompleteDelegates(false);
	}
	return Return;
}

bool FOnlineSessionDirect::CancelFindSessions()
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID CancelFindSessions"));

	uint32 Return = E_FAIL;
	if (CurrentSessionSearch.IsValid() && CurrentSessionSearch->SearchState == EB3atZOnlineAsyncTaskState::InProgress)
	{
		// Make sure it's the right type
		Return = ERROR_SUCCESS;

		FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EB3atZOnlineAsyncTaskState::Failed;
		CurrentSessionSearch = NULL;
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Can't cancel a search that isn't in progress"));
	}

	if (Return != ERROR_IO_PENDING)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(true);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionDirect::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect JoinSession"));

	uint32 Return = E_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	// Don't join a session if already in one or hosting one
	if (Session == NULL)
	{
		// Create a named session from the search result data
		Session = AddNamedSession(SessionName, DesiredSession.Session);
		Session->HostingPlayerNum = PlayerNum;

		// Create Internet or LAN match
		FOnlineSessionInfoDirect* NewSessionInfo = new FOnlineSessionInfoDirect();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		Return = JoinLANSession(PlayerNum, Session, &DesiredSession.Session);

		// turn off advertising on Join, to avoid clients advertising it over LAN
		Session->SessionSettings.bShouldAdvertise = false;

		if (Return != ERROR_IO_PENDING)
		{
			if (Return != ERROR_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Session (%s) already exists, can't join twice"), *SessionName.ToString());
	}

	if (Return != ERROR_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ERROR_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ERROR_SUCCESS || Return == ERROR_IO_PENDING;
}

bool FOnlineSessionDirect::JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	// Assuming player 0 should be OK here
	return JoinSession(0, SessionName, DesiredSession);
}

bool FOnlineSessionDirect::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Direct subsystem
	FOnlineSessionSearchResult EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, EmptySearchResult);
	return false;
};

bool FOnlineSessionDirect::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Direct subsystem
	FOnlineSessionSearchResult EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	return false;
}

bool FOnlineSessionDirect::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Direct subsystem
	return false;
};

bool FOnlineSessionDirect::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Direct subsystem
	return false;
}

bool FOnlineSessionDirect::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Direct subsystem
	return false;
};

bool FOnlineSessionDirect::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in Direct subsystem
	return false;
}

uint32 FOnlineSessionDirect::JoinLANSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession)
{
	check(Session != nullptr);

	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect JoinLANSession"));

	uint32 Result = E_FAIL;
	Session->SessionState = EB3atZOnlineSessionState::Pending;

	if (Session->SessionInfo.IsValid() && SearchSession != nullptr && SearchSession->SessionInfo.IsValid())
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect JoinLANSession SessionInfo is valid"));

		// Copy the session info over
		const FOnlineSessionInfoDirect* SearchSessionInfo = (const FOnlineSessionInfoDirect*)SearchSession->SessionInfo.Get();
		FOnlineSessionInfoDirect* SessionInfo = (FOnlineSessionInfoDirect*)Session->SessionInfo.Get();
		SessionInfo->SessionId = SearchSessionInfo->SessionId;

		if (!B3atZSessionManager.IsLANMatch)
		{
			if (B3atZSessionManager.HostSessionAddr)
			{
				uint32 IpAddr = B3atZSessionManager.HostSessionAddr;

				SessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(IpAddr, SearchSessionInfo->HostAddr->GetPort());

				UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect JoinLANSession Online Session to join HostAdrr is %s "), *SearchSessionInfo->HostAddr->ToString(true));

				Result = ERROR_SUCCESS;

			}
		}
		else
		{
			uint32 IpAddr;
			SearchSessionInfo->HostAddr->GetIp(IpAddr);
			SessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(IpAddr, SearchSessionInfo->HostAddr->GetPort());

			UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect JoinLANSession Session to join HostAdrr is %s "), *SearchSessionInfo->HostAddr->ToString(true));

			Result = ERROR_SUCCESS;
		}

		
	}

	return Result;
}

bool FOnlineSessionDirect::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	return false;
}

/** Get a resolved connection string from a session info */
static bool GetConnectStringFromSessionInfo(TSharedPtr<FOnlineSessionInfoDirect>& SessionInfo, FString& ConnectInfo, int32 PortOverride=0)
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSD GetConnectStringFromSessionInfo"));

	bool bSuccess = false;
	if (SessionInfo.IsValid())
	{
		if (SessionInfo->HostAddr.IsValid() && SessionInfo->HostAddr->IsValid())
		{
			if (PortOverride != 0)
			{
				ConnectInfo = FString::Printf(TEXT("%s:%d"), *SessionInfo->HostAddr->ToString(false), PortOverride);
			}
			else
			{
				ConnectInfo = FString::Printf(TEXT("%s"), *SessionInfo->HostAddr->ToString(true));
			}

			bSuccess = true;
		}
	}

	return bSuccess;
}

bool FOnlineSessionDirect::GetResolvedConnectString(FName SessionName, FString& ConnectInfo)
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSD GetResolvedConnectString 1"));

	bool bSuccess = false;
	// Find the session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session != NULL)
	{
		TSharedPtr<FOnlineSessionInfoDirect> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoDirect>(Session->SessionInfo);
		bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		if (!bSuccess)
		{
			UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Invalid session info for session %s in GetResolvedConnectString()"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose,
			TEXT("Unknown session name (%s) specified to GetResolvedConnectString()"),
			*SessionName.ToString());
	}

	return bSuccess;
}

bool FOnlineSessionDirect::GetResolvedConnectString(const FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSD GetResolvedConnectString 2"));

	bool bSuccess = false;
	if (SearchResult.Session.SessionInfo.IsValid())
	{
		TSharedPtr<FOnlineSessionInfoDirect> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoDirect>(SearchResult.Session.SessionInfo);

		if (PortType == BeaconPort)
		{
			int32 BeaconListenPort = DEFAULT_BEACON_PORT;
			if (!SearchResult.Session.SessionSettings.Get(SETTING_BEACONPORT, BeaconListenPort) || BeaconListenPort <= 0)
			{
				// Reset the default BeaconListenPort back to DEFAULT_BEACON_PORT because the SessionSettings value does not exist or was not valid
				BeaconListenPort = DEFAULT_BEACON_PORT;
			}
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);

		}
		else if (PortType == GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}
	}
	
	if (!bSuccess || ConnectInfo.IsEmpty())
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Invalid session info in search result to GetResolvedConnectString()"));
	}

	return bSuccess;
}

FOnlineSessionSettings* FOnlineSessionDirect::GetSessionSettings(FName SessionName) 
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		return &Session->SessionSettings;
	}
	return NULL;
}

void FOnlineSessionDirect::RegisterLocalPlayers(FNamedOnlineSession* Session)
{
	if (!DirectSubsystem->IsDedicated())
	{
		IOnlineB3atZVoicePtr VoiceInt = DirectSubsystem->GetB3atZVoiceInterface();
		if (VoiceInt.IsValid())
		{
			for (int32 Index = 0; Index < MAX_LOCAL_PLAYERS; Index++)
			{
				//Register the local player as a local talker
				VoiceInt->RegisterLocalTalker(Index);
			}
		}
	}
}

void FOnlineSessionDirect::RegisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineB3atZVoicePtr VoiceInt = DirectSubsystem->GetB3atZVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!DirectSubsystem->IsLocalPlayer(PlayerId))
		{
			VoiceInt->RegisterRemoteTalker(PlayerId);
		}
		else
		{
			// This is a local player. In case their PlayerState came last during replication, reprocess muting
			VoiceInt->ProcessMuteChangeNotification();
		}
	}
}

void FOnlineSessionDirect::UnregisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineB3atZVoicePtr VoiceInt = DirectSubsystem->GetB3atZVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!DirectSubsystem->IsLocalPlayer(PlayerId))
		{
			if (VoiceInt.IsValid())
			{
				VoiceInt->UnregisterRemoteTalker(PlayerId);
			}
		}
	}
}

bool FOnlineSessionDirect::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	TArray< TSharedRef<const FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FB3atZUniqueNetIdString(PlayerId)));
	return RegisterPlayers(SessionName, Players, bWasInvited);
}

bool FOnlineSessionDirect::RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited)
{
	bool bSuccess = false;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		bSuccess = true;

		for (int32 PlayerIdx=0; PlayerIdx<Players.Num(); PlayerIdx++)
		{
			const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

			FB3atZUniqueNetIdMatcher PlayerMatch(*PlayerId);
			if (Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) == INDEX_NONE)
			{
				Session->RegisteredPlayers.Add(PlayerId);
				RegisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections > 0)
				{
					Session->NumOpenPublicConnections--;
				}
				else if (Session->NumOpenPrivateConnections > 0)
				{
					Session->NumOpenPrivateConnections--;
				}
			}
			else
			{
				RegisterVoice(*PlayerId);
				UE_LOG_ONLINEB3ATZ(Log, TEXT("Player %s already registered in session %s"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}			
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

bool FOnlineSessionDirect::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	TArray< TSharedRef<const FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FB3atZUniqueNetIdString(PlayerId)));
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionDirect::UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players)
{
	bool bSuccess = true;

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		for (int32 PlayerIdx=0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

			FB3atZUniqueNetIdMatcher PlayerMatch(*PlayerId);
			int32 RegistrantIndex = Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch);
			if (RegistrantIndex != INDEX_NONE)
			{
				Session->RegisteredPlayers.RemoveAtSwap(RegistrantIndex);
				UnregisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections < Session->SessionSettings.NumPublicConnections)
				{
					Session->NumOpenPublicConnections++;
				}
				else if (Session->NumOpenPrivateConnections < Session->SessionSettings.NumPrivateConnections)
				{
					Session->NumOpenPrivateConnections++;
				}
			}
			else
			{
				UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}
		}
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("No game present to leave for session (%s)"), *SessionName.ToString());
		bSuccess = false;
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionDirect::Tick(float DeltaTime)
{
#if WITH_EDITOR

#else
	SCOPE_CYCLE_COUNTER(STAT_B3atZSession_Interface);
	TickLanTasks(DeltaTime);
#endif
}

void FOnlineSessionDirect::TickLanTasks(float DeltaTime)
{
	B3atZSessionManager.Tick(DeltaTime);
}

void FOnlineSessionDirect::AppendSessionToPacket(FNboSerializeToBufferDirect& Packet, FOnlineSession* Session)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect AppendSessionToPacket"));

	/** Owner of the session */
	Packet << *StaticCastSharedPtr<const FB3atZUniqueNetIdString>(Session->OwningUserId)
		<< Session->OwningUserName
		<< Session->NumOpenPrivateConnections
		<< Session->NumOpenPublicConnections;

	// Try to get the actual port the netdriver is using
	SetPortFromNetDriver(*DirectSubsystem, Session->SessionInfo);

	// Write host info (host addr, session id, and key)
	Packet << *StaticCastSharedPtr<FOnlineSessionInfoDirect>(Session->SessionInfo);

	// Now append per game settings
	AppendSessionSettingsToPacket(Packet, &Session->SessionSettings);
}

void FOnlineSessionDirect::AppendSessionSettingsToPacket(FNboSerializeToBufferDirect& Packet, FOnlineSessionSettings* SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Sending session settings to client"));
#endif 

	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect AppendSessionSettingsToPacket"));

	// Members of the session settings class
	Packet << SessionSettings->NumPublicConnections
		<< SessionSettings->NumPrivateConnections
		<< (uint8)SessionSettings->bShouldAdvertise
		<< (uint8)SessionSettings->bIsLANMatch
		<< (uint8)SessionSettings->bIsDedicated
		<< (uint8)SessionSettings->bUsesStats
		<< (uint8)SessionSettings->bAllowJoinInProgress
		<< (uint8)SessionSettings->bAllowInvites
		<< (uint8)SessionSettings->bUsesPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresenceFriendsOnly
		<< (uint8)SessionSettings->bAntiCheatProtected
	    << SessionSettings->BuildUniqueId;

	// First count number of advertised keys
	int32 NumAdvertisedProperties = 0;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{	
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EB3atZOnlineDataAdvertisementType::ViaOnlineService)
		{
			NumAdvertisedProperties++;
		}
	}

	// Add count of advertised keys and the data
	Packet << (int32)NumAdvertisedProperties;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EB3atZOnlineDataAdvertisementType::ViaOnlineService)
		{
			Packet << It.Key();
			Packet << Setting;
#if DEBUG_LAN_BEACON
			UE_LOG_ONLINEB3ATZ(Verbose, TEXT("%s"), *Setting.ToString());
#endif
		}
	}
}

void FOnlineSessionDirect::OnValidQueryPacketReceived(uint8* PacketData, int32 PacketLength, uint64 ClientNonce)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OSID OnValidQueryPacketReceived"));


	// Iterate through all registered sessions and respond for each one that can be joinable
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SessionIndex = 0; SessionIndex < Sessions.Num(); SessionIndex++)
	{
		UE_LOG(LogB3atZOnline, Verbose, TEXT("OSID OnValidQueryPacketReceived Session is Registered"));

		FNamedOnlineSession* Session = &Sessions[SessionIndex];
							
		// Don't respond to query if the session is not a joinable LAN match.
		if (Session)
		{
			UE_LOG(LogB3atZOnline, Verbose, TEXT("OSID OnValidQueryPacketReceived Session valid trough Session Index"));

			const FOnlineSessionSettings& Settings = Session->SessionSettings;

			const bool bIsMatchInProgress = Session->SessionState == EB3atZOnlineSessionState::InProgress;

			const bool bIsMatchJoinable = /*Settings.bIsLANMatch &&*/
				(!bIsMatchInProgress || Settings.bAllowJoinInProgress) &&
				Settings.NumPublicConnections > 0;

			if (bIsMatchJoinable)
			{
				UE_LOG(LogB3atZOnline, Verbose, TEXT("OSID OnValidQueryPacketReceived Match is joinabale"));

				FNboSerializeToBufferDirect Packet(LAN_BEACON_MAX_PACKET_SIZE);
				// Create the basic header before appending additional information
				B3atZSessionManager.CreateHostResponsePacket(Packet, ClientNonce);

				// Add all the session details
				AppendSessionToPacket(Packet, Session);

				// Broadcast this response so the client can see us
				if (!Packet.HasOverflow())
				{	
					if (!B3atZSessionManager.IsLANMatch)
					{
						B3atZSessionManager.BroadcastPacketFromSocket(Packet, Packet.GetByteCount());
					}
					else
					{
						B3atZSessionManager.BroadcastPacket(Packet, Packet.GetByteCount());
					}

				}
				else
				{
					UE_LOG_ONLINEB3ATZ(Verbose, TEXT("LAN broadcast packet overflow, cannot broadcast on LAN"));
				}
			}
		}
	}
}

void FOnlineSessionDirect::ReadSessionFromPacket(FNboSerializeFromBufferDirect& Packet, FOnlineSession* Session)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect ReadSessionFromPacket"));

#if DEBUG_LAN_BEACON
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Reading session information from server"));
#endif

	/** Owner of the session */
	FB3atZUniqueNetIdString* UniqueId = new FB3atZUniqueNetIdString;
	Packet >> *UniqueId
		>> Session->OwningUserName
		>> Session->NumOpenPrivateConnections
		>> Session->NumOpenPublicConnections;

	Session->OwningUserId = MakeShareable(UniqueId);

	// Allocate and read the connection data
	FOnlineSessionInfoDirect* DirectSessionInfo = new FOnlineSessionInfoDirect();
	DirectSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Packet >> *DirectSessionInfo;
	Session->SessionInfo = MakeShareable(DirectSessionInfo); 

	// Read any per object data using the server object
	ReadSettingsFromPacket(Packet, Session->SessionSettings);
}

void FOnlineSessionDirect::ReadSettingsFromPacket(FNboSerializeFromBufferDirect& Packet, FOnlineSessionSettings& SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Reading game settings from server"));
#endif

	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect ReadSettingsFromPacket"));

	// Clear out any old settings
	SessionSettings.Settings.Empty();

	// Members of the session settings class
	Packet >> SessionSettings.NumPublicConnections
		>> SessionSettings.NumPrivateConnections;
	uint8 Read = 0;
	// Read all the bools as bytes
	Packet >> Read;
	SessionSettings.bShouldAdvertise = !!Read;
	Packet >> Read;
	SessionSettings.bIsLANMatch = !!Read;
	Packet >> Read;
	SessionSettings.bIsDedicated = !!Read;
	Packet >> Read;
	SessionSettings.bUsesStats = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinInProgress = !!Read;
	Packet >> Read;
	SessionSettings.bAllowInvites = !!Read;
	Packet >> Read;
	SessionSettings.bUsesPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = !!Read;
	Packet >> Read;
	SessionSettings.bAntiCheatProtected = !!Read;

	// BuildId
	Packet >> SessionSettings.BuildUniqueId;

	// Now read the contexts and properties from the settings class
	int32 NumAdvertisedProperties = 0;
	// First, read the number of advertised properties involved, so we can presize the array
	Packet >> NumAdvertisedProperties;
	if (Packet.HasOverflow() == false)
	{
		FName Key;
		// Now read each context individually
		for (int32 Index = 0;
			Index < NumAdvertisedProperties && Packet.HasOverflow() == false;
			Index++)
		{
			FOnlineSessionSetting Setting;
			Packet >> Key;
			Packet >> Setting;
			SessionSettings.Set(Key, Setting);

#if DEBUG_LAN_BEACON
			UE_LOG_ONLINEB3ATZ(Verbose, TEXT("%s"), *Setting->ToString());
#endif
		}
	}
	
	// If there was an overflow, treat the string settings/properties as broken
	if (Packet.HasOverflow())
	{
		SessionSettings.Settings.Empty();
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Packet overflow detected in ReadGameSettingsFromPacket()"));
	}
}

void FOnlineSessionDirect::OnValidResponsePacketReceived(uint8* PacketData, int32 PacketLength)
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSIDirect OnValidResponsePacketReceived"));

	// Create an object that we'll copy the data to
	FOnlineSessionSettings NewServer;
	if (CurrentSessionSearch.IsValid())
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSIDirect OnValidResponsePacketReceived sessions search is valid"));
		// Add space in the search results array
		FOnlineSessionSearchResult* NewResult = new (CurrentSessionSearch->SearchResults) FOnlineSessionSearchResult();
		// this is not a correct ping, but better than nothing
		NewResult->PingInMs = static_cast<int32>((FPlatformTime::Seconds() - SessionSearchStartInSeconds) * 1000);

		FOnlineSession* NewSession = &NewResult->Session;

		// Prepare to read data from the packet
		FNboSerializeFromBufferDirect Packet(PacketData, PacketLength);
		
		ReadSessionFromPacket(Packet, NewSession);

		// NOTE: we don't notify until the timeout happens
	}
	else
	{
		UE_LOG_ONLINEB3ATZ(Verbose, TEXT("Failed to create new online game settings object"));
	}
}

uint32 FOnlineSessionDirect::FinalizeLANSearch()
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID FinalizeLANSearch"));

	if (B3atZSessionManager.GetBeaconState() == EB3atZBeaconState::Searching)
	{
		B3atZSessionManager.StopB3atZSession();
	}

	return UpdateLANStatus();
}

void FOnlineSessionDirect::OnLANSearchTimeout()
{
	UE_LOG_ONLINEB3ATZ(Verbose, TEXT("OSID OnLANSearchTimeout"));

	FinalizeLANSearch();

	if (CurrentSessionSearch.IsValid())
	{
		if (CurrentSessionSearch->SearchResults.Num() > 0)
		{
			// Allow game code to sort the servers
			CurrentSessionSearch->SortSearchResults();
		}
		CurrentSessionSearch->SearchState = EB3atZOnlineAsyncTaskState::Done;

		CurrentSessionSearch = NULL;
	}

	// Trigger the delegate as complete
	TriggerOnFindSessionsCompleteDelegates(true);
}

int32 FOnlineSessionDirect::GetNumSessions()
{
	FScopeLock ScopeLock(&SessionLock);
	return Sessions.Num();
}

void FOnlineSessionDirect::DumpSessionState()
{
	FScopeLock ScopeLock(&SessionLock);

	for (int32 SessionIdx=0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		DumpNamedSession(&Sessions[SessionIdx]);
	}
}

void FOnlineSessionDirect::RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::Success);
}

void FOnlineSessionDirect::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, true);
}

void FOnlineSessionDirect::SetPortFromNetDriver(const FOnlineSubsystemB3atZDirect& Subsystem, const TSharedPtr<FOnlineSessionInfoB3atZ>& SessionInfo)
{
	UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect SetPortFromNetDriver"));

	auto NetDriverPort = GetPortFromNetDriver(Subsystem.GetInstanceName());
	auto SessionInfoDirect = StaticCastSharedPtr<FOnlineSessionInfoDirect>(SessionInfo);
	if (SessionInfoDirect.IsValid() && SessionInfoDirect->HostAddr.IsValid())
	{
		SessionInfoDirect->HostAddr->SetPort(NetDriverPort);
		UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect SetPortFromNetDriver NetDriverPort is %u"), NetDriverPort);
		UE_LOG(LogB3atZOnline, Verbose, TEXT("OnlineSessionInterfaceDirect SetPortFromNetDriver Session HostAddr is %s"), *SessionInfoDirect->HostAddr->ToString(true));
	}
}

bool FOnlineSessionDirect::IsHost(const FNamedOnlineSession& Session) const
{
	if (DirectSubsystem->IsDedicated())
	{
		return true;
	}

	IOnlineIdentityPtr IdentityInt = DirectSubsystem->GetIdentityInterface(); 
	if (!IdentityInt.IsValid())
	{
		return false;
	}

	TSharedPtr<const FUniqueNetId> UserId = IdentityInt->GetUniquePlayerId(Session.HostingPlayerNum);
	return (UserId.IsValid() && (*UserId == *Session.OwningUserId));
}
