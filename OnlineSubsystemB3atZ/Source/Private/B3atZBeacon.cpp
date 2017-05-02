// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "B3atZBeacon.h"
#include "Misc/FeedbackContext.h"
#include "UObject/CoreNet.h"
#include "OnlineSubsystemB3atZ.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "NboSerializer.h"


/** Sets the broadcast address for this object */
FB3atZBeacon::FB3atZBeacon(void) 
	: ListenSocket(NULL),
	  SockAddr(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr())
{
}

/** Frees the broadcast socket */
FB3atZBeacon::~FB3atZBeacon(void)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	SocketSubsystem->DestroySocket(ListenSocket);
}

/** Return true if there is a valid ListenSocket */
bool FB3atZBeacon::IsListenSocketValid() const
{
	return (ListenSocket ? true : false);
}

bool FB3atZBeacon::Init(int32 Port)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Init"))

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	bool bSuccess = false;
	// Set our broadcast address
	BroadcastAddr = SocketSubsystem->CreateInternetAddr();
	BroadcastAddr->SetBroadcastAddress();
	BroadcastAddr->SetPort(Port);
	// Now the listen address
	ListenAddr = SocketSubsystem->GetLocalBindAddr(*GWarn);
	ListenAddr->SetPort(Port);
	// A temporary "received from" address
	SockAddr = SocketSubsystem->CreateInternetAddr();
	// Now create and set up our sockets (no VDP)
	ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("LAN beacon"), true);
	if (ListenSocket != NULL)
	{
		ListenSocket->SetReuseAddr();
		ListenSocket->SetNonBlocking();
		ListenSocket->SetRecvErr();
		// Bind to our listen port
		if (ListenSocket->Bind(*ListenAddr))
		{
			// Set it to broadcast mode, so we can send on it
			// NOTE: You must set this to broadcast mode on Xbox 360 or the
			// secure layer will eat any packets sent
			bSuccess = ListenSocket->SetBroadcast();
		}
		else
		{
			UE_LOG(LogB3atZOnline, Error, TEXT("Failed to bind listen socket to addr (%s) for LAN beacon"),
				*ListenAddr->ToString(true));
		}
	}
	else
	{
		UE_LOG(LogB3atZOnline, Error, TEXT("Failed to create listen socket for LAN beacon"));
	}
	return bSuccess && ListenSocket;
}

/**
 * Initializes the socket
 *
 * @param Port the port to listen on
 *
 * @return true if both socket was created successfully, false otherwise
 */
bool FB3atZBeacon::InitHost(int32 Port)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost"))

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	bool bSuccess = false;
	
	// Now the listen address
	ListenAddr = SocketSubsystem->GetLocalBindAddr(*GLog);
	ListenAddr->SetPort((int)Port);
	
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost Listen Address is %s "), *ListenAddr->ToString(true));

	// A temporary "received from" address
	SockAddr = SocketSubsystem->CreateInternetAddr();
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost SockAddr is %s"), *SockAddr->ToString(true));

	// Now create and set up our sockets (no VDP)
	ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("LAN beacon"), true);
	if (ListenSocket != NULL)
	{
		if (ListenSocket->SetReuseAddr())
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost ReuseAddr true"));
		}

		if (ListenSocket->SetNonBlocking())
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost NonBlocking true"));
		}
		if (ListenSocket->SetRecvErr())
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost SetRecvError true"));
		}
		// Bind to our listen port
		if (ListenSocket->Bind(*ListenAddr))
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitHost Listen Address bound is %s "), *ListenAddr->ToString(true));
			// Set it to broadcast mode, so we can send on it
			// NOTE: You must set this to broadcast mode on Xbox 360 or the
			// secure layer will eat any packets sent
			bSuccess = true;// ListenSocket->SetBroadcast();
		}
		else
		{
			////Try to find another port to bind to
			if(SocketSubsystem->BindNextPort(ListenSocket,*ListenAddr,8000,1) != 0)
			{
				UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("New ListenAddr after Port increment is %s"),
					*ListenAddr->ToString(true));
				bSuccess = true; // ListenSocket->SetBroadcast();

				TriggerOnPortChangedDelegates(ListenAddr->GetPort());

			

			}
			else
			{
				UE_LOG(LogB3atZOnline, Error, TEXT("Failed to bind listen socket to addr (%s) for LAN beacon"),
					*ListenAddr->ToString(true));
			}
		}
	}
	else
	{
		UE_LOG(LogB3atZOnline, Error, TEXT("Failed to create listen socket for LAN beacon"));
	}

	return bSuccess && ListenSocket;
}

bool FB3atZBeacon::InitClient(int32 IP, int32 Port, int32 ListenPort)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient IP from blueprint is %u"), IP);

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	bool bSuccess = false;
	
	// Set IP of Host to Connect To
	BroadcastAddr = SocketSubsystem->CreateInternetAddr(IP, (int)Port);
	BroadcastAddr->SetIp(IP);
	
	BroadcastAddr->SetPort((int)Port);

	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient Broadcast Address is %s "), *BroadcastAddr->ToString(true));

	// Now the listen address peep change object *GWarn to Glog
	ListenAddr = SocketSubsystem->GetLocalBindAddr(*GLog);

	//DefaultObjectWas Port working 63
	ListenAddr->SetPort((int)ListenPort);

	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient Listen Address is %s "), *ListenAddr->ToString(true));

	// A temporary "received from" address
	SockAddr = SocketSubsystem->CreateInternetAddr();

	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient SockAddr is %s"), *SockAddr->ToString(true));

	// Now create and set up our sockets (no VDP)
	ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("LAN beacon"), true);
	if (ListenSocket != NULL)
	{
		if (ListenSocket->SetReuseAddr())
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient ReuseAddr true"));
		}

		if (ListenSocket->SetNonBlocking())
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient NonBlocking true"));
		}
		if (ListenSocket->SetRecvErr())
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient SetRecvError true"));
		}
		//bSuccess = true;
		// Bind to our listen port
		if (ListenSocket->Bind(*ListenAddr))
		{
			
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon InitClient Listen Address after Socket bound is %s "), *ListenAddr->ToString(true));

			// Set it to broadcast mode, so we can send on it
			// NOTE: You must set this to broadcast mode on Xbox 360 or the
			// secure layer will eat any packets sent

			bSuccess = ListenSocket->SetBroadcast();
			//bSuccess = true;
		}
		else
		{
			////Try to find another port to bind to
			if (SocketSubsystem->BindNextPort(ListenSocket, *ListenAddr, 8000, 1) != 0)
			{
				UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("New ListenAddr after Port increment is %s"),
					*ListenAddr->ToString(true));
				bSuccess = true; // ListenSocket->SetBroadcast();

				//ToDo: Make Output in Blueprint node so user knows which ports to open (also in else)
				TriggerOnPortChangedDelegates(ListenAddr->GetPort());



			}
			else
			{
				UE_LOG(LogB3atZOnline, Error, TEXT("Failed to bind listen socket to addr (%s) for LAN beacon"),
					*ListenAddr->ToString(true));
			}
				
		}
	}
	else
	{
		
	}
	return bSuccess && ListenSocket;
}

/**
 * Called to poll the socket for pending data. Any data received is placed
 * in the specified packet buffer
 *
 * @param PacketData the buffer to get the socket's packet data
 * @param BufferSize the size of the packet buffer
 *
 * @return the number of bytes read (<= 0 if none or an error)
 */
int32 FB3atZBeacon::ReceivePacket(uint8* PacketData, int32 BufferSize)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon ReceivedPacket"));
	
	check(PacketData && BufferSize);
	// Default to no data being read
	int32 BytesRead = 0;
	if (ListenSocket != NULL)
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon ReceivedPacket SockAddr before winsock change is %s"), *SockAddr->ToString(true));
		// Read from the socket
		ListenSocket->RecvFrom(PacketData, BufferSize, BytesRead, *SockAddr);
		if (BytesRead > 0)
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon ReceivedPacket BytesRead bigger than 0 from %s"), *SockAddr->ToString(true));
			//UE_LOG(LogB3atZOnline, Verbose, TEXT("Received %d bytes from %s"), BytesRead, *SockAddr->ToString(true));
		}
	}

	return BytesRead;
}

/**
 * Uses the cached broadcast address to send packet to a subnet
 *
 * @param Packet the packet to send
 * @param Length the size of the packet to send
 */
bool FB3atZBeacon::BroadcastPacket(uint8* Packet, int32 Length)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon BroadcastPacket Beacon"));

	int32 BytesSent = 0;
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Sending %d bytes to %s"), Length, *BroadcastAddr->ToString(true) );
	UE_LOG(LogB3atZOnline, Verbose, TEXT("Sending %d bytes to %s"), Length, *BroadcastAddr->ToString(true));

	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Broadcast Listen Address before sending is %s "), *ListenAddr->ToString(true));
 
	return ListenSocket->SendTo(Packet, Length, BytesSent, *BroadcastAddr) && (BytesSent == Length);
}

bool FB3atZBeacon::BroadcastPacketFromSocket(uint8* Packet, int32 Length)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon BroadcastPacket2 Beacon"));

	int32 BytesSent = 16;
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Sending %d bytes to %s"), Length, *SockAddr->ToString(true));
	UE_LOG(LogB3atZOnline, Verbose, TEXT("Sending %d bytes to %s"), Length, *SockAddr->ToString(true));

	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Broadcast2 Listen Address before sending is %s "), *ListenAddr->ToString(true));

	return ListenSocket->SendTo(Packet, Length, BytesSent, *SockAddr) && (BytesSent == Length);

}

/**
* Creates the LAN beacon for queries/advertising servers
*/
bool FB3atZSession::Host(FOnValidQueryPacketDelegate& QueryDelegate, int32 Port)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Host Incoming Port is %u "), Port);

	bool bSuccess = false;
	if (B3atZBeacon != NULL)
	{
		StopB3atZSession();
	}
	
	// Bind a socket for LAN beacon activity
	B3atZBeacon = new FB3atZBeacon();

	//if its LAN Connection
	if (Port == -1)
	{
		if (B3atZBeacon->Init(LanAnnouncePort))
		{

			AddOnValidQueryPacketDelegate_Handle(QueryDelegate);
			// We successfully created everything so mark the socket as needing polling
			B3atZBeaconState = EB3atZBeaconState::Hosting;
			bSuccess = true;
			UE_LOG(LogB3atZOnline, Verbose, TEXT("Listening for LAN beacon requests on %d"), LanAnnouncePort);
		}
		else
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Failed to init LAN beacon %s"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError());
		}
	}
	else
	{
		if (B3atZBeacon->InitHost(Port))
		{

			AddOnValidQueryPacketDelegate_Handle(QueryDelegate);
			// We successfully created everything so mark the socket as needing polling
			B3atZBeaconState = EB3atZBeaconState::Hosting;
			bSuccess = true;
			UE_LOG(LogB3atZOnline, Verbose, TEXT("Listening for Online beacon requests on %u"), Port);
		}
		else
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Failed to init Online beacon %s"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError());
		}
	}
	
	

	return bSuccess;
}

/**
 * Creates the LAN beacon for queries/advertising servers
 *
 * @param Nonce unique identifier for this search
 * @param ResponseDelegate delegate to fire when a server response is received
 */
bool FB3atZSession::Search(FNboSerializeToBuffer& Packet, FOnValidResponsePacketDelegate& ResponseDelegate, FOnSearchingTimeoutDelegate& TimeoutDelegate)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Search"));

	bool bSuccess = true;
	if (B3atZBeacon != NULL)
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Search B3atZBeacon already exists stopping session"));
		StopB3atZSession();
	}

	
	B3atZBeacon = new FB3atZBeacon();
	if (IsLANMatch)
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Search Init B3atZBeacon"))
		// Bind a socket for LAN beacon activity
		if (B3atZBeacon->Init(LanAnnouncePort) == false)
		{
			UE_LOG(LogB3atZOnline, Warning, TEXT("Failed to create socket for lan announce port %s"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError());
			bSuccess = false;
		}
	}
	else
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Search Init OnlineBeacon"));
		// Bind a socket for Online beacon activity
		if (B3atZBeacon->InitClient(HostSessionAddr, HostSessionPort, ClientSessionPort) == false)
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Failed to create socket for lan announce port %s"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError());
			bSuccess = false;
		}
	}

	// If we have a socket and a nonce, broadcast a discovery packet
	if (B3atZBeacon && bSuccess)
	{
		// Now kick off our broadcast which hosts will respond to
		if (B3atZBeacon->BroadcastPacket(Packet, Packet.GetByteCount()))
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Sent query packet..."));
			// We need to poll for the return packets
			B3atZBeaconState = EB3atZBeaconState::Searching;
			// Set the timestamp for timing out a search
			B3atZQueryTimeLeft = B3atZQueryTimeout;

			AddOnValidResponsePacketDelegate_Handle(ResponseDelegate);
			AddOnSearchingTimeoutDelegate_Handle(TimeoutDelegate);
		}
		else
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Failed to send discovery broadcast %s"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError());
			bSuccess = false;
		}
	}
	else
	{
		UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Search B3atZBeacon not valid or bSucces false"));
	}

	return bSuccess;
}

/** Stops the B3atZ beacon from accepting broadcasts */
void FB3atZSession::StopB3atZSession()
{
	// Don't poll anymore since we are shutting it down
	B3atZBeaconState = EB3atZBeaconState::NotUsingB3atZBeacon;

	// Unbind the B3atZ beacon object
	if (B3atZBeacon)
	{	
		delete B3atZBeacon;
		B3atZBeacon = NULL;
	}

	// Clear delegates
	OnValidQueryPacketDelegates.Clear();
	OnValidResponsePacketDelegates.Clear();
	OnSearchingTimeoutDelegates.Clear();
}

void FB3atZSession::Tick(float DeltaTime)
{
	
	if (B3atZBeaconState == EB3atZBeaconState::NotUsingB3atZBeacon)
	{
		return;
	}

	uint8 PacketData[LAN_BEACON_MAX_PACKET_SIZE];
	bool bShouldRead = true;
	// Read each pending packet and pass it out for processing
	while (bShouldRead)
	{
		int32 NumRead = B3atZBeacon->ReceivePacket(PacketData, LAN_BEACON_MAX_PACKET_SIZE);
		if (NumRead > 0)
		{
			// Check our mode to determine the type of allowed packets
			if (B3atZBeaconState == EB3atZBeaconState::Hosting)
			{
				uint64 ClientNonce;
				// We can only accept Server Query packets
				if (IsValidLanQueryPacket(PacketData, NumRead, ClientNonce))
				{
					// Strip off the header
					TriggerOnValidQueryPacketDelegates(&PacketData[LAN_BEACON_PACKET_HEADER_SIZE], NumRead - LAN_BEACON_PACKET_HEADER_SIZE, ClientNonce);
				}
			}
			else if (B3atZBeaconState == EB3atZBeaconState::Searching)
			{
				// We can only accept Server Response packets
				if (IsValidLanResponsePacket(PacketData, NumRead))
				{
					// Strip off the header
					TriggerOnValidResponsePacketDelegates(&PacketData[LAN_BEACON_PACKET_HEADER_SIZE], NumRead - LAN_BEACON_PACKET_HEADER_SIZE);
				}
			}
		}
		else
		{
			if (B3atZBeaconState == EB3atZBeaconState::Searching)
			{
				// Decrement the amount of time remaining
				B3atZQueryTimeLeft -= DeltaTime;
				// Check for a timeout on the search packet
				if (B3atZQueryTimeLeft <= 0.f)
				{
					UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon Tick SearchTimeout"));
					TriggerOnSearchingTimeoutDelegates();
				}
			}
			bShouldRead = false;
		}
	}
}

void FB3atZSession::CreateHostResponsePacket(FNboSerializeToBuffer& Packet, uint64 ClientNonce)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon CreateHostResponsePacket Session"));


	// Add the supported version
	Packet << LAN_BEACON_PACKET_VERSION
		// Platform information
		<< (uint8)FPlatformProperties::IsLittleEndian()
		// Game id to prevent cross game lan packets
		<< LanGameUniqueId
		// Add the packet type
		<< LAN_SERVER_RESPONSE1 << LAN_SERVER_RESPONSE2
		// Append the client nonce as a uint64
		<< ClientNonce;
}

void FB3atZSession::CreateClientQueryPacket(FNboSerializeToBuffer& Packet, uint64 ClientNonce)
{
	// Build the discovery packet
	Packet << LAN_BEACON_PACKET_VERSION
		// Platform information
		<< (uint8)FPlatformProperties::IsLittleEndian()
		// Game id to prevent cross game lan packets
		<< LanGameUniqueId
		// Identify the packet type
		<< LAN_SERVER_QUERY1 << LAN_SERVER_QUERY2
		// Append the nonce as a uint64
		<< ClientNonce;
}

/**
 * Uses the cached broadcast address to send packet to a subnet
 *
 * @param Packet the packet to send
 * @param Length the size of the packet to send
 */
bool FB3atZSession::BroadcastPacket(uint8* Packet, int32 Length)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon BroadcastPacket Session"));


	bool bSuccess = false;
	if (B3atZBeacon)
	{
		bSuccess = B3atZBeacon->BroadcastPacket(Packet, Length);
		if (!bSuccess)
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Failed to send broadcast packet %d"), (int32)ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode());
		}
	}
	

	return bSuccess;
}

bool FB3atZSession::BroadcastPacketFromSocket(uint8* Packet, int32 Length)
{
	UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("B3atZBeacon BroadcastPacket2 Session"));

	bool bSuccess = false;
	if (B3atZBeacon)
	{
		bSuccess = B3atZBeacon->BroadcastPacketFromSocket(Packet, Length);
		if (!bSuccess)
		{
			UE_LOG(LogB3atZOnline, VeryVerbose, TEXT("Failed to send broadcast packet %d"), (int32)ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode());
		}
	}
	
	return bSuccess;
}

/**
 * Determines if the packet header is valid or not
 *
 * @param Packet the packet data to check
 * @param Length the size of the packet buffer
 * @param ClientNonce the client nonce contained within the packet
 *
 * @return true if the header is valid, false otherwise
 */
bool FB3atZSession::IsValidLanQueryPacket(const uint8* Packet, uint32 Length, uint64& ClientNonce)
{
	ClientNonce = 0;
	bool bIsValid = false;
	// Serialize out the data if the packet is the right size
	if (Length == LAN_BEACON_PACKET_HEADER_SIZE)
	{
		FNboSerializeFromBuffer PacketReader(Packet,Length);
		uint8 Version = 0;
		PacketReader >> Version;
		// Do the versions match?
		if (Version == LAN_BEACON_PACKET_VERSION)
		{
			uint8 Platform = 255;
			PacketReader >> Platform;
			// Can we communicate with this platform?
			if (Platform & LanPacketPlatformMask)
			{
				int32 GameId = -1;
				PacketReader >> GameId;
				// Is this our game?
				if (GameId == LanGameUniqueId)
				{
					uint8 SQ1 = 0;
					PacketReader >> SQ1;
					uint8 SQ2 = 0;
					PacketReader >> SQ2;
					// Is this a server query?
					bIsValid = (SQ1 == LAN_SERVER_QUERY1 && SQ2 == LAN_SERVER_QUERY2);
					// Read the client nonce as the outvalue
					PacketReader >> ClientNonce;
				}
			}
		}
	}
	return bIsValid;
}

/**
 * Determines if the packet header is valid or not
 *
 * @param Packet the packet data to check
 * @param Length the size of the packet buffer
 *
 * @return true if the header is valid, false otherwise
 */
bool FB3atZSession::IsValidLanResponsePacket(const uint8* Packet, uint32 Length)
{
	bool bIsValid = false;
	// Serialize out the data if the packet is the right size
	if (Length > LAN_BEACON_PACKET_HEADER_SIZE)
	{
		FNboSerializeFromBuffer PacketReader(Packet,Length);
		uint8 Version = 0;
		PacketReader >> Version;
		// Do the versions match?
		if (Version == LAN_BEACON_PACKET_VERSION)
		{
			uint8 Platform = 255;
			PacketReader >> Platform;
			// Can we communicate with this platform?
			if (Platform & LanPacketPlatformMask)
			{
				int32 GameId = -1;
				PacketReader >> GameId;
				// Is this our game?
				if (GameId == LanGameUniqueId)
				{
					uint8 SQ1 = 0;
					PacketReader >> SQ1;
					uint8 SQ2 = 0;
					PacketReader >> SQ2;
					// Is this a server response?
					if (SQ1 == LAN_SERVER_RESPONSE1 && SQ2 == LAN_SERVER_RESPONSE2)
					{
						uint64 Nonce = 0;
						PacketReader >> Nonce;
						bIsValid = (Nonce == B3atZNonce);
					}
				}
			}
		}
	}
	return bIsValid;
}
