// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_Paranoia.h"

#include "SCCabinet.h"
#include "SCContextKillDamageType.h"
#include "SCGameInstance.h"
#include "SCGameSession.h"
#include "SCGameState_Paranoia.h"
#include "SCImposterOutfit.h"
#include "SCParanoiaKillerCharacter.h"
#include "SCPlayerState_Paranoia.h"
#include "SCPlayerController_Paranoia.h"
#include "SCParanoiaHUD.h"

ASCGameMode_Paranoia::ASCGameMode_Paranoia(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameStateClass = ASCGameState_Paranoia::StaticClass();
	PlayerControllerClass = ASCPlayerController_Paranoia::StaticClass();
	PlayerStateClass = ASCPlayerState_Paranoia::StaticClass();
	HUDClass = ASCParanoiaHUD::StaticClass();

	ModeName = TEXT("Paranoia");

	MinimumPlayerCount = 2;
	MaxIdleTimeBeforeKick = 120.f;

	KillerCharacterClass = TSoftClassPtr<ASCParanoiaKillerCharacter>(FSoftObjectPath(TEXT("Pawn'/Game/Characters/Killers/Jason/Paranoia/Jason_Paranoia.Jason_Paranoia_C'")));

	static ConstructorHelpers::FClassFinder<ASCImposterOutfit> ImposterOutfitClassObject(TEXT("/Game/Blueprints/Items/Special/ParanoiaOutfit.ParanoiaOutfit_C"));
	ParanoiaOutfitClass = ImposterOutfitClassObject.Class;
	// Set Point Events
	static ConstructorHelpers::FClassFinder<USCParanoiaScore> KilledJasonObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/PointEvents/KilledJason.KilledJason_C'"));
	KilledJasonPointEvent = KilledJasonObject.Class;
	static ConstructorHelpers::FClassFinder<USCParanoiaScore> JasonKillObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/PointEvents/JasonKill.JasonKill_C'"));
	JasonKillPointEvent = JasonKillObject.Class;
	static ConstructorHelpers::FClassFinder<USCParanoiaScore> RoyKillObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/PointEvents/RoyKill.RoyKill_C'"));
	RoyKillPointEvent = RoyKillObject.Class;
	static ConstructorHelpers::FClassFinder<USCParanoiaScore> TeamKillObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/PointEvents/TeamKill.TeamKill_C'"));
	KilledTeammatePointEvent = TeamKillObject.Class;
	static ConstructorHelpers::FClassFinder<USCParanoiaScore> TeamDamageObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/PointEvents/TeamDamage.TeamDamage_C'"));
	WoundedTeammatePointEvent = TeamDamageObject.Class;


	static ConstructorHelpers::FClassFinder<USCScoringEvent> FriendlyKillObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/FriendlyKill_Paranoia.FriendlyKill_Paranoia_C'"));
	FriendlyKillXPEvent = FriendlyKillObject.Class;

	static ConstructorHelpers::FClassFinder<USCScoringEvent> FriendlyHitObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/FriendlyHit_Paranoia.FriendlyHit_Paranoia_C'"));
	FriendlyHitXPEvent = FriendlyHitObject.Class;

	static ConstructorHelpers::FClassFinder<USCScoringEvent> KillAsCounselorObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/KillAsCounselor_Paranoia.KillAsCounselor_Paranoia_C'"));
	KillAsCounselorXPEvent = KillAsCounselorObject.Class;

	static ConstructorHelpers::FClassFinder<USCScoringEvent> KillAsKillerObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/KillAsKiller_Paranoia.KillAsKiller_Paranoia_C'"));
	KillAsKillerXPEvent = KillAsKillerObject.Class;

	static ConstructorHelpers::FClassFinder<USCScoringEvent> ImposterKillObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/ImposterKill_Paranoia.ImposterKill_Paranoia_C'"));
	ImposterKillXPEvent = ImposterKillObject.Class;

	static ConstructorHelpers::FClassFinder<USCScoringEvent> HighestScoreObject(TEXT("Blueprint'/Game/Blueprints/ScoreEvents/HighestScore_Paranoia.HighestScore_Paranoia_C'"));
	HighestScoreXPEvent = HighestScoreObject.Class;
}

void ASCGameMode_Paranoia::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// spawn paranoia killer
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.Owner = nullptr;
	Params.Instigator = Instigator;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	KillerCharacterClass.LoadSynchronous(); // TODO: Make Async

	// spawn killer underground
	ParanoiaKiller = GetWorld()->SpawnActor<ASCParanoiaKillerCharacter>(KillerCharacterClass.Get(), FTransform(FRotator::ZeroRotator, FVector(0, 0, 300.0f)),Params);
	if (ParanoiaKiller)
	{
		ParanoiaKiller->SetCharacterDisabled(true);
	}
}

void ASCGameMode_Paranoia::HandlePreMatchIntro()
{
	UWorld* World = GetWorld();

	GetSCGameState()->DeadCounselorCount = 0;
	GetSCGameState()->EscapedCounselorCount = 0;

	// Pick a killer
	USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
	ASCGameSession* MyGameSession = Cast<ASCGameSession>(GameSession);
	ASCGameState_Paranoia* MyGameState = Cast<ASCGameState_Paranoia>(GameState);

	// Hand out all of the Counselor classes
	for (int32 PlayerIndex = 0; PlayerIndex < MyGameState->PlayerArray.Num(); ++PlayerIndex)
	{
		ASCPlayerState* PS = Cast<ASCPlayerState>(MyGameState->PlayerArray[PlayerIndex]);

		TSoftClassPtr<ASCCounselorCharacter> RequestedCounselor = PS->GetPickedCounselorClass();
		if (RequestedCounselor.IsNull())
		{
			// Fallback to a random counselor
			RequestedCounselor = GetRandomCounselorClass(PS);

			if (RequestedCounselor)
			{
				// Set this as their active character class
				PS->AsyncSetActiveCharacterClass(RequestedCounselor);

				// Request Perks and Outfits from client
				RequestedCounselor.LoadSynchronous(); // TODO: Make Async
				PS->ClientRequestPerksAndOutfit(RequestedCounselor.Get());
			}
		}
		else if (RequestedCounselor)
		{
			// Set this as their active character class
			PS->AsyncSetActiveCharacterClass(RequestedCounselor);

			// Set Active perks
			PS->SetActivePerks(PS->GetPickedPerks());
		}
	}

	// Broadcast BeginPlay
	ASCGameMode_Online::HandlePreMatchIntro();
}

void ASCGameMode_Paranoia::HandleWaitingPostMatchOutro()
{
	ASCGameMode_Online::HandleWaitingPostMatchOutro();

	UWorld* World = GetWorld();
	ASCGameState_Paranoia* GS = GetGameState<ASCGameState_Paranoia>();
	TArray<ASCCharacter*> AllPlayers = GS->PlayerCharacterList;

	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		// Check to see if this player won or lost
		bool IsWinner = true;
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
		{
			if (ASCPlayerState_Paranoia* PS = Cast<ASCPlayerState_Paranoia>(PC->PlayerState))
			{
				if (!PS->DidSpawnIntoMatch())
					continue; // this player never spawned in game so don't give them any XP or achievements

				// Track class played and award any related achievements
				PS->OnMatchEnded();

				// Find out if we have the high score! WTF do we do about ties???
				for (ASCCharacter* Player : AllPlayers)
				{
					if (!Player)
						continue;

					if (ASCPlayerState_Paranoia* OtherPS = Cast<ASCPlayerState_Paranoia>(Player->GetPlayerState()))
					{
						if (OtherPS == PS)
							continue;

						if (OtherPS->GetScore() > PS->GetScore())
							IsWinner = false;
					}
				}

				if (!PS->GetHasTakenDamage())
				{
					HandleScoreEvent(PS, UnscathedScoreEvent);
				}
			}

			HandleScoreEvent(PC->PlayerState, CompletedMatchScoreEvent);
		}

		(*It)->GameHasEnded((*It)->GetPawn(), IsWinner);

		if (IsWinner)
		{
			HandleScoreEvent((*It)->PlayerState, HighestScoreXPEvent);
		}
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

void ASCGameMode_Paranoia::GameSecondTimer()
{
	ASCGameMode_Online::GameSecondTimer();

	if (IsMatchInProgress())
	{
		// Check for end match
		ASCGameState_Paranoia* GS = Cast<ASCGameState_Paranoia>(GameState);
		if (GS->ElapsedInProgressTime > 10)
		{
			// Everybody is a counselor. if tommy has been called we need to assume another counselor has spawned.
			if ((GS->DeadCounselorCount) >= ((GS->bIsHunterSpawned && !GS->bHunterDied) ? GS->SpawnedCounselorCount : GS->SpawnedCounselorCount - 1))
			{
				// End the match when all counselors are dead or have escaped
				EndMatch();
			}
		}
	}
}

void ASCGameMode_Paranoia::CharacterKilled(AController* Killer, AController* KilledPlayer, ASCCharacter* KilledCharacter, const UDamageType* const DamageType)
{
	ASCPlayerState_Paranoia* KillerPlayerState = Killer ? Cast<ASCPlayerState_Paranoia>(Killer->PlayerState) : nullptr;
	ASCPlayerState_Paranoia* VictimPlayerState = KilledPlayer ? Cast<ASCPlayerState_Paranoia>(KilledPlayer->PlayerState) : nullptr;

	// We need the killer's controller and Character.
	ASCPlayerController_Paranoia* KillerController = Cast<ASCPlayerController_Paranoia>(Killer);
	ASCCharacter* KillerCharacter = KillerController ? Cast<ASCCharacter>(KillerController->GetPawn()) : nullptr;

	// Cast both to counselors for kill criteria testing.
	ASCCounselorCharacter* KillerCounselorCharacter = Cast<ASCCounselorCharacter>(KillerCharacter);
	ASCCounselorCharacter*VictimCounselorCharacter = Cast<ASCCounselorCharacter>(KilledCharacter);

	if (VictimPlayerState)
	{
		// Notify everybody of their "role" in this death
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			if (ASCPlayerState* TestPS = Cast<ASCPlayerState>((*It)->PlayerState))
			{
				if (TestPS == KillerPlayerState && TestPS == VictimPlayerState)
				{
					TestPS->InformAboutSuicide(DamageType);
				}
				else if (TestPS == KillerPlayerState)
				{
					TestPS->InformAboutKill(DamageType, VictimPlayerState);
				}
				else if (TestPS == VictimPlayerState)
				{
					TestPS->InformAboutKilled(KillerPlayerState, DamageType);
				}
				else
				{
					TestPS->InformAboutDeath(KillerPlayerState, DamageType, VictimPlayerState);
				}
			}
		}
	}

	// Scoring
	bool bWasContextKill = DamageType ? DamageType->IsA<USCContextKillDamageType>() : false;
	ASCCharacter* Character = nullptr;
	if (KilledPlayer && (Character = Cast<ASCCharacter>(KilledPlayer->GetCharacter())) != nullptr)
	{
		// Looks like we died while in a context kill without receiving the CK info... DC?
		if (Character->IsInContextKill())
		{
			bWasContextKill = true;
		}
	}
	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		if (!bWasContextKill)
			KillerPlayerState->ScoreKill(VictimPlayerState, /*PlayerKillFullScore*/0, DamageType);

		// Are we currently a killer character or a counselor holding the imposter outfit?
		if (KillerCharacter->IsA(ASCKillerCharacter::StaticClass()) || (KillerCounselorCharacter && KillerCounselorCharacter->HasSpecialItem(ASCImposterOutfit::StaticClass())))
		{
			if (!bWasContextKill)
			{
				HandleScoreEvent(KillerPlayerState, JasonWeaponKillScoreEvent);
			}

			HandleScoreEvent(KillerPlayerState, KillCounselorScoreEvent);

			// Are we jason-boy or roy-boy
			if (KillerCharacter->IsA(ASCKillerCharacter::StaticClass()))
			{
				// Kill as Jason
				HandleScoreEvent(KillerPlayerState, KillAsKillerXPEvent);
				HandlePointEvent(KillerPlayerState, JasonKillPointEvent);
			}
			else
			{
				// Kill as counselor
				HandleScoreEvent(KillerPlayerState, KillAsCounselorXPEvent);
				HandlePointEvent(KillerPlayerState, RoyKillPointEvent);
			}
		}
		else if (VictimPlayerState)
		{
			// Did we kill a jason or a counselor with the imposter item?
			if (KilledCharacter->IsA(ASCKillerCharacter::StaticClass()) || (VictimCounselorCharacter && VictimCounselorCharacter->HasSpecialItem(ASCImposterOutfit::StaticClass())))
			{
				// Killed Jason
				HandleScoreEvent(KillerPlayerState, ImposterKillXPEvent);
				HandlePointEvent(KillerPlayerState, KilledJasonPointEvent);
			}
			else
			{
				// Killed another counselor
				HandleScoreEvent(KillerPlayerState, FriendlyKillXPEvent);
				HandlePointEvent(KillerPlayerState, KilledTeammatePointEvent);
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, /*DeathScore*/0, DamageType);

		// Score alive time
		ASCGameState* GS = Cast<ASCGameState>(GameState);
		const float TimeAlive = GS->ElapsedInProgressTime / 60.f;
		HandleScoreEvent(VictimPlayerState, CounselorAliveTimeBonusScoreEvent, TimeAlive);
	}

	if (KilledCharacter)
	{
		 // Tell players to perform death cleanup
		if (ASCPlayerController_Paranoia* PC = Cast<ASCPlayerController_Paranoia>(KilledPlayer))
		{
			PC->CLIENT_OnCharacterDeath(KilledCharacter);

			if (KilledCharacter->IsA(ASCKillerCharacter::StaticClass()))
			{
				PC->SERVER_TransformIntoCounselor();
			}
			else
			{
				Spectate(KilledPlayer);
			}
		}
	}

	if (ASCCounselorCharacter* KilledCounselor = Cast<ASCCounselorCharacter>(KilledCharacter))
	{
		ASCGameState* GS = Cast<ASCGameState>(GameState);
		if (KilledCounselor->bIsHunter)
		{
			check(!GS->bHunterDied);
			GS->bHunterDied = true;
			++GS->DeadCounselorCount; // 02/03/2021 - re-enable Hunter Selection
		}
		else
		{
			++GS->DeadCounselorCount;

			if (CanSpawnHunter())
			{
				HunterToSpawn = ChooseHunter();
				if (HunterToSpawn == KilledPlayer)
				{
					FTimerDelegate Delegate;
					Delegate.BindLambda([this]()
					{
						if (!IsValid(HunterToSpawn))
							HunterToSpawn = ChooseHunter();

						if (HunterToSpawn)
							SpawnHunter();
						else
							StartHunterTimer();
					});

					if (GetWorld())
						GetWorldTimerManager().SetTimer(TimerHandle_HunterTimer, Delegate, GS->HunterDelayTime, false);
				}
				else
				{
					SpawnHunter();
				}
			}
		}
	}
}

void ASCGameMode_Paranoia::HandlePlayerTakeDamage(AController* Hitter, AController* Victim)
{
	if (!Hitter || !Victim || Hitter == Victim)
		return;

	// Cast both to counselors for kill criteria testing.
	ASCCounselorCharacter* KillerCounselorCharacter = Cast<ASCCounselorCharacter>(Hitter->GetPawn());
	ASCCounselorCharacter*VictimCounselorCharacter = Cast<ASCCounselorCharacter>(Victim->GetPawn());

	if  (
		(Hitter->GetPawn()->IsA(ASCKillerCharacter::StaticClass()) || (KillerCounselorCharacter && KillerCounselorCharacter->HasSpecialItem(ASCImposterOutfit::StaticClass()))) ||
		(Victim->GetPawn()->IsA(ASCKillerCharacter::StaticClass()) || (VictimCounselorCharacter && VictimCounselorCharacter->HasSpecialItem(ASCImposterOutfit::StaticClass())))
		)
		return;

	HandleScoreEvent(Hitter->PlayerState, FriendlyHitXPEvent);
	HandlePointEvent(Hitter->PlayerState, WoundedTeammatePointEvent);
}

void ASCGameMode_Paranoia::HandlePointEvent(APlayerState* ScoringPlayer, TSubclassOf<USCParanoiaScore> ScoreEventClass, const float ScoreModifier/* = 1.f*/)
{
	if (!ScoreEventClass)
		return;

	USCParanoiaScore* ScoreEventData = ScoreEventClass->GetDefaultObject<USCParanoiaScore>();

	if (ScoringPlayer)
	{
		if (ASCPlayerState_Paranoia* PS = Cast<ASCPlayerState_Paranoia>(ScoringPlayer))
		{
			if (PS->DidSpawnIntoMatch())
			{
				PS->HandleParanoiaPointEvent(ScoreEventData, ScoreModifier);
			}
		}
	}
}

void ASCGameMode_Paranoia::SpawnRepairItems()
{
	// spawn the paranoia outfit instead of repair items since this mode doenst have any repair escapes
	if (ParanoiaOutfitClass)
	{
		TArray<uint8> IndexArray;

		for (uint8 i(0); i<Cabinets.Num(); ++i)
			IndexArray.Add(i);

		IndexArray.Sort([this](const int Item1, const int Item2)
		{
			return FMath::FRand() > 0.5f;
		});


		for (uint8 i(0); i<Cabinets.Num(); ++i)
		{
			if (ASCCabinet* Cabinet = Cabinets[IndexArray[i]])
			{
				if (!Cabinet->IsCabinetFull())
				{
					if (Cabinet->SpawnItem(ParanoiaOutfitClass))
						return;
				}
			}
		}
	}

	ensureMsgf(false, TEXT("Paranoia outfit class failed to spawn!"));
}

void ASCGameMode_Paranoia::OnPlayerTransformed(ASCCharacter* NewCharacter, ASCCharacter* PreviousCharacter)
{
	UWorld* World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		if (ASCPlayerController_Paranoia* PC = Cast<ASCPlayerController_Paranoia>(*It))
		{
			PC->CLIENT_OnTransform(NewCharacter, PreviousCharacter);
		}
	}
}