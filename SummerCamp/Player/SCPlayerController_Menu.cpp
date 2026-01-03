// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController_Menu.h"

#include "SCLocalPlayer.h"
#include "SCPlayerState.h"

#include "OnlineKeyValuePair.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPlayerController_Menu"

ASCPlayerController_Menu::ASCPlayerController_Menu(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, SelectedChallengeIndex(-1) // -1 will be our invalid number to tell if we've actually selected a challenge at all yet.
{
	static ConstructorHelpers::FObjectFinder<USoundCue> StartTalkingObj(TEXT("/Game/UI/Sounds/StartTalking"));
	TalkingStartSound = StartTalkingObj.Object;
	static ConstructorHelpers::FObjectFinder<USoundCue> StopTalkingObj(TEXT("/Game/UI/Sounds/StopTalking"));
	TalkingStopSound = StopTalkingObj.Object;
}

void ASCPlayerController_Menu::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	SendPreferencesToServer();
}

void ASCPlayerController_Menu::SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId)
{
	Super::SetBackendAccountId(InBackendAccountId);

	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		// Update backend profile
		PS->RequestBackendProfileUpdate();

		// Tells the clients they can now request a loadout update.
		//PS->RequestLoadoutUpdate();

		// Get Player stats
		PS->RequestPlayerStats();

		// Get Player badges
		PS->RequestPlayerBadges();

		// Get this player's platform achievements ready
		PS->QueryAchievements();

#if PLATFORM_XBOXONE
		PS->UpdateRichPresence(FVariantData(FString(TEXT("InMenus"))));
#else
		PS->UpdateRichPresence(FVariantData(FString(FText(LOCTEXT("RichPresenceInMenus", "In the menus")).ToString())));
#endif
	}
}

void ASCPlayerController_Menu::AcknowledgePlayerState(APlayerState* PS)
{
	Super::AcknowledgePlayerState(PS);

	SendPreferencesToServer();
}

// NOTE: Modified copy of AILLPlayerController::SendPreferencesToServer
void ASCPlayerController_Menu::SendPreferencesToServer()
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
