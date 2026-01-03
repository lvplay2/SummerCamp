// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameState_Offline.h"
#include "SCGameState_SPChallenges.generated.h"

class ASCCharacter;
class USCDossier;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCOnMatchEnded);

/**
 * @class ASCGameState_SPChallenges
 */
UCLASS()
class SUMMERCAMP_API ASCGameState_SPChallenges
: public ASCGameState_Offline
{
	GENERATED_UCLASS_BODY()

protected:
	// Interface AGameState begin
	virtual void HandleMatchHasEnded() override;
	// Interface AGameState end

public:
	// Interface ASCGameState begin
	virtual void CharacterKilled(ASCCharacter* Victim, const UDamageType* const KillInfo) override;
	virtual void SpottedKiller(ASCKillerCharacter* KillerSpotted, ASCCharacter* Spotter) override;
	virtual void CharacterEarnedXP(ASCPlayerState* EarningPlayer, int32 ScoreEarned) override;
	virtual bool ShouldAwardExperience() const override { return false; }
	// Interface ASCGameState end

	// Called when the match has ended before we pass things along to the backend
	UPROPERTY(BlueprintAssignable)
	FSCOnMatchEnded OnMatchEnded;

	// The active mission dossier to track challenges
	UPROPERTY(BlueprintReadOnly)
	USCDossier* Dossier;

	/** Set time limit for single player match, used for out of bounds timer. */
	UFUNCTION(BlueprintCallable, Category = "Boundary")
	void SetOutOfBoundsTime(int32 SecondsRemaining);

	// The amount of time the player has to return to the play area
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 OutOfBoundsTime;

protected:
	void OnChallengeCompletedPosted(int32 ResponseCode, const FString& ResponseContents);

	/** Checks in on a counselor that has spotted the killer to see if the undetected skull should be locked. */
	void CheckOnSpotter(ASCCharacter* Spotter);

	// Timer handle for checking on spotters
	FTimerHandle TimerHandle_CheckOnSpotter;
};
