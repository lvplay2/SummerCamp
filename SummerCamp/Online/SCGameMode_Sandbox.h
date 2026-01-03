// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameMode_Offline.h"
#include "SCGameMode_Sandbox.generated.h"

class ASCCharacter;
class ASCPlayerController_Sandbox;
class ASCWeapon;

/**
 * @class ASCGameMode_Sandbox
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCGameMode_Sandbox
: public ASCGameMode_Offline
{
	GENERATED_UCLASS_BODY()

	// Begin AGameMode interface
	virtual void RestartPlayer(AController* NewPlayer) override;
	// End AGameMode interface

protected:
	// Begin AGameMode interface
	virtual void HandleMatchHasStarted() override;
	// End AGameMode interface

public:
	void SwapCharacter(ASCPlayerController_Sandbox* SandboxController, TSoftClassPtr<ASCCharacter> DesiredCharacterClass = nullptr);
	void SwapCharacterPrevious(ASCPlayerController_Sandbox* SandboxController);
};
