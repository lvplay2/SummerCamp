// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineBeaconHostObject.h"
#include "ILLOnlineBeaconHostObject.generated.h"

class AILLOnlineBeaconClient;

/**
 * @class AILLOnlineBeaconHostObject
 */
UCLASS(Config=Engine, NotPlaceable, Transient)
class ILLGAMEFRAMEWORK_API AILLOnlineBeaconHostObject
: public AOnlineBeaconHostObject
{
	GENERATED_UCLASS_BODY()

	/** @return Connected client found by looking up their EncryptionToken on their connection. */
	virtual AILLOnlineBeaconClient* FindClientByEncryptionToken(const FString& EncryptionToken) const;
};
