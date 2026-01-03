// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_Hunt.h"

#include "SCAIController.h"
#include "SCBackendRequestManager.h"
#include "SCContextKillDamageType.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCGameInstance.h"
#include "SCGameSession.h"
#include "SCGameState_Hunt.h"
#include "SCHidingSpot.h"
#include "SCHunterPlayerStart.h"
#include "SCInGameHUD_Hunt.h"
#include "SCKillerCharacter.h"
#include "SCPerkData.h"
#include "SCPlayerController_Hunt.h"
#include "SCPlayerStart.h"
#include "SCPlayerState_Hunt.h"
#include "SCSpecialItem.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCStatsAndScoringData.h"
#include "SCWalkieTalkie.h"
#include "SCWeapon.h"

ASCGameMode_Hunt::ASCGameMode_Hunt(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameStateClass = ASCGameState_Hunt::StaticClass();
	HUDClass = ASCInGameHUD_Hunt::StaticClass();
	PlayerControllerClass = ASCPlayerController_Hunt::StaticClass();
	PlayerStateClass = ASCPlayerState_Hunt::StaticClass();

	ModeName = TEXT("Hunt");

	MinimumPlayerCount = 2;
	MaxIdleTimeBeforeKick = 120.f;
}

void ASCGameMode_Hunt::HandleMatchHasStarted()
{
	// Broadcast BeginPlay and call RestartPlayer on all players
	Super::HandleMatchHasStarted();

	// Any players without a Pawn and ActiveCharacterClass are forced into spectator
	// This is because they clearly did not register with our previous lobby and so can not play
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ASCPlayerController* PC = Cast<ASCPlayerController>(*It);
		if (!PC || PC->GetPawn())
			continue;

		if (ASCPlayerState* PS = Cast<ASCPlayerState>(PC->PlayerState))
		{
			if (!PS->CanSpawnActiveCharacter())
			{
				Spectate(PC);
			}
		}
	}
}

void ASCGameMode_Hunt::HandleLeavingMap()
{
	Super::HandleLeavingMap();

	// Clear the SpawnedCharacterClass for everyone, so they are able to respawn on the next map
	for (APlayerState* Player : InactivePlayerArray)
	{
		if (ASCPlayerState_Hunt* PS = Cast<ASCPlayerState_Hunt>(Player))
		{
			PS->SpawnedCharacterClass = nullptr;
		}
	}
}

void ASCGameMode_Hunt::GameSecondTimer()
{
	// Tick match progression
	Super::GameSecondTimer();
	
	if (IsMatchInProgress())
	{
		// Check for end match
		ASCGameState_Hunt* GS = Cast<ASCGameState_Hunt>(GameState);
		if (GS->ElapsedInProgressTime > 10)
		{
			if ((GS->DeadCounselorCount+GS->EscapedCounselorCount) >= GS->SpawnedCounselorCount && ((!GS->bIsHunterSpawned && !IsSpawningHunter()) || GS->bHunterDied || GS->bHunterEscaped))
			{
				// End the match when all counselors are dead or have escaped
				EndMatch();
			}
			else if (!GS->CurrentKiller || GS->CurrentKiller->bIsDying)
			{
				// End the match when the killer leaves
				EndMatch();
			}
		}
	}
}

void ASCGameMode_Hunt::CharacterKilled(AController* Killer, AController* KilledPlayer, ASCCharacter* KilledCharacter, const UDamageType* const DamageType)
{
	Super::CharacterKilled(Killer, KilledPlayer, KilledCharacter, DamageType);
}

bool ASCGameMode_Hunt::CharacterEscaped(AController* CounselorController, ASCCounselorCharacter* CounselorCharacter)
{
	return Super::CharacterEscaped(CounselorController, CounselorCharacter);
}

bool ASCGameMode_Hunt::CanTeamKill() const
{
	if (ASCGameSession* SCGameSession = Cast<ASCGameSession>(GameSession))
	{
		return SCGameSession->IsPrivateMatch();
	}

	return false;
}

void ASCGameMode_Hunt::HandleWaitingPreMatch()
{
	Super::HandleWaitingPreMatch();

	ASCGameState_Hunt* MyGameState = Cast<ASCGameState_Hunt>(GameState);
	check(MyGameState->RemainingTime > 0); // Ensure we will have time for the request to come back
}

void ASCGameMode_Hunt::TickWaitingPreMatch()
{
	Super::TickWaitingPreMatch();

	// Wait for all player profiles to be received
	if (!HasReceivedAllPlayerProfiles())
		return;

	// Make sure we haven't already completed this
	if (bReceivedJasonSelectResponse)
		return;

	// Check if the JasonSelectRequest is pending or should just time out
	UWorld* World = GetWorld();
	if (JasonSelectRequest.IsValid())
	{
		if (World->GetRealTimeSeconds() - JasonSelectRealTimeSeconds >= PreMatchWaitTime*.75f) // Only wait a few seconds for the backend to respond
		{
			JasonSelectRequest = nullptr;
			bReceivedJasonSelectResponse = true;
		}
		return;
	}

	// Check if a killer has already been picked
	ASCGameSession* MyGameSession = Cast<ASCGameSession>(GameSession);
	USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
	if (GameInstance->HasPickedKillerPlayer() && MyGameSession->AllowKillerPicking())
	{
		bReceivedJasonSelectResponse = true;
		return;
	}

	// Check if we can send any requests
	USCBackendRequestManager* RequestManager = GameInstance->GetBackendRequestManager<USCBackendRequestManager>();
	if (!RequestManager->CanSendAnyRequests())
	{
		bReceivedJasonSelectResponse = true;
		return;
	}

	// Build request body
	uint32 PlayerPreferencesPackaged = 0;
	FString ContentString(TEXT("{\n\"PlayerSpawnPreferences\": [\n"));
	ASCGameState_Hunt* MyGameState = Cast<ASCGameState_Hunt>(GameState);
	for (int32 PlayerIndex = 0; PlayerIndex < MyGameState->PlayerArray.Num(); ++PlayerIndex)
	{
		if (ASCPlayerState_Hunt* PlayerState = Cast<ASCPlayerState_Hunt>(MyGameState->PlayerArray[PlayerIndex]))
		{
			if (ASCPlayerController* Controller = Cast<ASCPlayerController>(PlayerState->GetOwner()))
			{
				FString Preference(TEXT("None"));
				switch (PlayerState->GetSpawnPreference())
				{
				case ESCCharacterPreference::NoPreference: break;
				case ESCCharacterPreference::Counselor: Preference = TEXT("Counselor"); break;
				case ESCCharacterPreference::Killer: Preference = TEXT("Jason"); break;
				default: check(0); break;
				}
				if (PlayerPreferencesPackaged > 0)
					ContentString += TEXT(",\n");
				ContentString += FString::Printf(TEXT("{ \"AccountID\": \"%s\", \"Preference\": \"%s\" }"), *Controller->GetBackendAccountId().GetIdentifier(), *Preference);
				++PlayerPreferencesPackaged;
			}
		}
	}
	ContentString += TEXT("\n]\n}");
	if (PlayerPreferencesPackaged == 0)
	{
		// No killer to be picked
		bReceivedJasonSelectResponse = true;
		return;
	}

	// Create an process a request
	JasonSelectRequest = RequestManager->CreateSimpleRequest(RequestManager->ArbiterService_JasonSelect, FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::RequestJasonSelectionCallback));
	if (JasonSelectRequest.IsValid())
	{
		JasonSelectRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		JasonSelectRequest->SetContentAsString(ContentString);
		JasonSelectRequest->QueueRequest();

		// Track when the request was sent for timing out
		JasonSelectRealTimeSeconds = World->GetRealTimeSeconds();
	}
	else
	{
		// Failed to build a request
		bReceivedJasonSelectResponse = true;
	}
}

bool ASCGameMode_Hunt::CanCompleteWaitingPreMatch() const
{
	// Wait for all player profiles to be fetched
	if (!Super::CanCompleteWaitingPreMatch())
		return false;

	// Wait for the JasonSelectRequest to complete
	return bReceivedJasonSelectResponse;
}

void ASCGameMode_Hunt::HandlePreMatchIntro()
{
	UWorld* World = GetWorld();

	// Pick a killer
	ASCPlayerState* PickedKiller = nullptr;
	USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
	ASCGameSession* MyGameSession = Cast<ASCGameSession>(GameSession);
	ASCGameState_Hunt* MyGameState = Cast<ASCGameState_Hunt>(GameState);

	// Reset stats
	MyGameState->DeadCounselorCount = 0;
	MyGameState->EscapedCounselorCount = 0;

	// Check killer-picking first
	if (GameInstance->HasPickedKillerPlayer() && MyGameSession->AllowKillerPicking())
	{
		// Search for the picked killer player
		for (int32 PlayerIndex = 0; PlayerIndex < MyGameState->PlayerArray.Num(); ++PlayerIndex)
		{
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(MyGameState->PlayerArray[PlayerIndex]))
			{
				if (PS->IsPickedKiller())
				{
					MyGameState->KillerClass = PS->GetPickedKillerClass();
					if (MyGameState->KillerClass.IsNull())
					{
						// Fallback to random
						MyGameState->KillerClass = GetRandomKillerClass(PS);
					}
					if (!MyGameState->KillerClass.IsNull())
					{
						PickedKiller = PS;
						break;
					}
				}
			}
		}
	}

#if WITH_EDITOR
	if (!PickedKiller && World->IsPlayInEditor())
	{
		// Always make the first player the killer player in PIE
		PickedKiller = Cast<ASCPlayerState>(MyGameState->PlayerArray[0]);
		MyGameState->KillerClass = GetRandomKillerClass(PickedKiller);
	}
#endif

	// Check if we got a killer selected by the backend
	if (!PickedKiller && JasonSelectPlayer)
	{
		PickedKiller = JasonSelectPlayer;
		MyGameState->KillerClass = PickedKiller->GetPickedKillerClass();
		if (MyGameState->KillerClass.IsNull())
		{
			// Fallback to random
			MyGameState->KillerClass = GetRandomKillerClass(PickedKiller);
		}
	}

	if (!PickedKiller)
	{
		// Copy the PlayerArray and remove unusable entries as we go
		TArray<ASCPlayerState*> PossibleKillers;
		for (APlayerState* Player : MyGameState->PlayerArray)
		{
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
				PossibleKillers.Add(PS);
		}
		auto ShufflePossibleKillers = [&]() -> void
		{
			for (int32 Index = 0, LastIndex = PossibleKillers.Num()-1; Index <= LastIndex; ++Index)
			{
				const int32 SwapIndex = FMath::RandRange(0, LastIndex);
				if (Index != SwapIndex)
				{
					PossibleKillers.Swap(Index, SwapIndex);
				}
			}
		};

		// Shuffle the list to help randomize
		ShufflePossibleKillers();

		// When we have more than 1 option, remove the last person to be the killer
		if (GameInstance->LastKillerAccountId.IsValid() && PossibleKillers.Num() > 1)
		{
			for (int32 Index = 0; Index < PossibleKillers.Num(); ++Index)
			{
				if (!PossibleKillers[Index]->GetAccountId().IsValid() || PossibleKillers[Index]->GetAccountId() == GameInstance->LastKillerAccountId)
				{
					PossibleKillers.RemoveAtSwap(Index);
					break;
				}
			}
		}

		// Shuffle again
		ShufflePossibleKillers();

		// Remove random people with counselor preference, ensuring we keep at least one person in the list
		TArray<ASCPlayerState*> CounselorPreferencePlayers;
		for (int32 Index = 0; Index < PossibleKillers.Num(); ++Index)
		{
			if (ASCPlayerState_Hunt* HuntKiller = Cast<ASCPlayerState_Hunt>(PossibleKillers[Index]))
			{
				if (HuntKiller->GetSpawnPreference() == ESCCharacterPreference::Counselor)
				{
					CounselorPreferencePlayers.Add(HuntKiller);
				}
			}
		}
		while (PossibleKillers.Num() > 1 && CounselorPreferencePlayers.Num() > 0)
		{
			int32 RandomIndex = FMath::RandHelper(CounselorPreferencePlayers.Num());
			PossibleKillers.Remove(CounselorPreferencePlayers[RandomIndex]);
			CounselorPreferencePlayers.RemoveAt(RandomIndex);
		}

		// Shuffle the list again
		ShufflePossibleKillers();

		// Add redundant entries based on spawn preference to increase their odds
		for (int32 Index = PossibleKillers.Num()-1; Index >= 0; --Index)
		{
			if (ASCPlayerState_Hunt* HuntKiller = Cast<ASCPlayerState_Hunt>(PossibleKillers[Index]))
			{
				if (HuntKiller->GetSpawnPreference() == ESCCharacterPreference::Killer)
				{
					// Add twice
					PossibleKillers.Add(HuntKiller);
					PossibleKillers.Add(HuntKiller);
					PossibleKillers.Add(HuntKiller);
					PossibleKillers.Add(HuntKiller);
					PossibleKillers.Add(HuntKiller);
				}
				else if (HuntKiller->GetSpawnPreference() == ESCCharacterPreference::NoPreference)
				{
					// Add twice
					PossibleKillers.Add(HuntKiller);
					PossibleKillers.Add(HuntKiller);
				}
				else
				{
					check(HuntKiller->GetSpawnPreference() == ESCCharacterPreference::Counselor);
					// No addition, lowest probability
				}
			}
		}

		// Shuffle the list a bunch of times, now that is has more entries this should have more effect
		for (int32 Pass = 0; Pass < 4; ++Pass)
			ShufflePossibleKillers();

		if (PossibleKillers.Num() > 0)
		{
			const int32 RandomPick = FMath::RandHelper(PossibleKillers.Num());
			ASCPlayerState* PS = Cast<ASCPlayerState>(PossibleKillers[RandomPick]);
			MyGameState->KillerClass = PS->GetPickedKillerClass();
			if (MyGameState->KillerClass.IsNull())
			{
				// Fallback to random
				MyGameState->KillerClass = GetRandomKillerClass(PS);
			}
			PickedKiller = PS;
		}
	}
	if (MyGameState->PlayerArray.Num() > 0)
	{
		ensure(MyGameState->KillerClass);

		// Store off who the killer is
		MyGameState->CurrentKillerPlayerState = PickedKiller;

		if (ensure(PickedKiller))
		{
			// Store them as being the last killer
			GameInstance->LastKillerAccountId = PickedKiller->GetAccountId();

			// Assign them their killer character class
			PickedKiller->AsyncSetActiveCharacterClass(MyGameState->KillerClass);

			MyGameState->KillerClass.LoadSynchronous(); // TODO: Make Async

			// Request their grab kills
			PickedKiller->ClientRequestGrabKills(MyGameState->KillerClass.Get());

			// request their selected weapon
			PickedKiller->ClientRequestSelectedWeapon(MyGameState->KillerClass.Get());

		}
	}

	// Hand out all of the Counselor classes
	for (int32 PlayerIndex = 0; PlayerIndex < MyGameState->PlayerArray.Num(); ++PlayerIndex)
	{
		ASCPlayerState* PS = Cast<ASCPlayerState>(MyGameState->PlayerArray[PlayerIndex]);
		if (PS != PickedKiller)
		{
			UE_LOG(LogSC, Log, TEXT("Loading Counselor class for %s"), *PS->PlayerName);

			TSoftClassPtr<ASCCounselorCharacter> RequestedCounselor = PS->GetPickedCounselorClass();
			if (RequestedCounselor.IsNull())
			{
				// Fallback to a random counselor
				RequestedCounselor = GetRandomCounselorClass(PS);

				if (!RequestedCounselor.IsNull())
				{
					UE_LOG(LogSC, Log, TEXT("Random class %s chosen for %s"), *RequestedCounselor.GetAssetName(), *PS->PlayerName);

					// Set this as their active character class
					PS->AsyncSetActiveCharacterClass(RequestedCounselor);

					// Request Perks and Outfits from client
					PS->ClientRequestPerksAndOutfit(RequestedCounselor);
				}
			}
			else
			{
				// Set this as their active character class
				PS->AsyncSetActiveCharacterClass(RequestedCounselor);

				// Set Active perks
				PS->SetActivePerks(PS->GetPickedPerks());
			}
		}
	}

	// Broadcast BeginPlay
	Super::HandlePreMatchIntro();
}

void ASCGameMode_Hunt::HandleWaitingPostMatchOutro()
{
	Super::HandleWaitingPostMatchOutro();

	UWorld* World = GetWorld();
	ASCGameState_Hunt* GS = GetSCGameState();

	bool bAllCounselorsEscaped = true;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		// Check to see if this player won or lost
		bool IsWinnner = false;
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
		{
			if (ASCPlayerState_Hunt* PS = Cast<ASCPlayerState_Hunt>(PC->PlayerState))
			{
				if (!PS->DidSpawnIntoMatch())
					continue; // this player never spawned in game so don't give them any XP or achievements

				// Track class played and award any related achievements
				PS->OnMatchEnded();
				
				// Killer
				if (PS->IsActiveCharacterKiller())
				{
					if (PS->DidKillAllCounselors(GS->SpawnedCounselorCount, GS->bHunterDied) || (GS->DeadCounselorCount >= GS->SpawnedCounselorCount))
					{
						IsWinnner = true;
						HandleScoreEvent(PS, JasonKilledAllCounselorsScoreEvent);
					}
				}
				// Counselors
				else
				{
					if (!PS->GetEscaped())
						bAllCounselorsEscaped = false;

					if (!PS->GetIsDead())
					{
						IsWinnner = true;

						// Score alive time
						const float TimeAlive = GS->ElapsedInProgressTime / 60.f;
						HandleScoreEvent(PS, CounselorAliveTimeBonusScoreEvent, TimeAlive);
					}

					// Award badges for surviving the match
					if (!PS->GetEscaped() && !PS->GetIsDead() && !PS->GetSpectating())
					{
						PS->EarnedBadge(SurviveBadge);

						if (PS->GetHasBeenScared())
							PS->EarnedBadge(SurviveWithFearBadge);
					}
				}

				if (!PS->GetHasTakenDamage())
				{
					HandleScoreEvent(PS, UnscathedScoreEvent);
				}
			}

			HandleScoreEvent(PC->PlayerState, CompletedMatchScoreEvent);
		}

		(*It)->GameHasEnded((*It)->GetPawn(), IsWinnner);
	}

	if (bAllCounselorsEscaped)
	{
		HandleScoreEvent(nullptr, TeamEscapedScoreEvent);
	}

	// Store off player scoring data for the return to the lobby
	for (APlayerState* Player : GameState->PlayerArray)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(PS->GetOwner()))
			{
				FSCEndMatchPlayerInfo Info;
				Info.MatchScoreEvents = PS->GetEventCategoryScores();
				Info.BadgesUnlocked = PS->GetBadgesUnlocked();

				PC->ClientMatchSummaryInfo(Info);
			}
		}
	}

	// Don't attempt to spawn the hunter if the game is ending
	GetWorldTimerManager().ClearTimer(TimerHandle_HunterTimer);
}

void ASCGameMode_Hunt::RequestJasonSelectionCallback(int32 ResponseCode, const FString& ResponseContent)
{
	bReceivedJasonSelectResponse = true;

	// When the request handle is invalid that means we dropped the request to timeout in CanCompleteWaitingPreMatch
	if (!JasonSelectRequest.IsValid())
		return;

	// Clear our request handle to signal that we are done
	JasonSelectRequest = nullptr;

	// Check for a valid response
	if (ResponseCode == 200 && !ResponseContent.IsEmpty())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			FString StringAccountId;
			if (JsonObject->TryGetStringField(TEXT("SelectedID"), StringAccountId))
			{
				const FILLBackendPlayerIdentifier ParsedAccountId(StringAccountId);
				UWorld* World = GetWorld();
				for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
				{
					if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
					{
						if (PC->GetBackendAccountId() == ParsedAccountId)
						{
							JasonSelectPlayer = Cast<ASCPlayerState>(PC->PlayerState);
							break;
						}
					}
				}
			}
		}
	}
}

ASCGameState_Hunt* ASCGameMode_Hunt::GetSCGameState()
{
	return GetGameState<ASCGameState_Hunt>();
}
