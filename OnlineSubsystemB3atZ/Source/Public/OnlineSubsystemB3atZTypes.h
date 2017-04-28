// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"
#include "OnlineSubsystemPackage.h"

/** Maximum players supported on a given platform */
#if PLATFORM_XBOXONE
#define MAX_LOCAL_PLAYERS 4
#elif PLATFORM_PS4
#define MAX_LOCAL_PLAYERS 4
#elif PLATFORM_SWITCH
#define MAX_LOCAL_PLAYERS 8
#else
#define MAX_LOCAL_PLAYERS 1
#endif

/** TODO: Yuck. Public headers should not depend on redefining platform-specific macros like ERROR_SUCCESS below */
#if PLATFORM_WINDOWS
#include "WindowsHWrapper.h"
#endif

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0
#endif

#ifndef E_FAIL
#define E_FAIL (uint32)-1
#endif

#ifndef E_NOTIMPL
#define E_NOTIMPL (uint32)-2
#endif

#ifndef ERROR_IO_PENDING
#define ERROR_IO_PENDING 997
#endif

#ifndef S_OK
#define S_OK 0
#endif

/**
 * Generates a random nonce (number used once) of the desired length
 *
 * @param Nonce the buffer that will get the randomized data
 * @param Length the number of bytes to generate random values for
 */
inline void GenerateNonceB3atZ(uint8* Nonce, uint32 Length)
{
//@todo joeg -- switch to CryptGenRandom() if possible or something equivalent
	// Loop through generating a random value for each byte
	for (uint32 NonceIndex = 0; NonceIndex < Length; NonceIndex++)
	{
		Nonce[NonceIndex] = (uint8)(FMath::Rand() & 255);
	}
}

/**
 * Environment for the current online platform
 */
namespace EOnlineEnvironmentB3atZ
{
	enum Type
	{
		/** Dev environment */
		Development,
		/** Cert environment */
		Certification,
		/** Prod environment */
		Production,
		/** Not determined yet */
		Unknown
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EOnlineEnvironmentB3atZ::Type EnvironmentType)
	{
		switch (EnvironmentType)
		{
			case Development: return TEXT("Development");
			case Certification: return TEXT("Certification");
			case Production: return TEXT("Production");
			case Unknown: default: return TEXT("Unknown");
		};
	}
}

/** Possible login states */
namespace ELoginStatusB3atZ
{
	enum Type
	{
		/** Player has not logged in or chosen a local profile */
		NotLoggedIn,
		/** Player is using a local profile but is not logged in */
		UsingLocalProfile,
		/** Player has been validated by the platform specific authentication service */
		LoggedIn
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(ELoginStatusB3atZ::Type EnumVal)
	{
		switch (EnumVal)
		{
			case NotLoggedIn:
			{
				return TEXT("NotLoggedIn");
			}
			case UsingLocalProfile:
			{
				return TEXT("UsingLocalProfile");
			}
			case LoggedIn:
			{
				return TEXT("LoggedIn");
			}
		}
		return TEXT("");
	}
};

/** Possible connection states */
namespace EB3atZOnlineServerConnectionStatus
{
	enum Type : uint8
	{
		/** System normal (used for default state) */
		Normal = 0,
		/** Gracefully disconnected from the online servers */
		NotConnected,
		/** Connected to the online servers just fine */
		Connected,
		/** Connection was lost for some reason */
		ConnectionDropped,
		/** Can't connect because of missing network connection */
		NoNetworkConnection,
		/** Service is temporarily unavailable */
		ServiceUnavailable,
		/** An update is required before connecting is possible */
		UpdateRequired,
		/** Servers are too busy to handle the request right now */
		ServersTooBusy,
		/** Disconnected due to duplicate login */
		DuplicateLoginDetected,
		/** Can't connect because of an invalid/unknown user */
		InvalidUser,
		/** Not authorized */
		NotAuthorized,
		/** Session has been lost on the backend */
		InvalidSession
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineServerConnectionStatus::Type EnumVal)
	{
		switch (EnumVal)
		{
			case Normal:
			{
				return TEXT("Normal");
			}
			case NotConnected:
			{
				return TEXT("NotConnected");
			}
			case Connected:
			{
				return TEXT("Connected");
			}
			case ConnectionDropped:
			{
				return TEXT("ConnectionDropped");
			}
			case NoNetworkConnection:
			{
				return TEXT("NoNetworkConnection");
			}
			case ServiceUnavailable:
			{
				return TEXT("ServiceUnavailable");
			}
			case UpdateRequired:
			{
				return TEXT("UpdateRequired");
			}
			case ServersTooBusy:
			{
				return TEXT("ServersTooBusy");
			}
			case DuplicateLoginDetected:
			{
				return TEXT("DuplicateLoginDetected");
			}
			case InvalidUser:
			{
				return TEXT("InvalidUser");
			}
			case NotAuthorized:
			{
				return TEXT("Not Authorized");
			}
			case InvalidSession:
			{
				return TEXT("Invalid Session");
			}
		}
		return TEXT("");
	}
};

/** Possible feature privilege access levels */
namespace EB3atZFeaturePrivilegeLevel
{
	enum Type
	{
		/** Not defined for the platform service */
		Undefined,
		/** Parental controls have disabled this feature */
		Disabled,
		/** Parental controls allow this feature only with people on their friends list */
		EnabledFriendsOnly,
		/** Parental controls allow this feature everywhere */
		Enabled
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZFeaturePrivilegeLevel::Type EnumVal)
	{
		switch (EnumVal)
		{
			case Undefined:
			{
				return TEXT("Undefined");
			}
			case Disabled:
			{
				return TEXT("Disabled");
			}
			case EnabledFriendsOnly:
			{
				return TEXT("EnabledFriendsOnly");
			}
			case Enabled:
			{
				return TEXT("Enabled");
			}
		}
		return TEXT("");
	}
};

/** The state of an async task (read friends, read content, write cloud file, etc) request */
namespace EB3atZOnlineAsyncTaskState
{
	enum Type
	{
		/** The task has not been started */
		NotStarted,
		/** The task is currently being processed */
		InProgress,
		/** The task has completed successfully */
		Done,
		/** The task failed to complete */
		Failed
	};  

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineAsyncTaskState::Type EnumVal)
	{
		switch (EnumVal)
		{
			case NotStarted:
			{
				return TEXT("NotStarted");
			}
			case InProgress:
			{
				return TEXT("InProgress");
			}
			case Done:
			{
				return TEXT("Done");
			}
			case Failed:
			{
				return TEXT("Failed");
			}
		}
		return TEXT("");
	}
};

/** The possible friend states for a friend entry */
namespace EOnlineFriendState
{
	enum Type
	{
		/** Not currently online */
		Offline,
		/** Signed in and online */
		Online,
		/** Signed in, online, and idle */
		Away,
		/** Signed in, online, and asks to be left alone */
		Busy
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EOnlineFriendState::Type EnumVal)
	{
		switch (EnumVal)
		{
			case Offline:
			{
				return TEXT("Offline");
			}
			case Online:
			{
				return TEXT("Online");
			}
			case Away:
			{
				return TEXT("Away");
			}
			case Busy:
			{
				return TEXT("Busy");
			}
		}
		return TEXT("");
	}
};

/** Leaderboard entry sort types */
namespace EB3atZLeaderboardSort
{
	enum Type
	{
		/** Don't sort at all */
		None,
		/** Sort ascending */
		Ascending,
		/** Sort descending */
		Descending
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZLeaderboardSort::Type EnumVal)
	{
		switch (EnumVal)
		{
		case None:
			{
				return TEXT("None");
			}
		case Ascending:
			{
				return TEXT("Ascending");
			}
		case Descending:
			{
				return TEXT("Descending");
			}
		}
		return TEXT("");
	}
};

/** Leaderboard display format */
namespace EB3atZLeaderboardFormat
{
	enum Type
	{
		/** A raw number */
		Number,
		/** Time, in seconds */
		Seconds,
		/** Time, in milliseconds */
		Milliseconds
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZLeaderboardFormat::Type EnumVal)
	{
		switch (EnumVal)
		{
		case Number:
			{
				return TEXT("Number");
			}
		case Seconds:
			{
				return TEXT("Seconds");
			}
		case Milliseconds:
			{
				return TEXT("Milliseconds");
			}
		}
		return TEXT("");
	}
};

/** How to upload leaderboard score updates */
namespace EB3atZLeaderboardUpdateMethod
{
	enum Type
	{
		/** If current leaderboard score is better than the uploaded one, keep the current one */
		KeepBest,
		/** Leaderboard score is always replaced with uploaded value */
		Force
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZLeaderboardUpdateMethod::Type EnumVal)
	{
		switch (EnumVal)
		{
		case KeepBest:
			{
				return TEXT("KeepBest");
			}
		case Force:
			{
				return TEXT("Force");
			}
		}
		return TEXT("");
	}
};

/** Enum indicating the state the LAN beacon is in */
namespace EB3atZBeaconState
{
	enum Type
	{
		/** The lan beacon is disabled */
		NotUsingB3atZBeacon,
		/** The lan beacon is responding to client requests for information */
		Hosting,
		/** The lan beacon is querying servers for information */
		Searching
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZBeaconState::Type EnumVal)
	{
		switch (EnumVal)
		{

		case NotUsingB3atZBeacon:
			{
				return TEXT("NotUsingLanBeacon");
			}
		case Hosting:
			{
				return TEXT("Hosting");
			}
		case Searching:
			{
				return TEXT("Searching");
			}
		}
		return TEXT("");
	}
};

/** Enum indicating the current state of the online session (in progress, ended, etc.) */
namespace EB3atZOnlineSessionState
{
	enum Type
	{
		/** An online session has not been created yet */
		NoSession,
		/** An online session is in the process of being created */
		Creating,
		/** Session has been created but the session hasn't started (pre match lobby) */
		Pending,
		/** Session has been asked to start (may take time due to communication with backend) */
		Starting,
		/** The current session has started. Sessions with join in progress disabled are no longer joinable */
		InProgress,
		/** The session is still valid, but the session is no longer being played (post match lobby) */
		Ending,
		/** The session is closed and any stats committed */
		Ended,
		/** The session is being destroyed */
		Destroying
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineSessionState::Type EnumVal)
	{
		switch (EnumVal)
		{

		case NoSession:
			{
				return TEXT("NoSession");
			}
		case Creating:
			{
				return TEXT("Creating");
			}
		case Pending:
			{
				return TEXT("Pending");
			}
		case Starting:
			{
				return TEXT("Starting");
			}
		case InProgress:
			{
				return TEXT("InProgress");
			}
		case Ending:
			{
				return TEXT("Ending");
			}
		case Ended:
			{
				return TEXT("Ended");
			}
		case Destroying:
			{
				return TEXT("Destroying");
			}
		}
		return TEXT("");
	}
};

/** The types of advertisement of settings to use */
namespace EB3atZOnlineDataAdvertisementType
{
	enum Type
	{
		/** Don't advertise via the online service or QoS data */
		DontAdvertise,
		/** Advertise via the server ping data only */
		ViaPingOnly,
		/** Advertise via the online service only */
		ViaOnlineService,
		/** Advertise via the online service and via the ping data */
		ViaOnlineServiceAndPing
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineDataAdvertisementType::Type EnumVal)
	{
		switch (EnumVal)
		{
		case DontAdvertise:
			{
				return TEXT("DontAdvertise");
			}
		case ViaPingOnly:
			{
				return TEXT("ViaPingOnly");
			}
		case ViaOnlineService:
			{
				return TEXT("OnlineService");
			}
		case ViaOnlineServiceAndPing:
			{
				return TEXT("OnlineServiceAndPing");
			}
		}
		return TEXT("");
	}
}

/** The types of comparison operations for a given search query */
namespace EB3atZOnlineComparisonOp
{
	enum Type
	{
		Equals,
		NotEquals,
		GreaterThan,
		GreaterThanEquals,
		LessThan,
		LessThanEquals,
		Near,
		In,
		NotIn
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineComparisonOp::Type EnumVal)
	{
		switch (EnumVal)
		{
		case Equals:
			{
				return TEXT("Equals");
			}
		case NotEquals:
			{
				return TEXT("NotEquals");
			}
		case GreaterThan:
			{
				return TEXT("GreaterThan");
			}
		case GreaterThanEquals:
			{
				return TEXT("GreaterThanEquals");
			}
		case LessThan:
			{
				return TEXT("LessThan");
			}
		case LessThanEquals:
			{
				return TEXT("LessThanEquals");
			}
		case Near:
			{
				return TEXT("Near");
			}
		case In:
			{
				return TEXT("In");
			}
		case NotIn:
			{
				return TEXT("NotIn");
			}
		}
		return TEXT("");
	}
}

/** Return codes for the GetCached functions in the various subsystems. */
namespace EB3atZOnlineCachedResult
{
	enum Type
	{
		Success, /** The requested data was found and returned successfully. */
		NotFound /** The requested data was not found in the cache, and the out parameter was not modified. */
	};

	/**
	 * @param EnumVal the enum to convert to a string
	 * @return the stringified version of the enum passed in
	 */
	inline const TCHAR* ToString(EB3atZOnlineCachedResult::Type EnumVal)
	{
		switch (EnumVal)
		{
		case Success:
			{
				return TEXT("Success");
			}
		case NotFound:
			{
				return TEXT("NotFound");
			}
		}
		return TEXT("");
	}
}

/*
 *	Base class for anything meant to be opaque so that the data can be passed around 
 *  without consideration for the data it contains.
 *	A human readable version of the data is available via the ToString() function
 *	Otherwise, nothing but platform code should try to operate directly on the data
 */
class IB3atZOnlinePlatformData
{
protected:

	/** Hidden on purpose */
	IB3atZOnlinePlatformData()
	{
	}

	/** Hidden on purpose */
	IB3atZOnlinePlatformData(const IB3atZOnlinePlatformData& Src)
	{
	}

	/** Hidden on purpose */
	IB3atZOnlinePlatformData& operator=(const IB3atZOnlinePlatformData& Src)
	{
		return *this;
	}

	virtual bool Compare(const IB3atZOnlinePlatformData& Other) const
	{
		return (GetSize() == Other.GetSize()) &&
			(FMemory::Memcmp(GetBytes(), Other.GetBytes(), GetSize()) == 0);
	}

public:

	virtual ~IB3atZOnlinePlatformData() {}

	/**
	 *	Comparison operator
	 */
	bool operator==(const IB3atZOnlinePlatformData& Other) const
	{
		return Other.Compare(*this);
	}

	bool operator!=(const IB3atZOnlinePlatformData& Other) const
	{
		return !(IB3atZOnlinePlatformData::operator==(Other));
	}
	
	/** 
	 * Get the raw byte representation of this opaque data
	 * This data is platform dependent and shouldn't be manipulated directly
	 *
	 * @return byte array of size GetSize()
	 */
	virtual const uint8* GetBytes() const = 0;

	/** 
	 * Get the size of the opaque data
	 *
	 * @return size in bytes of the data representation
	 */
	virtual int32 GetSize() const = 0;

	/** 
	 * Check the validity of the opaque data
	 *
	 * @return true if this is well formed data, false otherwise
	 */
	virtual bool IsValid() const = 0;

	/** 
	 * Platform specific conversion to string representation of data
	 *
	 * @return data in string form 
	 */
	virtual FString ToString() const = 0;

	/** 
	 * Get a human readable representation of the opaque data
	 * Shouldn't be used for anything other than logging/debugging
	 *
	 * @return data in string form 
	 */
	virtual FString ToDebugString() const = 0;
};

/**
 * TArray helper for IndexOfByPredicate() function
 */
struct FB3atZUniqueNetIdMatcher
{
private:
	/** Target for comparison in the TArray */
	const FUniqueNetId& UniqueIdTarget;

public:
	FB3atZUniqueNetIdMatcher(const FUniqueNetId& InUniqueIdTarget) :
		UniqueIdTarget(InUniqueIdTarget)
	{
	}

	/**
	 * Match a given unique Id against the one stored in this struct
	 *
	 * @return true if they are an exact match, false otherwise
	 */
	bool operator()(const FUniqueNetId& Candidate) const
	{
		return UniqueIdTarget == Candidate;
	}
 
	/**
	 * Match a given unique Id against the one stored in this struct
	 *
	 * @return true if they are an exact match, false otherwise
	 */
	bool operator()(const TSharedPtr<const FUniqueNetId>& Candidate) const
	{
		return UniqueIdTarget == *Candidate;
	}

	/**
	 * Match a given unique Id against the one stored in this struct
	 *
	 * @return true if they are an exact match, false otherwise
	 */
	bool operator()(const TSharedRef<const FUniqueNetId>& Candidate) const
	{
		return UniqueIdTarget == *Candidate;
	}
};

/**
 * Unique net id wrapper for a string
 */
class FB3atZUniqueNetIdString : public FUniqueNetId
{
public:
	/** Holds the net id for a player */
	FString UniqueNetIdStr;

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
	FB3atZUniqueNetIdString() = default;
	virtual ~FB3atZUniqueNetIdString() = default;
	FB3atZUniqueNetIdString(const FB3atZUniqueNetIdString&) = default;
	FB3atZUniqueNetIdString(FB3atZUniqueNetIdString&&) = default;
	FB3atZUniqueNetIdString& operator=(const FB3atZUniqueNetIdString&) = default;
	FB3atZUniqueNetIdString& operator=(FB3atZUniqueNetIdString&&) = default;
#else
	/** Default constructor */
	FB3atZUniqueNetIdString()
	{
	}

	/** Destructor */
	virtual ~FB3atZUniqueNetIdString()
	{
	}

	/**
	 * Copy Constructor
	 *
	 * @param Src the id to copy
	 */
	explicit FB3atZUniqueNetIdString(const FB3atZUniqueNetIdString& Src)
		: UniqueNetIdStr(Src.UniqueNetIdStr)
	{
	}

	/**
	 * Move Constructor
	 *
	 * @param Src the id to copy
	 */
	explicit FB3atZUniqueNetIdString(FB3atZUniqueNetIdString&& Src)
		: UniqueNetIdStr(MoveTemp(Src.UniqueNetIdStr))
	{
	}

	/**
	 * Copy Assignment Operator
	 *
	 * @param Src the id to copy
	 */
	FB3atZUniqueNetIdString& operator=(const FB3atZUniqueNetIdString& Src)
	{
		if (this != &Src)
		{
			UniqueNetIdStr = Src.UniqueNetIdStr;
		}
		return *this;
	}

	/**
	 * Move Assignment Operator
	 *
	 * @param Src the id to copy
	 */
	FB3atZUniqueNetIdString& operator=(FB3atZUniqueNetIdString&& Src)
	{
		if (this != &Src)
		{
			UniqueNetIdStr = MoveTemp(Src.UniqueNetIdStr);
		}
		return *this;
	}
#endif

	/**
	 * Constructs this object with the specified net id
	 *
	 * @param InUniqueNetId the id to set ours to
	 */
	explicit FB3atZUniqueNetIdString(const FString& InUniqueNetId)
		: UniqueNetIdStr(InUniqueNetId)
	{
	}

	/**
	 * Constructs this object with the specified net id
	 *
	 * @param InUniqueNetId the id to set ours to
	 */
	explicit FB3atZUniqueNetIdString(FString&& InUniqueNetId)
		: UniqueNetIdStr(MoveTemp(InUniqueNetId))
	{
	}

	/**
	 * Constructs this object with the string value of the specified net id
	 *
	 * @param Src the id to copy
	 */
	explicit FB3atZUniqueNetIdString(const FUniqueNetId& Src)
		: UniqueNetIdStr(Src.ToString())
	{
	}

	// IB3atZOnlinePlatformData

	virtual const uint8* GetBytes() const override
	{
		return (const uint8*)UniqueNetIdStr.GetCharArray().GetData();
	}

	virtual int32 GetSize() const override
	{
		return UniqueNetIdStr.GetCharArray().GetTypeSize() * UniqueNetIdStr.GetCharArray().Num();
	}

	virtual bool IsValid() const override
	{
		return !UniqueNetIdStr.IsEmpty();
	}

	virtual FString ToString() const override
	{
		return UniqueNetIdStr;
	}

	virtual FString ToDebugString() const override
	{
		return UniqueNetIdStr;
	}

	/** Needed for TMap::GetTypeHash() */
	friend uint32 GetTypeHash(const FB3atZUniqueNetIdString& A)
	{
		return ::GetTypeHash(A.UniqueNetIdStr);
	}
};

/** 
 * Abstraction of a profile service shared file handle
 * The class is meant to be opaque (see IB3atZOnlinePlatformData)
 */
class FB3atZSharedContentHandle : public IB3atZOnlinePlatformData
{
protected:

	/** Hidden on purpose */
	FB3atZSharedContentHandle()
	{
	}

	/** Hidden on purpose */
	FB3atZSharedContentHandle(const FB3atZSharedContentHandle& Src)
	{
	}

	/** Hidden on purpose */
	FB3atZSharedContentHandle& operator=(const FB3atZSharedContentHandle& Src)
	{
		return *this;
	}

public:

	virtual ~FB3atZSharedContentHandle() {}
};

/** 
 * Abstraction of a session's platform specific info
 * The class is meant to be opaque (see IB3atZOnlinePlatformData)
 */
class FOnlineSessionInfoB3atZ : public IB3atZOnlinePlatformData
{
protected:

	/** Hidden on purpose */
	FOnlineSessionInfoB3atZ()
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoB3atZ(const FOnlineSessionInfoB3atZ& Src)
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoB3atZ& operator=(const FOnlineSessionInfoB3atZ& Src)
	{
		return *this;
	}

public:

	virtual ~FOnlineSessionInfoB3atZ() {}

	/**
	 * Get the session id associated with this session
	 *
	 * @return session id for this session
	 */
	virtual const FUniqueNetId& GetSessionId() const = 0;
};

/**
 * Paging info needed for a request that can return paged results
 */
class FPagedQueryB3atZ
{
public:
	FPagedQueryB3atZ(int32 InStart = 0, int32 InCount = -1)
		: Start(InStart)
		, Count(InCount)
	{}

	/** @return true if valid range */
	bool IsValidRange() const
	{
		return Start >= 0 && Count >= 0;
	}

	/** first entry to fetch */
	int32 Start;
	/** total entries to fetch. -1 means ALL */
	int32 Count;
};

/**
 * Info for a response with paged results
 */
class FB3atZOnlinePagedResult
{
public:
	FB3atZOnlinePagedResult()
		: Start(0)
		, Count(0)
		, Total(0)
	{}
	virtual ~FB3atZOnlinePagedResult() {}

	/** Starting entry */
	int32 Start;
	/** Number returned */
	int32 Count;
	/** Total available */
	int32 Total;
};

/** Locale and country code */
class FB3aZRegionInfo
{
public:

	FB3aZRegionInfo(const FString& InCountry = FString(), const FString& InLocale = FString())
		: Country(InCountry)
		, Locale(InLocale)
	{}

	/** country code used for configuring things like currency/pricing specific to a country. eg. US */
	FString Country;
	/** local code used to select the localization language. eg. en_US */
	FString Locale;
};

/** Holds metadata about a given downloadable file */
struct FB3atZCloudFileHeader
{	
	/** Hash value, if applicable, of the given file contents */
    FString Hash;
	/** The hash algorithm used to sign this file */
	FName HashType;
	/** Filename as downloaded */
    FString DLName;
	/** Logical filename, maps to the downloaded filename */
    FString FileName;
	/** File size */
    int32 FileSize;
	/** The full URL to download the file if it is stored in a CDN or separate host site */
	FString URL;

    /** Constructors */
    FB3atZCloudFileHeader() :
		FileSize(0)
	{}

	FB3atZCloudFileHeader(const FString& InFileName, const FString& InDLName, int32 InFileSize) :
		DLName(InDLName),
		FileName(InFileName),
		FileSize(InFileSize)
	{}

	bool operator==(const FB3atZCloudFileHeader& Other) const
	{
		return FileSize == Other.FileSize &&
			Hash == Other.Hash &&
			HashType == Other.HashType &&
			DLName == Other.DLName &&
			FileName == Other.FileName &&
			URL == Other.URL;
	}

	bool operator<(const FB3atZCloudFileHeader& Other) const
	{
		return FileName.Compare(Other.FileName, ESearchCase::IgnoreCase) < 0;
	}
};

/** Holds the data used in downloading a file asynchronously from the online service */
struct FB3atZCloudFile
{
	/** The name of the file as requested */
	FString FileName;
	/** The async state the file download is in */
	EB3atZOnlineAsyncTaskState::Type AsyncState;
	/** The buffer of data for the file */
	TArray<uint8> Data;

	/** Constructors */
	FB3atZCloudFile() :
		AsyncState(EB3atZOnlineAsyncTaskState::NotStarted)
	{
	}

	FB3atZCloudFile(const FString& InFileName) :
		FileName(InFileName),
		AsyncState(EB3atZOnlineAsyncTaskState::NotStarted)
	{
	}

	virtual ~FB3atZCloudFile() {}
};

/**
 * Base for all online user info
 */
class FB3atZOnlineUser
{
public:
	/**
	 * destructor
	 */
	virtual ~FB3atZOnlineUser() {}

	/** 
	 * @return Id associated with the user account provided by the online service during registration 
	 */
	virtual TSharedRef<const FUniqueNetId> GetUserId() const = 0;
	/**
	 * @return the real name for the user if known
	 */
	virtual FString GetRealName() const = 0;
	/**
	 * @return the nickname of the user if known
	 */
	virtual FString GetDisplayName(const FString& Platform = FString()) const = 0;
	/** 
	 * @return Any additional user data associated with a registered user
	 */
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const = 0;
};

/**
 * User account information returned via IOnlineIdentity interface
 */
class FB3atZUserOnlineAccount : public FB3atZOnlineUser
{
public:
	/**
	 * @return Access token which is provided to user once authenticated by the online service
	 */
	virtual FString GetAccessToken() const = 0;
	/** 
	 * @return Any additional auth data associated with a registered user
	 */
	virtual bool GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const = 0;
	/** 
	 * @return True, if the data has been changed
	 */
	virtual bool SetUserAttribute(const FString& AttrName, const FString& AttrValue) = 0;
};

/** 
 * Friend list invite states 
 */
namespace EB3atZInviteStatus
{
	enum Type
	{
		/** unknown state */
		Unknown,
		/** Friend has accepted the invite */
		Accepted,
		/** Friend has sent player an invite, but it has not been accepted/rejected */
		PendingInbound,
		/** Player has sent friend an invite, but it has not been accepted/rejected */
		PendingOutbound,
		/** Player has been blocked */
		Blocked,
	};

	/** 
	 * @return the stringified version of the enum passed in 
	 */
	inline const TCHAR* ToString(EB3atZInviteStatus::Type EnumVal)
	{
		switch (EnumVal)
		{
			case Unknown:
			{
				return TEXT("Unknown");
			}
			case Accepted:
			{
				return TEXT("Accepted");
			}
			case PendingInbound:
			{
				return TEXT("PendingInbound");
			}
			case PendingOutbound:
			{
				return TEXT("PendingOutbound");
			}
			case Blocked:
			{
				return TEXT("Blocked");
			}
		}
		return TEXT("");
	}
};

/**
 * Friend user info returned via IOnlineFriends interface
 */
class FB3atZOnlineFriend : public FB3atZOnlineUser
{
public:
	
	/**
	 * @return the current invite status of a friend wrt to user that queried
	 */
	virtual EB3atZInviteStatus::Type GetInviteStatus() const = 0;
	
	/**
	 * @return presence info for an online friend
	 */
	virtual const class FOnlineUserPresence& GetPresence() const = 0;
};

/**
 * Recent player user info returned via IOnlineFriends interface
 */
class FB3atZOnlineRecentPlayer : public FB3atZOnlineUser
{
public:
	
	/**
	 * @return last time the player was seen by the current user
	 */
	virtual FDateTime GetLastSeen() const = 0;
};

/**
 * Blocked user info returned via IOnlineFriends interface
 */
class FB3atZOnlineBlockedPlayer : public FB3atZOnlineUser
{
};

/** The possible permission categories we can choose from to read from the server */
namespace EB3atZOnlineSharingReadCategory
{
	enum Type
	{
		None			= 0x00,
		// Read access to posts on the users feeds
		Posts			= 0x01,
		// Read access for a users friend information, and all data about those friends. e.g. Friends List and Individual Friends Birthday
		Friends			= 0x02,
		// Read access to a users mailbox
		Mailbox			= 0x04,
		// Read the current online status of a user
		OnlineStatus	= 0x08,
		// Read a users profile information, e.g. Users Birthday
		ProfileInfo		= 0x10,	
		// Read information about the users locations and location history
		LocationInfo	= 0x20,

		Default			= ProfileInfo|LocationInfo,
	};



	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineSharingReadCategory::Type CategoryType)
	{
		switch (CategoryType)
		{
		case None:
			{
				return TEXT("Category undefined");
			}
		case Posts:
			{
				return TEXT("Posts");
			}
		case Friends:
			{
				return TEXT("Friends");
			}
		case Mailbox:
			{
				return TEXT("Mailbox");
			}
		case OnlineStatus:
			{
				return TEXT("Online Status");
			}
		case ProfileInfo:
			{
				return TEXT("Profile Information");
			}
		case LocationInfo:
			{
				return TEXT("Location Information");
			}
		}
		return TEXT("");
	}
}


/** The possible permission categories we can choose from to publish to the server */
namespace EB3atZOnlineSharingPublishingCategory
{
	enum Type
	{
		None			= 0x00,
		// Permission to post to a users news feed
		Posts			= 0x01,
		// Permission to manage a users friends list. Add/Remove contacts
		Friends			= 0x02,
		// Manage a users account settings, such as pages they subscribe to, or which notifications they receive
		AccountAdmin	= 0x04,
		// Manage a users events. This features the capacity to create events as well as respond to events.
		Events			= 0x08,

		Default			= None,
	};


	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EB3atZOnlineSharingPublishingCategory::Type CategoryType)
	{
		switch (CategoryType)
		{
		case None:
			{
				return TEXT("Category undefined");
			}
		case Posts:
			{
				return TEXT("Posts");
			}
		case Friends:
			{
				return TEXT("Friends");
			}
		case AccountAdmin:
			{
				return TEXT("Account Admin");
			}
		case Events:
			{
				return TEXT("Events");
			}
		}
		return TEXT("");
	}
}


/** Privacy permissions used for Online Status updates */
namespace EB3atZOnlineStatusUpdatePrivacy
{
	enum Type
	{
		OnlyMe,			// Post will only be visible to the user alone
		OnlyFriends,	// Post will only be visible to the user and the users friends
		Everyone,		// Post will be visible to everyone
	};

	inline const TCHAR* ToString(EB3atZOnlineStatusUpdatePrivacy::Type PrivacyType)
	{
		switch (PrivacyType)
		{
		case OnlyMe:
			return TEXT("Only Me");
		case OnlyFriends:
			return TEXT("Only Friends");
		case Everyone:
			return TEXT("Everyone");
		}
	}
}

/**
* unique identifier for notification transports
*/
typedef FString FNotificationTransportId;
