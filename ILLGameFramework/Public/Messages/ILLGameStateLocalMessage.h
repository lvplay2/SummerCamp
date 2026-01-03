// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/LocalMessage.h"
#include "ILLGameStateLocalMessage.generated.h"

/**
 * @class UILLGameStateLocalMessage
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API UILLGameStateLocalMessage
: public ULocalMessage
{
	GENERATED_UCLASS_BODY()

	/** @return Switch value to send for NewMatchState. */
	virtual int32 SwitchForMatchState(FName NewMatchState) const { return -1; }
};
