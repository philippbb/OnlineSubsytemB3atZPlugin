// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "K2Node_InAppPurchase.h"
#include "InAppPurchaseCallbackProxy.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_DirectInAppPurchase::UK2Node_DirectInAppPurchase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UB3atZInAppPurchaseCallbackProxy, CreateProxyObjectForInAppPurchase);
	ProxyFactoryClass = UB3atZInAppPurchaseCallbackProxy::StaticClass();

	ProxyClass = UB3atZInAppPurchaseCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
