// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "SCAnimNotify_PlayCinematicMusic.generated.h"

/**
 * @class USCAnimNotify_PlayCinematicMusic
 */
UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Play Cinematic Music"))
class SUMMERCAMP_API USCAnimNotify_PlayCinematicMusic
: public UAnimNotify
{
	GENERATED_BODY()

public:
	USCAnimNotify_PlayCinematicMusic();

	// Begin UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotify interface

	// Music Track to Play
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	USoundBase* MusicTrackToPlay;
};
