// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCLocalPlayer.h"

#include "Online.h"
#include "OnlineSubsystemUtilsClasses.h"

#include "SCAudioSettingsSaveGame.h"
#include "SCCharacterPreferencesSaveGame.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCGameSettingsSaveGame.h"
#include "SCInputMappingsSaveGame.h"
#include "SCInputSettingsSaveGame.h"
#include "SCSinglePlayerSaveGame.h"
#include "SCVideoSettingsSaveGame.h"

USCLocalPlayer::USCLocalPlayer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SettingsSaveGameClasses.Add(USCAudioSettingsSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCCharacterPreferencesSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCCharacterSelectionsSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCGameSettingsSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCInputMappingsSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCInputSettingsSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCSinglePlayerSaveGame::StaticClass());
	SettingsSaveGameClasses.Add(USCVideoSettingsSaveGame::StaticClass());
}

void USCLocalPlayer::OnQueryEntitlementsComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	Super::OnQueryEntitlementsComplete(bWasSuccessful, UserId, Error);

	if (bWasSuccessful)
	{
		if (AILLPlayerController* PC = CastChecked<AILLPlayerController>(PlayerController))
		{
			if (USCCharacterSelectionsSaveGame* Settings = GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				Settings->VerifySettingOwnership(PC, true);
			}
		}
	}
}

void USCLocalPlayer::HandleControllerDisconnected()
{
	// Pause first, push controller notification second.
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(PlayerController))
	{
		PC->OnPause();
	}

	Super::HandleControllerDisconnected();
}
