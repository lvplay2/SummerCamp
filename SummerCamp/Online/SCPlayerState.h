// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerState.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"
#include "SCPerkData.h"
#include "SCStatsAndScoringData.h"
#include "SCTypes.h"
#include "SCJasonSkin.h"

#include "SCPlayerState.generated.h"

class AContextKillActor;
class ASCCounselorCharacter;
class ASCDriveableVehicle;
class ASCFirstAid;
class ASCNoiseMaker;
class ASCPocketKnife;
class ASCTrap;
class ASCWeapon;

class USCContextKillDamageType;
class USCTapeDataAsset;
class USCTommyTapeDataAsset;
class USCPamelaTapeDataAsset;

class FVariantData;

/**
 * @class ASCPlayerState
 */
UCLASS()
class SUMMERCAMP_API ASCPlayerState
: public AILLPlayerState
{
	GENERATED_UCLASS_BODY()

	// Begin APlayerState interface
	virtual void Reset() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual bool GetOverrideVoiceAudioComponent(UAudioComponent*& VoIPAudioComponent, bool& bShouldAttenuate, UAudioComponent* CurrentAudioComponent = nullptr) const override;
	virtual FVector GetPlayerLocation() const override;
	virtual void OverrideWith(APlayerState* PlayerState) override;
	// End APlayerState interface

	// Begin AILLPlayerState interface
	virtual void SetAccountId(const FILLBackendPlayerIdentifier& InAccountId) override;
	// End AILLPlayerState interface

	/** Called when the match has ended */
	virtual void OnMatchEnded() {}
	
	//////////////////////////////////////////////////
	// Backend profile
public:
	/** Requests that the server update our backend profile. */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_RequestBackendProfileUpdate();

	/** Request the backend profile information for this user. */
	void RequestBackendProfileUpdate();

	/** Request the player stats from the backend */
	void RequestPlayerStats();

	/** Request player badges from the backend for this user */
	void RequestPlayerBadges();

	/** @return TotalExperience. */
	FORCEINLINE int64 GetTotalExperience() const { return TotalExperience; }

	/** Gets the current players level. */
	UFUNCTION(BlueprintPure, Category = "Player Profile")
	int32 GetPlayerLevel() const { return PlayerLevel; }

	UFUNCTION(BlueprintPure, Category = "Player Profile")
	int32 GetOldPlayerLevel() const { return OldPlayerLevel; }

	/** @return true if we have received profile data for this player. */
	bool HasReceivedProfile() const { return (bIsABot || PlayerLevel != 0); } // This initializes 0 but the initial level in the database is 1

	/** @return XPAmountToCurrentLevel. */
	FORCEINLINE int64 GetXPAmountToCurrentLevel() const { return XPAmountToCurrentLevel; }

	/** @return XPAmountToNextLevel. */
	FORCEINLINE int64 GetXPAmountToNextLevel() const { return XPAmountToNextLevel; }

	/** @return Ratio into the current level. */
	UFUNCTION(BlueprintPure, Category = "Player Profile")
	float GetCurrentLevelPercentage() const;

	/** Checks the OwnedCounselorPerks array to see if this player already owns this perk. */
	bool DoesPlayerOwnPerk(TSubclassOf<USCPerkData> PerkClass);

	/** Checks the OwnedJasonKills array to see if this player already owns this kill. */
	bool DoesPlayerOwnKill(TSubclassOf<ASCContextKillActor> KillClass);

	/** Delegate for letting the UI know the Perks have been updated. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCOwnedPerksUpdated);
	FSCOwnedPerksUpdated OwnedPerksUpdated;

	/** Delegate for letting the UI know the Jason Kills have been updated. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCJasonKillsUpdated);
	FSCJasonKillsUpdated OwnedJasonKillsUpdated;

	/** @return list of the currently owned counselor characters. */
	const TArray<TSoftClassPtr<ASCCounselorCharacter>>& GetOwnedCounselorCharacters() const { return OwnedCounselorCharacters; }

	/** @return list of the currently owned Jason characters. */
	const TArray<TSoftClassPtr<ASCKillerCharacter>>& GetOwnedJasonCharacters() const { return OwnedJasonCharacters; }

	// Owned counselor perks from profile - replicated to owner only
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OwnedCounselorPerks, Transient)
	TArray<FSCPerkBackendData> OwnedCounselorPerks;

	UPROPERTY(BlueprintReadOnly, Replicated)
	FString VirtualCabinRank;

protected:
	/** Callback for when the RequestBackendProfileUpdate request completes. */
	UFUNCTION()
	void OnRetrieveBackendProfile(int32 ResponseCode, const FString& ResponseContents);

	/** Callback for when the RequestPlayerStats request completes. */
	UFUNCTION()
	void OnRetrievePlayerStats(int32 ResponseCode, const FString& ResponseContents);

	/** Callback for when the RequestPlayerBadges request completes. */
	UFUNCTION()
	void OnRetrievePlayerBadges(int32 ResponseCode, const FString& ResponseContents);

	/** Used to verify ownership of the players profile settings. Also used as a callback when loading single player challenge data */
	UFUNCTION()
	void VerifyProfileData();

	// Experience from profile
	UPROPERTY(Replicated, Transient)
	int64 TotalExperience;

	// Level from profile
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	int32 PlayerLevel;

	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	int32 OldPlayerLevel;

	// Amount of XP needed to reach our current level
	UPROPERTY(Replicated, Transient)
	int64 XPAmountToCurrentLevel;

	// Amount of XP until the next Level from profile
	UPROPERTY(Replicated, Transient)
	int64 XPAmountToNextLevel;

	// Customization Points from profile
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	int32 TotalCustomizationPoints;

	// Player persistent stats
	UPROPERTY()
	FSCPlayerStats PersistentStats;

	// Player badges
	TMap<FName, int32> BadgesAwarded;

	// Owned Jason characters from profile - replicated to owner only
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	TArray<TSoftClassPtr<ASCKillerCharacter>> OwnedJasonCharacters;

	// Owned Jason kills from profile - replicated to owner only
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_OwnedJasonKills, Transient)
	TArray<TSubclassOf<ASCDynamicContextKill>> OwnedJasonKills;

	// Hit when OwnedJasonKills is replicated.
	UFUNCTION()
	void OnRep_OwnedJasonKills();

	// Owned counselor perks from profile - replicated to owner only
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	TArray<TSoftClassPtr<ASCCounselorCharacter>> OwnedCounselorCharacters;

	// Hit when OwnedCounselorPerks is replicated.
	UFUNCTION()
	void OnRep_OwnedCounselorPerks();

	//////////////////////////////////////////////////
	// Scoring
public:
	/** @return Number of kills. */
	FORCEINLINE int32 GetKills() const { return NumKills; }

	/** @return Player has taken damage. */
	FORCEINLINE bool GetHasTakenDamage() const { return bHasTakenDamage; }

	/** @return Player fear has reached 100. */
	FORCEINLINE bool GetHasBeenScared() const { return bHasBeenScared; }

	/** @return true if our ActiveCharacterClass is dead. */
	FORCEINLINE bool GetIsDead() const { return bDead; }

	/** @return true if our ActiveCharacterClass escaped. */
	FORCEINLINE bool GetEscaped() const { return bEscaped; }

	/** @return true if our PlayerController was picked to be the hunter. */
	FORCEINLINE bool GetIsHunter() const { return bIsHunter; }

	/** @return Total points. */
	FORCEINLINE float GetScore() const { return Score; }

	/** @return Scoring events by category. */
	FORCEINLINE const TArray<FSCCategorizedScoreEvent>& GetEventCategoryScores() const { return EventCategoryScores; }

	/** @return Scoring events for this player */
	FORCEINLINE const TArray<TSubclassOf<USCScoringEvent>> GetScoreEvents() const { return PlayerScoringEvents; }

	/** @return Badges this player earned this match */
	FORCEINLINE const TArray<TSubclassOf<USCStatBadge>>& GetBadgesEarned() const { return BadgesEarned; }

	/** @return Badges this player unlocked this match */
	const TArray<FSCBadgeAwardInfo> GetBadgesUnlocked() const;

	/** @return Match stats for this player */
	FORCEINLINE FSCPlayerStats GetMatchStats() const { return MatchStats; }

	/** @return The number of times this player has earned the specified badge */
	UFUNCTION(BlueprintPure, Category = "Badges")
	int32 GetBadgeAwardCount(const FName BackendId, const FName BadgeDisplayName) const;

	/** @return Time at which this player died */
	FORCEINLINE int64 GetDeathTime() const { return DeathTime; }

	/** @return Time at which this player escaped */
	FORCEINLINE int64 GetEscapeTime() const { return EscapedTime; }

	/** Mark that our ActiveCharacterClass has taken damage */
	FORCEINLINE void TookDamage() { bHasTakenDamage = true; }

	/** Mark that our ActiveCharacterClass has reached 100 fear level */
	FORCEINLINE void GotScared() { bHasBeenScared = true; }

	/** Set this player's categorized scores */
	FORCEINLINE void SetEventCategoryScores(const TArray<FSCCategorizedScoreEvent>& CategoryScores) { EventCategoryScores = CategoryScores; }

	/** Set this player's score events */
	FORCEINLINE void SetPlayerScoreEvents(const TArray<TSubclassOf<USCScoringEvent>>& ScoreEvents) { PlayerScoringEvents = ScoreEvents; }

	/** Notifies a client that they killed VictimInfo. */
	UFUNCTION(Reliable, Client)
	void InformAboutKill(const UDamageType* const KillerDamageType, ASCPlayerState* VictimPlayerState);

	/** Notifies a client that they were killed */
	UFUNCTION(Reliable, Client)
	void InformAboutKilled(ASCPlayerState* KillerPlayerState, const UDamageType* const KillerDamageType);

	/** Notifies a client when someone dies. */
	UFUNCTION(Reliable, Client)
	void InformAboutDeath(ASCPlayerState* KillerPlayerState, const UDamageType* const KillerDamageType, ASCPlayerState* VictimPlayerState);

	/** Notifies a client when someone killed himself. */
	UFUNCTION(Reliable, Client)
	void InformAboutSuicide(const UDamageType* const KillerDamagetType);

	UFUNCTION(Reliable, Client)
	void InformAboutEscape(ASCPlayerState* EscapedPlayerState);
	
	// Inform the server that the client is done playing their intro movie
	UFUNCTION(Reliable, Server, WithValidation)
	void InformIntroCompleted();

	/** Player killed someone. */
	void ScoreKill(const ASCPlayerState* Victim, const int32 Points, const UDamageType* const KillInfo);

	/** Player died. */
	void ScoreDeath(const ASCPlayerState* KilledBy, const int32 Points, const UDamageType* const KillInfo);

	/** Player escaped. */
	void ScoreEscape(const int32 Points, const ASCDriveableVehicle* EscapeVehicle);

	/** helper for scoring points */
	void ScorePoints(int32 Points);

	/** Track boat parts repaired */
	void BoatPartRepaired();

	/** Track car parts repaired */
	void CarPartRepaired();

	/** Track grid (phone and electrical) parts repaired */
	void GridPartRepaired();

	/** Track calls to police */
	void CalledPolice();

	/** Track calls to hunter */
	void CalledHunter();

	/** Track hunter matches played */
	void SpawnedAsHunter();

	/** Track Jason car slams */
	void CarSlammed();

	/** Track door break downs */
	void BrokeDoorDown();

	/** Track wall break downs */
	void BrokeWallDown() { MatchStats.WallsBrokenDown++; }

	/** Track throwing knife hits */
	void ThrowableHitCounselor(TSubclassOf<ASCThrowable> ThrowableClass);

	/** Track dives through closed windows */
	void JumpedThroughClosedWindow();

	/** Track knocking Jason's mask off */
	void KnockedJasonsMaskOff();

	/** Stunned Jason with Pamela's sweater */
	void SweaterStunnedJason();

	/** Escaped Jason's grab with a pocket knife */
	void PocketKnifeEscape(TSubclassOf<ASCPocketKnife> PocketKnife);

	/** Used a first aid spray */
	void UsedFirstAidSpray(TSubclassOf<ASCFirstAid> FirstAid);

	/** Track when trapping another player */
	void TrappedPlayer(TSubclassOf<ASCTrap> Trap);

	/** Shot Jason with a flare gun */
	void ShotJasonWithFlare(TSubclassOf<ASCWeapon> FlareGun);

	/** Hit Jason with a firecracker */
	void HitJasonWithFirecracker(TSubclassOf<ASCNoiseMaker> FireCracker);

	/** Found H2ODelirious' Teddy Bear */
	void FoundTeddyBear();

	/** Jason performed a context kill */
	void PerformedContextKill(TSubclassOf<USCContextKillDamageType> KillInfo);

	/** Checks if Jason killed all the counselors and updates those achievement/stats */
	bool DidKillAllCounselors(const int32 TotalCounselors, const bool bKilledHunter);

	/** Track item use */
	void TrackItemUse(TSubclassOf<ASCItem> ItemClass, const uint8 ItemStatFlags);

	/** Handle score events */
	void HandleScoreEvent(TSubclassOf<USCScoringEvent> ScoreEvent, const float ScoreModifier = 1.f);

	/** Sets all platform achievements to 100% and sets leaderboard stats to max unlock values */
	void UnlockAllAchievements();

	/** Clears all platform achievements earned for this player */
	void ClearAchievements();

	/** Reads achievement info for this player and caches them */
	void QueryAchievements();

	/** Update platform achievements */
	void UpdateAchievement(const FName AchievementName, const float AchievementProgress = 100.0f);

	/** Update platform achievements on the client */
	UFUNCTION(Reliable, Client)
	void ClientUpdateAchievement(const FName AchievementName, const float AchievementProgress);

	/** Update platform leaderboards (stats) */
	UFUNCTION(Reliable, Client)
	void UpdateLeaderboardStats(const FName LeaderboardStat, const int StatValue = 1);

	void SetRichPresence(const FString& MapName, bool Counselor);

	/** Update this player's platform presence */
	void UpdateRichPresence(const FVariantData& PresenceData);

	/** Unlocks a badge for this player */
	void EarnedBadge(TSubclassOf<USCStatBadge> BadgeInfo);

	/** Get the scoreboard death display text for this character */
	UFUNCTION(BlueprintPure, Category = "Scoreboard")
	const FText&  GetDeathDisplayText() const { return DeathScoreboardDisplayText; }

#if !UE_BUILD_SHIPPING
	/** Prints all stats (match + persistent) to the screen for this player */
	void PrintPlayerStats() const;
#endif

protected:
	/** Player Match Stats */
	FSCPlayerStats MatchStats;

	/** Checks to see if this player has unlocked every achievement and we should unlock the platinum */
	void CheckAchievementCompletion();

	/** Replicated handler for bDead and bEscaped. */
	UFUNCTION()
	void OnRep_DeadOrEscaped();

	// Total experience value for each score category
	TArray<FSCCategorizedScoreEvent> EventCategoryScores;

	// Scoring events
	UPROPERTY(Transient)
	TArray<TSubclassOf<USCScoringEvent>> PlayerScoringEvents;

	// Badges
	UPROPERTY(Transient)
	TArray<TSubclassOf<USCStatBadge>> BadgesEarned;

	// Number of kills
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	int32 NumKills;

	// Have we ever taken damage?
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	bool bHasTakenDamage;

	// Have we reached 100 fear?
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	bool bHasBeenScared;

	// Are we dead?
	UPROPERTY(Transient, ReplicatedUsing=OnRep_DeadOrEscaped, BlueprintReadOnly)
	bool bDead;

	// Did we manage to escape?
	UPROPERTY(Transient, ReplicatedUsing=OnRep_DeadOrEscaped, BlueprintReadOnly)
	bool bEscaped;

	// Are we the hunter?
	UPROPERTY(Transient, Replicated)
	bool bIsHunter;

	// Unix timestamp of when we died
	UPROPERTY(Transient)
	int64 DeathTime;

	// Unix timestamp of when we escaped
	UPROPERTY(Transient)
	int64 EscapedTime;

	// Scoreboard display text for how this player died
	UPROPERTY(Transient, Replicated, BlueprintReadOnly)
	FText DeathScoreboardDisplayText;
	
	//////////////////////////////////////////////////
	// Spectating
public:
	void SetSpectating(bool Spectating);

	/** @return Player is spectating. */
	FORCEINLINE bool GetSpectating() const { return bSpectating; }

protected:
	// Is this player spectating?
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Spectating, BlueprintReadOnly)
	bool bSpectating;

	UPROPERTY(Replicated, Transient)
	bool bDidSuicide;

	/** Replication handler for bSpectating. */
	UFUNCTION()
	void OnRep_Spectating();

	//////////////////////////////////////////////////
	// Character selection
public:
	// Character class they last spawned as, set on the server and client after the spawn intro
	// Will change to the hunter character class when that spawn occurs too!
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Player")
	TSubclassOf<ASCCharacter> SpawnedCharacterClass;

	/** @return true if we can change our picked characters. */
	UFUNCTION(BlueprintPure, Category = "Player|CharacterSelection")
	virtual bool CanChangePickedCharacters() const;

	/** @return Picked counselor character class. */
	FORCEINLINE TSoftClassPtr<ASCCounselorCharacter> GetPickedCounselorClass() const { return PickedCounselorClass; }

	/** Requests to change the PickedCounselorClass, and saves it to the local config. */
	UFUNCTION(BlueprintCallable, Category = "Player|CharacterSelection")
	virtual void RequestCounselorClass(TSoftClassPtr<ASCCounselorCharacter> NewCounselorClass);

	/** @return Picked perks for current character class. */
	FORCEINLINE TArray<FSCPerkBackendData> GetPickedPerks() const { return PickedPerks; }

	/** @return Picked outfit for current character class. */
	UFUNCTION(BlueprintPure, Category = "Player|CharacterSelection")
	const FSCCounselorOutfit& GetPickedCounselorOutfit() const { return PickedOutfit; }

	/** Requests to change the counselor profile. */
	UFUNCTION(BlueprintCallable, Category = "Player|CharacterSelection", Reliable, Server, WithValidation)
	void ServerRequestCounselor(const FSCCounselorCharacterProfile& NewCounselorProfile);

	/** @return Picked killer character class. */
	FORCEINLINE TSoftClassPtr<ASCKillerCharacter> GetPickedKillerClass() const { return PickedKillerClass; }

	/** @return Picked grab kills. */
	const TArray<TSubclassOf<ASCContextKillActor>>& GetPickedGrabKills() const { return PickedGrabKills; }

	/** Requests to change the killer profile. */
	UFUNCTION(BlueprintCallable, Category = "Player|CharacterSelection", Reliable, Server, WithValidation)
	void ServerRequestKiller(const FSCKillerCharacterProfile& NewKillerProfile);

	/** Requests to change the PickedKillerClass, and saves it to the local config. */
	UFUNCTION(BlueprintCallable, Category = "Player|CharacterSelection")
	virtual void RequestKillerClass(TSoftClassPtr<ASCKillerCharacter> NewKillerClass);

	/** Server request to make NewPickedKiller the killer. */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Player|CharacterSelection")
	void ServerSetPickedKillerPlayer(ASCPlayerState* NewPickedKiller);

	/** return the class of the selected Killer skin. */
	UFUNCTION(BlueprintPure, Category = "Player|CharacterSelection")
	const TSubclassOf<USCJasonSkin> GetKillerSkin() const { return PickedKillerSkin; }

	/** Have the client load their killer settings and push that data back up to the server to make sure it's loaded properly for intro videos */
	UFUNCTION(Client, Reliable, Category = "Player|CharacterSelection")
	void CLIENT_RequestLoadPlayerSettings();

	/** Notify the World settings that we've loaded our settings */
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SettingsLoaded();

	/** Jason-specific ability unlocking order. **/
	const TArray<EKillerAbilities>& GetAbilityUnlockOrder() const { return PickedKillerAbilityUnlockOrder; }
	
	UFUNCTION(BlueprintPure, Category = "Player|CharacterSelection")
	const TSoftClassPtr<ASCWeapon> GetKillerWeapon() const { return PickedKillerWeapon; }

protected:
	void SetCounselorClass(TSoftClassPtr<ASCCounselorCharacter> NewCounselorClass);
	void SetCounselorPerks(const TArray<FSCPerkBackendData>& NewPerks);
	void SetCounselorOutfit(const FSCCounselorOutfit& NewOutfit);
	void SetKillerClass(TSoftClassPtr<ASCKillerCharacter> NewKillerClass);
	void SetKillerGrabKills(const TArray<TSubclassOf<ASCContextKillActor>>& NewKills);
	void SetKillerSkin(TSubclassOf<USCJasonSkin> Skin);
	void SetKillerWeapon(const TSoftClassPtr<ASCWeapon>& Weapon);

	/** Loads in the user's selected ability unlock order for the selected Killer */
	void SetKillerAbilityUnlockOrder(const TArray<EKillerAbilities>& CustomOrder);
	
	/** Replication handler for PickedCounselorClass. */
	UFUNCTION()
	void OnRep_PickedCounselorClass(TSoftClassPtr<ASCCounselorCharacter> OldCharacter);

	/** Replication handler for PickedPerks. */
	UFUNCTION()
	void OnRep_PickedPerks();

	/** Replication handler for PickedOutfit */
	UFUNCTION()
	void OnRep_PickedOutfit();

	/** Replication handler for PickedKillerClass. */
	UFUNCTION()
	void OnRep_PickedKillerClass();

	UFUNCTION()
	void OnRep_PickedKillerSkin();

	UFUNCTION()
	void OnRep_PickedKillerWeapon();

	// Picked counselor character class
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PickedCounselorClass, Transient)
	TSoftClassPtr<ASCCounselorCharacter> PickedCounselorClass;

	// Picked counselor perks
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PickedPerks, Transient)
	TArray<FSCPerkBackendData> PickedPerks;

	// Picked counselor outfit
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PickedOutfit, Replicated, Transient)
	FSCCounselorOutfit PickedOutfit;

	// Picked killer character class
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PickedKillerClass, Transient)
	TSoftClassPtr<ASCKillerCharacter> PickedKillerClass;

	// Picked killer skin class
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PickedKillerSkin, Transient)
	TSubclassOf<USCJasonSkin> PickedKillerSkin;

	// Picked killer grab kills
	UPROPERTY(Transient)
	TArray<TSubclassOf<ASCContextKillActor>> PickedGrabKills;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PickedKillerWeapon, Transient)
	TSoftClassPtr<ASCWeapon> PickedKillerWeapon;

	// Array used for loading the ability unlock order for the selected Killer.
	TArray<EKillerAbilities> PickedKillerAbilityUnlockOrder;

	//////////////////////////////////////////////////
	// Killer picking
public:
	/** @return true if this player was the picked killer. */
	FORCEINLINE bool IsPickedKiller() const { return bIsPickedKiller; }

protected:
	/** Helper function to change bIsPickedKiller and simulate OnRep_bIsPickedKiller. */
	void SetIsPickedKiller(const bool bNewPickedKiller);

	/** Replication handler for bIsPickedKiller. */
	UFUNCTION()
	void OnRep_bIsPickedKiller();

	// Determines if player is ready to start match
	UPROPERTY(Transient, ReplicatedUsing=OnRep_bIsPickedKiller, BlueprintReadOnly, Category = "Player|KillerPicking")
	bool bIsPickedKiller;

	//////////////////////////////////////////////////
	// Active character class
public:
	/** @return true if this player can respawn as ActiveCharacterClass. */
	virtual bool CanSpawnActiveCharacter() const;

	virtual void OnRevivePlayer();

	/** @return ActiveCharacterClass. */
	FORCEINLINE TSubclassOf<ASCCharacter> GetActiveCharacterClass() const { return ActiveCharacterClass.Get(); }

	/** @return true if ActiveCharacterClass is a killer class. */
	UFUNCTION(BlueprintPure, Category = "Player|CharacterSelection")
	bool IsActiveCharacterKiller() const;

	/** Changes ActiveCharacterClass and simulates OnRep_ActiveCharacterClass. */
	void SetActiveCharacterClass(TSoftClassPtr<ASCCharacter> NewActiveCharacterClass);

	/** Loads NewSoftActiveCharacterClass asynchronously if needed and calls to SetActiveCharacterClass when loaded */
	void AsyncSetActiveCharacterClass(TSoftClassPtr<ASCCharacter> NewSoftActiveCharacterClass);

	/** Requests that this player call ServerReceivePerksAndOutfit with their preferred perks and outfit for CounselorClass. */
	UFUNCTION(Client, Reliable)
	void ClientRequestPerksAndOutfit(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass);

	/** Requests that this player call ServerSendGrabKills with their preferred kills for KillerClass. */
	UFUNCTION(Client, Reliable)
	void ClientRequestGrabKills(const TSoftClassPtr<ASCKillerCharacter>& KillerClass);

	UFUNCTION(Client, Reliable)
	void ClientRequestSelectedWeapon(const TSoftClassPtr<ASCKillerCharacter>& KillerClass);
	
protected:
	/** Reply for ClientRequestPerksAndOutfit. */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReceivePerksAndOutfit(const TArray<FSCPerkBackendData>& Perks, const FSCCounselorOutfit& Outfit);

	/** Reply for ClientRequestGrabKills. */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReceiveGrabKills(const TArray<TSubclassOf<ASCContextKillActor>>& GrabKills);

	/** Reply for ClientRequestSelectedWeapon. */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReceiveSelectedWeapon(FSoftObjectPath WeaponClassPath);

	/** Force Update of the killer selected skin */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetKillerSkin(TSubclassOf<USCJasonSkin> Skin);

	/** Replication handler for ActiveCharacterClass. */
	UFUNCTION()
	void OnRep_ActiveCharacterClass();

	// Character class picked for this player
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ActiveCharacterClass, Transient)
	TSoftClassPtr<ASCCharacter> ActiveCharacterClass;

	// Has this player spawned into the match
	bool bSpawnedInMathch;

public:
	// Has the spawn intro been viewed for this player?
	bool bPlayedSpawnIntro;

	// Set when we are done with the intro
	UPROPERTY(Transient)
	bool bIntroSequenceCompleted;

	/** Mark this player as spawned into the match */
	FORCEINLINE void CharacterSpawned() { bSpawnedInMathch = true; }

	/** @return true if this player spawned into the match */
	FORCEINLINE bool DidSpawnIntoMatch() const { return bSpawnedInMathch; }

	// Did this player disconnect for any reason?
	UPROPERTY(Transient, BlueprintReadOnly)
	bool bLoggedOut;

protected:

	//////////////////////////////////////////////////
	// Perk Data
public:
	/** Only Apply perks once */
	bool bPerksApplied;

	/** @return Array of Perks */
	UFUNCTION(BlueprintPure, Category = "Perks")
	FORCEINLINE TArray<USCPerkData*> GetActivePerks() const { return ActivePerks; }

	/** Sets this player's perks with data from the backend. */
	void SetActivePerks(const TArray<FSCPerkBackendData>& PerkData);

	/** Sets this player's perks with default perk data. This function should only be used for testing perks or giving perks to AI. */
	void SetActivePerks(const TArray<USCPerkData*>& PerkData);

private:
	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_SetActivePerks(const TArray<USCPerkData*>& PerkData);

	UFUNCTION(Client, Reliable)
	void CLIENT_SetActivePerks(const TArray<USCPerkData*>& PerkData);


protected:
	UPROPERTY(Transient)
	TArray<USCPerkData*> ActivePerks;

	//////////////////////////////////////////////////
	// Avatar
public:
	/** @return Avatar texture asset. */
	UFUNCTION(BlueprintCallable, Category = "Avatar")
	UTexture2D* GetAvatar();

private:
	// Cached avatar reference
	UPROPERTY()
	UTexture2D* CachedAvatar;

	//////////////////////////////////////////////////
	// Attenuated Voice
public:
	UPROPERTY()
	UAudioComponent* VoiceAudioComponent;

	/** 
	 * @param OtherPlayer the player to check if they are within attenuation range
	 * @return true if the specified player is within attenuation range 
	 */
	UFUNCTION(BlueprintPure, Category = "VoIP")
	bool IsPlayerInAttenuationRange(APlayerState* OtherPlayer) const;

	//////////////////////////////////////////////////
	// Inventory
public:
	/** @return true if this player's character has the walkie talkie. NOTE: Relies on charcters being always relevant! Will need to be made a repped property when that changes. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasWalkieTalkie() const;

	//////////////////////////////////////////////////
	// Pamela Tapes
public:
	/** Pamela tapes this player current owns. */
	UPROPERTY()
	TArray<TSubclassOf<USCPamelaTapeDataAsset>> UnlockedPamelaTapes;

	/** Tommy tapes this player currently owns. */
	UPROPERTY()
	TArray<TSubclassOf<USCTommyTapeDataAsset>> UnlockedTommyTapes;

	void AddTapeToInventory(TSubclassOf<USCTapeDataAsset> TapeClass);

protected:
	/** Callback for when AddPamelaTapeToInventory requests completes */
	UFUNCTION()
	void OnAddTapeToInventory(int32 ResponseCode, const FString& ResponseContents);
};
