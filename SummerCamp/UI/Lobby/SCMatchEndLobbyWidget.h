// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCStatsAndScoringData.h"
#include "SCMatchEndLobbyWidget.generated.h"

/**
 * @class USCMatchEndLobbyWidget
 */
UCLASS()
class SUMMERCAMP_API USCMatchEndLobbyWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	// Begin UWidget Interface
	virtual void RemoveFromParent() override;
	// End UWidget Interface
	
	/** Sends the score event catagory totals to the Match End Lobby Widget */
	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnUpdateScoreEvents(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, const int32& OldPlayerLevel, const TArray<FSCBadgeAwardInfo>& BadgesAwarded);

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnUpdateHunterScore();

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnDisplayUnlock(const TArray<FSCBadgeAwardInfo>& BadgesAwarded);

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnAddMatchEndLine(const FText& CategoryName, const FText& XPAmount, const float& Delay);

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnAddXPBonusLine(const FText& XPBonus, const float& Delay);

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnUpdateTotalScore(const FText& TotalScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void PlayLevelUp();

	UFUNCTION(BlueprintImplementableEvent, Category = "End Match")
	void OnNext();

	UFUNCTION(BlueprintCallable, Category = "End Match")
	void Native_UpdateScoreEvents(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, const int32& InOldPlayerLevel, const TArray<FSCBadgeAwardInfo>& BadgesAwarded);

	UFUNCTION()
	void DisplayScore();

	UFUNCTION()
	void DisplayPlayerScore(const TArray<FSCCategorizedScoreEvent>& ScoreEvents, bool AsHunter);

	UFUNCTION()
	void Native_DisplayUnlocks();

	UFUNCTION()
	void CheckLevelUp();

	UFUNCTION()
	void ShowTotal();

	UFUNCTION()
	void ShowHunterScore();

	UFUNCTION(BlueprintPure, Category = "Character UI")
	void GetCharacterDetails(TSoftClassPtr<ASCCharacter> CounselorClass, FText& CharacterName, FText& SecondaryName, UTexture2D*& LargeImage) const;

	UFUNCTION(BlueprintCallable, Category = "Character UI")
	bool WasCharacterJustUnlocked(TSoftClassPtr<ASCCharacter> CharacterClass);

	UFUNCTION(BlueprintPure, Category = "Character UI")
	ASCPlayerState* GetLocalPlayerState();

	UFUNCTION(BlueprintPure, Category = "Character UI")
	bool GetIsHunter();

	UFUNCTION(BlueprintCallable, Category = "End Match")
	void Native_OnNext();

	UFUNCTION(BlueprintCallable, Category = "End Match")
	void SetLocalPlayer(ASCPlayerState* InPlayerState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Character UI")
	void OnPlayerStateAcquired(ASCPlayerState* NewPlayerState);

protected:
	ASCPlayerState* LocalPlayerState;

	int32 CachedPlayerLevel;
	int32 AccumulatedScore;

	UPROPERTY(BlueprintReadOnly, Category = "End Match")
	bool ProcessedHunterScore;

	UPROPERTY(EditDefaultsOnly, Category = "End Match")
	float DisplayPlayerScoreDelay;

	UPROPERTY(EditDefaultsOnly, Category = "End Match")
	float DisplayHunterScoreDelay;

	UPROPERTY(EditDefaultsOnly, Category = "End Match")
	float LevelUpDisplayDelay;

	UPROPERTY(EditDefaultsOnly, Category = "Unlocks")
	TArray<TSoftClassPtr<ASCCharacter>> UnlockableCharacters;

	UPROPERTY(BlueprintReadOnly, Category = "Unlocks")
	TSoftClassPtr<ASCCharacter> UnlockedCharacter;

	FTimerHandle TimerHandle_EndMatchAnimTimer;

	TArray<FSCCategorizedScoreEvent> CachedScoreEvents;
	TArray<FSCBadgeAwardInfo> CachedBadgesAwarded;
};
