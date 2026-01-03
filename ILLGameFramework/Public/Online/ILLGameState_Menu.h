// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLGameState.h"
#include "ILLGameState_Menu.generated.h"

/**
 * @class AILLGameState_Menu
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLGameState_Menu
: public AILLGameState
{
	GENERATED_UCLASS_BODY()

	// Begin AILLGameState interface
	virtual bool IsUsingMultiplayerFeatures() const { return false; }
	// End AILLGameState interface
};
