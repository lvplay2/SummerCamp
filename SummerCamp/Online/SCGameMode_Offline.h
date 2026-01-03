// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameMode.h"
#include "SCGameMode_Offline.generated.h"

/**
 * @class ASCGameMode_Offline
 */
UCLASS(Abstract, Config=Game)
class SUMMERCAMP_API ASCGameMode_Offline
: public ASCGameMode
{
	GENERATED_UCLASS_BODY()

	// Begin AILLGameMode interface
	virtual bool IsOpenToMatchmaking() const override { return false; } // Nevar!
	// End AILLGameMode interface

protected:
	// Begin AGameMode interface
	virtual void RestartGame() override;
	// End AGameMode interface

	// Begin ASCGameMode interface
	virtual bool CanCompleteWaitingPreMatch() const override;
	// End ASCGameMode interface
};
