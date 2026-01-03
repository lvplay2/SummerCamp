// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_Online.h"

#include "SCBackendPlayer.h"
#include "SCGameSession.h"
#include "SCGameState_Online.h"
#include "SCInGameHUD_Online.h"
#include "SCLocalPlayer.h"
#include "SCPlayerController_Online.h"
#include "SCPlayerState.h"

ASCGameMode_Online::ASCGameMode_Online(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bPauseable = false;

	HoldWaitingForPlayersTimeNonFullQuickPlay = 30;
	HoldWaitingForPlayersTime = 3;
	PreMatchWaitTime = 5;
}

void ASCGameMode_Online::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ASCPlayerState* PS = Cast<ASCPlayerState>(NewPlayer->PlayerState))
	{
		// When fresh mid-game logins have no pawn and no active character class, force them into spectator mode
		// This should break them out of waiting for players
		if (IsMatchInProgress() && !NewPlayer->GetPawn())
		{
			if (!PS->CanSpawnActiveCharacter())
			{
				Spectate(NewPlayer);
			}
		}

		PS->bLoggedOut = false;
	}

	// Infraction related host tracking
	if (ASCPlayerController_Online* NewPC = Cast<ASCPlayerController_Online>(NewPlayer))
	{
		FILLBackendPlayerIdentifier QuickPlayHostAccountId;
		ASCGameSession* GS = CastChecked<ASCGameSession>(GameSession);
		if (GS->IsQuickPlayMatch())
		{
			if (USCLocalPlayer* FirstLocalPlayer = Cast<USCLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController()))
			{
				if (FirstLocalPlayer->GetBackendPlayer() && FirstLocalPlayer->PlayerController != NewPC)
					QuickPlayHostAccountId = FirstLocalPlayer->GetBackendPlayer()->GetAccountId();
			}
		}

		NewPC->ClientReceiveP2PHostAccountId(QuickPlayHostAccountId);
	}
}


void ASCGameMode_Online::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (ASCPlayerState* PS = Cast<ASCPlayerState>(Exiting->PlayerState))
	{
		// Mark them as logged out.
		PS->bLoggedOut = true;
	}
}
