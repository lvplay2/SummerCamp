// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineBeaconClient.h"
#include "TextProperty.h"
#include "ILLOnlineBeaconClient.generated.h"

/**
 * Delegate triggered when a beacon client is kicked from their host.
 *
 * @param KickReason Reason you were kicked.
 */
DECLARE_DELEGATE_OneParam(FILLOnBeaconClientKicked, const FText&/* KickReason*/);

/**
 * @class AILLOnlineBeaconClient
 */
UCLASS(Config=Engine, NotPlaceable, Transient)
class ILLGAMEFRAMEWORK_API AILLOnlineBeaconClient
: public AOnlineBeaconClient
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Kicking
public:
	/**
	 * This was client was kicked by the server.
	 *
	 * @param KickReason Reason the server kicked the local player.
	 */
	virtual void WasKicked(const FText& KickReason);

	/** @return Delegate fired when we are kicked from the host. */
	FORCEINLINE FILLOnBeaconClientKicked& OnBeaconClientKicked() { return BeaconClientKicked; }

protected:
	/** Client notification for being kicked. */
	UFUNCTION(Client, Reliable)
	virtual void ClientWasKicked(const FText& KickReason);

	// Delegate fired when we are kicked from the host
	FILLOnBeaconClientKicked BeaconClientKicked;
};
