// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameSession.h"

#include "OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

#include "SCBackendPlayer.h"
#include "SCBackendRequestManager.h"
#include "SCGame_Lobby.h"
#include "SCGameInstance.h"
#include "SCLocalPlayer.h"

ASCGameSession::ASCGameSession(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	UpperPlayerLimit = 8; // Never allow matches past this size
}

void ASCGameSession::ReturnToMainMenuHost()
{
	// NOTE: Flagged before the Super call for the sake of ASCPlayerController::PawnLeavingGame
	bHostReturningToMainMenu = true;

	Super::ReturnToMainMenuHost();

	// Report ourselves for killing a QuickPlay match
	if (IsQuickPlayMatch() && !GetWorld()->GetAuthGameMode<ASCGame_Lobby>())
	{
		if (USCLocalPlayer* FirstLocalPlayer = Cast<USCLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController()))
		{
			if (FirstLocalPlayer->GetBackendPlayer() && FirstLocalPlayer->GetBackendPlayer()->GetAccountId().IsValid())
			{
				USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
				GameInstance->GetBackendRequestManager<USCBackendRequestManager>()->ReportInfraction(FirstLocalPlayer->GetBackendPlayer()->GetAccountId(), InfractionTypes::HostAbandonedMatch);
			}
		}
	}
}
