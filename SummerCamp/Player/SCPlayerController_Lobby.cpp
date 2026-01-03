// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController_Lobby.h"

#include "OnlineKeyValuePair.h"

#include "SCCharacterSelectionsSaveGame.h"
#include "SCHUD_Lobby.h"
#include "SCLocalPlayer.h"
#include "SCPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPlayerController_Lobby"

ASCPlayerController_Lobby::ASCPlayerController_Lobby(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundCue> StartTalkingObj(TEXT("/Game/UI/Sounds/StartTalking"));
	TalkingStartSound = StartTalkingObj.Object;
	static ConstructorHelpers::FObjectFinder<USoundCue> StopTalkingObj(TEXT("/Game/UI/Sounds/StopTalking"));
	TalkingStopSound = StopTalkingObj.Object;
}

void ASCPlayerController_Lobby::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	SendPreferencesToServer();
}

void ASCPlayerController_Lobby::AcknowledgePlayerState(APlayerState* PS)
{
	Super::AcknowledgePlayerState(PS);
	
	SendPreferencesToServer();
}

void ASCPlayerController_Lobby::SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId)
{
	Super::SetBackendAccountId(InBackendAccountId);

	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		// Update backend profile
		PS->RequestBackendProfileUpdate();

		// Show this player as in the lobby
#if PLATFORM_XBOXONE
		PS->UpdateRichPresence(FVariantData(FString(TEXT("InLobby"))));
#else
		PS->UpdateRichPresence(FVariantData(FString(FText(LOCTEXT("RichPresenceInLobby", "In pre-match lobby")).ToString())));
#endif
	}
}

void ASCPlayerController_Lobby::ClientReceiveP2PHostAccountId_Implementation(const FILLBackendPlayerIdentifier& HostAccountId)
{
	USCGameInstance* GameInstance = CastChecked<USCGameInstance>(GetGameInstance());
	GameInstance->LastP2PQuickPlayHostAccountId = HostAccountId;
}

void ASCPlayerController_Lobby::EnsureViewingLobby()
{
	if (ASCHUD_Lobby* LobbyHUD = Cast<ASCHUD_Lobby>(GetHUD()))
	{
		LobbyHUD->ForceLobbyUI();
	}
}

// NOTE: Copy of ASCPlayerController::SendPreferencesToServer
void ASCPlayerController_Lobby::SendPreferencesToServer()
{
	if (!IsLocalController() || bSentPreferencesToServer)
	{
		return;
	}

	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(GetLocalPlayer()))
	{
		if (ASCPlayerState* SCPS = Cast<ASCPlayerState>(PlayerState))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				// Send our counselor and killer selections to the server
				SCPS->ServerRequestCounselor(Settings->GetPickedCounselorProfile());
				SCPS->ServerRequestKiller(Settings->GetPickedKillerProfile());

				bSentPreferencesToServer = true;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE