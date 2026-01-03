// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_SPChallenges.h"

#include "ILLGameBlueprintLibrary.h"
#include "ILLGameOnlineBlueprintLibrary.h"

#include "SCChallengeData.h"
#include "SCDossier.h"
#include "SCGameState_SPChallenges.h"
#include "SCLocalPlayer.h"
#include "SCMapRegistry.h"
#include "SCPlayerController_SPChallenges.h"
#include "SCPlayerState_SPChallenges.h"
#include "SCSinglePlayerBlueprintLibrary.h"
#include "SCSPInGameHUD.h"

ASCGameMode_SPChallenges::ASCGameMode_SPChallenges(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameStateClass = ASCGameState_SPChallenges::StaticClass();
	PlayerControllerClass = ASCPlayerController_SPChallenges::StaticClass();
	PlayerStateClass = ASCPlayerState_SPChallenges::StaticClass();
	HUDClass = ASCSPInGameHUD::StaticClass();

	JasonStealthRange = 1.f;
	PostMatchWaitTime = -1.f;
}

void ASCGameMode_SPChallenges::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	if (ASCGameState_SPChallenges* SCGameState = Cast<ASCGameState_SPChallenges>(GameState))
	{
		if (SCGameState->Dossier)
		{
			SCGameState->Dossier->DebugDraw(SCGameState);
		}
	}
#endif
}

void ASCGameMode_SPChallenges::StartPlay()
{
	Super::StartPlay();

	// Get the challenge data for this session
	FString ChallengeID;
	if (FParse::Value(*OptionsString, TEXT("chlng="), ChallengeID))
	{
		TSubclassOf<USCChallengeData> SelectedChallengeClass = USCSinglePlayerBlueprintLibrary::GetChallengeByID(ChallengeID);
		const USCChallengeData* SelectedChallenge = SelectedChallengeClass.GetDefaultObject();

		// Found our challenge, set up the dossier and load the sub level
		if (SelectedChallenge)
		{
			if (ASCGameState_SPChallenges* SCGameState = Cast<ASCGameState_SPChallenges>(GameState))
			{
				SCGameState->Dossier->InitializeChallengeObjectivesFromClass(SelectedChallengeClass);
			}

			// A sublevel is not required
			const FName SublevelName = SelectedChallenge->GetChallengeSublevelName();
			if (!SublevelName.IsNone())
			{
				// No callback needed
				const FLatentActionInfo NoCallback(0, (int32)'CHNG', TEXT(""), nullptr);
				UGameplayStatics::LoadStreamLevel(this, SublevelName, true, true, NoCallback);
			}
		}
		else
		{
			UE_LOG(LogSC, Error, TEXT("Challenge ID %s is invalid! No challenge will load!"), *ChallengeID);
		}
	}
	else
	{
		UE_LOG(LogSC, Error, TEXT("No challenge set! No challenge will load!"));
	}
}

void ASCGameMode_SPChallenges::GameSecondTimer()
{
	if (IsMatchInProgress())
	{
		// Check for end match
		ASCGameState_SPChallenges* GS = Cast<ASCGameState_SPChallenges>(GameState);

		if (GS->OutOfBoundsTime > 0)
			--GS->OutOfBoundsTime;

		// If the time runs out you don't earn the "not spotted" skull
		if ((GS->RemainingTime - 1) == 0) // test minus one to see if the timer is going to run out this frame.
		{
			GS->Dossier->OnSpottedByAI();
		}

		if (GS->ElapsedInProgressTime > 10)
		{
			bool bHasAliveCounselors = false;
			for (ASCCharacter* Character : GS->PlayerCharacterList)
			{
				ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character);
				if (Counselor && Counselor->IsAlive() && !Counselor->HasEscaped())
				{
					bHasAliveCounselors = true;
					break;
				}
			}

			if (!bHasAliveCounselors)
			{
				EndMatch();
			}
			// End the match when the killer leaves or if they've been out of bounds too long.
			else if (!GS->CurrentKiller || GS->CurrentKiller->bIsDying || GS->OutOfBoundsTime == 0)
			{
				// If the time runs out you don't earn the "not spotted" skull
				GS->Dossier->OnSpottedByAI();
				EndMatch();
			}
		}
	}

	Super::GameSecondTimer();
}

void ASCGameMode_SPChallenges::HandlePreMatchIntro()
{
	// Assign the player's picked Jason
	TSoftClassPtr<ASCKillerCharacter> KillerClass;
	UWorld* World = GetWorld();
	USCLocalPlayer* FirstLocalPlayer = CastChecked<USCLocalPlayer>(World->GetFirstLocalPlayerFromController());
	ASCPlayerController_SPChallenges* FirstPlayerController = CastChecked<ASCPlayerController_SPChallenges>(FirstLocalPlayer->PlayerController);
	ASCPlayerState* FirstPlayerState = CastChecked<ASCPlayerState>(FirstPlayerController->PlayerState);
#if WITH_EDITOR
	if (World->IsPlayInEditor())
	{
		KillerClass = KillerCharacterClasses[0];
	}
	else
#endif
	{
		USCCharacterSelectionsSaveGame* FirstPlayerSettings = FirstLocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>();
		TSoftClassPtr<ASCKillerCharacter> PickedKiller = FirstPlayerSettings->GetPickedKillerProfile().KillerCharacterClass;
		KillerClass =  PickedKiller ? PickedKiller : GetRandomKillerClass(FirstPlayerState);
	}
	ASCGameState_SPChallenges* GS = CastChecked<ASCGameState_SPChallenges>(GameState);
	GS->CurrentKillerPlayerState = FirstPlayerState;
	GS->CurrentKillerPlayerState->AsyncSetActiveCharacterClass(KillerClass);

	KillerClass.LoadSynchronous(); // TODO: Make Async
	GS->CurrentKillerPlayerState->ClientRequestGrabKills(KillerClass.Get());
	GS->CurrentKillerPlayerState->ClientRequestSelectedWeapon(KillerClass.Get());

	// Start the intro
	Super::HandlePreMatchIntro();
}

bool ASCGameMode_SPChallenges::CharacterEscaped(AController* CounselorController, ASCCounselorCharacter* CounselorCharacter)
{
	if (ASCGameState_SPChallenges* GS = CastChecked<ASCGameState_SPChallenges>(GameState))
	{
		GS->Dossier->OnCounselorEscaped();
	}

	return Super::CharacterEscaped(CounselorController, CounselorCharacter);
}

void ASCGameMode_SPChallenges::EndMatch()
{
	OnMatchEnd.Broadcast();

	Super::EndMatch();
}
