// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCHUD.h"
#include "SCStunDamageType.h"
#include "SCPlayerController.h"
#include "SCStatsAndScoringData.h"
#include "SCInGameHUD.generated.h"

class ASCCharacter;
class ASCPlayerState;
class UILLMinimapWidget;
class USCHUDWidget;
class USCInteractComponent;
class USCMatchEndLobbyWidget;
class USCMenuWidget;
class USCUserWidget;
enum class ESCCharacterHitDirection : uint8;
enum class EKillerAbilities : uint8;

#define HUD_ZORDER 10 // We need the HUD to render on top of movie players (their default is ZOrder=0)

/**
 * @class ASCInGameHUD
 */
UCLASS()
class SUMMERCAMP_API ASCInGameHUD
: public ASCHUD
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	// End AActor interface

	//////////////////////////////////////////////////
	// Level and spawn sequences
public:
	/** Notification when the level intro sequence has started. */
	virtual void OnLevelIntroSequenceStarted();

	/** Notification when the level intro sequence has finished. */
	virtual void OnLevelIntroSequenceCompleted();

	/** Notification when the level outro sequence has started. */
	virtual void OnLevelOutroSequenceStarted();

	/** Notification when the level outro sequence has finished. */
	virtual void OnLevelOutroSequenceCompleted();

	/** Called from ASCPlayerController::PlaySpawnIntroSequence. */
	virtual void OnSpawnIntroSequenceStarted();

	/** Called from ASCPlayerController::OnSpawnIntroCompleted. */
	virtual void OnSpawnIntroSequenceCompleted();

	/** Play HUD Animation for Match End */
	virtual void OnMatchWaitingPostMatchOutro(bool bIsDead, bool bKiller, bool bLateJoin);
	
	//////////////////////////////////////////////////
	// Character HUD
public:
	/** Checks for a valid Pawn from our PlayerOwner and for an updated PlayerState before creating a new HUDWidgetInstance. */
	virtual void UpdateCharacterHUD(const bool bForceSpectatorHUD = false);

	/** Called from ASCCharacter::MULTICAST_OnHit when a possessed Character takes damage. */
	virtual void OnCharacterTookDamage(ASCCharacter* Character, float ActualDamage, ESCCharacterHitDirection HitDirection);

	/** Called from ASCPlayerState::InformAboutKilled when our Character dies. */
	virtual void OnCharacterDeath(ASCPlayerState* KillerPlayerState, const UDamageType* const KillerDamageType);

	/** Called when the owning killer character wants to attempt to morph. */
	virtual void OnKillerAttemptMorph();

	/** Called to trigger combat specific widgets. */
	virtual void OnCombatStanceChanged(bool Enabled);

	/** Called when Jasons rage ability is activated. */
	virtual void OnRageActive();

	void OnJasonAbilityUnlocked(EKillerAbilities UnlockedAbility);

	void OnTwoMinuteWarning();

	/** Handle the player leaving play bounds */
	UFUNCTION(BlueprintCallable, Category = "Boundaries")
	void OnOutOfBoundsWarning();

	/** Handle the player entering the play bounds */
	UFUNCTION(BlueprintCallable, Category = "Boundaries")
	void OnInBounds();

	void OnShowKnifeReticle();

	void OnHideKnifeReticle();

protected:
	// Counselor HUD class
	UPROPERTY()
	TSubclassOf<USCHUDWidget> CounselorHUDClass;

	// Killer HUD class
	UPROPERTY()
	TSubclassOf<USCHUDWidget> KillerHUDClass;

	// Spectator HUD class
	UPROPERTY()
	TSubclassOf<USCHUDWidget> SpectatorHUDClass;

	// Intro HUD class
	UPROPERTY()
	TSubclassOf<USCHUDWidget> IntroHUDClass;

	// Outro HUD class
	UPROPERTY()
	TSubclassOf<USCHUDWidget> OutroHUDClass;

	// Match ended HUD class
	UPROPERTY()
	TSubclassOf<USCHUDWidget> MatchEndedHUDClass;

	//////////////////////////////////////////////////
	// HUD widget instance management
public:
	/** Tells the HUDWidgetInstance to fade in. */
	void FadeHUDWidgetIn();

	/** Tells the HUDWidgetInstance to fade out. */
	void FadeHUDWidgetOut();

	/** @return Opacity of HUDWidgetInstance. */
	float GetHUDWidgetOpacity() const;

	/** Changes the alpha of our HUDWidgetInstance to Opacity. */
	void SetHUDWidgetOpacity(float Opacity);

	/** Changes the visibility of the HUDWidgetInstance. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void HideHUD(bool ShouldHideHUD);

protected:
	// Instance of one of our HUDClasses above
	UPROPERTY(Transient)
	USCHUDWidget* HUDWidgetInstance;
	
	//////////////////////////////////////////////////
	// HUD wiggle widget
public:
	/** Initialize the wiggle mini-game widget. */
	void OnBeginWiggleMinigame();

	/** Ends the wiggle mini-game. */
	void OnEndWiggleMinigame();

	//////////////////////////////////////////////////
	// HUD interaction widgets
public:
	/** Called to start the interaction mini-game. */
	void StartInteractMinigame();

	/** Called to stop the interaction mini-game. */
	void StopInteractMinigame();

	/** Called when user pressed their InteractSkill button. */
	void OnInteractSkill(int InteractKey);

	//////////////////////////////////////////////////
	// HUD inventory widget
public:
	/** Notification for when the inventory changes to refresh the widget. */
	void InventoryChanged();

	void OnStartAiming();
	void OnEndAiming();

	//////////////////////////////////////////////////
	// In-game menus
public:
	/** Opens PauseMenuClass. */
	void ShowInGameMenu();

protected:
	// Class of our in-game pause menu
	UPROPERTY()
	TSubclassOf<USCMenuWidget> PauseMenuClass;

	//////////////////////////////////////////////////
	// Scoreboard Menu
public:
	/**
	 * Called to show the current match scoreboard.
	 *
	 * @param bShow Spawn it if not present?
	 * @param bLockUnlock Lock when bShow, remove and unlock when !bShow.
	 */
	virtual void DisplayScoreboard(const bool bShow, const bool bLockUnlock = false);

	 /** Sends the score event category totals to the End Match */
	void OnSetEndMatchMenus(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, const TArray<FSCBadgeAwardInfo>& BadgesAwarded);

protected:
	// Class of our Scoreboard
	UPROPERTY()
	TSubclassOf<USCUserWidget> ScoreboardClass;

	// Instance of ScoreboardClass
	UPROPERTY(Transient)
	USCUserWidget* ScoreboardInstance;

	// Class of our match end menu
	UPROPERTY()
	TSubclassOf<USCUserWidget> MatchEndMenuClass;

	// Is the scoreboard locked?
	bool bScoreboardLocked;

	//////////////////////////////////////////////////
	// Spectator Mode Menu
public:
	/** Called when the spectator HUD is first displayed */
	void OnShowSpectatorIntro(bool bDead, bool bLateJoin);

	/** Called when the spectator has finished the spectator intro */
	void OnSpectatorIntroFinished();

	/** Called to show the spectator HUD. */
	void OnToggleSpecatorHUD(bool bShow);

	/** Called to hide all the HUD. */
	void OnToggleSpecatorShowAll(bool bShow);

	/** Updates the current spectator mode text. */
	void OnSpectatorUpdateStatus(const FString& StatusText, const FString& NextMode);

	//////////////////////////////////////////////////
	// Score Events (XP)
public:
	void OnShowScoreEvent(const FString& Message, const uint8 Category);

	//////////////////////////////////////////////////
	// Counselor Map
public:
	void OnShowCounselorMap();

	void CloseCounselorMap();

	void SetRotateMinimapWithPlayer(const bool bShouldRotate);

	//////////////////////////////////////////////////
	// Debug Functions
public:
	/** Set if the health bar should be shown */
	void OnShowHealthBar(bool bShow);

	/////////////////////////////////////////////////
	// Breath
public:
	void SetBreathVisible(bool Visible);

	/////////////////////////////////////////////////
	// Police
public:
	void OnShowPoliceHasBeenCalled(bool bHasPoliceArrived);

	/////////////////////////////////////////////////
	// Abilities
	void SetSweaterAbilityAvailable(bool Available);

	/////////////////////////////////////////////////
	// Emotes
protected:
	// Class of our Emote Widget
	UPROPERTY()
	TSubclassOf<UILLUserWidget> EmoteWidgetClass;

	// Instance of EmoteWidgetClass
	UPROPERTY(Transient)
	UILLUserWidget* EmoteWidgetInstance;

public:
	/** Toggles visibility of the EmoteWidgetInstance, created and destroyed as needed */
	void DisplayEmoteSelect(const bool bShow);

	/** Returns true if the EmoteWidgetInstance exists, will return false if it is transitioning out or null */
	FORCEINLINE bool IsEmoteSelectDisplayed() const { return EmoteWidgetInstance != nullptr; }

protected:
	FSCEndMatchPlayerInfo CurrentCategorizedScoreEvents;
};
