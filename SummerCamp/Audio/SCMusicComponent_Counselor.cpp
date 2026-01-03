// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMusicComponent_Counselor.h"

#include "SCCounselorCharacter.h"
#include "SCGameMode_SPChallenges.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"
#include "SCPlayerController.h"

USCMusicComponent_Counselor::USCMusicComponent_Counselor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCMusicComponent_Counselor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	const bool bWasSpectating = bIsSpectating;
	bIsSpectating = IsSpectating();
	if (bWasSpectating != bIsSpectating)
	{
		// Was Spectating but no longer spectating we need to Stop the music here.
		if (bWasSpectating)
		{
			CleanupMusic();
		}
		else
		{
			// This should restart the music logic as its an entry point for it.
			if (JasonMusic)
			{
				// Set an idle music timer to play the idle music if they are not near Jason.
				// Jason's music will cancel the timer if its going to play.
				if (CounselorMusicObject)
				{
					GetWorld()->GetTimerManager().SetTimer(TimerHandle_IdleTimer, this, &USCMusicComponent_Counselor::PlayIdleMusic, FMath::RandRange(2.0f, 5.0f));
				}

				// Startup Jasons music again.
				JasonMusic->Play();
			}
		}
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsPlaying2MinuteWarning && !PlayingCinematicMusic)
	{
		CheckJasonDistance(DeltaTime);
	}
}

void USCMusicComponent_Counselor::PostInitMusicComponenet()
{
	// Sets up the timer function to check for the distance Jason is away from the counselor
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_JasonFearTimer, this, &USCMusicComponent_Counselor::CheckFear, 0.1f, true);

	// Setup our CounselorMusicObject
	CounselorMusicObject = Cast<USCMusicObject_Counselor>(MusicObject);
}

void USCMusicComponent_Counselor::CheckJasonDistance(float DeltaTime)
{
	// If Jason music isn't setup yet we need to set it up.
	if (!JasonMusic)
	{
		// Always be playing the Jason music just make it not hearable until he is nearby
		if (ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (ASCKillerCharacter* Jason = State->CurrentKiller)
			{
				if (Jason && Jason->GetNearJasonMusic())
				{
					JasonMusic = UGameplayStatics::CreateSound2D(GetWorld(), Jason->GetNearJasonMusic());
					if (JasonMusic)
					{
						JasonMusic->bAutoDestroy = false;
						JasonMusic->Play();
						JasonMusic->SetVolumeMultiplier(0.f);
					}
				}
			}
		}
	}

	if ((IsLocalPlayerController() || bIsSpectating) && JasonMusic && CounselorMusicObject)
	{
		if (ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (ASCKillerCharacter* Jason = State->CurrentKiller)
			{
				if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
				{
					const float JasonDistance = (Jason->GetActorLocation() - CounselorCharacter->GetActorLocation()).Size();

					// handle the case that Jason is using stalk on a counselor.
					bool PlayMusic = true;
					bool TimerActive = GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_JasonMusicSilencedButSawHimCooldown);
					if (Jason->IsJasonMusicSilenced())
					{
						bool JasonVisible = CounselorCharacter->IsCharacterVisible(Jason);
						
						// Player just lost sight of Jason and no timer is active. Start the cooldown.
						if(JasonMusicSilencedAndCharacterVisible != JasonVisible && !JasonVisible && !TimerActive)
						{
							// set the timer
							GetWorld()->GetTimerManager().SetTimer(TimerHandle_JasonMusicSilencedButSawHimCooldown, 3.0f, false);
						}
						// Jason just became visible stop the Timer.
						else if (JasonVisible && TimerActive)
						{
							GetWorld()->GetTimerManager().ClearTimer(TimerHandle_JasonMusicSilencedButSawHimCooldown);
						}
						// No cooldown timer on we need to check to see if Jason is visible to play the music or not.
						else if (!TimerActive)
						{
							PlayMusic = CounselorCharacter->IsCharacterVisible(Jason);
						}

						JasonMusicSilencedAndCharacterVisible = JasonVisible;
					}

					JasonInRange = (JasonDistance <= Jason->NearCounselorMusicDistance) && PlayMusic && !Jason->IsCharacterDisabled();

					// Dont play anything if a high priority stinger is playing.
					if (!HighPriorityStinger)
					{
						if (!JasonInRange)
						{
							// Start an idle music timer if its not already set yet
							if (JasonMusicVolume > 0.001f && !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_IdleTimer) && CounselorCharacter->IsAlive())
							{
								GetWorld()->GetTimerManager().SetTimer(TimerHandle_IdleTimer, this, &USCMusicComponent_Counselor::PlayIdleMusic, FMath::RandRange(CounselorMusicObject->MinIdleMusicTime, CounselorMusicObject->MaxIdleMusicTime));
							}

							JasonMusicVolume = 0.001f;
						}
						else
						{
							// Clear the idle timer
							if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_IdleTimer))
							{
								GetWorld()->GetTimerManager().ClearTimer(TimerHandle_IdleTimer);
							}

							// Store off old music volume
							float NewJasonMusicVolume = 1 - FMath::Clamp(((JasonDistance - Jason->NearCounselorMusicMaxVolumeDistance) / Jason->NearCounselorMusicDistance), 0.001f, 1.0f);

							// Music is jumping up from 0.
							if (JasonMusicVolume <= 0.001f)
							{
								// 
								if (NewJasonMusicVolume >= 0.5f)
								{
									// for now until we get more music lets reset it
									JasonMusic->Stop();
									JasonMusic->Play();
									JasonMusic->SetVolumeMultiplier(NewJasonMusicVolume);
								}
							}

							// Calculate new Jason music volume
							JasonMusicVolume = NewJasonMusicVolume;
						}

						JasonMusic->SetVolumeMultiplier(FMath::FInterpConstantTo(JasonMusic->VolumeMultiplier, JasonMusicVolume, DeltaTime, 0.5f));

						// Check to see if a current music track is playing and its not a stinger.
						if (JasonMusicVolume > 0.001f && CurrentTrack && CurrentTrack->IsPlaying() && CurrentMusicTrack.MusicEventType != EMusicEventType::STINGER)
						{
							// Make sure its not already fading out
							if (!bFadingCurrentTrackOut)
							{
								CurrentTrack->FadeOut(0.2f, 0.f);
								bFadingCurrentTrackOut = true;
							}
						}
					}
				}
			}
		}
	}
}

void USCMusicComponent_Counselor::CheckFear()
{
	if ((IsLocalPlayerController() || bIsSpectating) && CounselorMusicObject && IsAlive())
	{
		if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
		{
			// Get current fear amount
			float FearAmount = CounselorCharacter->GetFear();
	
			// Check to see if fear level is above 70%. If so lets start tripping the player out.
			if ((FearAmount >= CounselorMusicObject->ScarySoundsFearLevel) && (!ScarySounds || !ScarySounds->IsPlaying()) && !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_ScraySoundsCooldown))
			{
				float X = FMath::RandRange(CounselorMusicObject->ScarySoundsHearingRadiusMin, CounselorMusicObject->ScarySoundsHearingRadiusMax);
				float Y = FMath::RandRange(CounselorMusicObject->ScarySoundsHearingRadiusMin, CounselorMusicObject->ScarySoundsHearingRadiusMax);
				ScarySounds = UGameplayStatics::SpawnSoundAtLocation(GetWorld(), CounselorCharacter->IsIndoors() ? CounselorMusicObject->Indoor_ScarySounds_Sounds : CounselorMusicObject->Outdoor_ScarySounds_Sounds, CounselorCharacter->GetActorLocation() + FVector(X, Y, CounselorCharacter->GetActorLocation().Z));
				if (ScarySounds)
				{
					ScarySounds->Play(0.f);

					if (CounselorMusicObject->ScarySoundCooldownCurve)
					{
						const float CoolDownPercentage = CounselorMusicObject->ScarySoundCooldownCurve->GetFloatValue(FearAmount * 0.01f);
						const float CoolDownRandom = FMath::RandRange(CounselorMusicObject->ScarySoundCooldownMin, CounselorMusicObject->ScarySoundCooldownMax);
						const float CoolDownAmount = CoolDownPercentage * CoolDownRandom;

						// Start the cooldown timer.
						GetWorld()->GetTimerManager().SetTimer(TimerHandle_ScraySoundsCooldown, CoolDownAmount, false);
					}
				}
			}
		}
	}
}

void USCMusicComponent_Counselor::HandleHighPriorityStingerPlaying()
{
	// Check to see if Jason's music is playing
	if (JasonMusic && JasonMusic->VolumeMultiplier > 0.f)
	{
		// Its playing so we need to turn it off immediately for the high priority stinger
		JasonMusic->SetVolumeMultiplier(0.f);
	}
}

void USCMusicComponent_Counselor::PlayIdleMusic()
{
	PlayMusic(TEXT("CounselorIdle"));
}

void USCMusicComponent_Counselor::CleanupMusic(bool SkipTwoMinuteWarningMusic /* = false */)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_JasonMusicSilencedButSawHimCooldown);
		World->GetTimerManager().ClearTimer(TimerHandle_IdleTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_StingerTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_JasonFearTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_ScraySoundsCooldown);
	}

	// Stop any scary sounds from playing in case the users fear is high.
	if (ScarySounds)
	{
		ScarySounds->FadeOut(2.f, 0.f);
	}

	// Stop Jason's music and block his music system from playing until we go into spectator mode.
	if (JasonMusic)
	{
		JasonMusic->Stop();
	}

	Super::CleanupMusic(SkipTwoMinuteWarningMusic);
}

bool USCMusicComponent_Counselor::PlayDeadFriendlyStinger()
{
	if ((IsLocalPlayerController() || bIsSpectating))
	{
		// Play the stinger
		static FName NAME_SeeCounselorDeadBody(TEXT("SeeCounselorDeadBody"));
		PlayMusic(NAME_SeeCounselorDeadBody);

		// Play some music after if Jason isn't in range to keep the intensity up.
		if (!JasonInRange)
		{
			static FName NAME_AfterCounselorSeesDeadBody(TEXT("AfterCounselorSeesDeadBody"));
			PlayMusic(NAME_AfterCounselorSeesDeadBody);
		}
		return true;
	}
	return false;
}

void USCMusicComponent_Counselor::PostTrackFinished()
{
	if (JasonMusic && !CurrentTrack && JasonMusic->VolumeMultiplier <= 0.001f && IsAlive())
	{
		if (CounselorMusicObject)
		{
			// Start an idle music timer set from a random tuned range.
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_IdleTimer, this, &USCMusicComponent_Counselor::PlayIdleMusic, FMath::RandRange(CounselorMusicObject->MinIdleMusicTime, CounselorMusicObject->MaxIdleMusicTime));
		}
	}
}

void USCMusicComponent_Counselor::UpdateAdjustPitchMultiplier(const float DeltaTime)
{
	if (JasonMusic && JasonMusic->VolumeMultiplier > 0.001f)
	{
		JasonMusic->SetPitchMultiplier(FMath::FInterpConstantTo(JasonMusic->PitchMultiplier, NewAdjustedPitch, DeltaTime, NewLerpVal));
	}
	else
	{
		Super::UpdateAdjustPitchMultiplier(DeltaTime);
	}
}