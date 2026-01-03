// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Object.h"
#include "SCMusicObject.generated.h"

/**
 * @enum EMusicEvent
 */
UENUM(BlueprintType)
enum class EMusicEventType : uint8
{
	MUSICTRACK,
	STINGER,
};

/**
 * @struct FMusicTrack
 */
USTRUCT(BlueprintType)
struct FMusicTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName EventName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EMusicEventType MusicEventType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool HighPriorityStinger;

	/** Music Track */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundBase* MusicTrack; // OPTIMIZE: pjackson: TAssetPtr

	/** Amount of time (in seconds) to Fade In the Music Track */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FadeInTime;

	/** Amount of time (in seconds) to Fade Out the Music Track */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FadeOutTime;
};

/**
 * @class USCMusicObject
 */
UCLASS(Blueprintable)
class USCMusicObject
: public UDataAsset
{
	GENERATED_BODY()

public:
	USCMusicObject();

	/** Array of all the different music events that the designers define. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TArray<FMusicTrack> MusicData;

	/** Cooldown time between stringers so that we don't play to many near each other. Could get annoying. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	float StingerCooldownTime;

	/** Finds the FMusicTrack by its event name. */
	UFUNCTION(BlueprintCallable, Category="Music")
	FMusicTrack GetMusicDataByEventName(FName EventName);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	float AddLayerChangeDelayTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	float RemoveLayerChangeDelayTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	USoundCue* JasonAbilityUnlockWarning;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundCue* TwoMinuteWarningMusic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundCue* DeathMusic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundCue* SurviveMusic;
};
