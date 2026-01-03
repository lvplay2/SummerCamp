// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameState.h"
#include "SCConversationManager.h"
#include "SCGameState.generated.h"

class ASCCBRadio;
class ASCCharacter;
class ASCCounselorCharacter;
class ASCDriveableVehicle;
class ASCItem;
class ASCKillerCharacter;
class ASCPhoneJunctionBox;
class ASCPlayerState;
class ASCProjectile;
class ASCRoadPoint;
class ASCStaticSpectatorCamera;

class USCActorSpawnerComponent;
class USCPowerPlant;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCAbilityUsedEvent, EKillerAbilities, ActivatedAbility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCActorBrokenEvent, AActor*, BrokenActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCCharacterKilledEvent, ASCCharacter*, Victim, const UDamageType*, KillInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCKillerSpotted, ASCKillerCharacter*, SpottedKiller, ASCCharacter*, Spotter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCCharacterEarnedXP, ASCPlayerState*, EarningPlayer, int32, ScoreEarned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSCTrapTriggeredEvent, ASCTrap*, Trap, ASCCharacter*, Character, bool, bDisarmed);

/**
 * @class ASCGameState
 */
UCLASS()
class SUMMERCAMP_API ASCGameState
: public AILLGameState
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	// Begin AGameState interface
	virtual bool HasMatchStarted() const override;
	virtual bool HasMatchEnded() const override;
	// End AGameState interface

protected:
	// Begin AGameState interface
	virtual void OnRep_MatchState() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void HandleLeavingMap() override;
	// End AGameState interface

public:
	// Begin AILLGameState interface
	virtual void CharacterInitialized(AILLCharacter* Character) override;
	virtual void CharacterDestroyed(AILLCharacter* Character) override;
	virtual bool IsUsingMultiplayerFeatures() const override;
	// End AILLGameState interface

	//////////////////////////////////////////////////
	// Match states
protected:
	/** Called when the MatchState changes to WaitingPreMatch. */
	virtual void HandleWaitingPreMatch();

	/** Called when the MatchState changes to PreMatchIntro. */
	virtual void HandlePreMatchIntro();

	/** Called when the MatchState changes to WaitingPostMatchOutro. */
	virtual void HandleWaitingPostMatchOutro();

	/** Called when the MatchState changes to PostMatchOutro. */
	virtual void HandlePostMatchOutro();

public:
	/** @return true if we are in the cool down period between WaitingToStart and PreMatchIntro. */
	UFUNCTION(BlueprintPure, Category="GameState")
	virtual bool IsMatchWaitingPreMatch() const;

	/** @return true if we are playing the match intro before InProgress. */
	UFUNCTION(BlueprintPure, Category="GameState")
	virtual bool IsInPreMatchIntro() const;

	/** @return true if we are in the cool down period between InProgress and PostMatchOutro. */
	UFUNCTION(BlueprintPure, Category="GameState")
	virtual bool IsMatchWaitingPostMatchOutro() const;

	/** @return true if we are playing the match outro. */
	UFUNCTION(BlueprintPure, Category="GameState")
	virtual bool IsInPostMatchOutro() const;

	// Time that the match has been InProgress
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	int32 ElapsedInProgressTime;

	// The amount of time in seconds that the current match phase has
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	int32 RemainingTime;

	// The unix time this match started
	UPROPERTY(Transient)
	int64 MatchStartTime;

	// The unix time this match ended
	UPROPERTY(Transient)
	int64 MatchEndTime;

	// Number of counselors that have escaped this match
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	int32 EscapedCounselorCount;

	// Number of counselors that have been killed this match
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	int32 DeadCounselorCount;

	UPROPERTY(Transient)
	bool bPlayJasonAbilityUnlockSounds;

	//////////////////////////////////////////////////
	// Actor caches
public:
	// Pointer to the current killer
	UPROPERTY(Transient, BlueprintReadOnly)
	ASCKillerCharacter* CurrentKiller;

	// List of known items
	UPROPERTY(Transient)
	TArray<ASCItem*> ItemList;

	// List of player characters
	UPROPERTY(Transient)
	TArray<ASCCharacter*> PlayerCharacterList;

	// List of spawned projectiles
	UPROPERTY(Transient)
	TArray<ASCProjectile*> ProjectileList;

	// List of static spectator cameras in the world
	UPROPERTY(Transient)
	TArray<ASCStaticSpectatorCamera*> StaticSpectatorCameras;

	// List of players who have walkie talkies
	UPROPERTY(Transient)
	TArray<ASCPlayerState*> WalkieTalkiePlayers;

	// List of road points for AI markup
	UPROPERTY(Transient)
	TArray<ASCRoadPoint*> RoadPoints;

	// Number of counselors spawned at the beginning of the match
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	int32 SpawnedCounselorCount;

	// Pointer to the current killer player
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	ASCPlayerState* CurrentKillerPlayerState;

	/**
	 * Counts how many counselors are alive
	 * @param PlayerToIgnore The player we shouldn't count
	 * @return The number of alive counselors
	 */
	UFUNCTION(BlueprintCallable, Category = "GameState") // Adding blueprint callable functionality to get counselor count for SP scoreboard
	int32 GetAliveCounselorCount(ASCPlayerState* PlayerToIgnore = nullptr) const;

	/**
	 * Checks to see if the specified player is the only counselor that survived the match
	 * @param SurvivingPlayer The player that survived the match
	 * @return True if the SurvivingPlayer is the only counselor that survived the match
	 */
	bool IsOnlySurvivingCounselor(ASCPlayerState* SurvivingPlayer) const;

	/** @return Power plant instance. */
	UFUNCTION(BlueprintPure, Category = "Power")
	USCPowerPlant* GetPowerPlant() const { return PowerPlant; }

private:
	// Used to manage powered objects
	UPROPERTY(Transient)
	USCPowerPlant* PowerPlant;
	
	//////////////////////////////////////////////////
	// Component caches
public:
	// List of all USCActorSpawnerComponent
	UPROPERTY(Transient)
	TArray<USCActorSpawnerComponent*> ActorSpawnerList;

	//////////////////////////////////////////////////
	// Event Callbacks
public:
	/** Calls OnKillerAbilityUsed delegate to provide blueprint hooks */
	virtual void KillerAbilityUsed(EKillerAbilities ActivatedAbility);

	/** Calls OnActorBroken delegate to provide blueprint hooks */
	virtual void ActorBroken(AActor* BrokenActor);

	/** Calls OnCharacterKilled delegate to provide blueprint hooks */
	virtual void CharacterKilled(ASCCharacter* Victim, const UDamageType* const KillInfo);

	/** Called when an AI spots a killer */
	virtual void SpottedKiller(ASCKillerCharacter* SpottedKiller, ASCCharacter* Spotter);

	/** Called when a character earns XP */
	virtual void CharacterEarnedXP(ASCPlayerState* EarningPlayer, int32 ScoreEarned);

	/** Called when a trap is triggered */
	virtual void TrapTriggered(ASCTrap* Trap, ASCCharacter* Character, bool bDisarmed);

protected:
	// Called whenever a killer uses their ability
	UPROPERTY(BlueprintAssignable)
	FSCAbilityUsedEvent OnKillerAbilityUsed;

	// Called whenever a destructable actor gives up the ghost
	UPROPERTY(BlueprintAssignable)
	FSCActorBrokenEvent OnActorBroken;

	// Called whenever a character gives up the ghost
	UPROPERTY(BlueprintAssignable)
	FSCCharacterKilledEvent OnCharacterKilled;

	// Called when an AI spots a killer
	UPROPERTY(BlueprintAssignable)
	FSCKillerSpotted OnSpottedKiller;

	// Called when a character earns XP
	UPROPERTY(BlueprintAssignable)
	FSCCharacterEarnedXP OnCharacterEarnedXP;

	// Called when a character triggers a trap
	UPROPERTY(BlueprintAssignable)
	FSCTrapTriggeredEvent OnTrapTriggered;

	//////////////////////////////////////////////////
	// Game settings
protected:
	// Can the host pick a killer for the match?
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	bool bHostCanPickKiller;


public:
	/** The xp modifier pulled from the backend. */
	UPROPERTY(BlueprintReadOnly, Replicated, Transient, Category = "XP")
	float XPModifier;

	void OnReceivedModifiersCallback(int32 ResponseCode, const FString& ResponseContents);

	/** Should experience be awarded to players? */
	virtual bool ShouldAwardExperience() const { return true; }

	//////////////////////////////////////////////////////
	// Hunter
public:
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	bool bIsHunterSpawned;

	// Seconds remaining before hunter can spawn. -1 for disabled
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	int32 HunterTimeRemaining;

	// Default hunter timer
	UPROPERTY(Config)
	int32 HunterResponseTime;

	// Default hunter delay if character that was just killed/escaped is chosen as the hunter
	UPROPERTY(Config)
	int32 HunterDelayTime;

	bool bHunterDied;
	bool bHunterEscaped;

	//////////////////////////////////////////////////////
	// Police
public:
	// Seconds remaining before police arrive. -1 for disabled
	UPROPERTY(ReplicatedUsing = OnRep_PoliceTimeRemaining, Transient, BlueprintReadOnly)
	int32 PoliceTimeRemaining;

	// Function for when the Police Timer Replicates
	UFUNCTION()
	void OnRep_PoliceTimeRemaining();

	// Default police timer
	UPROPERTY(Config)
	int32 PoliceResponseTime;

	// This is true when the Police has been called SFX has been played.
	UPROPERTY()
	bool HasPoliceCalledSFXPlayed;

	//////////////////////////////////////////////////////
	// Bridge Repair
public:
	// Tracker for if the Grendel Nav Card Repair has been completed
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	bool bNavCardRepaired;

	// Tracker for if the Grendel Core has been repaired
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	bool bCoreRepaired;

	//////////////////////////////////////////////////////
	//Gameplay items
public:
	UFUNCTION(BlueprintPure, Category = "Objectives")
	virtual ASCDriveableVehicle* GetCarVehicle(bool Car4Seats) const { return Car4Seats ? Car4SeatVehicle : Car2SeatVehicle; }

	UFUNCTION(BlueprintPure, Category = "Objectives")
	virtual ASCDriveableVehicle* GetBoatVehicle() const { return BoatVehicle; }

	UFUNCTION(BlueprintPure, Category = "Objectives")
	virtual ASCCBRadio* GetCBRadio() const { return CBRadio; }

	UFUNCTION(BlueprintPure, Category = "Objectives")
	virtual ASCPhoneJunctionBox* GetPhone() const { return Phone; }

	UFUNCTION(BlueprintPure, Category = "Objectives")
	virtual ASCBridge* GetBridge() const { return Bridge; }

	UPROPERTY(Replicated, Transient)
	TSoftClassPtr<ASCKillerCharacter> KillerClass;

	UFUNCTION()
	void SetCarEscaped(bool Was4SeatCar, bool DidEscape) { (Was4SeatCar ? Car4SeatEscaped : Car2SeatEscaped) = DidEscape; }

	UFUNCTION()
	void SetBoatEscaped(bool DidEscape) { BoatEscaped = DidEscape; }

	UPROPERTY(Transient, Replicated)
	ASCBridge* Bridge;

protected:
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "CarEscape")
	bool Car4SeatEscaped;

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "CarEscape")
	bool Car2SeatEscaped;

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "BoatEscape")
	bool BoatEscaped;

private:
	UPROPERTY(Transient, Replicated)
	ASCDriveableVehicle* Car4SeatVehicle;

	UPROPERTY(Transient, Replicated)
	ASCDriveableVehicle* Car2SeatVehicle;

	UPROPERTY(Transient, Replicated)
	ASCDriveableVehicle* BoatVehicle;

	UPROPERTY(Transient, Replicated)
	ASCCBRadio* CBRadio;

	UPROPERTY(Transient, Replicated)
	ASCPhoneJunctionBox* Phone;

	//////////////////////////////////////////////////
	// Conversations
public:

	/** Returns the active Conversation Manager and creates a new one if one hasn't been created already. */
	USCConversationManager* GetConversationManager();

private:

	// The Conversation manager for single player mode.
	UPROPERTY()
	USCConversationManager* WorldConversationManager;

	//////////////////////////////////////////////////
	// AI
public:
	/** @return true if AI should ignore distances for item searching */
	bool ShouldAIIgnoreSearchDistances() const;

protected:
	// Time at which AI should ignore search distances in normal difficulty
	UPROPERTY(config)
	float NormalSearchDistanceIgnoreTime;

	// Time at which AI should ignore search distances in hard difficulty
	UPROPERTY(config)
	float HardSearchDistanceIgnoreTime;
};
