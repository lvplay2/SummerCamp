// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCLobbyBeaconState.h"

#include "SCLobbyBeaconMemberState.h"

ASCLobbyBeaconState::ASCLobbyBeaconState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MemberStateClass = ASCLobbyBeaconMemberState::StaticClass();
}
