// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLCheatManager.h"
#include "SCCheatManager.generated.h"

class USCScoringEvent;

/**
 * @class USCCheatManager
 */
UCLASS(Within=SCPlayerController)
class SUMMERCAMP_API USCCheatManager
: public UILLCheatManager
{
	GENERATED_UCLASS_BODY()

	/** Repairs the Cars. */
	UFUNCTION(Exec)
	void RepairCar();

	/** Repairs and starts the Cars. */
	UFUNCTION(Exec)
	void StartCar();

	/** Repairs the Boats. */
	UFUNCTION(Exec)
	void RepairBoat();

	/** Repairs and starts the Boats. */
	UFUNCTION(Exec)
	void StartBoat();

	/** Gives experience. */
	UFUNCTION(Exec)
	void GiveXP(TSubclassOf<USCScoringEvent> XPClass);

	/** Attempts to respawn the local player. */
	UFUNCTION(Exec)
	void RevivePlayer();

	/** Rain control. */
	UFUNCTION(Exec)
	void SetRaining(bool bRain);

	/** Infinite sweater for Jason kill testing */
	UFUNCTION(Exec)
	void ToggleInfiniteSweater();
};
