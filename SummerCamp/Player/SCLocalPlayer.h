// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLLocalPlayer.h"
#include "SCLocalPlayer.generated.h"

class USCCharacterPreferencesSaveGame;
class USCCharacterSelectionsSaveGame;
class USCSinglePlayerSaveGame;

/**
 * @class USCLocalPlayer
 */
UCLASS()
class SUMMERCAMP_API USCLocalPlayer
: public UILLLocalPlayer
{
	GENERATED_UCLASS_BODY()

protected:
	// Begin UILLLocalPlayer interface
	virtual void OnQueryEntitlementsComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error) override;

	virtual void HandleControllerDisconnected() override;
	// End UILLLocalPlayer interface
};
