// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerStart.h"
#include "SCPlayerStart.generated.h"

class ACameraActor;
class ASCPlayerController;
class UMediaSource;
class ALevelSequenceActor;

class ULevelSequence;
class ULevelSequencePlayer;

/**
 * @class ASCPlayerStart
 */
UCLASS(Abstract)
class SUMMERCAMP_API ASCPlayerStart
: public AILLPlayerStart
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Spawn sequence
public:
	/** @return Duration of the spawn sequence that will be used for Player. */
	float GetSpawnSequenceLength(ASCPlayerController* Player) const;

	/**
	 * Starts playing the SpawnIntroSequence on Player if available, falling back to DefaultSpawnSequence.
	 *
	 * @param Player Controller that the spawn sequence is for.
	 * @param bOutSpawnIntroBlocksInput Set to true/false depending on input should be blocked by this spawn intro.
	 * @return Playing sequence player.
	 */
	ULevelSequencePlayer* PlaySpawnSequence(ASCPlayerController* Player, bool& bOutSpawnIntroBlocksInput);

protected:
	// Spawn intro camera actor
	UPROPERTY(EditAnywhere, Category = "Intro")
	ACameraActor* SpawnIntroCamera;

	// Spawn intro sequence
	UPROPERTY(EditAnywhere, Category = "PlayerStart")
	ALevelSequenceActor* SpawnIntroSequence;

	// Default sequence to play when there is no SpawnIntroSequence
	UPROPERTY()
	ULevelSequence* DefaultSpawnSequence;
};
