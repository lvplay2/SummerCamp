// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLobbyBeaconState.h"

#include "ILLLobbyBeaconMemberState.h"

AILLLobbyBeaconState::AILLLobbyBeaconState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MemberStateClass = AILLLobbyBeaconMemberState::StaticClass();
}
