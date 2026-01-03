// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameState_SPChallenges.h"

#include "SCBackendRequestManager.h"
#include "SCDossier.h"
#include "SCGameInstance.h"
#include "SCLocalPlayer.h"
#include "SCPlayerState.h"
#include "SCSinglePlayerSaveGame.h"

// Time (in seconds) to wait until the killer is flagged as spotted
#define SPOTTER_GRACE_PERIOD 0.5f

ASCGameState_SPChallenges::ASCGameState_SPChallenges(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, OutOfBoundsTime(-1) // Default to in bounds, just in case.
{
	Dossier = CreateDefaultSubobject<USCDossier>(TEXT("Dossier"));
	bPlayJasonAbilityUnlockSounds = false;
}

void ASCGameState_SPChallenges::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// Must be called before our calls to the backend so that we can handle match complete objectives
	OnMatchEnded.Broadcast();

	// Clear our check on spotter timer
	GetWorldTimerManager().ClearTimer(TimerHandle_CheckOnSpotter);

	// Let the backend know we're good to go!
	USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
	USCBackendRequestManager* BackendRequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
	ASCPlayerController* LocalPlayerController = Cast<ASCPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!LocalPlayerController)
		return;

	if (BackendRequestManager->CanSendAnyRequests())
	{
		FOnILLBackendSimpleRequestDelegate Delegate = FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnChallengeCompletedPosted);
		const FString& MatchCompletionJson = Dossier->GetMatchCompletionJson(this);
		BackendRequestManager->ReportChallengeComplete(LocalPlayerController->GetBackendAccountId(), MatchCompletionJson, Delegate);
	}

	// Finally update our local storage
	if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(LocalPlayerController->GetLocalPlayer()))
	{
		if (USCSinglePlayerSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCSinglePlayerSaveGame>())
		{
			SaveGame->OnMatchCompleted(LocalPlayerController, Dossier);
		}
	}
}

void ASCGameState_SPChallenges::CharacterKilled(ASCCharacter* Victim, const UDamageType* const KillInfo)
{
	Super::CharacterKilled(Victim, KillInfo);

	Dossier->OnCounselorKilled(this);
}

void ASCGameState_SPChallenges::SpottedKiller(ASCKillerCharacter* KillerSpotted, ASCCharacter* Spotter)
{
	Super::SpottedKiller(KillerSpotted, Spotter);

	// No need to update our spotter list once we've locked the stealth skull
	if (Dossier->IsStealthSkullUnlocked())
	{
		// Check back in a little to see if the counselor that spotted the killer is in 'offline' mode
		FTimerDelegate CheckOnSpotterDelegate;
		CheckOnSpotterDelegate.BindUObject(this, &ThisClass::CheckOnSpotter, Spotter);
		GetWorldTimerManager().SetTimer(TimerHandle_CheckOnSpotter, CheckOnSpotterDelegate, SPOTTER_GRACE_PERIOD, false);
	}
}

void ASCGameState_SPChallenges::CharacterEarnedXP(ASCPlayerState* EarningPlayer, int32 ScoreEarned)
{
	Super::CharacterEarnedXP(EarningPlayer, ScoreEarned);

	if (EarningPlayer->IsActiveCharacterKiller())
		Dossier->OnXPEarned(this);
}

void ASCGameState_SPChallenges::SetOutOfBoundsTime(int32 SecondsRemaining)
{
	OutOfBoundsTime = SecondsRemaining;
}

void ASCGameState_SPChallenges::OnChallengeCompletedPosted(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnChallengeCompletedPosted: ResponseCode %i Reason: %s"), ResponseCode, *ResponseContents);
	}
}

void ASCGameState_SPChallenges::CheckOnSpotter(ASCCharacter* Spotter)
{
	const ASCCounselorCharacter* SpotterCounselor = Cast<ASCCounselorCharacter>(Spotter);
	if (SpotterCounselor && SpotterCounselor->IsUsingOfflineLogic() && !SpotterCounselor->bIsGrabbed && !SpotterCounselor->IsInContextKill())
	{
		// Lock the undetected skull if the AI is in 'offline' mode and they aren't grabbed or are being killed
		Dossier->OnSpottedByAI();
		//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Cyan, FString::Printf(TEXT("Spotted by %s"), *GetNameSafe(Spotter)));
	}
}
