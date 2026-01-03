// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerController.h"
#include "SCStatsAndScoringData.h"
#include "SCPlayerController.generated.h"

class ASCPlayerStart;

/**
 * @enum ESCSpectatorMode
 */
enum class ESCSpectatorMode : uint8
{
	SpectateIntro,	// Display you died/escaped
	DeadBody,		// Static camera looking at the player's dead body
	FreeCam,		// Free roam camera
	Player,			// Spectating other players
	LevelStatic,	// Viewing static cameras placed in the level
	MAX
};

/**
 * @struct FSCEndMatchPlayerInfo
 */
USTRUCT()
struct FSCEndMatchPlayerInfo
{
	GENERATED_USTRUCT_BODY()

	// Total experience value for each score category for the last match
	UPROPERTY()
	TArray<FSCCategorizedScoreEvent> MatchScoreEvents;

	// Badges unlocked in the last match
	UPROPERTY()
	TArray<FSCBadgeAwardInfo> BadgesUnlocked;
};

/**
 * @class ASCPlayerController
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCPlayerController
: public AILLPlayerController
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor interface
	
	// Begin AController interface
	virtual void SeamlessTravelFrom(APlayerController* OldPC) override;
	virtual void FailedToSpawnPawn() override;
	virtual void PawnPendingDestroy(APawn* P) override;
	virtual void ChangeState(FName NewState) override;
	// End AController interface

	// Begin APlayerController interface
	virtual void SetupInputComponent() override;
	virtual bool IsMoveInputIgnored() const override;
	virtual bool IsLookInputIgnored() const override;
	virtual bool SetPause(bool bPause, FCanUnpause CanUpauseDelegate = FCanUnpause()) override;
	virtual bool CanRestartPlayer() override;
	virtual void ServerAcknowledgePossession_Implementation(APawn* P) override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning) override;
	virtual void UnFreeze() override;
	virtual void PawnLeavingGame() override;
	virtual void ReceivedPlayer() override;
	virtual void Destroyed() override;
	// End APlayerController interface

	void OnExpPushComplete(int32 ResponseCode, const FString& ResponseContents);

	// Begin AILLPlayerController interface
	virtual void AcknowledgePlayerState(APlayerState* PS) override;
	virtual void ServerAcknowledgePlayerState_Implementation(APlayerState* PS) override;
	virtual void SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId) override;
	virtual bool IsGameInputAllowed() const override;
	virtual bool IsUsingMultiplayerFeatures() const override;
	// End AILLPlayerController interface

	// CLEANUP: The fuck is this?
	virtual void ModifyInputSensitivity(FKey InputKey, float SensitivityMod);
	virtual void RestoreInputSensitivity(FKey InputKey);
	virtual void RestoreAllInputSensitivity();
	
	//////////////////////////////////////////////////
	// Level intro/outro sequences
public:
	/** Notification from SCWorldSettings when the level intro sequence has started. */
	virtual void NotifyLevelIntroSequenceStarted();

	/** Notification from SCWorldSettings when the level intro sequence has finished. */
	virtual void NotifyLevelIntroSequenceCompleted();

	/** Notification from SCWorldSettings when the level outro sequence has started. */
	virtual void NotifyLevelOutroSequenceStarted();

	/** Notification from SCWorldSettings when the level outro sequence has finished. */
	virtual void NotifyLevelOutroSequenceCompleted();

	//////////////////////////////////////////////////
	// Spawn intro sequence
public:
	/**
	 * Server call to conditionally start the spawn intro sequence.
	 *
	 * @return true if the necessary replicated properties have been acked and the spawn intro started.
	 */
	bool TryStartSpawnIntroSequence(ASCPlayerStart* OverrideStartSpot = nullptr);

protected:
	/** Server+client simulated path for the spawn sequence. */
	void PlaySpawnIntroSequence(ASCPlayerStart* StartSpawnedAt);

	/** Tell this client to start playing their spawn intro sequence. */
	UFUNCTION(Reliable, Client)
	void ClientPlaySpawnIntroSequence(ASCPlayerStart* StartSpawnedAt);

	/** Callback for when the spawn intro sequence finishes. */
	UFUNCTION()
	void OnSpawnIntroCompleted();

	// Timer handle for host simulating the spawn intro sequence duration
	FTimerHandle TimerHandle_SpawnIntroCompleted;

	// Are we playing our spawn intro?
	bool bPlayingSpawnIntro;

	// Does our playing spawn intro block input?
	bool bSpawnIntroBlocksInput;

	//////////////////////////////////////////////////
	// Idle checker
public:
	/** [Authority] Bump the time this player was active. */
	virtual void BumpIdleTime();

	/** [Authority] @return true if this player has been idle too long and was kicked. */
	virtual bool IdleCheck();

protected:
	/** Warns the player they will be kicked for idling in TimeUntilKick seconds. */
	UFUNCTION(Client, Reliable)
	void ClientIdleKickWarning(const float TimeUntilKick);

	// Last time this user reported activity
	float LastActivityTimeSeconds;

	// Idle time estimated in the last IdleCheck, which is Reset when BumpIdleTime is called
	float LastCheckIdleTime;

	///////////////////////////////////////////////////
	// Debug Utility....
public:
	/** Calls the protected BuildInputStack */
	void DebugDrawInputStack(const float DisplayTime, const FColor TextColor);

	//////////////////////////////////////////////////
	// Uncategorized!
public:
	/** [Server] Notification from SCPlayerState when this character's score changes. */
	void ScoreChanged(const int32 PreviousScore, const int32 NewScore);

	/** Notify the player about a score event (this should only be temp) */
	UFUNCTION(Client, Reliable)
	void ClientDisplayScoreEvent(TSubclassOf<USCScoringEvent> ScoringEvent, const uint8 Category, const float ScoreModifier);

	UFUNCTION(BlueprintImplementableEvent, Category="Events")
	void PlayerKilled(FText& KillerName, FText& WeaponName, FText& KilledName);

	/** sets spectator location and rotation */
	UFUNCTION(reliable, client)
	void ClientSetSpectatorCamera(FVector CameraLocation, FRotator CameraRotation);

	/** used for input simulation from blueprint (for automatic perf tests) */
	UFUNCTION(BlueprintCallable, Category="Input")
	void SimulateInputKey(FKey Key, bool bPressed = true);

	/** set god mode cheat */
	UFUNCTION(exec)
	void SetGodMode(bool bEnable);

	/** get god mode cheat */
	bool HasGodMode() const;

	/** Pause action handler. */
	virtual void OnPause();

	/** Returns a pointer to the sc game hud. May return nullptr. */
	class ASCInGameHUD* GetSCHUD() const;

	UFUNCTION(BlueprintCallable, Category="Player Input")
	void SetInvertedYAxis(bool Inverted);

	/** Associate a new UPlayer with this PlayerController. */
	virtual void SetPlayer(UPlayer* Player);

	/** Show or Hide the scoreboard */
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void OnShowScoreboard(const bool bLock = false);
	void ShowScoreboardAction();

	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void OnHideScoreboard(const bool bUnlock = false);
	void HideScoreboardAction();

	// Emotes
	/** Show Emote Select */
	void OnShowEmoteSelect();

	/** Hide Emote Select */
	void OnHideEmoteSelect();

	/** Returns true if the emote selection is shown */
	bool IsEmoteSelectDisplayed() const;

	void ShowEndMatchMenu(const bool bIsWinner);
	void CloseEndMatchMenu();

	void PushSpectateInputComponent();
	void PopSpectateInputComponent();
	void SetSpectateBlocking(bool bBlockInput);

	// Check whether this player controller has the component on the stack
	bool ContainsInputComponent(const UInputComponent* InComponent) const;

	/** Get the player we are currently spectating */
	class ASCCharacter* GetSpectatingPlayer() const { return SpectatingPlayer; }

	/** Updates the spectator HUD */
	void UpdateSpectatorHUD();

	/** Tell the HUD to do a dumb thing */
	UFUNCTION(Client, Reliable)
	void CLIENT_PlayHUDEndMatchOutro(bool bIsDead, bool bIsKiller, bool bLateJoin);

protected:
	// Cache for modified sensitivity settings.
	TMap<FKey, float> SensitivityCache;

	/** gode mode cheat */
	UPROPERTY(Transient, Replicated)
	uint8 bGodMode : 1;

	/** try to find spot for death cam */
	bool FindDeathCameraSpot(const APawn* CurrentPawn, FVector& CameraLocation, FRotator& CameraRotation);

	UPROPERTY()
	UInputComponent* SpectateInputComponent;

	/** Cuases the player to commit suicide */
	UFUNCTION(exec)
	virtual void Suicide();

	/** Notifies the server that the client has suicided */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSuicide();

	/** Selects the next viewable spectator camera/player */
	void OnViewNextSpectatorCamera();

	/** Selects the previous spectator camera/player */
	void OnViewPreviousSpectatorCamera();

	/** Views the specified static spectator camera in the level */
	void OnViewStaticSpectatorCamera(int32& Index);

	/** Views the specified player in GameState::PlayerArray */
	void OnViewPlayer(int32 SearchDirection);

	/** Toggles the spectator mode */
	UFUNCTION()
	void OnToggleSpectateMode();

	/** Timer handle for spectator intro */
	FTimerHandle SpectatorIntroTimer;

	/** Current spectating mode */
	ESCSpectatorMode CurrentSpectatingMode;

	/** Current static spectating camera index */
	int32 CurrentSpectatingCameraIndex;

	/** Current player we are spectating index */
	int32 CurrentPlayerSpectatingIndex;

	/** The palyer we are currently spectating */
	UPROPERTY()
	class ASCCharacter* SpectatingPlayer;

	/** Camera that focus on our dead body */
	UPROPERTY()
	class ACameraActor* DeathCamera;

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerRequestSpectate();

	/** Unlocks platform achievements */
	UFUNCTION(Exec)
	void UnlockAchievements();

	/** Reset platform achievements */
	UFUNCTION(Exec)
	void ClearAchievements();

	/** Print player stats to the screen */
	UFUNCTION(Exec)
	void PrintPlayerStats();

public:
	/** Sends character preferences to server */
	void SendPreferencesToServer();
	bool bSentPreferencesToServer;

	/** Get the current spectating mode */
	ESCSpectatorMode GetCurrentSpectatingMode() const { return CurrentSpectatingMode; }

	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void SetCurrentCursor(TEnumAsByte<EMouseCursor::Type> CursorType);

	UFUNCTION(Client, Reliable)
	void CLIENT_OnCharacterDeath(ASCCharacter* DeadCharacter);

	/** Tells the client to display the match summary for the previous game. */
	UFUNCTION(Client, Reliable)
	void ClientMatchSummaryInfo(const FSCEndMatchPlayerInfo& CategorizedScoreEvents);

	void UpdateStreamerSettings(bool bStreamerMode);

	bool CurrentStreamerMode;
};
