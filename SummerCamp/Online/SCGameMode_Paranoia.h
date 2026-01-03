// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameMode_Hunt.h"
#include "SCGameMode_Paranoia.generated.h"

class ASCParanoiaKillerCharacter;

/**
 * @class ASCGameMode_Paranoia
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCGameMode_Paranoia
: public ASCGameMode_Hunt
{
	GENERATED_UCLASS_BODY()

public:
	virtual void HandleMatchHasStarted() override;
	virtual	void HandlePreMatchIntro() override;
	virtual void HandleWaitingPostMatchOutro() override;
	virtual void GameSecondTimer() override;
	virtual void CharacterKilled(AController* Killer, AController* KilledPlayer, ASCCharacter* KilledCharacter, const UDamageType* const DamageType) override;

	// we dont want vehicles in paranoia
	virtual void SpawnVehicles() override { return; }
	// we dont want repairs in paranoia
	virtual void SpawnRepairItems() override;

	ASCParanoiaKillerCharacter* GetKillerPawn() { return ParanoiaKiller; }

	virtual bool CanTeamKill() const override { return true; }

	virtual void HandlePointEvent(APlayerState* ScoringPlayer, TSubclassOf<class USCParanoiaScore> ScoreEventClass, const float ScoreModifier = 1.f);
	virtual void HandlePlayerTakeDamage(AController* Hitter, AController* Victim);


	void OnPlayerTransformed(ASCCharacter* NewCharacter, ASCCharacter* PreviousCharacter);

private:
	UPROPERTY()
	TSoftClassPtr<ASCParanoiaKillerCharacter> KillerCharacterClass;

	UPROPERTY()
	ASCParanoiaKillerCharacter* ParanoiaKiller;

	UPROPERTY()
	TSubclassOf<class ASCImposterOutfit> ParanoiaOutfitClass;

	/** Score Event for the time a counselor has been alive */
	UPROPERTY()
	TSubclassOf<USCParanoiaScore> KilledJasonPointEvent;

	/** Score Event for the time a counselor has been alive */
	UPROPERTY()
	TSubclassOf<USCParanoiaScore> RoyKillPointEvent;

	/** Score Event for the time a counselor has been alive */
	UPROPERTY()
	TSubclassOf<USCParanoiaScore> JasonKillPointEvent;

	/** Score Event for the time a counselor has been alive */
	UPROPERTY()
	TSubclassOf<USCParanoiaScore> KilledTeammatePointEvent;

	/** Score Event for the time a counselor has been alive */
	UPROPERTY()
	TSubclassOf<USCParanoiaScore> WoundedTeammatePointEvent;


	// XPEvents
	UPROPERTY()
	TSubclassOf<USCScoringEvent> FriendlyKillXPEvent;

	UPROPERTY()
	TSubclassOf<USCScoringEvent> FriendlyHitXPEvent;

	UPROPERTY()
	TSubclassOf<USCScoringEvent> KillAsCounselorXPEvent;

	UPROPERTY()
	TSubclassOf<USCScoringEvent> KillAsKillerXPEvent;

	UPROPERTY()
	TSubclassOf<USCScoringEvent> ImposterKillXPEvent;

	UPROPERTY()
	TSubclassOf<USCScoringEvent> HighestScoreXPEvent;
};
