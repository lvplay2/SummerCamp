// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGame_Menu.h"

#include "ILLGameState_Menu.h"

AILLGame_Menu::AILLGame_Menu(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameStateClass = AILLGameState_Menu::StaticClass();
}
