// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/CheatManager.h"
#include "ILLCheatManager.generated.h"

/**
 * @class UILLCheatManager
 */
UCLASS(Within=ILLPlayerController)
class ILLGAMEFRAMEWORK_API UILLCheatManager
: public UCheatManager
{
	GENERATED_UCLASS_BODY()

	/**
	 * Makes the server call RestartGame, which will just move to the next map by default.
	 *
	 * @param bForceRecycle Flag the server for instance recycle/replacement first.
	 */
	UFUNCTION(Exec)
	void ForceServerInstanceRecycle();

	/**
	 * Forces a delayed crash.
	 *
	 * @param Delay Optional time delay before crashing.
	 */
	UFUNCTION(Exec)
	void TestCrash(float Delay = 0.f);

private:
	/** Used by TestCrash to go boom. */
	void CrashDummy();
};
