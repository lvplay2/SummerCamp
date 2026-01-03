// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCOnlineMatchmakingClient.h"

USCOnlineMatchmakingClient::USCOnlineMatchmakingClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
#if PLATFORM_XBOXONE && MATCHMAKING_SUPPORTS_HTTP_API
	QoSTemplateName = TEXT("DedicatedQoSTrafficUdp");
#endif
}
