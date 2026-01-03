// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerState.h"
#include "SCPlayerState_Online.generated.h"

/**
 * @class ASCPlayerState_Online
 */
UCLASS(Abstract)
class ASCPlayerState_Online
: public ASCPlayerState
{
	GENERATED_UCLASS_BODY()
	
	// Begin ASCPlayerState interface
	virtual void OnMatchEnded() override;
	// End ASCPlayerState interface
};
