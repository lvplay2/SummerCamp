// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPartyBeaconHostObject.h"

#include "SCPartyBeaconClient.h"
#include "SCPartyBeaconState.h"

ASCPartyBeaconHostObject::ASCPartyBeaconHostObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClientBeaconActorClass = ASCPartyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();

	BeaconStateClass = ASCPartyBeaconState::StaticClass();
}
