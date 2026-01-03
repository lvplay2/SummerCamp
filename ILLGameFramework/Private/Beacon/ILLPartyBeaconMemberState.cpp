// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyBeaconMemberState.h"

#include "OnlineBeacon.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLPartyBeaconState.h"

AILLPartyBeaconMemberState::AILLPartyBeaconMemberState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLPartyBeaconMemberState::Destroyed()
{
	Super::Destroyed();

	if (bRegisteredWithPartySession)
	{
		UnregisterWithPartySession();
	}
}

void AILLPartyBeaconMemberState::CheckHasSynchronized()
{
	Super::CheckHasSynchronized();

	// Attempt to register with the party session as soon as we synchronize
	if (!bRegisteredWithPartySession && bHasSynchronized)
	{
		RegisterWithPartySession();
	}
}

void AILLPartyBeaconMemberState::RegisterWithPartySession()
{
	if (bRegisteredWithPartySession || !bHasSynchronized)
		return;
	if (!UniqueId.IsValid())
		return;

	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	if (Sessions->GetSessionState(NAME_PartySession) != EOnlineSessionState::InProgress)
		return;

	UE_LOG(LogBeacon, Verbose, TEXT("%s::RegisterWithPartySession (%s)"), *GetName(), *DisplayName);

	if (Sessions->RegisterPlayer(NAME_PartySession, *UniqueId, false))
	{
		bRegisteredWithPartySession = true;
	}
}

void AILLPartyBeaconMemberState::UnregisterWithPartySession()
{
	if (!UniqueId.IsValid() || !bRegisteredWithPartySession)
		return;

	UE_LOG(LogBeacon, Verbose, TEXT("%s::UnregisterWithPartySession (%s)"), *GetName(), *DisplayName);

	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	Sessions->UnregisterPlayer(NAME_PartySession, *UniqueId);

	bRegisteredWithPartySession = false;
}
