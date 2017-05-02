// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "K2Node_LeaderboardFlush.h"
#include "LeaderboardFlushCallbackProxy.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_DirectLeaderboardFlush::UK2Node_DirectLeaderboardFlush(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UDirectLeaderboardFlushCallbackProxy, CreateProxyObjectForFlush);
	ProxyFactoryClass = UDirectLeaderboardFlushCallbackProxy::StaticClass();

	ProxyClass = UDirectLeaderboardFlushCallbackProxy::StaticClass();
}

FText UK2Node_DirectLeaderboardFlush::GetTooltipText() const
{
	return LOCTEXT("K2Node_LeaderboardFlush_Tooltip", "Flushes leaderboards for a session");
}

FText UK2Node_DirectLeaderboardFlush::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("FlushLeaderboards", "Flush Leaderboards");
}

FText UK2Node_DirectLeaderboardFlush::GetMenuCategory() const
{
	return LOCTEXT("LeaderboardFlushCategory", "Online|Leaderboard");
}

#undef LOCTEXT_NAMESPACE
