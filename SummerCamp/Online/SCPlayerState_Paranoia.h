// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerState_Hunt.h"
#include "SCPlayerState_Paranoia.generated.h"

/**
* @class ASCPlayerState_Paranoia
*/
UCLASS()
class ASCPlayerState_Paranoia
	: public ASCPlayerState_Hunt
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Paranoia Score")
	FORCEINLINE int32 GetScore() const { return AccumulatedScore; }

	void HandleParanoiaPointEvent(const USCParanoiaScore* ScoreEvent, const float ScoreModifier = 1.f);

	// trap count for killer inventory
	UPROPERTY()
	int32 TrapCount;

	// knife count for killer inventory
	UPROPERTY()
	int32 KnifeCount;

protected:
	TArray<FSCCategorizedScoreEvent> CategorizedPointEvents;

	// Scoring events
	UPROPERTY(Transient)
	TArray<const USCParanoiaScore*> PlayerPointEvents;

	UPROPERTY(Replicated)
	int32 AccumulatedScore;
};
