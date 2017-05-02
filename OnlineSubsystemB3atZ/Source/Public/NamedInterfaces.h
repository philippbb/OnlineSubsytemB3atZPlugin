// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

//
// Named interfaces for Online Subsystem
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "NamedInterfaces.generated.h"



/** Holds a named object interface for dynamically bound interfaces */
USTRUCT()
struct FB3atZNamedInterface
{
	GENERATED_USTRUCT_BODY()

	FB3atZNamedInterface() :
		InterfaceName(NAME_None),
		InterfaceObject(NULL)
	{
	}

	/** The name to bind this object to */
	UPROPERTY()
	FName InterfaceName;
	/** The object to store at this location */
	UPROPERTY()
	UObject* InterfaceObject;
};

/** Holds a name to class name mapping for adding the named interfaces automatically */
USTRUCT()
struct FB3atZNamedInterfaceDef
{
	GENERATED_USTRUCT_BODY()

	FB3atZNamedInterfaceDef() : 
		InterfaceName(NAME_None)
	{
	}

	/** The name to bind this object to */
	UPROPERTY()
	FName InterfaceName;
	/** The class to load and create for the named interface */
	UPROPERTY()
	FString InterfaceClassName;
};

/**
 * Named interfaces are a registry of UObjects accessible by an FName key that will persist for the lifetime of the process
 */
UCLASS(transient, config=Engine)
class UB3atZNamedInterfaces: public UObject
{
	GENERATED_UCLASS_BODY()

	/**
	 * Cleanup delegate fired on destruction
	 */
	DECLARE_MULTICAST_DELEGATE(FOnCleanup);

	// UObject interface begin
	virtual void BeginDestroy() override;
	// UObject interface end

	/**
	 * Initialize the named interface and create any predefined interfaces
	 */
	void Initialize();

	/** @return an interface stored by name */
	class UObject* GetNamedInterface(FName InterfaceName) const;

	/** set an interface stored by name, replacing any existing */
	void SetNamedInterface(FName InterfaceName, class UObject* NewInterface);

	/** get number of current named interfaces */
	int32 GetNumInterfaces() { return NamedInterfaces.Num(); }

	/** @return delegate fired on cleanup */
	FOnCleanup& OnCleanup() { return CleanupDelegates; }

private:

	/** Holds the set of registered named interfaces */
	UPROPERTY()
	TArray<FB3atZNamedInterface> NamedInterfaces;

	/** The list of named interfaces to automatically create and store */
	UPROPERTY(config)
	TArray<FB3atZNamedInterfaceDef> NamedInterfaceDefs;

	FOnCleanup CleanupDelegates;
};

