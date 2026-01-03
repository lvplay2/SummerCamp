// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameSession.h"
#include "SCGameSession.generated.h"

/**
 * @class ASCGameSession
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCGameSession
: public AILLGameSession
{
	GENERATED_UCLASS_BODY()

	// Begin AGameSession interface
	virtual void ReturnToMainMenuHost() override;
	// End AGameSession interface

	/** @return true if the current session type allows for killer-picking. */
	FORCEINLINE bool AllowKillerPicking() const
	{
#if WITH_EDITOR
		// Hack to always allow this in PIE, for testing it
		if (GetWorld()->IsPlayInEditor())
			return true;
#endif

		return (IsLANMatch() || IsPrivateMatch());
	}

	// Was ReturnToMainMenuHost just called?
	bool bHostReturningToMainMenu;
};
