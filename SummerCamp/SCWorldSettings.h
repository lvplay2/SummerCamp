// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLWorldSettings.h"
#include "SCCounselorCharacter.h"
#include "SCWorldSettings.generated.h"

class ACameraActor;
class ASCEscapePod;

class UMediaSource;
class UMaterialParameterCollection;
class USkeletalMesh;

class ASCItem;
class ASCKillerCharacter;
class ASCPamelaTape;
class ASCTommyTape;

class USCVideoPlayerWidget;
class USCChallengeData;

class USCJasonSkin;

#if WITH_EDITOR
extern TAutoConsoleVariable<int32> CVarEmulateMatchStartInPIE;
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKillerClassSetEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCOnRainingStateChanged, bool, bRainEnabled);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSCLevelIntroDelegate, TSubclassOf<ASCKillerCharacter>, KillerClass, TSubclassOf<USCJasonSkin>, KillerSkin, UStaticMesh*, KillerWeapon);

/**
 * @struct FLevelSequenceVideo
 */
USTRUCT()
struct FLevelSequenceVideo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<ASCKillerCharacter> KillerClass;

	UPROPERTY(EditAnywhere)
	ALevelSequenceActor* Sequence;
};

/**
 * @struct FLargeItemSpawnData
 */
USTRUCT()
struct FLargeItemSpawnData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere)
	int32 MinCount;

	UPROPERTY(EditAnywhere)
	int32 MaxCount;
};

/**
 * @struct FSmallItemSpawnData
 */
USTRUCT()
struct FSmallItemSpawnData
{
	GENERATED_USTRUCT_BODY()

public:
	FSmallItemSpawnData()
	: MinSpawnDistance(1000.f)
	{
	}

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASCItem> ItemClass;

	UPROPERTY(EditAnywhere)
	int32 MinCount;
	
	UPROPERTY(EditAnywhere)
	int32 MaxCount;

	UPROPERTY(EditAnywhere)
	float MinSpawnDistance;
};

/**
 * @struct FSCVehicleSpawnData
 */
USTRUCT()
struct FSCVehicleSpawnData
{
	GENERATED_USTRUCT_BODY()

public:
	/** Select a random vehicle from this list to spawn */
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class ASCDriveableVehicle>> VehicleClasses;
};

/**
 * @struct FSCJasonCustomizeData
 */
USTRUCT()
struct FSCJasonCustomizeData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ASCThrowable> ThrowingKinfeClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ASCTrap> TrapClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ASCContextKillActor> ContextDeathClass;
};

/**
 * @class ASCWorldSettings
 */
UCLASS()
class SUMMERCAMP_API ASCWorldSettings
: public AILLWorldSettings
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UObject interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UObject interface

	// Begin AWorldSettings interface
	virtual void NotifyBeginPlay() override;
	virtual void NotifyMatchStarted() override;
	// End AWorldSettings interface

	/** @return ASCWorldSettings instance for WorldContextObject. */
	UFUNCTION(BlueprintCallable, Category="Default", meta=(WorldContext="WorldContextObject"))
	static ASCWorldSettings* GetSCWorldSettings(UObject* WorldContextObject);

	UFUNCTION()
	void SettingsLoaded();

private:
	/** Helper function for IntroLevels and OutroLevels. */
	void SetLocalVisibilityForLevels(const TArray<FName>& LevelNames, const bool bShouldBeVisible, const bool bShouldBeLoaded);
	
public:
	/** Skips to the end of the currently playing level intro/outro. */
	UFUNCTION(BlueprintCallable, Category="IntroOutro")
	void SkipLevelIntroOutro();

	//////////////////////////////////////////////////
	// Level intro sequence
public:
	/** Gets the correct video media asset to pass to the UMG video player for the selected killer. */
	ALevelSequenceActor* GetIntroVideo() const;

	/**
	 * Called from the game mode to start the level intro sequence for everybody.
	 *
	 * @param bSkipToEnd Skip to the end of the level intro state?
	 */
	void StartLevelIntro(bool bSkipToEnd);

	/**
	 * Stops the level intro sequence if it's playing.
	 *
	 * @param bSkipCooldown Immediately call to IntroCompleted instead of waiting for post-intro cool down to complete.
	 */
	void StopLevelIntro(const bool bSkipCooldown = false);

	/** Helper function to set intro killer mesh. */
	UFUNCTION(BlueprintPure, Category="Intro")
	USkeletalMesh* GetKillerMesh() const;

	/** Helper function to set intro killer mask mesh. */
	UFUNCTION(BlueprintPure, Category="Intro")
	USkeletalMesh* GetKillerMaskMesh() const;

	// Delegate fired before the level intro starts
	UPROPERTY(BlueprintAssignable, Category="Intro")
	FSCLevelIntroDelegate LevelIntroStartingDelegate;

	// Delegate fired after the level intro starts
	UPROPERTY(BlueprintAssignable, Category="Intro")
	FSCLevelIntroDelegate LevelIntroStartedDelegate;

	// Delegate fired after the level intro skipped
	UPROPERTY(BlueprintAssignable, Category="Intro")
	FSCLevelIntroDelegate LevelIntroSkippedDelegate;

	// Delegate fired after the level intro completes
	UPROPERTY(BlueprintAssignable, Category="Intro")
	FSCLevelIntroDelegate LevelIntroCompletedDelegate;

	UPROPERTY(BlueprintAssignable)
	FKillerClassSetEvent KillerClassSet;

protected:
	/** Broadcast function to play the level IntroSequences. */
	UFUNCTION(NetMulticast, Reliable)
	void ClientsPlayIntro(ALevelSequenceActor* Sequence, TSubclassOf<ASCKillerCharacter> SelectedKillerClass, const bool bSkipToEnd, TSubclassOf<USCJasonSkin> KillerSkin, const TSoftClassPtr<ASCWeapon>& KillerWeapon);

	/** Helper for playing the IntroSequences. */
	void IntroStart(const bool bSkipToEnd, TSubclassOf<USCJasonSkin> KillerSkin, UStaticMesh* KillerWeapon);

	/** Event called by the intro sequence when it's ready to be completed */
	UFUNCTION()
	void IntroSequenceCompleted();

	/** Called in OnStop in the intro sequence */
	UFUNCTION()
	void IntroStopped();

	/** Called when the intro is complete for all players */
	UFUNCTION()
	void IntroCompleted();

	// Intro level(s): only visible during the intro
	UPROPERTY(EditDefaultsOnly, Category="Intro")
	TArray<FName> IntroLevels;

	// Intro sequences
	UPROPERTY(EditDefaultsOnly, Category="Intro")
	TArray<FLevelSequenceVideo> IntroSequences;

	// Sequence picked from IntroSequences that is going to play on BeginPlay or is already playing
	UPROPERTY(Transient)
	ALevelSequenceActor* LevelIntroSequence;

	UPROPERTY(Transient)
	bool WaitingOnSettingsLoad;

	// Timer handle for IntroVideoCompleted
	FTimerHandle TimerHandle_IntroVideoCompleted;

	// Are we waiting for NotifyBeginPlay before attempting to skip to the end of the intro?
	bool bSkipIntroPendingBeginPlay;

	// Is a level IntroSequences playing?
	bool bIntroPlaying;

	UPROPERTY(Transient, BlueprintReadOnly)
	TSubclassOf<ASCKillerCharacter> KillerClass;

	// Cached killer skin so we can use it just in case the level isn't fully loaded when the intro request is made.
	UPROPERTY(Transient)
	TSubclassOf<USCJasonSkin> CachedKillerSkin;

	// Cached killer weapon so we can use it just in case the level isn't fully loaded when the intro request is made.
	UPROPERTY(Transient)
	TSubclassOf<ASCWeapon> CachedKillerWeapon;

	//////////////////////////////////////////////////
	// Level outro sequence
public:
	/** Check to see whether clients are all done playing the intro movie */
	void CheckClientsIntroState();

	/** Gets the correct video media asset to pass to the UMG video player for the selected killer. */
	ALevelSequenceActor* GetOutroVideo() const;

	/** Picks a level outro to play, telling all connected clients to do the same. */
	void StartLevelOutro();

	/** Stops the currently playing level outro. */
	void StopLevelOutro();

	// Camera for post-match
	UPROPERTY(EditDefaultsOnly, Category="Outro|Camera")
	ACameraActor* PostMatchCamera;

protected:
	/** Tells all clients to start playing the outro. Called from StartLevelOutro. */
	UFUNCTION(NetMulticast, Reliable)
	void ClientsPlayOutro(ALevelSequenceActor* Sequence, TSubclassOf<ASCKillerCharacter> SelectedKillerClass, TSubclassOf<USCJasonSkin> KillerSkin, const TSoftClassPtr<ASCWeapon>& KillerWeapon);

	/** Outro playback completed, wait a little while before ending the match to ensure everyone finishes it. */
	UFUNCTION()
	void OutroStopCooldown();

	/** Outro playback + cool down completed, or no outro played. End the match completely. */
	UFUNCTION()
	void OutroCompleted();

	// Outro level(s): only visible during the outro
	UPROPERTY(EditDefaultsOnly, Category="Outro")
	TArray<FName> OutroLevels;

	// Outro sequences
	UPROPERTY(EditDefaultsOnly, Category="Outro")
	TArray<FLevelSequenceVideo> OutroSequences;

	// Sequence picked from OutroSequences that is playing
	UPROPERTY(Transient)
	ALevelSequenceActor* LevelOutroSequence;

	// Delegate fired before the level outro starts
	UPROPERTY(BlueprintAssignable, Category="Outro")
	FSCLevelIntroDelegate LevelOutroStartingDelegate;

	// Delegate fired after the level outro starts
	UPROPERTY(BlueprintAssignable, Category="Outro")
	FSCLevelIntroDelegate LevelOutroStartedDelegate;

	// Delegate fired after the level outro skipped
	UPROPERTY(BlueprintAssignable, Category="Outro")
	FSCLevelIntroDelegate LevelOutroSkippedDelegate;

	// Delegate fired after the level outro completes
	UPROPERTY(BlueprintAssignable, Category="Outro")
	FSCLevelIntroDelegate LevelOutroCompletedDelegate;

	// Timer handle for OutroCompleted
	FTimerHandle TimerHandle_OutroCompleted;

	// Timer handle for OutroVideoCompleted
	FTimerHandle TimerHandle_OutroVideoCompleted;

	// Is a level OutroSequences playing?
	bool bOutroPlaying;

	//////////////////////////////////////////////////
	// Spawner settings
public:
	// - Cabins

	// The total number of cabins allowed to spawn on the map
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Cabins")
	int32 TotalCabinCount;

	// - Vehicles

	UPROPERTY(EditDefaultsOnly, Category="Spawning|Vehicles")
	TArray<FSCVehicleSpawnData> VehiclesToSpawn;

	// The maximum distance a car part can spawn from its respective vehicle (0 is no limit)
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Vehicles")
	float MaxCarPartDistance;

	// The maximum distance a boat part can spawn from its respective vehicle (0 is no limit)
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Vehicles")
	float MaxBoatPartDistance;

	// The Minimum distance vehicles need to spawn apart
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Vehicles")
	float MinVehicleSpawnDistance;

	// The Minimum distance vehicle PARTS need to spawn apart
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Vehicles")
	float MinVehiclePartSpawnDistance;

	// - Phone fuses

	// The maximum distance a fuse can spawn from a phone junction box (0 is no limit)
	UPROPERTY(EditDefaultsOnly, Category="Spawning|PhoneFuse")
	float MaxFuseDistance;

	// The minimum distance fuses need to spawn apart
	UPROPERTY(EditDefaultsOnly, Category="Spawning|PhoneFuse")
	float MinFuseSpawnDistance;

	// The total number of fuses allowed to spawn on the map
	UPROPERTY(EditDefaultsOnly, Category="Spawning|PhoneFuse")
	int32 TotalFuseCount;

	// - Items and other Actors
	// Large "items" that spawn outside of cabinets (can be normal actors, not necessarily items. Bad naming)
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Items")
	TArray<FLargeItemSpawnData> LargeItemSpawnData;

	// Small items to spawn in cabinets
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Items")
	TArray<FSmallItemSpawnData> SmallItemSpawnData;

	UPROPERTY(EditDefaultsOnly, Category="Spawning|Items")
	TSubclassOf<ASCPamelaTape> PamelaTapeClass;

	UPROPERTY(EditDefaultsOnly, Category="Spawning|Items")
	TSubclassOf<ASCTommyTape> TommyTapeClass;

	// - Override
	// Overrides the item spawning system and reverts to actor spawners spawning using random logic
	UPROPERTY(EditDefaultsOnly, Category="Spawning")
	bool bOverrideSpawning;

	UPROPERTY(EditDefaultsOnly, Category="Spawning|Characters")
	FSCJasonCustomizeData JasonCustomizeData;

	// Character class to spawn in for the hunter/hero character
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Characters")
	TSoftClassPtr<ASCCounselorCharacter> HeroCharacterClass;

	//////////////////////////////////////////////////
	// Random level streaming
public:
	void LoadRandomLevel();
	void UnloadRandomLevel();

	// Randomly selected level index
	UPROPERTY(Transient, Replicated)
	int32 RandomLevelIndex;

	// List of levels to potentially be streamed in
	UPROPERTY(EditDefaultsOnly, Category="Spawning|Level")
	TArray<FName> RandomLevels;

protected:
	/** Notification for when a random level has finished loading, or when no random level was loaded. */
	UFUNCTION()
	void OnRandomLevelLoaded();

	//////////////////////////////////////////////////////
	// Escape Pods
public:

	// The list of available escape pods in the level
	UPROPERTY(Transient, BlueprintReadOnly)
	TArray<ASCEscapePod*> ActivePods;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_RandomizedPods)
	TArray<ASCEscapePod*> RandomizedPods;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning|Level")
	int32 NumActiveEscapePods;

	// Handle setting deactivated pods.
	UFUNCTION()
	void OnRep_RandomizedPods();

	//////////////////////////////////////////////////
	// Rain
public:
	// Level to load if it is raining
	UPROPERTY(EditDefaultsOnly, Category="Rain")
	FName RainEnabledLevel;

	// Level to load if it is a clear night
	UPROPERTY(EditDefaultsOnly, Category="Rain")
	FName RainDisabledLevel;

	// Rain chance (0->1 percentage) for level
	UPROPERTY(EditDefaultsOnly, Category="Rain", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float RainChance;

	// Material parameter collection to turn rain on for all materials
	UPROPERTY(EditDefaultsOnly, Category="Rain")
	UMaterialParameterCollection* RainMaterialParameterColleciton;

	// Called from OnRep_IsRaining
	FSCOnRainingStateChanged OnRainingStateChanged;

	/** Turns rain on or off for the entire level, should be called once at match start */
	void SetRaining(bool bShouldRain);

	/** Returns bIsRaining */
	FORCEINLINE bool GetIsRaining() const { return bIsRaining; }

protected:
	// Set to bRainPending when OnRainLevelLoaded is called
	UPROPERTY(Transient, ReplicatedUsing=OnRep_IsRaining)
	bool bIsRaining;

	/** Called when the enabled rain level is loaded */
	UFUNCTION()
	void OnRainLevelLoaded();

	/** Called when the disabled rain level is loaded */
	UFUNCTION()
	void OnDryLevelLoaded();

	/** Sets up world for the rain */
	UFUNCTION()
	void OnRep_IsRaining();

	//////////////////////////////////////////////////
	// Police
public:
	// Sound effect to play when the police are called
	UPROPERTY(EditDefaultsOnly, Category="Police Escape")
	USoundCue* PoliceCalledSFX;

	//////////////////////////////////////////////////
	// Single player
public:
	UPROPERTY(EditDefaultsOnly, Category="Single Player")
	TSubclassOf<USCChallengeData> ChallengeOverride;

	// Do we want this level to have a non default match time?
	UPROPERTY(EditDefaultsOnly, Category="Single Player", meta=(InlineEditConditionToggle))
	bool bOverrideMatchTime;

	// The time in seconds that this level should have for a time limit.
	UPROPERTY(EditDefaultsOnly, Category="Single Player", meta=(EditCondition="bOverrideMatchTime", ClampMin="1.0", ClampMax="6000.0", UIMin="1.0", UIMax="6000.0"))
	int32 LevelMatchTimeOverride;
};
