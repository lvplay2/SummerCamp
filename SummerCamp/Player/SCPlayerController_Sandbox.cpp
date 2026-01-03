// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController_Sandbox.h"

#include "SCGameMode_Sandbox.h"
#include "SCKillerCharacter.h"

ASCPlayerController_Sandbox::ASCPlayerController_Sandbox(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPlayerController_Sandbox::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("SNBX_NextCharacter", EInputEvent::IE_Pressed, this, &ASCPlayerController_Sandbox::SERVER_RequestNextCharacter);
		InputComponent->BindAction("SNBX_PreviousCharacter", EInputEvent::IE_Pressed, this, &ASCPlayerController_Sandbox::SERVER_RequestPreviousCharacter);
		InputComponent->BindAction("SNBX_FirstJason", EInputEvent::IE_Pressed, this, &ASCPlayerController_Sandbox::SERVER_RequestFirstJason);
	}
}

bool ASCPlayerController_Sandbox::SERVER_RequestNextCharacter_Validate()
{
	return true;
}

void ASCPlayerController_Sandbox::SERVER_RequestNextCharacter_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameMode_Sandbox* GameMode = World->GetAuthGameMode<ASCGameMode_Sandbox>())
		{
			GameMode->SwapCharacter(this);
		}
	}
}

bool ASCPlayerController_Sandbox::SERVER_RequestPreviousCharacter_Validate()
{
	return true;
}

void ASCPlayerController_Sandbox::SERVER_RequestPreviousCharacter_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameMode_Sandbox* GameMode = World->GetAuthGameMode<ASCGameMode_Sandbox>())
		{
			GameMode->SwapCharacterPrevious(this);
		}
	}
}

bool ASCPlayerController_Sandbox::SERVER_RequestFirstJason_Validate()
{
	return true;
}

void ASCPlayerController_Sandbox::SERVER_RequestFirstJason_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCGameMode_Sandbox* GameMode = World->GetAuthGameMode<ASCGameMode_Sandbox>())
		{
			GameMode->SwapCharacter(this, GameMode->GetKillerCharacterClasses()[0]);
		}
	}
}
