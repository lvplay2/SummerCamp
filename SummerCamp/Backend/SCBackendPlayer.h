// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLBackendPlayer.h"
#include "SCBackendPlayer.generated.h"

class USCBackendInventory;

/**
 * @class USCBackendPlayer
 */
UCLASS()
class SUMMERCAMP_API USCBackendPlayer
: public UILLBackendPlayer
{
	GENERATED_UCLASS_BODY()

	// Begin UILLBackendPlayer interface
	virtual void NotifyArbitrated() override;
	// End UILLBackendPlayer interface

	/** @return Inventory Manager instance. */
	FORCEINLINE USCBackendInventory* GetBackendInventory() const { return BackendInventory; }

private:
	// Backend Inventory, one for each player
	UPROPERTY(Transient)
	USCBackendInventory* BackendInventory;
};
