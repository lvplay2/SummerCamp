// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerController.h"
#include "SCPlayerController_Online.generated.h"

/**
 * @class ASCPlayerController_Online
 */
UCLASS(Abstract, Config=Game)
class SUMMERCAMP_API ASCPlayerController_Online
: public ASCPlayerController
{
	GENERATED_UCLASS_BODY()

	/** Notifies clients of their P2P host backend account ID in QuickPlay matches. Otherwise this is blank to ensure Private Match host drops do not get an infraction. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReceiveP2PHostAccountId(const FILLBackendPlayerIdentifier& HostAccountId);
};
