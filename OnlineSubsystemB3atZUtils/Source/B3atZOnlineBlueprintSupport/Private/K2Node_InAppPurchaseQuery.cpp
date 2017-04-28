// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "K2Node_InAppPurchaseQuery.h"
#include "InAppPurchaseQueryCallbackProxy.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_DirectInAppPurchaseQuery::UK2Node_DirectInAppPurchaseQuery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UDirectInAppPurchaseQueryCallbackProxy, CreateProxyObjectForInAppPurchaseQuery);
	ProxyFactoryClass = UDirectInAppPurchaseQueryCallbackProxy::StaticClass();

	ProxyClass = UDirectInAppPurchaseQueryCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
