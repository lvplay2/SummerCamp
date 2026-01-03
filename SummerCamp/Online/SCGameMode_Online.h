// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameMode.h"
#include "SCGameMode_Online.generated.h"

/**
 * @class ASCGameMode_Online
 */
UCLASS(Abstract, Config=Game)
class SUMMERCAMP_API ASCGameMode_Online
: public ASCGameMode
{
	GENERATED_UCLASS_BODY()

	// Begin AGameMode interface
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	// End AGameMode interface
};
