// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerController_Hunt.h"
#include "SCPlayerController_Paranoia.generated.h"

class ASCCounselorCharacter;

/**
* @class ASCPlayerController_Paranoia
*/
UCLASS(Config = Game)
class SUMMERCAMP_API ASCPlayerController_Paranoia
	: public ASCPlayerController_Hunt
{
	GENERATED_UCLASS_BODY()

	/* BEGIN AController interface */
	void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	/* END AController interface */

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_TransformIntoKiller();


	UFUNCTION(Server, Reliable, WithValidation)
	void SERVER_TransformIntoCounselor();

	UFUNCTION()
	ASCCounselorCharacter* GetCounselorPawn() const {return CounselorPawn; }

	UFUNCTION(BlueprintCallable, Category = "Scoring")
	FORCEINLINE bool GetIsWinner() const { return bWonMatch; }


	/** Notify the player about a score event (this should only be temp) */
	UFUNCTION(Client, Reliable)
	void ClientDisplayPointsEvent(TSubclassOf<USCParanoiaScore> ScoringEvent, const uint8 Category, const float ScoreModifier);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transformation")
	bool CanPlayerTransform() const { return TransformCooldownTimer <= 0.0f; }

	float GetUseAbilityPercentage() const;

	UFUNCTION(Client, Reliable)
	void CLIENT_OnTransform(ASCCharacter* NewCharacter, ASCCharacter* PreviousCharacter);

protected:
	UPROPERTY(Replicated, Transient)
	ASCCounselorCharacter* CounselorPawn;

	UFUNCTION()
	void PossessKiller();

	UFUNCTION(Client, Reliable)
	void CLIENT_PossessKiller();

	UFUNCTION(Client, Reliable)
	void CLIENT_PossessCounselor();

	UFUNCTION()
	void PossessCounselor();

	UPROPERTY(Replicated)
	bool bWonMatch;

	UPROPERTY(Replicated, Transient)
	float TransformCooldownTimer;

	UPROPERTY()
	bool bHasPossessedKiller;
};
