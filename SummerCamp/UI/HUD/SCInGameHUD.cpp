// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCInGameHUD.h"

#include "ILLPlayerInput.h"

#include "SCCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCDriveableVehicle.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCHUDWidget.h"
#include "SCInteractMinigameWidget.h"
#include "SCKillerCharacter.h"
#include "SCMenuStackInGameHUDComponent.h"
#include "SCMenuWidget.h"
#include "SCMiniMapBase.h"
#include "SCPlayerState.h"
#include "SCSpectatorPawn.h"
#include "SCWiggleWidget.h"
#include "SCMatchEndLobbyWidget.h"
#include "SCWorldSettings.h"

#if WITH_EDITORONLY_DATA
# include "Editor.h"
#endif

ASCInGameHUD::ASCInGameHUD(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<USCMenuStackInGameHUDComponent>(ASCHUD::MenuStackCompName))
{
	CounselorHUDClass = StaticLoadClass(USCHUDWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/Counselor/CounselorHUDWidget.CounselorHUDWidget_C"));
	KillerHUDClass = StaticLoadClass(USCHUDWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/Killer/KillerHUDWidget.KillerHUDWidget_C"));
	SpectatorHUDClass = StaticLoadClass(USCHUDWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/Spectator/SpectatorHUDWidget.SpectatorHUDWidget_C"));
	IntroHUDClass = StaticLoadClass(USCHUDWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/MatchIntroOutroHUDWidget.MatchIntroOutroHUDWidget_C"));
	OutroHUDClass = IntroHUDClass;
	MatchEndedHUDClass = StaticLoadClass(USCHUDWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/EndMatch/EndMatchHUDWidget.EndMatchHUDWidget_C"));

	PauseMenuClass = StaticLoadClass(USCMenuWidget::StaticClass(), nullptr, TEXT("/Game/UI/Menu/InGame/PauseMenuWidget.PauseMenuWidget_C"));

	ScoreboardClass = StaticLoadClass(USCUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/HUD/Scoreboard/ScoreboardWidget.ScoreboardWidget_C"));

	MatchEndMenuClass = StaticLoadClass(USCUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Menu/MatchEnd/MatchEnd.MatchEnd_C"));

	EmoteWidgetClass = StaticLoadClass(UILLUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Menu/Character/Counselor/Counselor_Emote_Select.Counselor_Emote_Select_C"));
}

void ASCInGameHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Hide the loading screen
	UWorld* World = GetWorld();
	if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
	{
		GameInstance->HideLoadingScreen();
	}
}

void ASCInGameHUD::BeginPlay()
{
	Super::BeginPlay();

	UpdateCharacterHUD();
}

void ASCInGameHUD::Destroyed()
{
	Super::Destroyed();

	if (IsValid(EmoteWidgetInstance))
	{
		EmoteWidgetInstance->RemoveFromParent();
		EmoteWidgetInstance = nullptr;
	}

	if (IsValid(ScoreboardInstance))
	{
		ScoreboardInstance->RemoveFromParent();
		ScoreboardInstance = nullptr;
	}
	bScoreboardLocked = false;

	if (IsValid(HUDWidgetInstance))
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}
}

void ASCInGameHUD::OnLevelIntroSequenceStarted()
{
	// Hide the mouse cursor if not in a menu
	if (!GetMenuStackComp()->GetCurrentMenu())
	{
		PlayerOwner->bShowMouseCursor = false;
	}
}

void ASCInGameHUD::OnLevelIntroSequenceCompleted()
{
}

void ASCInGameHUD::OnLevelOutroSequenceStarted()
{
}

void ASCInGameHUD::OnLevelOutroSequenceCompleted()
{
	// Hide any user-requested scoreboard, since we are about to spawn a menu
	DisplayScoreboard(false, true);
	DisplayEmoteSelect(false);

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(PlayerOwner))
	{
		// Change our camera
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings()))
		{
			if (WorldSettings->PostMatchCamera)
			{
				PC->SetViewTarget(WorldSettings->PostMatchCamera);
				PC->ClientSetCameraFade(false);
			}
		}

		if (USCMatchEndLobbyWidget* MatchEndLobbyWidgetInstance = Cast<USCMatchEndLobbyWidget>(MenuStackComponent->PushMenu(MatchEndMenuClass)))
		{
			if (ASCPlayerState* PS = Cast<ASCPlayerState>(PC->PlayerState))
			{
				// Push scoring information
				MatchEndLobbyWidgetInstance->Native_UpdateScoreEvents(CurrentCategorizedScoreEvents.MatchScoreEvents, PS->GetOldPlayerLevel(), CurrentCategorizedScoreEvents.BadgesUnlocked);
				MatchEndLobbyWidgetInstance->SetLocalPlayer(PS);
			}
		}
	}
}

void ASCInGameHUD::OnSpawnIntroSequenceStarted()
{
	// Give focus to the game when no menu is up
	if (!GetMenuStackComp()->GetCurrentMenu())
	{
#if WITH_EDITOR
		if (GetWorld() && !GetWorld()->IsPlayInEditor()) // This is annoying in PIE
#endif
		{
			FInputModeGameOnly InputMode;
			PlayerOwner->SetInputMode(InputMode);
		}
		PlayerOwner->bShowMouseCursor = false;
	}

	// Update the HUD
	UpdateCharacterHUD();

	// Prepare the HUD for the spawn intro
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnPrepMatchStart();
	}
}

void ASCInGameHUD::OnSpawnIntroSequenceCompleted()
{
	// Update the HUD
	UpdateCharacterHUD();

	// Play our HUD match start animation
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnMatchStarted();
	}
}

void ASCInGameHUD::OnMatchWaitingPostMatchOutro(bool bIsDead, bool bKiller, bool bLateJoin)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnMatchIsWaitingPostMatchOutro(bIsDead, bKiller, bLateJoin);
	}
}

void ASCInGameHUD::UpdateCharacterHUD(const bool bForceSpectatorHUD /*= false*/)
{
#if WITH_EDITOR
	if (GEditor && GEditor->bIsSimulatingInEditor)
	{
		return;
	}
#endif

	ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState());
	ASCPlayerController* PC = Cast<ASCPlayerController>(PlayerOwner);
	ASCPlayerState* PlayerState = Cast<ASCPlayerState>(PC->PlayerState);
	APawn* Pawn = PC->GetPawn();
	if (!GameState || !PlayerState)
	{
		return;
	}

	// Determine the HUD class that matches this Pawn
	// When there is no Pawn, there is no HUD unless you're flagged as a spectator
	TSubclassOf<USCHUDWidget> DesiredHUDClass = nullptr;
	if (GameState->IsInPreMatchIntro())
	{
		DesiredHUDClass = IntroHUDClass;
	}
	else if (GameState->IsInPostMatchOutro())
	{
		DesiredHUDClass = OutroHUDClass;
	}
	else if (GameState->IsMatchWaitingPreMatch() || GameState->IsMatchWaitingPostMatchOutro() || bForceSpectatorHUD)
	{
		DesiredHUDClass = SpectatorHUDClass;
	}
	else if (GameState->HasMatchEnded())
	{
		DesiredHUDClass = MatchEndedHUDClass;
	}
	else
	{
		if (PlayerState->GetSpectating() || (Pawn && Pawn->IsA(ASCSpectatorPawn::StaticClass())))
		{
			DesiredHUDClass = SpectatorHUDClass;
		}
		else if (Pawn)
		{
			if (Pawn->IsA<ASCKillerCharacter>())
			{
				DesiredHUDClass = KillerHUDClass;
				bIsKiller = true; // FIXME: pjackson: This is AIDS
			}
			else if (Pawn->IsA<ASCCounselorCharacter>() || Pawn->IsA<ASCDriveableVehicle>())
			{
				DesiredHUDClass = CounselorHUDClass;
			}
		}
	}

	// Handle HUD widget class change
	if (HUDWidgetInstance && HUDWidgetInstance->GetClass() != DesiredHUDClass)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}

	// Create a new HUDWidgetInstance if necessary
	if (!HUDWidgetInstance && DesiredHUDClass)
	{
		// Spawn the new HUD
		HUDWidgetInstance = NewObject<USCHUDWidget>(this, DesiredHUDClass);
		HUDWidgetInstance->SetPlayerContext(PC);
		HUDWidgetInstance->Initialize();

		if (!GameState->HasMatchStarted())
		{
			// Prepare the HUD for the level/spawn intro
			HUDWidgetInstance->OnPrepMatchStart();
		}

		// Add it to the screen
		HUDWidgetInstance->AddToViewport(HUD_ZORDER);
	}

	// Make sure the spectator HUD is displaying the correct info
	if (PlayerState->GetSpectating())
	{
		PC->UpdateSpectatorHUD();
	}

	// Update character inventory if necessary
	InventoryChanged();
}

void ASCInGameHUD::OnCharacterTookDamage(ASCCharacter* Character, float ActualDamage, ESCCharacterHitDirection HitDirection)
{
}

void ASCInGameHUD::OnCharacterDeath(ASCPlayerState* KillerPlayerState, const UDamageType* const KillerDamageType)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}
}

void ASCInGameHUD::OnKillerAttemptMorph()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnAttemptMorph();
	}
}

void ASCInGameHUD::OnTwoMinuteWarning()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnTwoMinuteWarning();
	}
}

void ASCInGameHUD::OnOutOfBoundsWarning()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnOutOfBoundsWarning();
	}
}

void ASCInGameHUD::OnInBounds()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnInBounds();
	}
}

void ASCInGameHUD::OnShowKnifeReticle()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnShowKnifeReticle();
	}
}

void ASCInGameHUD::OnHideKnifeReticle()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnHideKnifeReticle();
	}
}

void ASCInGameHUD::OnCombatStanceChanged(bool Enabled)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnCombatStanceChanged(Enabled);
	}
}

void ASCInGameHUD::OnRageActive()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnRageActive();
	}
}

void ASCInGameHUD::OnJasonAbilityUnlocked(EKillerAbilities UnlockedAbility)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnJasonAbilityUnlocked(UnlockedAbility);
	}
}

void ASCInGameHUD::FadeHUDWidgetIn()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnStartFadeIn();
	}
}

void ASCInGameHUD::FadeHUDWidgetOut()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnStartFadeOut();
	}
}

float ASCInGameHUD::GetHUDWidgetOpacity() const
{
	if (HUDWidgetInstance)
	{
		return HUDWidgetInstance->FearOpacity.A;
	}

	return 0.0f;
}

void ASCInGameHUD::SetHUDWidgetOpacity(float Opacity)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->FearOpacity = FLinearColor(1.0f, 1.0f, 1.0f, Opacity);
		//HUDWidgetInstance->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, Opacity));
	}
}

void ASCInGameHUD::HideHUD(bool ShouldHideHUD)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetNonPersistentWidgetVisibility(ShouldHideHUD);
	}
}

void ASCInGameHUD::OnBeginWiggleMinigame()
{
	if (HUDWidgetInstance && HUDWidgetInstance->WiggleWidgetInstance == nullptr)
	{
		HUDWidgetInstance->OnStartWiggleWidget();
	}
}

void ASCInGameHUD::OnEndWiggleMinigame()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnStopWiggleWidget();
	}
}

void ASCInGameHUD::StartInteractMinigame()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnStartInteractMinigame();
		if (HUDWidgetInstance->InteractMinigameInstance)
		{
			HUDWidgetInstance->InteractMinigameInstance->Start();
		}
	}
}

void ASCInGameHUD::StopInteractMinigame()
{
	if (HUDWidgetInstance)
	{
		if (HUDWidgetInstance->InteractMinigameInstance)
		{
			HUDWidgetInstance->InteractMinigameInstance->Stop();
		}
		HUDWidgetInstance->OnStopInteractMinigame();
	}
}

void ASCInGameHUD::OnInteractSkill(int InteractKey)
{
	if (HUDWidgetInstance && HUDWidgetInstance->InteractMinigameInstance)
	{
		HUDWidgetInstance->InteractMinigameInstance->OnInteractSkill(InteractKey);
	}
}

void ASCInGameHUD::InventoryChanged()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnInventoryChanged();
	}
}

void ASCInGameHUD::ShowInGameMenu()
{
	// Do not push the PauseMenuClass twice, or when any other menu is up for that matter...
	if (!GetCurrentMenu())
	{
		PushMenu(PauseMenuClass);
	}
}

void ASCInGameHUD::DisplayScoreboard(const bool bShow, const bool bLockUnlock/* = false*/)
{
	// Only if unlocked or unlocking
	if (!bScoreboardLocked || bLockUnlock)
	{
		bScoreboardLocked = (bLockUnlock && bShow); // Lock on show, unlock on hide

		if (HUDWidgetInstance)
		{
			HideHUD(false);
			HUDWidgetInstance->OnShowScoreboard(bShow);
		}

		if (bShow)
		{
			if (ScoreboardClass && !ScoreboardInstance)
			{
				ScoreboardInstance = NewObject<USCUserWidget>(this, ScoreboardClass);
				ScoreboardInstance->SetPlayerContext(PlayerOwner);
				ScoreboardInstance->Initialize();
			}

			if (ScoreboardInstance)
			{
				ScoreboardInstance->AddToViewport(HUD_ZORDER);
				ScoreboardInstance->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else if (ScoreboardInstance)
		{
			ScoreboardInstance->PlayTransitionOut();
			ScoreboardInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ASCInGameHUD::OnShowSpectatorIntro(bool bDead, bool bLateJoin)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnShowSpectatorIntro(bDead, bLateJoin);
	}
}

void ASCInGameHUD::OnSpectatorIntroFinished()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnSpectatorIntroFinished();
	}
}

void ASCInGameHUD::OnToggleSpecatorHUD(bool bShow)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnToggleSpecatorHUD(bShow);
	}
}

void ASCInGameHUD::OnToggleSpecatorShowAll(bool bShow)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnToggleSpecatorShowAll(bShow);
	}
}

void ASCInGameHUD::OnSpectatorUpdateStatus(const FString& StatusText, const FString& NextMode)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnSpectatorUpdateStatus(StatusText, NextMode);
	}
}

void ASCInGameHUD::OnShowScoreEvent(const FString& Message, const uint8 Category)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnShowScoreEvent(Message, Category);
	}
}

void ASCInGameHUD::OnShowHealthBar(bool bShow)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnShowHealthBar(bShow);
	}
}

void ASCInGameHUD::OnShowCounselorMap()
{
	if (HUDWidgetInstance && (!GetCurrentMenu() || GetCurrentMenu()->GetClass() == HUDWidgetInstance->ObjectiveMenuClass))
	{
		HUDWidgetInstance->OnShowCounselorMap();
	}
}

void ASCInGameHUD::CloseCounselorMap()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->CloseCounselorMap();
	}
}

void ASCInGameHUD::SetRotateMinimapWithPlayer(const bool bShouldRotate)
{
	if (HUDWidgetInstance)
	{
		if (USCMiniMapBase* Minimap = HUDWidgetInstance->GetMinimap())
		{
			Minimap->bRotateMap = (bShouldRotate && Minimap->bCanEverRotate);
		}
	}
}

void ASCInGameHUD::SetBreathVisible(bool Visible)
{
	if (HUDWidgetInstance)
	{
		if (Visible)
			HUDWidgetInstance->OnShowBreath();
		else
			HUDWidgetInstance->OnHideBreath();
	}
}

void ASCInGameHUD::OnShowPoliceHasBeenCalled(bool bHasPoliceArrived)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnShowPoliceHasBeenCalled(bHasPoliceArrived);
	}
}

void ASCInGameHUD::SetSweaterAbilityAvailable(bool Available)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetSweaterAbilityAvailable(Available);
	}
}

void ASCInGameHUD::DisplayEmoteSelect(const bool bShow)
{
	if (bShow != IsEmoteSelectDisplayed())
	{
		if (HUDWidgetInstance)
		{
			HideHUD(false);
			HUDWidgetInstance->OnShowEmoteSelect(bShow);
		}

		if (bShow)
		{
			if (EmoteWidgetClass && !EmoteWidgetInstance)
			{
				EmoteWidgetInstance = NewObject<UILLUserWidget>(this, EmoteWidgetClass);
				EmoteWidgetInstance->SetPlayerContext(PlayerOwner);
				EmoteWidgetInstance->Initialize();
				EmoteWidgetInstance->AddToViewport(HUD_ZORDER);
				PlayerOwner->SetInputMode(FInputModeGameAndUI());
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PlayerOwner->PlayerInput))
				{
					if (!Input->IsUsingGamepad())
					{
						PlayerOwner->bShowMouseCursor = true;
					}
				}
			}
		}
		else if (EmoteWidgetInstance)
		{
			EmoteWidgetInstance->PlayTransitionOut();
			EmoteWidgetInstance = nullptr;
			PlayerOwner->SetInputMode(FInputModeGameOnly());
			PlayerOwner->bShowMouseCursor = false;
		}
	}
}

void ASCInGameHUD::OnStartAiming()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnStartAiming();
	}
}

void ASCInGameHUD::OnEndAiming()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->OnEndAiming();
	}
}

void ASCInGameHUD::OnSetEndMatchMenus(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, const TArray<FSCBadgeAwardInfo>& BadgesAwarded)
{
	CurrentCategorizedScoreEvents.MatchScoreEvents = ScoreEvents;
	CurrentCategorizedScoreEvents.BadgesUnlocked = BadgesAwarded;
}
