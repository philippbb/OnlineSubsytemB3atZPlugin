// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemB3atZTypes.h"
#include "OnlineDelegateMacros.h"


/**
 * This value indicates which packet version the server is sending. Clients with
 * differing versions will ignore these packets. This prevents crashing when
 * changing the packet format and there are existing servers on the network
 * Current format:
 *
 *	<Ver byte><Platform byte><Game unique 4 bytes><packet type 2 bytes><nonce 8 bytes><payload>
 */
#define LAN_BEACON_PACKET_VERSION (uint8)10

/** The size of the header for validation */
#define LAN_BEACON_PACKET_HEADER_SIZE 16
	
// Offsets for various fields
#define LAN_BEACON_VER_OFFSET 0
#define LAN_BEACON_PLATFORM_OFFSET 1
#define LAN_BEACON_GAMEID_OFFSET 2
#define LAN_BEACON_PACKETTYPE1_OFFSET 6
#define LAN_BEACON_PACKETTYPE2_OFFSET 7
#define LAN_BEACON_NONCE_OFFSET 8

// Packet types in 2 byte readable form
#define LAN_SERVER_QUERY1 (uint8)'S'
#define LAN_SERVER_QUERY2 (uint8)'Q'

#define LAN_SERVER_RESPONSE1 (uint8)'S'
#define LAN_SERVER_RESPONSE2 (uint8)'R'

class FInternetAddr;
class FNboSerializeToBuffer;

// LAN Session Delegates
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnValidQueryPacket, uint8*, int32, uint64);
typedef FOnValidQueryPacket::FDelegate FOnValidQueryPacketDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnValidResponsePacket, uint8*, int32);
typedef FOnValidResponsePacket::FDelegate FOnValidResponsePacketDelegate;

DECLARE_MULTICAST_DELEGATE(FOnSearchingTimeout);
typedef FOnSearchingTimeout::FDelegate FOnSearchingTimeoutDelegate;

//LAN Beacon Delegate
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPortChanged, int32);
typedef FOnPortChanged::FDelegate FOnPortChangedDelegate;

/**
 * Class responsible for sending/receiving UDP broadcasts for LAN match
 * discovery
 */
class ONLINESUBSYSTEMB3ATZ_API FB3atZBeacon
{
	/** Builds the broadcast address and caches it */
	TSharedPtr<class FInternetAddr> BroadcastAddr;
	/** The socket to listen for requests on */
	class FSocket* ListenSocket;
	/** The address in bound requests come in on */
	TSharedPtr<class FInternetAddr> ListenAddr;
	/** Temporary address when receiving packets*/
	TSharedRef<class FInternetAddr> SockAddr;

public:
	/** Sets the broadcast address for this object */
	FB3atZBeacon();

	/** Frees the broadcast socket */
	~FB3atZBeacon();

	/** Return true if there is a valid ListenSocket */
	bool IsListenSocketValid() const;

	/**
	* Initializes the socket for LAN Connection
	*
	* @param Port the port to listen on
	*
	* @return true if both socket was created successfully, false otherwise
	*/
	bool Init(int32 Port);

	/**
	 * Initializes the socket for host in online connection
	 *
	 * @param Port the port to listen on
	 *
	 * @return true if both socket was created successfully, false otherwise
	 */
	bool InitHost(int32 Port);

	/**
	* Initializes the socket for the client in online connection
	*
	* @param Port the port to listen on
	*
	* @return true if both socket was created successfully, false otherwise
	*/
	bool InitClient(int32 IP, int32 Port, int32 ListenPort);

	/**
	 * Called to poll the socket for pending data. Any data received is placed
	 * in the specified packet buffer
	 *
	 * @param PacketData the buffer to get the socket's packet data
	 * @param BufferSize the size of the packet buffer
	 *
	 * @return the number of bytes read (0 if none or an error)
	 */
	int32 ReceivePacket(uint8* PacketData, int32 BufferSize);

	/**
	 * Uses the cached broadcast address to send packet to a subnet
	 *
	 * @param Packet the packet to send
	 * @param Length the size of the packet to send
	 */
	bool BroadcastPacket(uint8* Packet, int32 Length);

	/**
	* Uses the cached socket address to send packet to a subnet
	*
	* @param Packet the packet to send
	* @param Length the size of the packet to send
	*/
	bool BroadcastPacketFromSocket(uint8* Packet, int32 Length);

	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnPortChanged, int32);
};

#define LAN_ANNOUNCE_PORT 14001
#define B3atZ_UNIQUE_ID 9999
#define B3atZ_QUERY_TIMEOUT 5
#define B3atZ_PLATFORMMASK 0xffffffff

/**
 *	Encapsulate functionality related to LAN broadcast data
 */
class ONLINESUBSYSTEMB3ATZ_API FB3atZSession
{
protected:
	/**
	 * Determines if the packet header is valid or not
	 *
	 * @param Packet the packet data to check
	 * @param Length the size of the packet buffer
	 *
	 * @return true if the header is valid, false otherwise
	 */
	bool IsValidLanQueryPacket(const uint8* Packet, uint32 Length, uint64& ClientNonce);

	/**
	 * Determines if the packet header is valid or not
	 *
	 * @param Packet the packet data to check
	 * @param Length the size of the packet buffer
	 *
	 * @return true if the header is valid, false otherwise
	 */
	bool IsValidLanResponsePacket(const uint8* Packet, uint32 Length);

public:

	FDateTime PeepTime;
	bool WarmUp = true;
	bool B3atZBeaconSuccess = false;

	/** Port to listen on for LAN queries/responses */
	int32 LanAnnouncePort;

	/** IP to listen on for LAN queries/responses */
	int32 HostSessionAddr;

	/** Port to listen on for LAN queries/responses */
	int32 HostSessionPort;

	/** Port to listen on for LAN queries/responses */
	int32 ClientSessionPort;

	/** Is it a match over LAN*/
	bool IsLANMatch;

	/** Unique id to keep UE3 games from seeing each others' LAN packets */
	int32 LanGameUniqueId;

	/** Mask containing which platforms can cross communicate */
	int32 LanPacketPlatformMask;

	/** The amount of time to wait before timing out query request */
	float B3atZQueryTimeout;

	/** LAN beacon for packet broadcast */
	class FB3atZBeacon* B3atZBeacon;

	/** State of the LAN beacon */
	EB3atZBeaconState::Type B3atZBeaconState;

	/** Used by a client to uniquely identify itself during B3atZ match discovery */
	uint64 B3atZNonce;

	/** The amount of time before the LAN query is considered done */
	float B3atZQueryTimeLeft;

	FB3atZSession() :
		LanAnnouncePort(LAN_ANNOUNCE_PORT),
		LanGameUniqueId(B3atZ_UNIQUE_ID),
		LanPacketPlatformMask(B3atZ_PLATFORMMASK),
		B3atZQueryTimeout(B3atZ_QUERY_TIMEOUT),
		B3atZBeacon(NULL),
		B3atZBeaconState(EB3atZBeaconState::NotUsingB3atZBeacon),
		B3atZNonce(0),
		B3atZQueryTimeLeft(0.0f),
		HostSessionAddr(0)
	{
	}

	virtual ~FB3atZSession()
	{
		StopB3atZSession();
	}

	/**
	 * Creates the LAN beacon for queries/advertising servers
	 *
	 * @param QueryDelegate delegate to fire when a client query is received
	 */
	bool Host(FOnValidQueryPacketDelegate& QueryDelegate, int32 Port);

	/**
	 * Creates the LAN beacon for queries/advertising servers
	 *
	 * @param Packet packet to be search when broadcasting the search
	 * @param ResponseDelegate delegate to fire when a server response is received
	 * @param TimeoutDelegate delegate to fire if we exceed maximum search time
	 *
	 * @return true if search was started successfully, false otherwise
	 */
	bool Search(class FNboSerializeToBuffer& Packet, FOnValidResponsePacketDelegate& ResponseDelegate, FOnSearchingTimeoutDelegate& TimeoutDelegate);

	/**
	 * Stops the LAN beacon from accepting broadcasts
	 */
	void StopB3atZSession();

	void Tick(float DeltaTime);

	/** create packet of MAX size */
	void CreateHostResponsePacket(FNboSerializeToBuffer& Packet, uint64 ClientNonce);
	void CreateClientQueryPacket(FNboSerializeToBuffer& Packet, uint64 ClientNonce);

	/**
	 * Uses the cached broadcast address to send packet to a subnet
	 *
	 * @param Packet the packet to send
	 * @param Length the size of the packet to send
	 */
	bool BroadcastPacket(uint8* Packet, int32 Length);

	bool BroadcastPacketFromSocket(uint8* Packet, int32 Length);

	EB3atZBeaconState::Type GetBeaconState() const
	{
		return B3atZBeaconState;
	}

	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnValidQueryPacket, uint8*, int32, uint64);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnValidResponsePacket, uint8*, int32);
	DEFINE_ONLINE_DELEGATE(OnSearchingTimeout);
};
