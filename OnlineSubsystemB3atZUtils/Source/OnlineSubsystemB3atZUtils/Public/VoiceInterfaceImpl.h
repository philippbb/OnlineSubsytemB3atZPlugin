// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Public/OnlineSubsystemB3atZTypes.h"
#include "Interfaces/B3atZVoiceInterface.h"
#include "Net/VoiceDataCommon.h"
#include "VoicePacketImpl.h"
#include "OnlineSubsystemUtilsPackage.h"

/**
 * The generic implementation of the voice interface 
 */

class ONLINESUBSYSTEMB3ATZUTILS_API FB3atZOnlineVoiceImpl : public IB3atZOnlineVoice
{
	/** Reference to the main online subsystem */
	class IOnlineSubsystemB3atZ* OnlineSubsystem;
	/** Reference to the sessions interface */
	class IOnlineSession* SessionInt;
	/** Reference to the profile interface */
	class IOnlineIdentity* IdentityInt;
	/** Reference to the voice engine for acquiring voice data */
	IVoiceEnginePtr VoiceEngine;

	/** Maximum permitted local talkers */
	int32 MaxLocalTalkers;
	/** Maximum permitted remote talkers */
	int32 MaxRemoteTalkers;

	/** State of all possible local talkers */
	TArray<FLocalTalker> LocalTalkers;
	/** State of all possible remote talkers */
	TArray<FRemoteTalker> RemoteTalkers;
	/** Remote players locally muted explicitly */
	TArray<FB3atZUniqueNetIdString> SystemMuteList;
	/** All remote players locally muted (super set of SystemMuteList) */
	TArray<FB3atZUniqueNetIdString> MuteList;

	/** Time to wait for new data before triggering "not talking" */
	float VoiceNotificationDelta;

	/** Buffered voice data I/O */
	FVoiceDataImpl VoiceData;

	/**
	 * Finds a remote talker in the cached list
	 *
	 * @param UniqueId the net id of the player to search for
	 *
	 * @return pointer to the remote talker or NULL if not found
	 */
	struct FRemoteTalker* FindRemoteTalker(const FUniqueNetId& UniqueId);

	/**
	 * Is a given id presently muted (either by system mute or game server)
	 *
	 * @param UniqueId the net id to query
	 *
	 * @return true if the net id is muted at all, false otherwise
	 */
	bool IsLocallyMuted(const FUniqueNetId& UniqueId) const;

	/**
	 * Does a given id exist in the system wide mute list
	 *
	 * @param UniqueId the net id to query
	 *
	 * @return true if the net id is on the system wide mute list, false otherwise
	 */
	bool IsSystemWideMuted(const FUniqueNetId& UniqueId) const;

PACKAGE_SCOPE:

	FB3atZOnlineVoiceImpl() :
		OnlineSubsystem(NULL),
		SessionInt(NULL),
		IdentityInt(NULL),
		VoiceEngine(NULL),
		MaxLocalTalkers(MAX_SPLITSCREEN_TALKERS),
		MaxRemoteTalkers(MAX_REMOTE_TALKERS),
		VoiceNotificationDelta(0.0f)
	{};

	// IB3atZOnlineVoice
	virtual bool Init() override;
	void ProcessMuteChangeNotification() override;

	/**
	 * Cleanup voice interface
	 */
	void Shutdown();

	/**
	 * Processes any talking delegates that need to be fired off
	 *
	 * @param DeltaTime the amount of time that has elapsed since the last tick
	 */
	void ProcessTalkingDelegates(float DeltaTime);

	/**
	 * Reads any data that is currently queued
	 */
	void ProcessLocalVoicePackets();

	/**
	 * Submits network packets to audio system for playback
	 */
	void ProcessRemoteVoicePackets();

	/**
	 * Figures out which remote talkers need to be muted for a given local talker
	 *
	 * @param TalkerIndex the talker that needs the mute list checked for
	 * @param PlayerController the player controller associated with this talker
	 */
	void UpdateMuteListForLocalTalker(int32 TalkerIndex, class APlayerController* PlayerController);

public:

	/** Constructor */
	FB3atZOnlineVoiceImpl(class IOnlineSubsystemB3atZ* InOnlineSubsystem);

	/** Virtual destructor to force proper child cleanup */
	virtual ~FB3atZOnlineVoiceImpl();

	// IB3atZOnlineVoice
	virtual void StartNetworkedVoice(uint8 LocalUserNum) override;
	virtual void StopNetworkedVoice(uint8 LocalUserNum) override;
    virtual bool RegisterLocalTalker(uint32 LocalUserNum) override;
	virtual void RegisterLocalTalkers() override;
    virtual bool UnregisterLocalTalker(uint32 LocalUserNum) override;
	virtual void UnregisterLocalTalkers() override;
    virtual bool RegisterRemoteTalker(const FUniqueNetId& UniqueId) override;
    virtual bool UnregisterRemoteTalker(const FUniqueNetId& UniqueId) override;
	virtual void RemoveAllRemoteTalkers() override;
    virtual bool IsHeadsetPresent(uint32 LocalUserNum) override;
    virtual bool IsLocalPlayerTalking(uint32 LocalUserNum) override;
	virtual bool IsRemotePlayerTalking(const FUniqueNetId& UniqueId) override;
	bool IsMuted(uint32 LocalUserNum, const FUniqueNetId& UniqueId) const override;
	bool MuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) override;
	bool UnmuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) override;
	virtual TSharedPtr<class FVoicePacket> SerializeRemotePacket(FArchive& Ar) override;
	virtual TSharedPtr<class FVoicePacket> GetLocalPacket(uint32 LocalUserNum) override;
	virtual int32 GetNumLocalTalkers() override { return LocalTalkers.Num(); };
	virtual void ClearVoicePackets() override;
	virtual void Tick(float DeltaTime) override;
	virtual FString GetVoiceDebugState() const override;
};

typedef TSharedPtr<FB3atZOnlineVoiceImpl, ESPMode::ThreadSafe> FB3atZOnlineVoiceImplPtr;
