// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameState_Online.h"
#include "SCGameState_Hunt.generated.h"

class ASCPlayerState;

/**
 * @class ASCGameState_Hunt
 */
UCLASS(Config = Game)
class SUMMERCAMP_API ASCGameState_Hunt
: public ASCGameState_Online
{
	GENERATED_UCLASS_BODY()
	
protected:
	// Begin AGameState interface
	virtual void HandleMatchHasEnded() override;
	virtual void HandleLeavingMap() override;
	// End AGameState interface

	/** Call back for when match stats are posted */
	void OnMatchStatsPosted(int32 ResponseCode, const FString& ResponseContents);

public:
	// Number of times a counselor has been wounded this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 CounselorsWoundedCount;

	// Number of counselors that have been pulled out of a car this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 CounselorsPulledFromCarCount;

	// Number of times Jason has been damaged this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 JasonDamagedCount;

	// Number of times Jason has been stunned this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 JasonStunnedCount;

	// Number of counselors that have escaped in a car this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 EscapedInCarCount;

	// Number of counselors that have escaped in a car this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 EscapedWithPoliceCount;

	// Number of doors destroyed this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 DoorsDestroyedCount;

	// Number of walls destroyed this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 WallsDestroyedCount;

	// Number of window dives executed this match
	UPROPERTY(Transient, BlueprintReadOnly)
	int32 WindowDiveCount;
};
