// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCLobbyBeaconHostObject.h"

#include "SCLobbyBeaconClient.h"
#include "SCLobbyBeaconState.h"

ASCLobbyBeaconHostObject::ASCLobbyBeaconHostObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClientBeaconActorClass = ASCLobbyBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();

	BeaconStateClass = ASCLobbyBeaconState::StaticClass();
}
