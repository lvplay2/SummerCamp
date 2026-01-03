// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCDossier.generated.h"

class ASCGameState;
class USCObjectiveData;

/**
 * @struct FSCDossierSaveData
 * @brief Save data that we can rebuild a dossier from
 */
USTRUCT()
struct FSCDossierSaveData
{
	GENERATED_BODY()

	// ID of the challenge class
	UPROPERTY()
	FString ChallengeID;

	// Objective IDs for any completed objectives
	UPROPERTY()
	TArray<FString> CompletedObjectives;

	// Number of attempts on this challenge
	UPROPERTY()
	int32 AttemptCount;

	// Number of completed attempts on this challenge
	UPROPERTY()
	int32 ClearedCount;

	// Best score for this Challenge, for the current score look at SCPlayerState
	UPROPERTY()
	int32 XPHighScore;

	// Fastest time for this Challenge, for the current time look at SCGameState
	UPROPERTY()
	int32 FastestCompletionTime;

	// If true, the player was not seen throughout this playthrough
	UPROPERTY()
	bool bStealthSkullUnlocked;

	// If true, the player successfully killed all counselors
	UPROPERTY()
	bool bPartyWipeSkullUnlocked;

	// If true, the player gained more XP than USCChallengeData::XPRequiredForSkull in the ActiveChallengeClass
	UPROPERTY()
	bool bHighScoreSkullUnlocked;
};

/**
* @enum ESCSkullTypes
*/
UENUM(BlueprintType)
enum class ESCSkullTypes : uint8
{
	Stealth	UMETA(DisplayName = "Stealth"),
	KillAll	UMETA(DisplayName = "KillAll"),
	Score	UMETA(DisplayName = "Score"),

	MAX		UMETA(Hidden)
};

/**
 * @struct FSCObjectiveState
 * @brief Pair of objective class and if the objective has been completed
 */
USTRUCT(BlueprintType)
struct FSCObjectiveState
{
	GENERATED_BODY()

	// Class for this objective, used to find out of it's hidden, how much CP it's worth, etc.
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<USCObjectiveData> ObjectiveClass;

	// Completed this run, or on previous runs
	UPROPERTY(BlueprintReadOnly)
	bool bCompleted;

	// Completed on previous runs only (in the backend profile)
	UPROPERTY(BlueprintReadOnly)
	bool bPreviouslyCompleted;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCObjectiveCompleted, FSCObjectiveState, ObjectiveState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCSkullCompleted, ESCSkullTypes, CompletedSkullType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCSkullFailed, ESCSkullTypes, FailedSkullType);

/**
 * @class USCDossier
 * @brief Collection of data about a given Challenge, may be used when playing the Challenge or just viewing it through the UI
 */
UCLASS()
class SUMMERCAMP_API USCDossier
: public UObject
{
	GENERATED_BODY()

public:
	USCDossier(const FObjectInitializer& ObjectInitializer);

	// Interface UObject begin
	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }
	// Interface UObject end

	//////////////////////////////////////////////////
	// Objective Status
public:
	/**
	 * Sets up the Challenge for either playing through or viewing the status of the Challenge in the UI
	 * @param ChallengeClass - Challenge this Dossier will be associated with
	 * @param bInGame - If true, skull values will not be read from the player profile and will unlock as the match progresses
	 * @param bSkipSaveFile - If true, will not load data from the save file, just fills out an empty class
	 */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void InitializeChallengeObjectivesFromClass(const TSubclassOf<USCChallengeData> ChallengeClass, const bool bInGame = true, const bool bSkipSaveFile = false);

	/** Sets up the Challenge for viewing based on the passed in Json data */
	void InitializeChellengeObjectivesFromJson(const TSharedPtr<FJsonValue>& JsonChallengeValue);

	/** Sets up the Challenge for viewing from save data */
	void InitializeChallengeObjectivesFromSaveData(const FSCDossierSaveData& SaveData);

	/** Builds a struct of all data needed to save this dossier to disk */
	FSCDossierSaveData GenerateSaveData() const;

	/** Builds a json object based on completed objectives/skulls/etc. */
	FString GetMatchCompletionJson(UObject* WorldContextObject);

	/** Combines the two dossiers and returns a JSON packet with any data in this Dossier that's not in the passed in Dossier */
	FString MergeDossiers(const USCDossier* BackendDossier);

	/** Called when design logic dictates that a specific Objective is complete */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void CompleteObjective(const TSubclassOf<USCObjectiveData> ObjectiveClass);

	/** Called on match completion to update our persistent dossiers from our match dossier */
	void HandleMatchCompleted(UObject* WorldContextObject, const USCDossier* MatchDossier);

	/** If true, this Objective should not be displayed to the user (Objective is marked as hidden and has not been completed) */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsObjectiveHidden(const TSubclassOf<USCObjectiveData> ObjectiveClass) const;

	/** If true, this Objective has been completed on this playthrough */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsObjectiveCompleted(const TSubclassOf<USCObjectiveData> ObjectiveClass) const;

	/** If true, this Objective has been completed on either this or any previous playthrough */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool HasObjectiveEverBeenCompleted(const TSubclassOf<USCObjectiveData> ObjectiveClass) const;

	/** Gets the CurrentXP / XPRequiredForSkull for the Challenge */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	float GetXPPercentComplete(UObject* WorldContextObject) const;

	/** Returns the number of completed skulls for this challenge */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	int32 GetCompletedSkullCount() const { return (int32)bStealthSkullUnlocked + (int32)bPartyWipeSkullUnlocked + (int32)bHighScoreSkullUnlocked; }

	/** Get the Active challenge class */
	FORCEINLINE TSubclassOf<USCChallengeData> GetActiveChallengeClass() const { return ActiveChallengeClass; }

	/** Gets the fastest completion time, 0 if it has never been completed */
	FORCEINLINE int32 GetFastestCompletionTime() const { return FastestCompletionTime; }

	/** Gets bStealthSkullUnlocked */
	FORCEINLINE bool IsStealthSkullUnlocked() const { return bStealthSkullUnlocked; }

	/** Gets bPartyWipeSkullUnlocked */
	FORCEINLINE bool IsPartyWipeSkillUnlocked() const { return bPartyWipeSkullUnlocked; }

	/** Gets bHighScoreSkullUnlocked */
	FORCEINLINE bool IsHighScoreSkullUnlocked() const { return bHighScoreSkullUnlocked; }

	/** Used to update bStealthSkullUnlocked */
	UFUNCTION()
	void OnSpottedByAI();

	/** Used to update bPartyWipeSkullUnlocked */
	UFUNCTION()
	void OnCounselorKilled(UObject* WorldContextObject);

	/** Used to update bHighScoreSkullUnlocked */
	UFUNCTION()
	void OnXPEarned(UObject* WorldContextObject);

	/** Exposing the counselor escaping trigger so that the Dossier can track it. */
	UFUNCTION()
	void OnCounselorEscaped();

	// Called when objectives are completed so widgets can update
	UPROPERTY(Transient, BlueprintAssignable)
	FSCObjectiveCompleted OnObjectiveCompleted;

	// Delegate for when skulls are completed. 
	FSCSkullCompleted OnSkullCompleted;
	
	// Delegate for when skulls are failed. 
	FSCSkullFailed OnSkullFailed;

	/** Gets the player's current XP. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	int32 GetXPCurrent() const;

	/** Gets the required XP for unlocking the High Score skull. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	int32 GetXPRequired() const;
	
	/** Draws debug information about the current state of the dossier */
	void DebugDraw(const ASCGameState* GameState) const;

	/** Return the number of times the challenge has been cleared */
	FORCEINLINE int32 GetClearedCount() const { return ClearedCount; }

	/** Return the number of counselors that have escaped during this mission */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	FORCEINLINE int32 GetEscapedCounselorCount() const { return EscapedCounselorCount; }

protected:
	// Stored from InitializeChallengeObjectives, this is the Challenge we're currently tracking Objectives for
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<USCChallengeData> ActiveChallengeClass;

	// List of Objectives we're currently tracking
	UPROPERTY(BlueprintReadOnly)
	TArray<FSCObjectiveState> ActiveObjectives;

	// Number of attempts on this challenge
	UPROPERTY(BlueprintReadOnly)
	int32 AttemptCount;

	// Number of completed attempts on this challenge
	UPROPERTY(BlueprintReadOnly)
	int32 ClearedCount;

	// Best score for this Challenge, for the current score look at SCPlayerState
	UPROPERTY(BlueprintReadOnly)
	int32 XPHighScore;

	// Fastest time for this Challenge, for the current time look at SCGameState
	UPROPERTY(BlueprintReadOnly)
	int32 FastestCompletionTime;

	// If true, the player was not seen throughout this playthrough
	UPROPERTY(BlueprintReadOnly)
	bool bStealthSkullUnlocked;

	// If true, the player successfully killed all counselors
	UPROPERTY(BlueprintReadOnly)
	bool bPartyWipeSkullUnlocked;

	// If true, the player gained more XP than USCChallengeData::XPRequiredForSkull in the ActiveChallengeClass
	UPROPERTY(BlueprintReadOnly)
	bool bHighScoreSkullUnlocked;

	// Tracks the number of escaped counselors per single player level/map playthrough. 
	int32 EscapedCounselorCount;
};
