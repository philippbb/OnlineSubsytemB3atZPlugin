// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "K2Node_InAppPurchaseRestore.h"
#include "InAppPurchaseRestoreCallbackProxy.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_DirectInAppPurchaseRestore::UK2Node_DirectInAppPurchaseRestore(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UDirectInAppPurchaseRestoreCallbackProxy, CreateProxyObjectForInAppPurchaseRestore);
	ProxyFactoryClass = UDirectInAppPurchaseRestoreCallbackProxy::StaticClass();

	ProxyClass = UDirectInAppPurchaseRestoreCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
