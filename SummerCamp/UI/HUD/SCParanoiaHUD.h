// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCInGameHUD.h"
#include "SCParanoiaHUD.generated.h"


/**
* @class ASCParanoiaHUD
*/
UCLASS()
class SUMMERCAMP_API ASCParanoiaHUD
	: public ASCInGameHUD
{
	GENERATED_UCLASS_BODY()

	// no scoreboard in paranoia mode
	virtual void DisplayScoreboard(const bool bShow, const bool bLockUnlock = false) override;
};