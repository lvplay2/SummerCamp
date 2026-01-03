// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_OfflineBots.h"

#include "SCCounselorCharacter.h"
#include "SCCounselorAIController.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCDriveableVehicle.h"
#include "SCGameState_OfflineBots.h"
#include "SCLocalPlayer.h"
#include "SCPlayerController_OfflineBots.h"
#include "SCPlayerState_OfflineBots.h"

ASCGameMode_OfflineBots::ASCGameMode_OfflineBots(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GameStateClass = ASCGameState_OfflineBots::StaticClass();
	PlayerControllerClass = ASCPlayerController_OfflineBots::StaticClass();
	PlayerStateClass = ASCPlayerState_OfflineBots::StaticClass();
}

void ASCGameMode_OfflineBots::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// Spawn counselors
	// Build a list of randomly selected unique counselors to spawn
	int32 NumCounselorsToSpawn = 7;
	const FString NumCounselorsString = UGameplayStatics::ParseOption(OptionsString, TEXT("NumCounselors"));
	if (!NumCounselorsString.IsEmpty())
	{
		NumCounselorsToSpawn = FCString::Atoi(*NumCounselorsString);
	}
	NumCounselorsToSpawn = FMath::Clamp(NumCounselorsToSpawn, 1, 7);
	const int32 CounselorClassCount = GetNumberOfCounselorClasses();
	check(CounselorClassCount >= NumCounselorsToSpawn);
	TArray<TSoftClassPtr<ASCCounselorCharacter>> PickedCharacters;
	PickedCharacters.Empty(NumCounselorsToSpawn);
	do 
	{
		PickedCharacters.AddUnique(CounselorCharacterClasses[FMath::RandHelper(CounselorClassCount)]);
	} while (PickedCharacters.Num() < NumCounselorsToSpawn);

	// Now spawn them
	for (TSoftClassPtr<ASCCounselorCharacter> CounselorClass : PickedCharacters)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ASCCounselorAIController* NewAI = GetWorld()->SpawnActor<ASCCounselorAIController>(SpawnParams))
		{
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(NewAI->PlayerState))
			{
				CounselorClass.LoadSynchronous(); // TODO: Make Async

				// Assign a random counselor, for now
				const ASCCounselorCharacter* DefaultCounselor = CounselorClass.Get()->GetDefaultObject<ASCCounselorCharacter>();
				PS->AsyncSetActiveCharacterClass(CounselorClass);
				PS->PlayerName = DefaultCounselor->GetCharacterName().ToString();
				RestartPlayer(NewAI);

				// Flag them as fully traveled
				PS->HasFullyTraveled(true);

				NumBots++;
			}
		}
	}
}

void ASCGameMode_OfflineBots::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (ASCCounselorAIController* NewCounselor = Cast<ASCCounselorAIController>(NewPlayer))
	{
		NewCounselor->RunBehaviorTree(CounselorBehaviorTree);
	}
}

void ASCGameMode_OfflineBots::GameSecondTimer()
{
	Super::GameSecondTimer();

	if (IsMatchInProgress())
	{
		// Check for end match
		ASCGameState_OfflineBots* GS = Cast<ASCGameState_OfflineBots>(GameState);
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
			else if (!GS->CurrentKiller || GS->CurrentKiller->bIsDying)
			{
				// End the match when the killer leaves
				EndMatch();
			}
		}
	}
}

void ASCGameMode_OfflineBots::HandlePreMatchIntro()
{
	// Assign the player's picked Jason
	TSoftClassPtr<ASCKillerCharacter> KillerClass;
	UWorld* World = GetWorld();
	USCLocalPlayer* FirstLocalPlayer = CastChecked<USCLocalPlayer>(World->GetFirstLocalPlayerFromController());
	ASCPlayerController_OfflineBots* FirstPlayerController = CastChecked<ASCPlayerController_OfflineBots>(FirstLocalPlayer->PlayerController);
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
	ASCGameState_OfflineBots* GS = CastChecked<ASCGameState_OfflineBots>(GameState);
	GS->CurrentKillerPlayerState = FirstPlayerState;
	GS->CurrentKillerPlayerState->AsyncSetActiveCharacterClass(KillerClass);

	KillerClass.LoadSynchronous(); // TODO: Make async
	GS->CurrentKillerPlayerState->ClientRequestGrabKills(KillerClass.Get());
	GS->CurrentKillerPlayerState->ClientRequestSelectedWeapon(KillerClass.Get());

	// Start the intro
	Super::HandlePreMatchIntro();
}

void ASCGameMode_OfflineBots::HandleWaitingPostMatchOutro()
{
	Super::HandleWaitingPostMatchOutro();

	// Store off player scoring data for the return to the lobby
	for (APlayerState* Player : GameState->PlayerArray)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(PS->GetOwner()))
			{
				FSCEndMatchPlayerInfo Info;
				Info.MatchScoreEvents = PS->GetEventCategoryScores();
				// No Badges are unlocked in offline bots

				PC->ClientMatchSummaryInfo(Info);
			}
		}
	}
}

bool ASCGameMode_OfflineBots::IsAllowedVehicleClass(TSubclassOf<ASCDriveableVehicle> VehicleClass) const
{
	if (Super::IsAllowedVehicleClass(VehicleClass))
	{
		// Only allow cars until boats are supported
		if (VehicleClass && VehicleClass->GetDefaultObject<ASCDriveableVehicle>()->VehicleType == EDriveableVehicleType::Car)
			return true;
	}

	return false;
}
