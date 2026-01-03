// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameMode_Sandbox.h"

#include "SCCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCGameSession.h"
#include "SCGameState_Sandbox.h"
#include "SCInGameHUD.h"
#include "SCKillerCharacter.h"
#include "SCMusicComponent.h"
#include "SCNameplateComponent.h"
#include "SCPlayerController_Sandbox.h"
#include "SCPlayerState.h"
#include "SCSpectatorPawn.h"
#include "SCWeapon.h"

ASCGameMode_Sandbox::ASCGameMode_Sandbox(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PlayerControllerClass = ASCPlayerController_Sandbox::StaticClass();
	GameStateClass = ASCGameState_Sandbox::StaticClass();

	bSkipLevelIntro = true;

	HoldWaitingForPlayersTime = 3;
	PreMatchWaitTime = 5;
}

void ASCGameMode_Sandbox::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == NULL || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	if (NewPlayer->PlayerState && NewPlayer->PlayerState->bOnlySpectator)
	{
		return;
	}

	ASCPlayerState* NewPS = Cast<ASCPlayerState>(NewPlayer->PlayerState);
	APawn* OldPawn = NewPlayer->GetPawn();
	if (OldPawn)
	{
		const FRotator StartRotation = NewPlayer->GetPawn()->GetActorRotation();
		const FVector StartLocation = NewPlayer->GetPawn()->GetActorLocation();

		// Destroy old pawn
		NewPlayer->UnPossess();
		OldPawn->Destroy(false, false);

		// Attempt to spawn new pawn
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Instigator = Instigator;
		SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
		APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(NewPS->GetActiveCharacterClass(), StartLocation, StartRotation, SpawnInfo);
		NewPlayer->SetPawn(ResultPawn);
		NewPlayer->Possess(NewPlayer->GetPawn());
		if (TSoftClassPtr<ASCCounselorCharacter> CounselorClass = (UClass*)NewPS->GetActiveCharacterClass())
			NewPS->ClientRequestPerksAndOutfit(CounselorClass);

		if (NewPlayer->GetPawn() == NULL)
		{
			NewPlayer->FailedToSpawnPawn();
		}
		else
		{
			// set initial control rotation to player start's rotation
			NewPlayer->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation(), true);

			FRotator NewControllerRot = StartRotation;
			NewControllerRot.Roll = 0.f;
			NewPlayer->SetControlRotation(NewControllerRot);

			SetPlayerDefaults(NewPlayer->GetPawn());

			K2_OnRestartPlayer(NewPlayer);

			// We need to restart the councelors music class in sandbox mode.
			ASCCounselorCharacter* NewCounselor = Cast<ASCCounselorCharacter>(NewPlayer->GetPawn());
			if (NewCounselor && NewCounselor->MusicComponent)
			{
				NewCounselor->MusicComponent->InitMusicComponent(NewCounselor->MusicObjectClass);
			}
		}
	}
	else
	{
		if (ASCPlayerController_Sandbox* PC = Cast<ASCPlayerController_Sandbox>(NewPlayer))
		{
			Super::RestartPlayer(NewPlayer);
		}
	}

	// Special case hack for the hunter that's in here instead of being a feature of the SCCounselorCharacter..?
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(NewPlayer->GetPawn()))
	{
		if (Counselor->bIsHunter)
		{
			TSoftClassPtr<ASCItem> DefaultWeaponClass = Counselor->DefaultLargeItem ? Counselor->DefaultLargeItem : HunterWeaponClass;
			Counselor->GiveItem(DefaultWeaponClass);
		}
	}
}

void ASCGameMode_Sandbox::HandleMatchHasStarted()
{
	// Just assign the first character on the first spawn
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ASCPlayerState* NewPS = Cast<ASCPlayerState>(PlayerState);
		if (NewPS && !NewPS->GetActiveCharacterClass())
		{
			// Do a hard load
			const TSoftClassPtr<ASCCounselorCharacter> CounselorClass = TSoftClassPtr<ASCCounselorCharacter>(CharacterClasses[0].ToSoftObjectPath());
			CounselorClass.LoadSynchronous();

			// Set the character and perks
			NewPS->SetActiveCharacterClass(CounselorClass.Get());

			// Request Perks and Outfits from client
			NewPS->ClientRequestPerksAndOutfit(CounselorClass.Get());
		}
	}

	// Broadcast BeginPlay and call RestartPlayer on all players
	Super::HandleMatchHasStarted();
}

void ASCGameMode_Sandbox::SwapCharacter(ASCPlayerController_Sandbox* SandboxController, TSoftClassPtr<ASCCharacter> DesiredCharacterClass/* = nullptr*/)
{
	if (!Cast<ASCCharacter>(SandboxController->GetPawn()))
		return;

	ASCPlayerState* PlayerState = Cast<ASCPlayerState>(SandboxController->PlayerState);

	// Check for an override
	if (!DesiredCharacterClass.IsNull())
	{
		PlayerState->AsyncSetActiveCharacterClass(DesiredCharacterClass);
		RestartPlayer(SandboxController);
		return;
	}

	// Attempt to find the character class they currently use
	int32 CurrentCharacterIndex = INDEX_NONE;
	if (PlayerState->GetActiveCharacterClass())
	{
		CurrentCharacterIndex = CharacterClasses.Find(TSoftClassPtr<ASCCharacter>(PlayerState->GetActiveCharacterClass()));
	}
	else if (APawn* OldPawn = SandboxController->GetPawn())
	{
		CurrentCharacterIndex = CharacterClasses.Find(TSoftClassPtr<ASCCharacter>(OldPawn->GetClass()));
	}

	// Now assign the next class
	TSoftClassPtr<ASCCharacter> NextCharacterClass = (CurrentCharacterIndex == INDEX_NONE || CurrentCharacterIndex+1 >= CharacterClasses.Num()) ? CharacterClasses[0] : CharacterClasses[CurrentCharacterIndex+1];
	PlayerState->AsyncSetActiveCharacterClass(NextCharacterClass);

	// Restart!
	RestartPlayer(SandboxController);
}

void ASCGameMode_Sandbox::SwapCharacterPrevious(ASCPlayerController_Sandbox* SandboxController)
{
	if (!Cast<ASCCharacter>(SandboxController->GetPawn()))
		return;

	ASCPlayerState* PlayerState = Cast<ASCPlayerState>(SandboxController->PlayerState);

	// Attempt to find the character class they currently use
	int32 CurrentCharacterIndex = INDEX_NONE;
	if (PlayerState->GetActiveCharacterClass())
	{
		CurrentCharacterIndex = CharacterClasses.Find(TSoftClassPtr<ASCCharacter>(PlayerState->GetActiveCharacterClass()));
	}
	else if (APawn* OldPawn = SandboxController->GetPawn())
	{
		CurrentCharacterIndex = CharacterClasses.Find(TSoftClassPtr<ASCCharacter>(OldPawn->GetClass()));
	}

	// Now assign the next class
	TSoftClassPtr<ASCCharacter> PreviousCharacterClass = (CurrentCharacterIndex == INDEX_NONE || CurrentCharacterIndex - 1 < 0) ? CharacterClasses[CharacterClasses.Num() - 1] : CharacterClasses[CurrentCharacterIndex - 1];
	PlayerState->AsyncSetActiveCharacterClass(PreviousCharacterClass);

	// Restart!
	RestartPlayer(SandboxController);
}
