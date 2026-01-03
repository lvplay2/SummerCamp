// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerController_Paranoia.h"
#include "SCPlayerState_Paranoia.h"
#include "SCGameMode_Paranoia.h"
#include "SCParanoiaKillerCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCInGameHUD.h"

#define TRANSFORM_COOLDOWN_TIME 10

ASCPlayerController_Paranoia::ASCPlayerController_Paranoia(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TransformCooldownTimer = 0.0f;
}

void ASCPlayerController_Paranoia::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASCPlayerController_Paranoia, CounselorPawn, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPlayerController_Paranoia, bWonMatch, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPlayerController_Paranoia, TransformCooldownTimer, COND_OwnerOnly);
}

void ASCPlayerController_Paranoia::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Role == ROLE_Authority)
	{
		if (TransformCooldownTimer > 0.0f)
			TransformCooldownTimer -= DeltaSeconds;
	}
}

bool ASCPlayerController_Paranoia::SERVER_TransformIntoKiller_Validate()
{
	return true;
}

void ASCPlayerController_Paranoia::GameHasEnded(class AActor* EndGameFocus /* = NULL*/, bool bIsWinner /*= false*/)
{
	// Uh... do the win or lose thing here?

	bWonMatch = bIsWinner;
}

void ASCPlayerController_Paranoia::SERVER_TransformIntoKiller_Implementation()
{
	if (ASCGameMode_Paranoia* GM = GetWorld()->GetAuthGameMode<ASCGameMode_Paranoia>())
	{
		if (ASCParanoiaKillerCharacter* Killer = GM->GetKillerPawn())
		{
			CounselorPawn = Cast<ASCCounselorCharacter>(GetPawn());

			// hide and disable counselor
			CounselorPawn->SetCharacterDisabled(true);

			// start vhs effects
			Killer->SERVER_OnTeleportEffect(true);
			// set timer to switch to killer
			FTimerHandle PossessTimerHandle;
			GetWorldTimerManager().SetTimer(PossessTimerHandle, this, &ThisClass::PossessKiller, 1.f);
			TransformCooldownTimer = TRANSFORM_COOLDOWN_TIME;

			// if we have never transformed into the killer before
			if (!bHasPossessedKiller) // this gets set to true in PossessKiller after the killer is actually possessed
			{
				if (ASCParanoiaKillerCharacter* DefaultKiller = Cast<ASCParanoiaKillerCharacter>(Killer->GetClass()->GetDefaultObject()))
				{
					if (ASCPlayerState_Paranoia* SCPlayerState = Cast<ASCPlayerState_Paranoia>(PlayerState))
					{
						SCPlayerState->KnifeCount = DefaultKiller->NumKnives;
						SCPlayerState->TrapCount = DefaultKiller->InitialTrapCount;
					}
				}
			}
		}
	}
}

bool ASCPlayerController_Paranoia::SERVER_TransformIntoCounselor_Validate()
{
	return true;
}

void ASCPlayerController_Paranoia::SERVER_TransformIntoCounselor_Implementation()
{
	if (ASCGameMode_Paranoia* GM = GetWorld()->GetAuthGameMode<ASCGameMode_Paranoia>())
	{
		if (ASCParanoiaKillerCharacter* Killer = GM->GetKillerPawn())
		{
			// hide killer
			Killer->SetCharacterDisabled(true);

			// reset the player classes in the player state
			if (!Killer->IsAlive())
			{
				if (ASCPlayerState_Paranoia* SCPlayerState = Cast<ASCPlayerState_Paranoia>(PlayerState))
				{
					SCPlayerState->SetActiveCharacterClass(nullptr);
					SCPlayerState->SpawnedCharacterClass = nullptr;
				}
			}

			// start vhs effect
			Killer->SERVER_OnTeleportEffect(true);
			// set timer to switch back to counselor
			FTimerHandle PossessTimerHandle;
			GetWorldTimerManager().SetTimer(PossessTimerHandle, this, &ThisClass::PossessCounselor, 1.f);
			TransformCooldownTimer = TRANSFORM_COOLDOWN_TIME;
		}
	}
}

void ASCPlayerController_Paranoia::PossessKiller()
{
	if (ASCGameMode_Paranoia* GM = GetWorld()->GetAuthGameMode<ASCGameMode_Paranoia>())
	{
		if (ASCParanoiaKillerCharacter* Killer = GM->GetKillerPawn())
		{
			// possess killer
			Possess(Killer);

			// if we have never possessed the killer before, make sure to fill the abilities
			if (!bHasPossessedKiller)
			{
				Killer->FillAbilities();
				bHasPossessedKiller = true;
			}

			GM->OnPlayerTransformed(Killer, CounselorPawn);

			if (ASCPlayerState_Paranoia* SCPlayerState = Cast<ASCPlayerState_Paranoia>(PlayerState))
			{
				Killer->NumKnives = SCPlayerState->KnifeCount;
				Killer->TrapCount = SCPlayerState->TrapCount;
			}

			// move killer to counselor location
			Killer->SetActorTransform(CounselorPawn->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
			// make sure to return camera to character just in case
			SetViewTarget(Killer);
			// re-enable character
			Killer->SetCharacterDisabled(false);
			// disable the vhs effect
			Killer->SERVER_OnTeleportEffect(false);

			// set the killers health to the counselors health
			Killer->Health = CounselorPawn->Health;

			// notify the controller owner that they are gonna possess the killer and should update their hud and such
			CLIENT_PossessKiller();

			// make sure we are in the playing state
			ChangeState(NAME_Playing);
			ClientGotoState(NAME_Playing);
		}
	}
}

void ASCPlayerController_Paranoia::CLIENT_PossessKiller_Implementation()
{
	// update hud
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->UpdateCharacterHUD();
		HUD->OnSpawnIntroSequenceCompleted();
		HUD->FadeHUDWidgetIn();

		HUD->OnJasonAbilityUnlocked(EKillerAbilities::Morph);
		HUD->OnJasonAbilityUnlocked(EKillerAbilities::Sense);
		HUD->OnJasonAbilityUnlocked(EKillerAbilities::Shift);
		HUD->OnJasonAbilityUnlocked(EKillerAbilities::Stalk);
	}

	// reset the camera and notify the local actor they received the player controller
	if (ASCCharacter* PawnCharacter = Cast<ASCCharacter>(GetPawn()))
	{
		PawnCharacter->ResetCameras();
		PawnCharacter->OnReceivedController();
	}
}

void ASCPlayerController_Paranoia::PossessCounselor()
{
	if (ASCGameMode_Paranoia* GM = GetWorld()->GetAuthGameMode<ASCGameMode_Paranoia>())
	{
		if (CounselorPawn)
		{
			if (ASCParanoiaKillerCharacter* Killer = GM->GetKillerPawn())
			{
				// possess killer
				Possess(CounselorPawn);

				GM->OnPlayerTransformed(CounselorPawn, Killer);

				if (ASCPlayerState_Paranoia* SCPlayerState = Cast<ASCPlayerState_Paranoia>(PlayerState))
				{
					SCPlayerState->SetActiveCharacterClass(CounselorPawn->GetClass());
					SCPlayerState->SpawnedCharacterClass = CounselorPawn->GetClass();

					// save off killer inventory
					SCPlayerState->KnifeCount = Killer->NumKnives;
					SCPlayerState->TrapCount = Killer->TrapCount;
				}

				// move counselor to killer location
				CounselorPawn->SetActorTransform(Killer->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
				// make sure to return camera to character just in case
				SetViewTarget(CounselorPawn);
				// re-enable character
				CounselorPawn->SetCharacterDisabled(false);
				// disable the vhs effect
				Killer->SERVER_OnTeleportEffect(false);

				// make sure the health is set back correctly
				CounselorPawn->Health = Killer->Health;

				// if counselor was killed as jason, make sure they die as a counselor
				if (CounselorPawn->Health <= 0.0f)
				{
					CounselorPawn->Die(100.0f, FDamageEvent(), this, nullptr);
				}

				// notify the controller owner that they are gonna possess the killer and should update their hud and such
				CLIENT_PossessCounselor();

				// make sure we are in the playing state
				ChangeState(NAME_Playing);
				ClientGotoState(NAME_Playing);
			}
		}
	}
}

void ASCPlayerController_Paranoia::CLIENT_PossessCounselor_Implementation()
{
	// update hud
	if (ASCInGameHUD* HUD = GetSCHUD())
	{
		HUD->UpdateCharacterHUD();
		HUD->OnSpawnIntroSequenceCompleted();
		HUD->FadeHUDWidgetIn();
	}

	// reset the camera and notify the local actor they received the player controller
	if (ASCCharacter* PawnCharacter = Cast<ASCCharacter>(GetPawn()))
	{
		PawnCharacter->ResetCameras();
		PawnCharacter->OnReceivedController();
	}
}

void ASCPlayerController_Paranoia::ClientDisplayPointsEvent_Implementation(TSubclassOf<USCParanoiaScore> ScoringEvent, const uint8 Category, const float ScoreModifier)
{
	if (USCParanoiaScore* ScoreEvent = ScoringEvent->GetDefaultObject<USCParanoiaScore>())
	{
		if (ASCPlayerState* SCPlayerState = Cast<ASCPlayerState>(PlayerState))
		{
			if (ASCInGameHUD* Hud = GetSCHUD())
			{
				Hud->OnShowScoreEvent(ScoreEvent->GetDisplayMessage(ScoreModifier, SCPlayerState->GetActivePerks()), Category);
			}
		}
	}
}

float ASCPlayerController_Paranoia::GetUseAbilityPercentage() const
{
	return 1.0f - (TransformCooldownTimer / TRANSFORM_COOLDOWN_TIME);
}

void ASCPlayerController_Paranoia::CLIENT_OnTransform_Implementation(ASCCharacter* NewCharacter, ASCCharacter* PreviousCharacter)
{
	if (CurrentSpectatingMode == ESCSpectatorMode::Player)
	{
		if (SpectatingPlayer == PreviousCharacter)
			SpectatingPlayer = NewCharacter;
	}
}