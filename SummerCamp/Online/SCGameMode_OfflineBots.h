// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameMode_Offline.h"
#include "SCGameMode_OfflineBots.generated.h"

/**
 * @class ASCGameMode_OfflineBots
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCGameMode_OfflineBots
: public ASCGameMode_Offline
{
	GENERATED_UCLASS_BODY()

protected:
	// Begin AGameMode interface
	virtual void HandleMatchHasStarted() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	// End AGameMode interface

	// Begin AILLGameMode interface
	virtual void GameSecondTimer() override;
	// End AILLGameMode interface

	// Begin ASCGameMode interface
	virtual void HandlePreMatchIntro() override;
	virtual void HandleWaitingPostMatchOutro() override;
	virtual bool IsAllowedVehicleClass(TSubclassOf<ASCDriveableVehicle> VehicleClass) const override;
	// End ASCGameMode interface
};
