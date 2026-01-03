// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameSettingsSaveGame.h"

#include "SCPlayerController.h"
#include "SCPlayerController_Lobby.h"
#include "SCInGameHUD.h"

USCGameSettingsSaveGame::USCGameSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bRotateMinimapWithPlayer = true;
	bShowHelpText = true;
}

void USCGameSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave)
{
	Super::ApplyPlayerSettings(PlayerController, bAndSave);

	if (bAndSave)
	{
		// Force changes to the server
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(PlayerController))
		{
			PC->bSentPreferencesToServer = false;
			PC->SendPreferencesToServer();

			if (ASCInGameHUD* InGameHUD = PC->GetSCHUD())
			{
				InGameHUD->SetRotateMinimapWithPlayer(bRotateMinimapWithPlayer);
			}
		}
		else if (ASCPlayerController_Lobby* LobbyPC = Cast<ASCPlayerController_Lobby>(PlayerController))
		{
			LobbyPC->bSentPreferencesToServer = false;
			LobbyPC->SendPreferencesToServer();
		}
	}
}

bool USCGameSettingsSaveGame::HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const
{
	USCGameSettingsSaveGame* Other = CastChecked<USCGameSettingsSaveGame>(OtherSave);
	return(Super::HasChangedFrom(OtherSave)
		|| bRotateMinimapWithPlayer != Other->bRotateMinimapWithPlayer
		|| bShowHelpText != Other->bShowHelpText);
}
