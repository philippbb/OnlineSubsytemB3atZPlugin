// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NamedInterfaces.h"
#include "UObject/Package.h"
#include "OnlineSubsystemB3atZ.h"

UB3atZNamedInterfaces::UB3atZNamedInterfaces(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
}

void UB3atZNamedInterfaces::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnCleanup().Broadcast();
	}
	Super::BeginDestroy();
}

void UB3atZNamedInterfaces::Initialize()
{
	// Iterate through each configured named interface load it and create an instance
	for (int32 InterfaceIndex = 0; InterfaceIndex < NamedInterfaceDefs.Num(); InterfaceIndex++)
	{
		const FB3atZNamedInterfaceDef& Def = NamedInterfaceDefs[InterfaceIndex];
		// Load the specified interface class name
		UClass* Class = LoadClass<UObject>(NULL, *Def.InterfaceClassName, NULL, LOAD_None, NULL);
		if (Class)
		{
			int32 AddIndex = NamedInterfaces.AddZeroed();
			FB3atZNamedInterface& Interface = NamedInterfaces[AddIndex];
			// Set the object and interface names
			Interface.InterfaceName = Def.InterfaceName;
			Interface.InterfaceObject = NewObject<UObject>(GetTransientPackage(), Class);
			UE_LOG(LogB3atZOnline, Display,
				TEXT("Created named interface (%s) of type (%s)"),
				*Def.InterfaceName.ToString(),
				*Def.InterfaceClassName);
		}
		else
		{
			UE_LOG(LogB3atZOnline, Warning,
				TEXT("Failed to load class (%s) for named interface (%s)"),
				*Def.InterfaceClassName,
				*Def.InterfaceName.ToString());
		}
	}
}

UObject* UB3atZNamedInterfaces::GetNamedInterface(FName InterfaceName) const
{
	for (const FB3atZNamedInterface& Interface : NamedInterfaces)
	{
		if (Interface.InterfaceName == InterfaceName)
		{
			return Interface.InterfaceObject;
		}
	}

	return NULL;
}

void UB3atZNamedInterfaces::SetNamedInterface(FName InterfaceName, UObject* NewInterface)
{
	int32 InterfaceIdx = 0;
	for (; InterfaceIdx < NamedInterfaces.Num(); InterfaceIdx++)
	{
		const FB3atZNamedInterface& Interface = NamedInterfaces[InterfaceIdx];
		if (Interface.InterfaceName == InterfaceName)
		{
			break;
		}
	}

	if (InterfaceIdx >= NamedInterfaces.Num())
	{
		int32 AddIndex = NamedInterfaces.AddZeroed();
		FB3atZNamedInterface& Interface = NamedInterfaces[AddIndex];
		// Set the object and interface names
		Interface.InterfaceName = InterfaceName;
		Interface.InterfaceObject = NewInterface;
	}
	else
	{
		NamedInterfaces[InterfaceIdx].InterfaceObject = NewInterface;
	}
}
