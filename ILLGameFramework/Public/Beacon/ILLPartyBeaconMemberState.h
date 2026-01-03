// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSessionBeaconMemberState.h"
#include "ILLPartyBeaconMemberState.generated.h"

/**
 * @class AILLPartyBeaconMemberState
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLPartyBeaconMemberState
: public AILLSessionBeaconMemberState
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void Destroyed() override;
	// End AActor interface

	// Begin AILLSessionBeaconMemberState interface
	virtual void CheckHasSynchronized() override;
	// End AILLSessionBeaconMemberState interface

	// Is this member pending lobby authorization?
	bool bLobbyReservationPending;

	//////////////////////////////////////////////////
	// Session registration
public:
	/** Registers this party member with the session. Session should be in-progress! */
	virtual void RegisterWithPartySession();

	/** Unregisters this party member with the session. */
	virtual void UnregisterWithPartySession();

protected:
	// Have we registered with our party session?
	bool bRegisteredWithPartySession;
};
