// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMusicComponent.h"
#include "SCMusicObject_Counselor.h"
#include "SCMusicComponent_Counselor.generated.h"

/**
 * @class USCMusicComponent_Counselor
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCMusicComponent_Counselor
: public USCMusicComponent
{
	GENERATED_BODY()

public:
	USCMusicComponent_Counselor(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface

	/** Function to cleanup any playing music and Jasons music. */
	virtual void CleanupMusic(bool SkipTwoMinuteWarningMusic = false ) override;

	/** Checks to see if the dead friendly is in range and plays a stinger if so. Returns true if stinger played. */
	bool PlayDeadFriendlyStinger();

protected:
	virtual void HandleHighPriorityStingerPlaying() override;

	virtual void PostInitMusicComponenet() override;

	virtual void PostTrackFinished() override;

	virtual void UpdateAdjustPitchMultiplier(const float DeltaTime) override;

private:

	/** Function is called from a timer when Idle music is allowed to play.*/
	void PlayIdleMusic();

	/** Distance check to see if Jason is nearby. Volume multiplier is set based on his distance. */
	void CheckJasonDistance(float DeltaTime);

	/** Function that checks the counselors fear level and executes and action based on that. */
	void CheckFear();

	/** Music to be played when Jason is nearby. */
	UPROPERTY()
	UAudioComponent* JasonMusic;

	/** Scary sounds audio componenet. */
	UPROPERTY()
	UAudioComponent* ScarySounds;

	UPROPERTY()
	USCMusicObject_Counselor* CounselorMusicObject;

	/** Jason's music current volume. */
	float JasonMusicVolume;

	/** Timer for various gameplay checks to change the music. */
	FTimerHandle TimerHandle_JasonFearTimer;
	FTimerHandle TimerHandle_ScraySoundsCooldown;
	FTimerHandle TimerHandle_JasonMusicSilencedButSawHimCooldown;

	bool JasonMusicSilencedAndCharacterVisible;
	bool JasonInRange;
};
