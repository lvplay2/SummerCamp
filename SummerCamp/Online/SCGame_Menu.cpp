// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGame_Menu.h"

#include "SCFrontEndHUD.h"
#include "SCGameSession.h"
#include "SCPlayerController_Menu.h"
#include "SCPlayerState.h"

ASCGame_Menu::ASCGame_Menu(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = ASCFrontEndHUD::StaticClass();
	PlayerControllerClass = ASCPlayerController_Menu::StaticClass();
	PlayerStateClass = ASCPlayerState::StaticClass();
}

void ASCGame_Menu::RestartPlayer(class AController* NewPlayer)
{
	// Don't restart!
}

TSubclassOf<AGameSession> ASCGame_Menu::GetGameSessionClass() const
{
	return ASCGameSession::StaticClass();
}
