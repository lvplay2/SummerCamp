// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameState.h"

#include "ILLGameBlueprintLibrary.h"

#include "SCActorSpawnerComponent.h"
#include "SCBackendRequestManager.h"
#include "SCCounselorCharacter.h"
#include "SCGameInstance.h"
#include "SCInGameHUD.h"
#include "SCKillerCharacter.h"
#include "SCGameMode.h"
#include "SCGameSession.h"
#include "SCGameStateLocalMessage.h"
#include "SCMusicComponent.h"
#include "SCPowerPlant.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCRoadPoint.h"
#include "SCStaticSpectatorCamera.h"
#include "SCWorldSettings.h"
#include "SCPoliceSpawner.h"
#include "SCDriveableVehicle.h"
#include "SCCBRadio.h"
#include "SCPhoneJunctionBox.h"
#include "SCVoiceOverComponent.h"

ASCGameState::ASCGameState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, HunterTimeRemaining(-1)
, PoliceTimeRemaining(-1)
{
	GameStateMessageClass = USCGameStateLocalMessage::StaticClass();

	PowerPlant = CreateDefaultSubobject<USCPowerPlant>(TEXT("PowerPlant"));

	XPModifier = 1.0f;

	bPlayJasonAbilityUnlockSounds = true;

	NormalSearchDistanceIgnoreTime = 600.f; // 10 min
	HardSearchDistanceIgnoreTime = 900.f; // 15 min
}

void ASCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCGameState, CurrentKillerPlayerState);

	DOREPLIFETIME(ASCGameState, ElapsedInProgressTime);
	DOREPLIFETIME(ASCGameState, RemainingTime);
	DOREPLIFETIME(ASCGameState, SpawnedCounselorCount);
	DOREPLIFETIME(ASCGameState, bHostCanPickKiller);
	DOREPLIFETIME(ASCGameState, XPModifier);
	DOREPLIFETIME(ASCGameState, HunterTimeRemaining);
	DOREPLIFETIME(ASCGameState, bIsHunterSpawned);
	DOREPLIFETIME(ASCGameState, EscapedCounselorCount);
	DOREPLIFETIME(ASCGameState, DeadCounselorCount);
	DOREPLIFETIME(ASCGameState, PoliceTimeRemaining);

	DOREPLIFETIME(ASCGameState, Car4SeatVehicle);
	DOREPLIFETIME(ASCGameState, Car2SeatVehicle);
	DOREPLIFETIME(ASCGameState, BoatVehicle);
	DOREPLIFETIME(ASCGameState, CBRadio);
	DOREPLIFETIME(ASCGameState, Phone);
	DOREPLIFETIME(ASCGameState, KillerClass);
	DOREPLIFETIME(ASCGameState, Car4SeatEscaped);
	DOREPLIFETIME(ASCGameState, Car2SeatEscaped);
	DOREPLIFETIME(ASCGameState, BoatEscaped);
	DOREPLIFETIME(ASCGameState, bNavCardRepaired);
	DOREPLIFETIME(ASCGameState, bCoreRepaired);
}

void ASCGameState::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority)
	{
		// request to get the xp modifier bonus
		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
		USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
		RequestManager->GetModifiers(FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnReceivedModifiersCallback));

		// find all of the item references
		for (TActorIterator<ASCDriveableVehicle> It(GetWorld(), ASCDriveableVehicle::StaticClass()); It; ++It)
		{
			if (ASCDriveableVehicle* Vehicle = Cast<ASCDriveableVehicle>(*It))
			{
				if (Vehicle->VehicleType == EDriveableVehicleType::Car)
				{
					if (Vehicle->Seats.Num() > 2)
						Car4SeatVehicle = Vehicle;
					else
						Car2SeatVehicle = Vehicle;
				}
				else if (Vehicle->VehicleType == EDriveableVehicleType::Boat)
				{
					BoatVehicle = Vehicle;
				}
			}
		}

		for (TActorIterator<ASCCBRadio> It(GetWorld(), ASCCBRadio::StaticClass()); It; ++It)
		{
			CBRadio = Cast<ASCCBRadio>(*It);
			break;
		}

		for (TActorIterator<ASCPhoneJunctionBox> It(GetWorld(), ASCPhoneJunctionBox::StaticClass()); It; ++It)
		{
			Phone = Cast<ASCPhoneJunctionBox>(*It);
			break;
		}
	}
}

bool ASCGameState::HasMatchStarted() const
{
	if (!Super::HasMatchStarted())
		return false;

	return (GetMatchState() != MatchState::WaitingPreMatch && GetMatchState() != MatchState::PreMatchIntro);
}

bool ASCGameState::HasMatchEnded() const
{
	if (GetMatchState() == MatchState::PostMatchOutro)
		return true;

	return Super::HasMatchEnded();
}

void ASCGameState::OnRep_MatchState()
{
	if (MatchState == MatchState::WaitingPreMatch)
	{
		HandleWaitingPreMatch();
	}
	else if (MatchState == MatchState::PreMatchIntro)
	{
		HandlePreMatchIntro();
	}
	else if (MatchState == MatchState::WaitingPostMatchOutro)
	{
		HandleWaitingPostMatchOutro();
	}
	else if (MatchState == MatchState::PostMatchOutro)
	{
		HandlePostMatchOutro();
	}
	
	// Update the local character HUD for these state changes
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			HUD->UpdateCharacterHUD();
		}
	}

	Super::OnRep_MatchState();
}

void ASCGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (UWorld* World = GetWorld())
	{
		if (Role == ROLE_Authority)
		{
			// Let members know if the host can pick a killer
			ASCGameSession* GS = World->GetAuthGameMode() ? Cast<ASCGameSession>(World->GetAuthGameMode()->GameSession) : nullptr;
			if (ensure(GS))
				bHostCanPickKiller = GS->AllowKillerPicking();
		}

		// Keep track of spectator cameras in the level
		for (TActorIterator<ASCStaticSpectatorCamera> It(World); It; ++It)
		{
			StaticSpectatorCameras.AddUnique(*It);
		}

		for (TActorIterator<ASCRoadPoint> It(World); It; ++It)
		{
			RoadPoints.AddUnique(*It);
		}

		// Catch actor spawner components already created
		for (TObjectIterator<USCActorSpawnerComponent> It; It; ++It)
		{
			USCActorSpawnerComponent* Component = *It;
			if (Component->GetWorld() != World)
			{
				continue;
			}
			ActorSpawnerList.AddUnique(*It);
		}
	}
}

void ASCGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	UWorld* World = GetWorld();

	if (Role == ROLE_Authority)
	{
		// No longer allowing killer-picking
		ASCGameSession* GS = World->GetAuthGameMode() ? Cast<ASCGameSession>(World->GetAuthGameMode()->GameSession) : nullptr;
		if (ensure(GS))
			bHostCanPickKiller = false;
	}

	// Pre-fill our character list with any characters that have already been loaded in
	for (FConstPawnIterator PawnIt = World->GetPawnIterator(); PawnIt; ++PawnIt)
	{
		ASCCharacter* PlayerCharacter = Cast<ASCCharacter>(*PawnIt);
		if (IsValid(PlayerCharacter))
			PlayerCharacterList.AddUnique(PlayerCharacter);
	}

	MatchStartTime = FDateTime::UtcNow().ToUnixTimestamp();
}

void ASCGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	MatchEndTime = FDateTime::UtcNow().ToUnixTimestamp();
}

void ASCGameState::HandleLeavingMap()
{
	Super::HandleLeavingMap();

	if (Role == ROLE_Authority)
	{
		// Unload the random sub-level
		ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());
		WorldSettings->UnloadRandomLevel();
	}

	// Cleanup references
	CurrentKiller = nullptr;
	PlayerCharacterList.Empty();
	StaticSpectatorCameras.Empty();
	RoadPoints.Empty();
	PowerPlant = nullptr;
	ActorSpawnerList.Empty();
	WalkieTalkiePlayers.Empty();
}

void ASCGameState::CharacterInitialized(AILLCharacter* Character)
{
	if (PlayerCharacterList.Contains(Character))
		return;

	Super::CharacterInitialized(Character);

	if (ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character))
	{
		PlayerCharacterList.AddUnique(SCCharacter);

		if (Character->IsA(ASCKillerCharacter::StaticClass()))
		{
			CurrentKiller = Cast<ASCKillerCharacter>(Character);
		}
		else if (Role == ROLE_Authority)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				//if (!Counselor->bIsHunter)
				//{
					++SpawnedCounselorCount;
				//} // 02/03/2021 - re-enable Hunter Selection
			}
		}
	}
}

void ASCGameState::CharacterDestroyed(AILLCharacter* Character)
{
	Super::CharacterDestroyed(Character);

	if (CurrentKiller == Character)
	{
		CurrentKiller = nullptr;
	}
}

bool ASCGameState::IsUsingMultiplayerFeatures() const
{
	if (GetMatchState() == MatchState::WaitingPostMatchOutro)
		return false;

	return Super::IsUsingMultiplayerFeatures();
}

void ASCGameState::HandleWaitingPreMatch()
{
	// Stop allowing the host to pick a killer if they could
	bHostCanPickKiller = false;
}

void ASCGameState::HandlePreMatchIntro()
{
	// We need to begin play now so that sequence actors can work. @see ASCGameMode::HandlePreMatchIntro
	GetWorldSettings()->NotifyBeginPlay();
}

void ASCGameState::HandleWaitingPostMatchOutro()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(PC->GetPawn()))
		{
			// Shut down the music component when the outro cinematic starts.
			if (Character->MusicComponent)
			{
				Character->MusicComponent->ShutdownMusicComponent();
			}
		}
	}

	if (Role == ROLE_Authority)
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());

		// Store persistent player data
		GI->StoreAllPersistentPlayerData();

		// Report scores to the backend
		if (ShouldAwardExperience())
		{
			USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
			if (RequestManager->CanSendAnyRequests())
			{
				for (APlayerState* PlayerState : PlayerArray)
				{
					if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
					{
						if (const AILLPlayerController* const OwnerController = Cast<AILLPlayerController>(PS->GetOwner()))
						{
							const int32 PlayerScore = (PS->Score < 0 ? 1 : PS->Score) * XPModifier; // if the player has a negative score, give them at least 1 point, add xp modifer
							FOnILLBackendSimpleRequestDelegate Delegate;
							RequestManager->AddExperienceToProfile(OwnerController->GetBackendAccountId(), PlayerScore, Delegate);
						}
					}
				}
			}
		}
	}
}

void ASCGameState::HandlePostMatchOutro()
{
	UWorld* World = GetWorld();

	// Destroy all SCCharacters so they do not show up during the outro, and so that they don't attempt to seamless travel needlessly
	// Really this exists to mitigate some random animation crash, apparently they're still animating during travel for some fucking reason
	if (Role == ROLE_Authority)
	{
		for (FConstPawnIterator PawnIt = World->GetPawnIterator(); PawnIt; )
		{
			ASCCharacter* Pawn = Cast<ASCCharacter>(*PawnIt);
			if (IsValid(Pawn))
			{
				Pawn->Destroy();
			}
			else
			{
				++PawnIt;
			}
		}
	}
	CurrentKiller = nullptr;
	PlayerCharacterList.Empty();

	// Destroy all actor spawner components
	for (int32 ActorSpawnerIndex = 0; ActorSpawnerIndex < ActorSpawnerList.Num(); ++ActorSpawnerIndex)
	{
		USCActorSpawnerComponent* SpawnerComponent = ActorSpawnerList[ActorSpawnerIndex];
		if (IsValid(SpawnerComponent) && SpawnerComponent->GetOwnerRole() == ROLE_Authority)
		{
			SpawnerComponent->DestroySpawnedActor();
		}
	}
	ActorSpawnerList.Empty();
}

bool ASCGameState::IsMatchWaitingPreMatch() const
{
	return (GetMatchState() == MatchState::WaitingPreMatch);
}

bool ASCGameState::IsInPreMatchIntro() const
{
	return (GetMatchState() == MatchState::PreMatchIntro);
}

bool ASCGameState::IsMatchWaitingPostMatchOutro() const
{
	return (GetMatchState() == MatchState::WaitingPostMatchOutro);
}

bool ASCGameState::IsInPostMatchOutro() const
{
	return (GetMatchState() == MatchState::PostMatchOutro);
}

int32 ASCGameState::GetAliveCounselorCount(ASCPlayerState* PlayerToIgnore /*= nullptr*/) const
{
	int32 AliveCounselors = 0;

	for (const ASCCharacter* Character : PlayerCharacterList)
	{
		if (Character->IsA<ASCCounselorCharacter>() && Character->PlayerState != PlayerToIgnore)
		{
			const ASCPlayerState* OtherPS = Character->GetPlayerState();
			if (OtherPS && !OtherPS->GetIsDead() && !OtherPS->GetSpectating() && !OtherPS->GetEscaped())
				AliveCounselors++;
		}
	}

	return AliveCounselors;
}

bool ASCGameState::IsOnlySurvivingCounselor(ASCPlayerState* SurvivingPlayer) const
{
	for (APlayerState* PS : PlayerArray)
	{
		const ASCPlayerState* OtherPS = Cast<ASCPlayerState>(PS);
		if (OtherPS && !OtherPS->IsActiveCharacterKiller())
		{
			if (OtherPS == SurvivingPlayer)
			{
				// Make sure SurvivingPlayer is still alive and spawned into the match
				if (SurvivingPlayer->GetIsDead() || !SurvivingPlayer->DidSpawnIntoMatch())
					return false;

				continue;
			}

			// Ignore late joiners
			if (!OtherPS->DidSpawnIntoMatch())
				continue;

			// Other player is alive or has escaped, so SurvivingPlayer isn't the only surviver
			if (!OtherPS->GetIsDead() || OtherPS->GetEscaped())
				return false;
		}
	}

	return true;
}

void ASCGameState::KillerAbilityUsed(EKillerAbilities ActivatedAbility)
{
	OnKillerAbilityUsed.Broadcast(ActivatedAbility);
}

void ASCGameState::ActorBroken(AActor* BrokenActor)
{
	OnActorBroken.Broadcast(BrokenActor);
}

void ASCGameState::CharacterKilled(ASCCharacter* Victim, const UDamageType* const KillInfo)
{
	OnCharacterKilled.Broadcast(Victim, KillInfo);
}

void ASCGameState::SpottedKiller(ASCKillerCharacter* SpottedKiller, ASCCharacter* Spotter)
{
	OnSpottedKiller.Broadcast(SpottedKiller, Spotter);
}

void ASCGameState::CharacterEarnedXP(ASCPlayerState* EarningPlayer, int32 ScoreEarned)
{
	OnCharacterEarnedXP.Broadcast(EarningPlayer, ScoreEarned);
}

void ASCGameState::TrapTriggered(ASCTrap* Trap, ASCCharacter* Character, bool bDisarmed)
{
	OnTrapTriggered.Broadcast(Trap, Character, bDisarmed);
}

void ASCGameState::OnReceivedModifiersCallback(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		// we got an error, default the xp modifier back to 1 just to be safe
		XPModifier = 1.0f;
		UE_LOG(LogSC, Warning, TEXT("ASCGameState::OnRevceivedModifiersCallback: Failed! ResponseCode: %i, Error: %s"), ResponseCode, *ResponseContents);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Grab the total amount of articles
		XPModifier = (float) JsonObject->GetNumberField(TEXT("ExperienceMultiplier"));
	}
}

void ASCGameState::OnRep_PoliceTimeRemaining()
{
	if (GetWorld() && !HasPoliceCalledSFXPlayed && PoliceTimeRemaining > 0)
	{
		ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings());
		if (WorldSettings)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), WorldSettings->PoliceCalledSFX);
			HasPoliceCalledSFXPlayed = true;
		}

		for (ASCCharacter* Character : PlayerCharacterList)
		{
			if (ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(Character))
			{
				if (KillerCharacter->IsLocallyControlled())
				{
					if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(KillerCharacter->GetController()))
					{
						if (ASCInGameHUD* HUD = PlayerController->GetSCHUD())
						{
							HUD->OnShowPoliceHasBeenCalled(false);
						}
					}
				}
			}
		}
	}
}

USCConversationManager* ASCGameState::GetConversationManager()
{
	if (!WorldConversationManager)
	{
		WorldConversationManager = NewObject<USCConversationManager>(this);
	}

	return WorldConversationManager;
}

bool ASCGameState::ShouldAIIgnoreSearchDistances() const
{
	if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		switch (GameMode->GetModeDifficulty())
		{
		case ESCGameModeDifficulty::Normal:
			return RemainingTime <= NormalSearchDistanceIgnoreTime;
		case ESCGameModeDifficulty::Hard:
			return RemainingTime <= HardSearchDistanceIgnoreTime;
		default:
			return false;
		}
	}

	return false;
}
