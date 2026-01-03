// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Object.h"
#include "SCMusicObject.h"
#include "SCMusicObject_Counselor.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class USCMusicObject_Counselor : public USCMusicObject
{
	GENERATED_BODY()

public:
	USCMusicObject_Counselor();

	/** Min amount of time of no music playing before idle music plays. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor Music")
	float MinIdleMusicTime;

	/** Max amount of time of no music playing before idle music plays. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Counselor Music")
	float MaxIdleMusicTime;

	/** This sets the fear level the player needs to be at or greater when we start playing the hearing things sounds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	float ScarySoundsFearLevel;

	/** This sets the min radius around the player where they hear scary sounds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	float ScarySoundsHearingRadiusMin;

	/** This sets the max radius around the player where they hear scary sounds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	float ScarySoundsHearingRadiusMax;

	/** Cooldown curve that is multiplied by the min and max time ranges. */
	UPROPERTY(EditDefaultsOnly, Category="Scary Sounds")
	UCurveFloat* ScarySoundCooldownCurve;

	/** Sets the minimum amount for the cooldown delay time when. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	float ScarySoundCooldownMin;

	/** Sets the maximum amount for the cooldown delay time when. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	float ScarySoundCooldownMax;

	/** Sounds that play when fear is high and the player is indoors. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	USoundCue* Indoor_ScarySounds_Sounds;

	/** Sounds that play when fear is high and the player is outdoors. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Scary Sounds")
	USoundCue* Outdoor_ScarySounds_Sounds;
};
