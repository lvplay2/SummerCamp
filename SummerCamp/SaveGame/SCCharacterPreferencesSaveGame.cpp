// Copyright (C) 2015-2018 IllFonic, LLC.

#include "SummerCamp.h"
#include "SCCharacterPreferencesSaveGame.h"

#include "SCPlayerController.h"
#include "SCPlayerController_Lobby.h"

USCCharacterPreferencesSaveGame::USCCharacterPreferencesSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCCharacterPreferencesSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	if (bAndSave)
	{
		// Force changes to the server
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(PlayerController))
		{
			PC->bSentPreferencesToServer = false;
			PC->SendPreferencesToServer();
		}
		else if (ASCPlayerController_Lobby* LobbyPC = Cast<ASCPlayerController_Lobby>(PlayerController))
		{
			LobbyPC->bSentPreferencesToServer = false;
			LobbyPC->SendPreferencesToServer();
		}
	}
}

bool USCCharacterPreferencesSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	USCCharacterPreferencesSaveGame* Other = CastChecked<USCCharacterPreferencesSaveGame>(OtherSave);
	return(CharacterPreference != Other->CharacterPreference);
}
