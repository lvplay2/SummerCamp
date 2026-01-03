// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/ActorComponent.h"
#include "SCMusicObject_Counselor.h"
#include "SCMusicComponent.generated.h"

/**
 * @enum ESCKillerMusicLayer
 */
UENUM()
enum class ESCKillerMusicLayer : uint8
{
	Searching,
	TargetGone,
	SpotTarget,
	ClosingIn,
	Panic,
};

/**
 * @class USCMusicComponent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API USCMusicComponent
: public UActorComponent
{
	GENERATED_BODY()

public:
	USCMusicComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent interface

protected:
	// Begin UActorComponent interface
	virtual void OnUnregister() override;
	// End UActorComponent interface

public:
	/** Function call to play music based on a music event name which is mapped in BP. */
	virtual void PlayMusic(FName MusicEventName);

	/** Function that initializes music if its the locally controlled player. */
	virtual void InitMusicComponent(TSubclassOf<USCMusicObject> MusicObjectClass);

	/** Function to cleanup any playing music and Jason's music. */
	virtual void CleanupMusic(bool SkipTwoMinuteWarningMusic = false);

	/** Stops all other music to play a Cinematic Kill Music. */
	virtual void PlayCinematicMusic(USoundBase* MusicTrack);

	void ChangeMusicLayer(ESCKillerMusicLayer CurrentLayer);

	void SetAdjustPitchMultiplier(float NewPitch, float LerpVal = 1.0f);

	void PlayAbilitiesWarning();

	void ShutdownMusicComponent();

protected:
	virtual void HandleHighPriorityStingerPlaying();

	virtual void PostInitMusicComponenet();
	
	UFUNCTION()
	void CurrentTrackFinished();

	virtual void PostTrackFinished();

	/** Helper function to quiclky get the owner as an ASCCounselorCharacter* */
	class ASCCharacter* GetCharacterOwner() const;

	/** Helper function to quickly tell if this is locally controlled. */
	bool IsLocalPlayerController() const;

	/** Helper function to quickly tell if this is someone spectating this player. */
	bool IsSpectating();
	
	/** Helper function to quickly tell if someone is alive or not. */
	bool IsAlive() const;
	
	float NewAdjustedPitch;

	float NewLerpVal;

	virtual void UpdateAdjustPitchMultiplier(const float DeltaTime);

	bool bIsSpectating;
	
private:
	UFUNCTION()
	void AllowStingerToPlay();

	UFUNCTION()
	void CinematicKillMusicFinished();

	void UpdateAdjustVolumeMultiplier(const float DeltaTime);

	void SetupLayeredMusicCue(USoundBase* SoundBase);

protected:
	/** Music object that this component uses that has all the information about music in it. */
	UPROPERTY()
	USCMusicObject* MusicObject;

	/** Music data variables. */
	UPROPERTY(Transient)
	FMusicTrack CurrentMusicTrack;

	UPROPERTY(Transient)
	FMusicTrack NextMusicTrack;

	// Current playing music track
	UPROPERTY(Transient)
	UAudioComponent* CurrentTrack;

	// Did we request to fade out CurrentTrack?
	bool bFadingCurrentTrackOut;

	/** Current playing music track. */
	UPROPERTY()
	UAudioComponent* TwoMinuteWarningTrack;

	/** Next track cued up to play. We use this variable to smoothly mix between one song to another. */
	UPROPERTY()
	UAudioComponent* NextTrack;

	bool HighPriorityStinger;

	/** Timer for various game play checks to change the music. */
	FTimerHandle TimerHandle_IdleTimer;
	FTimerHandle TimerHandle_StingerTimer;

	/** Set to true when a stinger can play. */
	bool CanStingerPlayer;

	ESCKillerMusicLayer CurrentSoundLayer;

	UPROPERTY()
	class USoundNodeMixer* CurrentMixerNode;

	UPROPERTY()
	bool IsPlaying2MinuteWarning;

	UPROPERTY()
	bool PlayingCinematicMusic;
};
