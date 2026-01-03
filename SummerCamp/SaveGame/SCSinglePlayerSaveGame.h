// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"

#include "SCDossier.h"

#include "SCSinglePlayerSaveGame.generated.h"

class USCChallengeData;

DECLARE_DYNAMIC_DELEGATE(FSCChallengesLoadedDelegate);

/**
 * @class USCSinglePlayerSaveGame
 */
UCLASS()
class USCSinglePlayerSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_BODY()

public:
	USCSinglePlayerSaveGame(const FObjectInitializer& ObjectInitializer);

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	virtual void SaveGameLoaded(AILLPlayerController* PlayerController) override;
	// End UILLPlayerSettingsSaveGame interface


	////////////////////////////////////////////////
	// Singleplayer Challenges
public:
	/**
	 * Gets a dossier matching the passed in challenge class
	 * @param ChallengeClass - Class to get the ChallengeID from
	 * @param bRequireBackendDossier - If true, will only return a dossier from our internal ChallengeDossiers, if false a new Dossier will be created based on the class passed in
	 * @return Dossier matching the passed in class (may be null if bRequireBackendDossier is true, or the class is bad)
	 */
	UFUNCTION(BlueprintPure, Category = "Dossier")
	USCDossier* GetChallengeDossierByClass(const TSubclassOf<USCChallengeData>& ChallengeClass, const bool bRequireBackendDossier);

	/**
	 * Gets a dossier matching the passed in challenge ID
	 * @param ChallengeID - Unique ID of the challenge
	 * @param bRequireBackendDossier - If true, will only return a dossier from our internal ChallengeDossiers, if false a new Dossier will be created based on the ID passed in
	 * @return Dossier matching the passed in ID (may be null if bRequireBackendDossier is true, or the ID is bad)
	 */
	UFUNCTION(BlueprintPure, Category = "Dossier")
	USCDossier* GetChallengeDossierByID(const FString& ChallengeID, const bool bRequireBackendDossier);

	void OnMatchCompleted(AILLPlayerController* PlayerController, const USCDossier* MatchDossier);

	/** Gets the highest completed challenge index, used to track what challenges should remain locked (HighestIndex + 1) */
	UFUNCTION(BlueprintPure, Category = "Dossier")
	int32 GetHighestCompletedChallengeIndex() const;

	/** Request the challenge profile for this user. */
	void RequestChallengeProfile(UObject* WorldContextObject);

	/** return wether the passed in emote is unlocked or doesn't require an unlock */
	bool IsEmoteUnlocked(FName EmoteUnlockID);

	/** Callback when the challenge data is received and processed from the backend */
	UPROPERTY(Transient)
	FSCChallengesLoadedDelegate OnChallengesLoaded;

	/** Used to note if we've viewed the singe player tutorial screens, if not the challenges menu will have a button to go to them */
	UPROPERTY()
	bool bViewedChallengesTutorial;

	/** Sets our variable noting that we've viewed the Challenges Tutorial */
	UFUNCTION()
	void SetHasViewedChallengesTutorial(AILLPlayerController* PlayerController);

	/** Check to see if we've received data from the backend */
	UFUNCTION()
	bool HasReceivedProfile() const {return bBackendLoaded;}

protected:
	/** Callback for when RequestChallengeProfile request completes */
	UFUNCTION()
	void OnRetrieveChallengeProfile(int32 ResponseCode, const FString& ResponseContents);

	/** Callback for when ReportChallengeComplete completes */
	UFUNCTION()
	void OnChallengeCompletedPosted(int32 ResponseCode, const FString& ResponseContents);

	// Controller used when saving data so we can access the world in further updates
	UPROPERTY(Transient)
	AILLPlayerController* OwningController;

	// List of challenges from the save file, merged with the backend
	UPROPERTY(Transient)
	TArray<USCDossier*> ChallengeDossiers;

	// Actual list of save data, duplicates the data found in ChallengeDossiers, just in a more compact file-friendly way
	UPROPERTY()
	TArray<FSCDossierSaveData> ChallengeSaveData;

	// List of unlocked emote IDs
	UPROPERTY()
	TArray<FName> EmoteUnlockIDs;

	// Number of merged challenge updates we sent to the backend. Once all are processed we can ask for a backend profile update to make sure our CP is correct
	int32 PendingChallengeUpdates;

	// True if we have received data from the server after requesting loading the players challenge data
	bool bBackendLoaded;
};
