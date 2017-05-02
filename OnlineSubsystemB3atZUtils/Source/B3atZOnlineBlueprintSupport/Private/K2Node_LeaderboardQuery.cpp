// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "K2Node_LeaderboardQuery.h"
#include "LeaderboardQueryCallbackProxy.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_DirectLeaderboardQuery::UK2Node_DirectLeaderboardQuery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UDirectLeaderboardQueryCallbackProxy, CreateProxyObjectForIntQuery);
	ProxyFactoryClass = UDirectLeaderboardQueryCallbackProxy::StaticClass();

	ProxyClass = UDirectLeaderboardQueryCallbackProxy::StaticClass();
}

#undef LOCTEXT_NAMESPACE
