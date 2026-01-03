// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameMode.h"
#include "SCGameMode.generated.h"

class ASCBridgeCore;
class ASCBridgeNavUnit;
class ASCCabin;
class ASCCabinet;
class ASCCBRadio;
class ASCCharacter;
class ASCCounselorCharacter;
class ASCDriveableVehicle;
class ASCEscapePod;
class ASCEscapeVolume;
class ASCHidingSpot;
class ASCItem;
class ASCKillerCharacter;
class ASCPhoneJunctionBox;
class ASCPlayerController;
class ASCPlayerState;
class ASCRepairPart;
class ASCRepairPartSpawner;
class ASCShoreItemSpawner;
class ASCSpecialItem;
class ASCThrowable;
class ASCWeapon;

class UBehaviorTree;
class USCActorSpawnerComponent;
class USCContextKillDamageType;
class USCPamelaTapeDataAsset;
class USCTommyTapeDataAsset;
class USCScoringEvent;
class USCStatBadge;

namespace MatchState
{
	extern SUMMERCAMP_API const FName WaitingPreMatch; // Right after WaitingToStart, a wait period before PreMatchIntro
	extern SUMMERCAMP_API const FName PreMatchIntro; // Playing the pre-match intro, sequence before InProgress
	extern SUMMERCAMP_API const FName WaitingPostMatchOutro; // Wait period after InProgress before PostMatchOutro
	extern SUMMERCAMP_API const FName PostMatchOutro; // Playing the post-match outro
}

/**
 * @enum ESCGameModeDifficulty
 */
UENUM()
enum class ESCGameModeDifficulty : uint8
{
	Normal, // Aka ?Difficulty=1
	Easy, // Aka ?Difficulty=0
	Hard, // Aka ?Difficulty=2
	MAX UMETA(Hidden)
};

/**
 * @class ASCGameMode
 */
UCLASS(Abstract, Config = Game)
class SUMMERCAMP_API ASCGameMode
: public AILLGameMode
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject interface

	// Begin AGameModeBase interface
	virtual void SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	// End AGameModeBase interface

	// Begin AGameMode interface
	virtual bool HasMatchStarted() const override;
	virtual bool HasMatchEnded() const override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual void StartPlay() override;
	virtual void EndMatch() override;
	virtual bool AllowPausing(APlayerController* PC = nullptr) override;
	virtual void InitStartSpot_Implementation(AActor* StartSpot, AController* NewPlayer) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool CanSpectate_Implementation(APlayerController* Viewer, APlayerState* ViewTarget) override;
	// End AGameMode interface

	UFUNCTION()
	virtual void LoadRandomItems();

protected:
	// Begin AGameMode interface
	virtual void OnMatchStateSet() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void HandleLeavingMap() override;
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	// End AGameMode interface

	// Begin AILLGameMode interface
	virtual void GameSecondTimer() override;
	virtual bool IsOpenToMatchmaking() const override;
	// End AILLGameMode interface

	/** @return true if all ASCPlayerState::HasReceivedProfile calls pass. */
	bool HasReceivedAllPlayerProfiles() const;
	
	//////////////////////////////////////////////////
	// Difficulty
public:
	/** @return Game mode difficulty. */
	FORCEINLINE ESCGameModeDifficulty GetModeDifficulty() const { return ModeDifficulty; }

	/** @return "Downward" (Easy = Base + Deviation) difficulty multiplier for the current ModeDifficulty. */
	float GetCurrentDifficultyDownwardAlpha(const float Deviation = .5f, const float Base = 1.f) const;

	/** @return "Upward" (Hard = Base + Deviation) difficulty multiplier for the current ModeDifficulty. */
	float GetCurrentDifficultyUpwardAlpha(const float Deviation = .5f, const float Base = 1.f) const;

	// Minimum Jason stealth, increase this to reduce the impact of stealth on game play
	float JasonStealthRange;

protected:
	// Game mode difficulty
	ESCGameModeDifficulty ModeDifficulty;

	//////////////////////////////////////////////////
	// Match states
public:
	/** Called when the post match outro + cooldown completes. */
	virtual void OnPostMatchOutroComplete();

protected:
	/** Called when the MatchState changes to WaitingPreMatch. */
	virtual void HandleWaitingPreMatch();

	/** Called every GameSecondTimer hit while the MatchState is WaitingPreMatch. */
	virtual void TickWaitingPreMatch() {}

	/** @return true if we can transition from WaitingPreMatch to PreMatchIntro. */
	virtual bool CanCompleteWaitingPreMatch() const { return HasReceivedAllPlayerProfiles(); }

	/** Called when the MatchState changes to PreMatchIntro. */
	virtual void HandlePreMatchIntro();

	/** Called when the MatchState changes to PostMatchOutroWait. */
	virtual void HandleWaitingPostMatchOutro();

	/** Called when the MatchState changes to PostMatchOutro. */
	virtual void HandlePostMatchOutro();

	//////////////////////////////////////////////////
	// Match settings
public:
	/** Finish current round and lock players. */
	UFUNCTION(Exec)
	void FinishMatch();

	// Minimum amount of players to start the match
	UPROPERTY(Config)
	int32 MinimumPlayerCount;

	// Amount of time to hold waiting for players after everybody loads in QuickPlay and the match is not full
	// Serves as a padding time to remain open to matchmaking while we wait for more players to discover our session
	UPROPERTY(Config)
	int32 HoldWaitingForPlayersTimeNonFullQuickPlay;

	// Amount of time to hold waiting for players after everybody loads (when full or a QuickPlay match)
	UPROPERTY(Config)
	int32 HoldWaitingForPlayersTime;

	// Match start delay after everybody has loaded
	UPROPERTY(Config)
	int32 PreMatchWaitTime;

	// Match time limit
	UPROPERTY(Config)
	int32 RoundTime;

	// Time before the match outro to wait
	UPROPERTY(Config)
	int32 PostMatchOutroWaitTime;

	// Time after the match outro to wait
	UPROPERTY(Config)
	int32 PostMatchWaitTime;

	// Time after the match outro to wait when this instance is pending recycle-replacement
	UPROPERTY(Config)
	int32 PostMatchWaitPendingReplacementTime;

	// Should this mode skip the level intro sequence?
	UPROPERTY(Config)
	bool bSkipLevelIntro;

protected:
	/** @return How long to hold waiting for players. */
	int32 GetHoldWaitingForPlayersTime() const;

	/** @return How long to hold WaitingPostMatch. */
	int32 GetHoldWaitingPostMatchTime() const;

	//////////////////////////////////////////////////
	// Shore locations
public:
	UFUNCTION()
	bool GetShoreLocation(const FVector& PlayerLocation, ASCItem* Item, FTransform& OutTransform);

private:
	// Shore Item Spawners for respawning items when a counselor dies in the water or escapes
	UPROPERTY()
	TArray<ASCShoreItemSpawner*> ShoreLocations;

	//////////////////////////////////////////////////
	// Idle checking
public:
	// How long a player can be idle before getting kicked, set to >0.f to define the maximum time someone can be idle
	UPROPERTY(Config)
	float MaxIdleTimeBeforeKick;

	//////////////////////////////////////////////////
	// Character classes
public:
	/** @return List of all possible counselor character classes. */
	FORCEINLINE const TArray<TSoftClassPtr<ASCCounselorCharacter>>& GetCounselorCharacterClasses() const { return CounselorCharacterClasses; }

	/** @return List of all possible killer character classes. */
	FORCEINLINE const TArray<TSoftClassPtr<ASCKillerCharacter>>& GetKillerCharacterClasses() const { return KillerCharacterClasses; }

	/** @return Flirt character class */
	FORCEINLINE const TSoftClassPtr<ASCCounselorCharacter>& GetFlirtCharacterClass() const { return FlirtCharacterClass; }

	/** @return Hunter character class */
	FORCEINLINE const TSoftClassPtr<ASCCounselorCharacter>& GetHunterCharacterClass() const { return HunterCharacterClass; }

	/** @return Shotgun weapon class */
	FORCEINLINE const TSoftClassPtr<ASCWeapon>& GetShotgunClass() const { return HunterWeaponClass; }

	/** @return Baseball Bat weapon class */
	FORCEINLINE const TSoftClassPtr<ASCWeapon>& GetBaseballBatClass() const { return BaseballBatClass; }

	/** @return Car keys class */
	FORCEINLINE const TSoftClassPtr<ASCRepairPart>& GetCarKeysClass() const { return CarKeysClass; }

	/** @return List of killer classes used for achievement tracking */
	FORCEINLINE const TArray<TSoftClassPtr<ASCKillerCharacter>>& GetAchievementTrackedKillerClasses() const { return AchievementTrackedKillerClasses; }

	/** @return List of counselor classes used for achievement tracking */
	FORCEINLINE const TArray<TSoftClassPtr<ASCCounselorCharacter>>& GetAchievementTrackedCounselorClasses() const { return AchievementTrackedCounselorClasses; }

	/** Returns the number of counselor classes available to pick from (used in random counselors and bot selection) */
	int32 GetNumberOfCounselorClasses() const;

	/** @return Counselor class that the provided player state is allowed to use based on level */
	const TSoftClassPtr<ASCCounselorCharacter> GetRandomCounselorClass(const ASCPlayerState* PickingPlayerState) const;

	/** @return Killer class that the provided player state is allowed to use based on level */
	const TSoftClassPtr<ASCKillerCharacter> GetRandomKillerClass(const ASCPlayerState* PickingPlayerState) const;

	/** @return Hero Character class based on the world settings (defaults to HunterCharacterClass if none provided) */
	TSoftClassPtr<ASCCounselorCharacter> GetHeroCharacterClass() const;

protected:
	// Counselor class types
	UPROPERTY()
	TArray<TSoftClassPtr<ASCCounselorCharacter>> CounselorCharacterClasses;

	// Character class for the flirt
	UPROPERTY()
	TSoftClassPtr<ASCCounselorCharacter> FlirtCharacterClass;

	// Character class for the hunter
	UPROPERTY()
	TSoftClassPtr<ASCCounselorCharacter> HunterCharacterClass;

	// Character class for the android
	UPROPERTY()
	TSoftClassPtr<ASCCounselorCharacter> AndroidCharacterClass;

	// Hunter weapon class type
	UPROPERTY()
	TSoftClassPtr<ASCWeapon> HunterWeaponClass;

	// Baseball Bat weapon class
	UPROPERTY()
	TSoftClassPtr<ASCWeapon> BaseballBatClass;

	// Map class
	UPROPERTY()
	TSoftClassPtr<ASCSpecialItem> MapClass;

	// Walkie Talkie class
	UPROPERTY()
	TSoftClassPtr<ASCSpecialItem> WalkieTalkieClass;

	// Pocket Knife class
	UPROPERTY()
	TSoftClassPtr<ASCItem> PocketKnifeClass;

	// Med Spray class
	UPROPERTY()
	TSoftClassPtr<ASCItem> MedSprayClass;

	// Car keys class
	UPROPERTY()
	TSoftClassPtr<ASCRepairPart> CarKeysClass;

	// Killer class types
	UPROPERTY()
	TArray<TSoftClassPtr<ASCKillerCharacter>> KillerCharacterClasses;

	// Character (counselor and killer) class types
	UPROPERTY()
	TArray<TSoftClassPtr<ASCCharacter>> CharacterClasses;

	// Killer classes used for achievements
	UPROPERTY()
	TArray<TSoftClassPtr<ASCKillerCharacter>> AchievementTrackedKillerClasses;

	// Counselor classes used for achievements
	UPROPERTY()
	TArray<TSoftClassPtr<ASCCounselorCharacter>> AchievementTrackedCounselorClasses;
	
	//////////////////////////////////////////////////
	// Character damage
public:
	/** Notify about kills */
	virtual void CharacterKilled(AController* Killer, AController* KilledPlayer, ASCCharacter* KilledCharacter, const UDamageType* const DamageType);

	/** Update scoring for context kills */
	virtual void ContextKillScored(AController* Killer, AController* KilledPlayer, const UDamageType* const DamageType);

	/** Get the master kill list */
	FORCEINLINE const TArray<TSubclassOf<USCContextKillDamageType>>& GetMasterKillList() const { return MasterKillList; }

	virtual bool CanTeamKill() const { return true; }

protected:
	// List of all possible kills
	UPROPERTY()
	TArray<TSubclassOf<USCContextKillDamageType>> MasterKillList;
	
	//////////////////////////////////////////////////
	// Character escaping
public:
	/** Notification that a character has escaped the map. */
	virtual bool CharacterEscaped(AController* CounselorController, ASCCounselorCharacter* CounselorCharacter);

	//////////////////////////////////////////////////
	// Spectating
public:
	/** Puts Player into spectator mode. */
	virtual void Spectate(AController* Player);

	//////////////////////////////////////////////////
	// Gameplay item spawning
public:
	/** @return a list of all cabins in the level */
	FORCEINLINE const TArray<ASCCabin*>& GetAllCabins() const { return AllCabins; }

	/** @return a list of all hiding spots in the level */
	FORCEINLINE const TArray<ASCHidingSpot*>& GetAllHidingSpots() const { return AllHidingSpots; }

	/** @return a list of all part spawners in the level */
	FORCEINLINE const TArray<ASCRepairPartSpawner*>& GetAllPartSpawners() const { return PartSpawners; }

	/** @return a list of all spawned vehicles in the level */
	FORCEINLINE const TArray<ASCDriveableVehicle*>& GetAllSpawnedVehicles() const { return SpawnedVehicles; }

	/** @return a list of all spawned phone junction boxes in the level */
	FORCEINLINE const TArray<ASCPhoneJunctionBox*>& GetAllPhoneJunctionBoxes() const { return SpawnedPhoneBoxes; }

	/** @return a list of all spawned CB radios in the level */
	FORCEINLINE const TArray<ASCCBRadio*>& GetAllCBRadios() const { return SpawnedCBRadios; }

	/** @return a list of all escape volumes in the level */
	FORCEINLINE const TArray<ASCEscapeVolume*>& GetAllEscapeVolumes() const { return EscapeVolumes; }

	/** @return a list of all escape pods in the level */
	FORCEINLINE const TArray<ASCEscapePod*>& GetAllEscapePods() const { return EscapePods; }

	/** @return a list of all bridge cores in the level */
	FORCEINLINE const TArray<ASCBridgeCore*>& GetAllBridgeCores() const { return BridgeCores; }

	/** @return a list of all bridge nav units in the level */
	FORCEINLINE const TArray<ASCBridgeNavUnit*>& GetAllBridgeNavUnits() const { return BridgeNavUnits; }

protected:
	/** @return true if VehicleClass is allowed to spawn in this mode. */
	virtual bool IsAllowedVehicleClass(TSubclassOf<ASCDriveableVehicle> VehicleClass) const { return true; }

	virtual void SpawnCabins();

	virtual void SpawnVehicles();

	virtual void SpawnLargeItems();

	virtual void SpawnSmallItems();

	virtual void SpawnRepairItems();

	/**
	 * Spawn repair parts for cars/boats/etc...
	 * @param ActorsNeedingRepair - Array of actors that need to be repaired
	 * @param MinDistanceBetweenParts - The minimum distance between spawned parts
	 * @param MaxDistanceFromPartOwner - The maximum distance from the actor needing repair that a part can spawn
	 * @param SpawnedPartLocations - Will use the minimum distance to try and avoid these passed in locations as well as the newly spawned part locations
	 * @param NumPartsToSpawn - The number of each part to spawn
	 */
	template <typename ActorType>
	void SpawnRepairParts(const TArray<ActorType*>& ActorsNeedingRepair, const float MinDistanceBetweenParts, const float MaxDistanceFromPartOwner, TArray<FVector> SpawnedPartLocations = TArray<FVector>(), const int32 NumPartsToSpawn = 1);

	UPROPERTY(Transient)
	TArray<ASCCabin*> AllCabins;

	UPROPERTY(Transient)
	TArray<ASCHidingSpot*> AllHidingSpots;

	UPROPERTY(Transient)
	TArray<ASCDriveableVehicle*> SpawnedVehicles;

	UPROPERTY(Transient)
	TArray<ASCRepairPartSpawner*> PartSpawners;

	UPROPERTY(Transient)
	TArray<ASCCabinet*> Cabinets;

	UPROPERTY(Transient)
	TArray<USCActorSpawnerComponent*> ActorSpawners;

	// NOTE: Can not make these UPROPERTY due to being nested containers, consider making them TWeakObjPtr<>s
	TMap<TSubclassOf<AActor>, TArray<USCActorSpawnerComponent*>> SpawnerMap;
	TMap<TSubclassOf<ASCItem>, TArray<ASCCabinet*>> CabinetMap;

	// Phone junction boxes in the map
	UPROPERTY()
	TArray<ASCPhoneJunctionBox*> SpawnedPhoneBoxes;

	// CB Radios in the map
	UPROPERTY()
	TArray<ASCCBRadio*> SpawnedCBRadios;

	// Escape Volumes in the map
	UPROPERTY()
	TArray<ASCEscapeVolume*> EscapeVolumes;

	// Escape Pods in the map
	UPROPERTY()
	TArray<ASCEscapePod*> EscapePods;

	// Bridge Cores in the map
	UPROPERTY()
	TArray<ASCBridgeCore*> BridgeCores;

	// Bridge Nav Units in the map
	UPROPERTY()
	TArray<ASCBridgeNavUnit*> BridgeNavUnits;

	//////////////////////////////////////////////////
	// Scoring Events
public:
	/** Handle scoring events */
	virtual void HandleScoreEvent(APlayerState* ScoringPlayer, TSubclassOf<USCScoringEvent> ScoreEventClass, const float ScoreModifier = 1.f);

	/** Score Event for the time a counselor has been alive */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> CounselorAliveTimeBonusScoreEvent;

	/** Score Event for when Jason kills all counselors */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> JasonKilledAllCounselorsScoreEvent;

	/** Score Event for a counselor escaping */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> CounselorEscapedScoreEvent;

	/** Score Event for when all counselors escape */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> TeamEscapedScoreEvent;

	/** Score Event for killing another counselor (friendly fire) */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> FriendlyFireScoreEvent;

	/** Score Event for killing Jason */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> KillJasonScoreEvent;

	/** Score Event for killing a counselor (as Jason) */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> KillCounselorScoreEvent;

	/** Score Event for killing a counselor with a weapon (as Jason) */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> JasonWeaponKillScoreEvent;

	/** Score Event for not taking damage in a match */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> UnscathedScoreEvent;

	/** Score Event for completing match */
	UPROPERTY()
	TSubclassOf<USCScoringEvent> CompletedMatchScoreEvent;

	//////////////////////////////////////////////////
	// Badges
public:
	/** Badge for playing as TJ */
	UPROPERTY()
	TSubclassOf<USCStatBadge> TommyJarvisBadge;

	/** Badge for using a police escape */
	UPROPERTY()
	TSubclassOf<USCStatBadge> EscapeWithPoliceBadge;

	/** Badge for surviving the match (no escape/death) */
	UPROPERTY()
	TSubclassOf<USCStatBadge> SurviveBadge;

	/** Badge for surviving the match when fear reached 100 (no escape/death) */
	UPROPERTY()
	TSubclassOf<USCStatBadge> SurviveWithFearBadge;

	//////////////////////////////////////////////////
	// Pamela Tapes
private:
	/** Requests what Pamela Tapes we're allowed to spawn today from the Backend */
	void AttemptSpawnPamelaTapes();

	/** Callback for Backend Request */
	UFUNCTION()
	void RequestPamelaTapesCallback(int32 ResponseCode, const FString& ResponseContents);

	void SpawnPamelaTapes();

	// Current active tapes that can spawn in the world
	UPROPERTY()
	TArray<TSubclassOf<USCPamelaTapeDataAsset>> ActivePamelaTapes;

public:
	FORCEINLINE TArray<TSubclassOf<USCPamelaTapeDataAsset>> GetActivePamelaTapes() { return ActivePamelaTapes; }

	//////////////////////////////////////////////////
	// Tommy Tapes
private:
	/** Requests what Tommy Tapes we're allowed to spawn today from the Backend */
	void AttemptSpawnTommyTapes();

	/** Callback for Backend Request */
	UFUNCTION()
	void RequestTommyTapesCallback(int32 ResponseCode, const FString& ResponseContents);

	void SpawnTommyTapes();

	// Current active tapes that can spawn in the world
	UPROPERTY()
	TArray<TSubclassOf<USCTommyTapeDataAsset>> ActiveTommyTapes;

public:
	FORCEINLINE TArray<TSubclassOf<USCTommyTapeDataAsset>> GetActiveTommyTapes() { return ActiveTommyTapes; }

	//////////////////////////////////////////////////
	// Hunter spawning
public:
	bool CanSpawnHunter();
	AController* ChooseHunter();

	UPROPERTY()
	AController* HunterToSpawn;

	UFUNCTION()
	void AttemptSpawnHunter();

	/** Will attempt to spawn HunterToSpawn, will not check CanSpawnHunter() */
	UFUNCTION()
	void AttemptSpawnChoosenHunter();

	UFUNCTION()
	void SpawnHunter();

	/** Begin the hunter timer */
	UFUNCTION()
	void StartHunterTimer();

	FTimerHandle TimerHandle_HunterTimer;

	/** Are we attempting to spawn the hunter? */
	FORCEINLINE bool IsSpawningHunter() const { return HunterToSpawn != nullptr || GetWorldTimerManager().GetTimerRemaining(TimerHandle_HunterTimer) > 0; }

protected:
	UFUNCTION()
	void HunterIntroComplete();

	UPROPERTY(Config)
	float MinHunterSpawnDistFromJason;

	// Timer handle for host simulating the spawn intro sequence duration
	FTimerHandle TimerHandle_HunterSpawnIntroCompleted;

	FVector HunterSpawnLocation;
	FRotator HunterSpawnRotation;

	//////////////////////////////////////////////////
	// Police
public:
	/** Begin the police timer */
	UFUNCTION()
	void StartPoliceTimer(APlayerState* CallingPlayer);

	UFUNCTION()
	void SpawnPolice();

	//////////////////////////////////////////////////
	// AI
public:
	// BehaviorTree to use for counselors
	UPROPERTY(BlueprintReadOnly)
	UBehaviorTree* CounselorBehaviorTree;
};
