// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

/*=============================================================================
	IpConnection: Unreal IP network connection.
Notes:
	* See \msdev\vc98\include\winsock.h and \msdev\vc98\include\winsock2.h 
	  for Winsock WSAE* errors returned by Windows Sockets.
=============================================================================*/

#include "IpConnection.h"
#include "SocketSubsystem.h"

#include "IPAddress.h"
#include "Sockets.h"
#include "Net/NetworkProfiler.h"
#include "Net/DataChannel.h"

/*-----------------------------------------------------------------------------
	Declarations.
-----------------------------------------------------------------------------*/

// Size of a UDP header.
#define IP_HEADER_SIZE     (20)
#define UDP_HEADER_SIZE    (IP_HEADER_SIZE+8)

UIpConnectionB3atZ::UIpConnectionB3atZ(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	RemoteAddr(NULL),
	Socket(NULL),
	ResolveInfo(NULL)
{
}

void UIpConnectionB3atZ::InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	// Pass the call up the chain
	Super::InitBase(InDriver, InSocket, InURL, InState, 
		// Use the default packet size/overhead unless overridden by a child class
		(InMaxPacket == 0 || InMaxPacket > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : InMaxPacket,
		InPacketOverhead == 0 ? UDP_HEADER_SIZE : InPacketOverhead);

	Socket = InSocket;
	ResolveInfo = NULL;
}

void UIpConnectionB3atZ::InitLocalConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	UE_LOG(LogNet, Warning, TEXT("IPConnection InitLocalConnection inURL is %s with Port %u"), *InURL.ToString(), InURL.Port);

	InitBase(InDriver, InSocket, InURL, InState, 
		// Use the default packet size/overhead unless overridden by a child class
		(InMaxPacket == 0 || InMaxPacket > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : InMaxPacket,
		InPacketOverhead == 0 ? UDP_HEADER_SIZE : InPacketOverhead);

	// Figure out IP address from the host URL
	bool bIsValid = false;
	// Get numerical address directly.
	RemoteAddr = InDriver->GetSocketSubsystem()->CreateInternetAddr();
	RemoteAddr->SetIp(*InURL.Host, bIsValid);
	RemoteAddr->SetPort(InURL.Port);

	// Try to resolve it if it failed
	if (bIsValid == false)
	{
		UE_LOG(LogNet, Verbose, TEXT("IPConnection InitLocalConnection not valid, trying to resolve"));
		// Create thread to resolve the address.
		ResolveInfo = InDriver->GetSocketSubsystem()->GetHostByName(TCHAR_TO_ANSI(*InURL.Host));
		if (ResolveInfo == NULL)
		{
			Close();
			UE_LOG(LogNet, Verbose, TEXT("IpConnection::InitConnection: Unable to resolve %s"), *InURL.Host);
		}
	}

	// Initialize our send bunch
	InitSendBuffer();
}

void UIpConnectionB3atZ::InitRemoteConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, const class FInternetAddr& InRemoteAddr, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	UE_LOG(LogNet, Verbose, TEXT("IPConnection InitRemoteConnection inURL is %s with Port %u"), *InURL.ToString(), InURL.Port);

	InitBase(InDriver, InSocket, InURL, InState, 
		// Use the default packet size/overhead unless overridden by a child class
		(InMaxPacket == 0 || InMaxPacket > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : InMaxPacket,
		InPacketOverhead == 0 ? UDP_HEADER_SIZE : InPacketOverhead);

	// Copy the remote IPAddress passed in
	bool bIsValid = false;
	FString IpAddrStr = InRemoteAddr.ToString(false);
	RemoteAddr = InDriver->GetSocketSubsystem()->CreateInternetAddr();
	RemoteAddr->SetIp(*IpAddrStr, bIsValid);
	RemoteAddr->SetPort(InRemoteAddr.GetPort());

	URL.Host = RemoteAddr->ToString(false);

	// Initialize our send bunch
	InitSendBuffer();

	// This is for a client that needs to log in, setup ClientLoginState and ExpectedClientLoginMsgType to reflect that
	SetClientLoginState( EClientLoginState::LoggingIn );
	SetExpectedClientLoginMsgType( NMT_Hello );
}

void UIpConnectionB3atZ::LowLevelSend(void* Data, int32 CountBytes, int32 CountBits)
{
	UE_LOG(LogNet, VeryVerbose, TEXT("IPConnection LowLevelSend"));

	const uint8* DataToSend = reinterpret_cast<uint8*>(Data);

	if( ResolveInfo )
	{
		UE_LOG(LogNet, VeryVerbose, TEXT("IPConnection LowLevelSend ResolveInfo valid"));
		// If destination address isn't resolved yet, send nowhere.
		if( !ResolveInfo->IsComplete() )
		{
			UE_LOG(LogNet, VeryVerbose, TEXT("IPConnection LowLevelSend ResolveInfo not complete"));
			// Host name still resolving.
			return;
		}
		else if( ResolveInfo->GetErrorCode() != SE_NO_ERROR )
		{
			// Host name resolution just now failed.
			UE_LOG(LogNet, Warning,  TEXT("IPConnection LowLevelSend Host name resolution failed with %d"), ResolveInfo->GetErrorCode() );
			Driver->ServerConnection->State = USOCK_Closed;
			delete ResolveInfo;
			ResolveInfo = NULL;
			return;
		}
		else
		{
			uint32 Addr;
			// Host name resolution just now succeeded.
			ResolveInfo->GetResolvedAddress().GetIp(Addr);
			RemoteAddr->SetIp(Addr);
			UE_LOG(LogNet, VeryVerbose, TEXT("IPConnection LowLevelSend Host name resolution completed with remoteaddr set to %u"), Addr);
			delete ResolveInfo;
			ResolveInfo = NULL;
		}
	}


	// Process any packet modifiers
	if (Handler.IsValid() && !Handler->GetRawSend())
	{
		const ProcessedPacket ProcessedData = Handler->Outgoing(reinterpret_cast<uint8*>(Data), CountBits);

		if (!ProcessedData.bError)
		{
			DataToSend = ProcessedData.Data;
			CountBytes = FMath::DivideAndRoundUp(ProcessedData.CountBits, 8);
			CountBits = ProcessedData.CountBits;
		}
		else
		{
			CountBytes = 0;
			CountBits = 0;
		}
	}

	// Send to remote.
	int32 BytesSent = 0;
	CLOCK_CYCLES(Driver->SendCycles);

	if ( CountBytes > MaxPacket )
	{
		UE_LOG( LogNet, Warning, TEXT( "UIpConnectionB3atZ::LowLevelSend: CountBytes > MaxPacketSize! Count: %i, MaxPacket: %i %s" ), CountBytes, MaxPacket, *Describe() );
	}

	if (CountBytes > 0)
	{
		//UE_LOG(LogNet, Warning, TEXT("IPConnection LowLevelSend CountBytes > 0 sending now on active socket with sendto"));
		Socket->SendTo(DataToSend, CountBytes, BytesSent, *RemoteAddr);
	}

	UNCLOCK_CYCLES(Driver->SendCycles);
	NETWORK_PROFILER(GNetworkProfiler.FlushOutgoingBunches(this));
	NETWORK_PROFILER(GNetworkProfiler.TrackSocketSendTo(Socket->GetDescription(),DataToSend,BytesSent,NumPacketIdBits,NumBunchBits,NumAckBits,NumPaddingBits,this));
}

FString UIpConnectionB3atZ::LowLevelGetRemoteAddress(bool bAppendPort)
{
	return RemoteAddr->ToString(bAppendPort);
}

FString UIpConnectionB3atZ::LowLevelDescribe()
{
	TSharedRef<FInternetAddr> LocalAddr = Driver->GetSocketSubsystem()->CreateInternetAddr();
	Socket->GetAddress(*LocalAddr);
	return FString::Printf
	(
		TEXT("url=%s remote=%s local=%s state: %s"),
		*URL.Host,
		*RemoteAddr->ToString(true),
		*LocalAddr->ToString(true),
			State==USOCK_Pending	?	TEXT("Pending")
		:	State==USOCK_Open		?	TEXT("Open")
		:	State==USOCK_Closed		?	TEXT("Closed")
		:								TEXT("Invalid")
	);
}

int32 UIpConnectionB3atZ::GetAddrAsInt(void)
{
	uint32 OutAddr = 0;
	// Get the host byte order ip addr
	RemoteAddr->GetIp(OutAddr);
	return (int32)OutAddr;
}

int32 UIpConnectionB3atZ::GetAddrPort(void)
{
	int32 OutPort = 0;
	// Get the host byte order ip port
	RemoteAddr->GetPort(OutPort);
	return OutPort;
}

FString UIpConnectionB3atZ::RemoteAddressToString()
{
	return RemoteAddr->ToString(true);
}
