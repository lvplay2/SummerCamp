// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPlayerStart.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"

#include "SCPlayerController.h"

ASCPlayerStart::ASCPlayerStart(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<ULevelSequence> DefaultSpawnIntroObject(TEXT("LevelSequence'/Game/SequenceNodes/DefaultSpawnIntro.DefaultSpawnIntro'"));
	DefaultSpawnSequence = DefaultSpawnIntroObject.Object;

	GroundDistanceLimit = 20.f;
}

float ASCPlayerStart::GetSpawnSequenceLength(ASCPlayerController* Player) const
{
	if (SpawnIntroSequence)
	{
		return SpawnIntroSequence->SequencePlayer->GetLength();
	}
	else if (UMovieScene* MovieScene = DefaultSpawnSequence ? DefaultSpawnSequence->GetMovieScene() : nullptr)
	{
		return MovieScene->GetPlaybackRange().Size<float>();
	}

	return 0.f;
}

ULevelSequencePlayer* ASCPlayerStart::PlaySpawnSequence(ASCPlayerController* Player, bool& bOutSpawnIntroBlocksInput)
{
	ULevelSequencePlayer* SequencePlayer = nullptr;

	if (SpawnIntroSequence)
	{
		// Use the configured sequence
		// These block input by default
		SequencePlayer = SpawnIntroSequence->SequencePlayer;
		bOutSpawnIntroBlocksInput = true;
	}
	else if (DefaultSpawnSequence)
	{
		// Fallback to a default sequence
		// Really this just exists to cleanup the fade to black the level intros do
		ALevelSequenceActor* SequenceActor = nullptr;
		if (ULevelSequencePlayer* NewPlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this, DefaultSpawnSequence, FMovieSceneSequencePlaybackSettings(), SequenceActor))
		{
			Player->SetViewTarget(Player->GetPawn());
			SequencePlayer = NewPlayer;
			bOutSpawnIntroBlocksInput = false;
		}
	}

	if (SequencePlayer)
	{
		SequencePlayer->Play();
	}

	return SequencePlayer;
}
