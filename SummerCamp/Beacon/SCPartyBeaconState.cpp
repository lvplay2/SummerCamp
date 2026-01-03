// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPartyBeaconState.h"

#include "SCPartyBeaconMemberState.h"

ASCPartyBeaconState::ASCPartyBeaconState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MemberStateClass = ASCPartyBeaconMemberState::StaticClass();
}
