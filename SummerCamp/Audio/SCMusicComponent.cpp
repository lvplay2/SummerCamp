// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMusicComponent.h"

#include "Sound/SoundNodeMixer.h"

#include "ILLGameBlueprintLibrary.h"

#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCInGameHUD.h"
#include "SCKillerCharacter.h"
#include "SCPlayerController.h"

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarDebugMusic(TEXT("sc.DebugMusic"), 0,
										   TEXT("Displays debug information for music.\n")
										   TEXT(" 0: None\n")
										   TEXT(" 1: Display debug info"));
#endif

USCMusicComponent::USCMusicComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;

	CanStingerPlayer = true;
}

void USCMusicComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateAdjustVolumeMultiplier(DeltaTime);

	UpdateAdjustPitchMultiplier(DeltaTime);

	if (IsLocalPlayerController())
	{
		if (UWorld* World = GetWorld())
		{
			// Check to see if we need to change to the 2 minute warning music.
			if (ASCGameState* SCGameState = World->GetGameState<ASCGameState>())
			{
				if (MusicObject && SCGameState->IsMatchInProgress() && !IsPlaying2MinuteWarning && !PlayingCinematicMusic && SCGameState->RemainingTime > 0 && SCGameState->RemainingTime <= 120) // FIXME: Hard-coded 120
				{
					// Stop and Cleanup all music and any that might be in cue.
					CleanupMusic();

					// Play the 2 minute warning Music
					TwoMinuteWarningTrack = UGameplayStatics::CreateSound2D(GetWorld(), MusicObject->TwoMinuteWarningMusic);
					if (TwoMinuteWarningTrack)
					{
						TwoMinuteWarningTrack->Play();

						// Make sure we lock down the music system when this music is playing.
						IsPlaying2MinuteWarning = true;

						// Play the HUD warning too
						if (ASCCharacter* Character = GetCharacterOwner())
						{
							if (ASCPlayerController* PC = Cast<ASCPlayerController>(Character->GetCharacterController()))
							{
								if (ASCInGameHUD* HUD = Cast<ASCInGameHUD>(PC->GetHUD()))
								{
									HUD->OnTwoMinuteWarning();
								}
							}
						}
					}
				}
				// If we're in single player then the remaining time will get set to -1 when we're in bounds so we need to stop the TwoMinute warning music.
				else if (MusicObject && SCGameState->IsMatchInProgress() && IsPlaying2MinuteWarning && !PlayingCinematicMusic && SCGameState->RemainingTime < 0)
				{
					// Stop and Cleanup all music and any that might be in cue.
					TwoMinuteWarningTrack->FadeOut(3.f, 0.f);
					IsPlaying2MinuteWarning = false;
				}
			}

#if !UE_BUILD_SHIPPING
			// Debug Stuff
			if (CVarDebugMusic.GetValueOnGameThread() > 0)
			{
				if (CurrentMixerNode)
				{
					for (int i = 0; i < CurrentMixerNode->InputVolume.Num(); ++i)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, FString::Printf(TEXT("Track %d - Volume Multiplier: %f"), i, CurrentMixerNode->InputVolume[i]), true, FVector2D(1.2f, 1.2f));
					}
				}

				if (!CurrentMusicTrack.EventName.IsNone())
				{
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, FString::Printf(TEXT("Current Music Name: %s"), *CurrentMusicTrack.EventName.ToString()), true, FVector2D(1.2f, 1.2f));
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, FString::Printf(TEXT("Current Music Event Type: %s"), *(CurrentMusicTrack.MusicEventType == EMusicEventType::MUSICTRACK ? FString(TEXT("MUSIC TRACK")) : FString(TEXT("STINGER")))), true, FVector2D(1.2f, 1.2f));
				}

				if (!CanStingerPlayer)
				{
					float TimeUntilStingerCanPlay = World->GetTimerManager().GetTimerRemaining(TimerHandle_StingerTimer);

					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, FString::Printf(TEXT("Time Until Stringer Can Play: %f"), TimeUntilStingerCanPlay), true, FVector2D(1.2f, 1.2f));
				}
			}
#endif
		}
	}
}

void USCMusicComponent::OnUnregister()
{
	Super::OnUnregister();

	// Clear delegates
	if (CurrentTrack)
	{
		CurrentTrack->OnAudioFinished.RemoveAll(this);
	}

	// Clear timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_IdleTimer);
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_StingerTimer);
	}

	// Cleanup any currently playing stuff
	CleanupMusic();
}

ASCCharacter* USCMusicComponent::GetCharacterOwner() const
{
	if (ASCCharacter* CharacterOwner = Cast<ASCCharacter>(GetOwner()))
	{
		return CharacterOwner;
	}

	return nullptr;
}

bool USCMusicComponent::IsLocalPlayerController() const
{
	ASCCharacter* Character = GetCharacterOwner();
	ASCPlayerController* PC = Character ? Cast<ASCPlayerController>(Character->GetCharacterController()) : nullptr;
	return PC ? PC->IsLocalPlayerController() : false;
}

bool USCMusicComponent::IsSpectating()
{
	if (UWorld* World = GetWorld())
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(World)))
		{
			return (PC->GetSpectatingPlayer() == GetCharacterOwner());
		}
	}

	return false;
}

bool USCMusicComponent::IsAlive() const
{
	return (GetCharacterOwner() ? GetCharacterOwner()->IsAlive() : false);
}

void USCMusicComponent::InitMusicComponent(TSubclassOf<USCMusicObject> MusicObjectClass)
{
	if (MusicObjectClass)
	{
		if (UWorld* World = GetWorld())
		{
			if (UObject* Outer = World->GetGameInstance() ? StaticCast<UObject*>(World->GetGameInstance()) : StaticCast<UObject*>(World))
			{
				MusicObject = NewObject<USCMusicObject>(Outer, MusicObjectClass);

				PostInitMusicComponenet();
			}
		}
	}
}

void USCMusicComponent::PlayMusic(FName MusicEventName)
{
	if ((IsLocalPlayerController() || IsSpectating()) && MusicObject)
	{
		// Don't play any other music if we are currently playing the 2 minute warning.
		if (IsPlaying2MinuteWarning)
			return;

		FMusicTrack MusicTrack = MusicObject->GetMusicDataByEventName(MusicEventName);
		if (!MusicTrack.EventName.IsNone() && MusicTrack.MusicTrack)
		{
			// Throw out the stinger if the time didn't elapse since the last stinger played.
			if (MusicTrack.MusicEventType == EMusicEventType::STINGER && (!CanStingerPlayer || !MusicTrack.HighPriorityStinger))
				return;

			// Make sure everything else is blocked as this is a priority.
			HighPriorityStinger = MusicTrack.HighPriorityStinger;

			// High priority stinger we need to shut off all other music playing.
			if (HighPriorityStinger)
			{
				HandleHighPriorityStingerPlaying();
			}

			// Current music track is playing
			if (CurrentTrack && CurrentTrack->IsPlaying())
			{
				// Check to see if this music track is already playing. If so throw it away.
				if (CurrentMusicTrack.EventName == MusicTrack.EventName)
					return;

				// Don't want to play two stingers back to back. Throw this track away and let the current stinger play out.
				if (CurrentMusicTrack.MusicEventType == EMusicEventType::STINGER && MusicTrack.MusicEventType == EMusicEventType::STINGER)
					return;

				float FadeOutTime = .01f;
				// Not a stinger we can fade out normally as defined in BP.
				if (MusicTrack.MusicEventType != EMusicEventType::STINGER)
					FadeOutTime = CurrentMusicTrack.FadeOutTime;

				CurrentTrack->FadeOut(FadeOutTime, 0.f);
				bFadingCurrentTrackOut = true;

				// Fade in new track
				NextTrack = UGameplayStatics::CreateSound2D(GetWorld(), MusicTrack.MusicTrack);
				NextTrack->FadeIn(MusicTrack.FadeInTime, 1.f);
				NextMusicTrack = MusicTrack;

				// Reset the stinger counter
				if (NextMusicTrack.MusicEventType == EMusicEventType::STINGER)
				{
					CanStingerPlayer = false;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle_StingerTimer, this, &USCMusicComponent::AllowStingerToPlay, MusicObject->StingerCooldownTime);
				}
			}
			else
			{
				// Clear the current track idle timer since a new track is playing.
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_IdleTimer);

				// No tracks are playing. Play the new one.
				CurrentTrack = UGameplayStatics::CreateSound2D(GetWorld(), MusicTrack.MusicTrack);
				bFadingCurrentTrackOut = false;
				if (CurrentTrack)
				{
					CurrentTrack->Play();
					CurrentTrack->OnAudioFinished.AddDynamic(this, &USCMusicComponent::CurrentTrackFinished);
					CurrentMusicTrack = MusicTrack;

					NewAdjustedPitch = CurrentTrack->PitchMultiplier;

					// If this is a layered music track it will set it up with the music system.
					SetupLayeredMusicCue(CurrentTrack->Sound);

					// Reset the stinger counter
					if (CurrentMusicTrack.MusicEventType == EMusicEventType::STINGER)
					{
						CanStingerPlayer = false;
						GetWorld()->GetTimerManager().SetTimer(TimerHandle_StingerTimer, this, &USCMusicComponent::AllowStingerToPlay, MusicObject->StingerCooldownTime);
					}
				}
			}
		}
	}
}

void USCMusicComponent::UpdateAdjustVolumeMultiplier(const float DeltaTime)
{
	if (CurrentMixerNode)
	{
		for (int i = 0; i < CurrentMixerNode->InputVolume.Num(); ++i)
		{
			if (i <= (int32)CurrentSoundLayer)
			{
				if (CurrentSoundLayer >= ESCKillerMusicLayer::SpotTarget && i == (int32)ESCKillerMusicLayer::TargetGone)
				{
					CurrentMixerNode->InputVolume[i] = FMath::FInterpConstantTo(CurrentMixerNode->InputVolume[i], 0.001f, DeltaTime, 0.25f);
					continue;
				}

				CurrentMixerNode->InputVolume[i] = FMath::FInterpConstantTo(CurrentMixerNode->InputVolume[i], 1.0f, DeltaTime, 0.25f);
			}
			else
			{
				CurrentMixerNode->InputVolume[i] = FMath::FInterpConstantTo(CurrentMixerNode->InputVolume[i], 0.001f, DeltaTime, 0.25f);
			}
		}
	}
}

void USCMusicComponent::SetupLayeredMusicCue(USoundBase* SoundBase)
{
	if (!CurrentMixerNode)
	{
		USoundCue* SoundCue = Cast<USoundCue>(SoundBase);
		if (SoundCue)
		{
			TArray<USoundNodeMixer*> Mixers;
			SoundCue->RecursiveFindNode(SoundCue->FirstNode, Mixers);

			for (int32 MixerIndex = 0; MixerIndex < Mixers.Num(); ++MixerIndex)
			{
				CurrentMixerNode = Mixers[MixerIndex];
			}
		}
	}
}

void USCMusicComponent::UpdateAdjustPitchMultiplier(const float DeltaTime)
{
	if (CurrentTrack && CurrentTrack->PitchMultiplier != NewAdjustedPitch)
	{
		CurrentTrack->SetPitchMultiplier(FMath::FInterpConstantTo(CurrentTrack->PitchMultiplier, NewAdjustedPitch, DeltaTime, NewLerpVal));
	}
}

void USCMusicComponent::SetAdjustPitchMultiplier(float NewPitch, float LerpVal/*= 1.f*/)
{
	NewAdjustedPitch = NewPitch;
	NewLerpVal = LerpVal;
}

void USCMusicComponent::ChangeMusicLayer(ESCKillerMusicLayer CurrentLayer)
{	
	CurrentSoundLayer = CurrentLayer;
}

void USCMusicComponent::CleanupMusic(bool SkipTwoMinuteWarningMusic/*= false*/)
{
	if (CurrentTrack)
	{
		if (CurrentTrack->OnAudioFinished.IsBound())
		{
			CurrentTrack->OnAudioFinished.RemoveDynamic(this, &USCMusicComponent::CurrentTrackFinished);
		}

		CurrentTrack->Stop();
		CurrentTrack->DestroyComponent();
		CurrentTrack = nullptr;
		bFadingCurrentTrackOut = false;
	}

	CurrentMusicTrack = FMusicTrack();

	if (NextTrack)
	{
		NextTrack->Stop();
		NextTrack->DestroyComponent();
		NextTrack = nullptr;
	}

	if (CurrentMixerNode)
	{
		// Reset all the mixer channels
		for (int i = 0; i < CurrentMixerNode->InputVolume.Num(); ++i)
		{
			CurrentMixerNode->InputVolume[i] = 0.001;
		}

		CurrentMixerNode = nullptr;
	}

	if (!SkipTwoMinuteWarningMusic && TwoMinuteWarningTrack)
	{
		TwoMinuteWarningTrack->Stop();
		TwoMinuteWarningTrack->DestroyComponent();
		IsPlaying2MinuteWarning = false;
		TwoMinuteWarningTrack = nullptr;
	}
	else if (TwoMinuteWarningTrack)
	{
		TwoMinuteWarningTrack->SetVolumeMultiplier(0.01f);
	}
}

void USCMusicComponent::CurrentTrackFinished()
{
	if (CurrentTrack)
	{
		CurrentTrack->DestroyComponent();
		CurrentTrack = nullptr;
		bFadingCurrentTrackOut = false;
	}
	CurrentMusicTrack = FMusicTrack();

	if (CurrentMixerNode)
	{
		// Reset all the mixer channels
		for (int i = 0; i < CurrentMixerNode->InputVolume.Num(); ++i)
		{
			CurrentMixerNode->InputVolume[i] = 0.001;
		}

		CurrentMixerNode = nullptr;
	}

	HighPriorityStinger = false;

	if (NextTrack)
	{
		CurrentTrack = NextTrack;
		bFadingCurrentTrackOut = false;
		CurrentTrack->OnAudioFinished.AddDynamic(this, &USCMusicComponent::CurrentTrackFinished);
		CurrentMusicTrack = NextMusicTrack;

		HighPriorityStinger = CurrentMusicTrack.HighPriorityStinger;

		SetupLayeredMusicCue(CurrentTrack->Sound);

		NextTrack = nullptr;
	}

	PostTrackFinished();
}

void USCMusicComponent::AllowStingerToPlay()
{
	CanStingerPlayer = true;
}

void USCMusicComponent::PlayCinematicMusic(USoundBase* MusicTrack)
{
	CleanupMusic(IsPlaying2MinuteWarning);

	if (MusicTrack)
	{
		// Play the cinematic music
		CurrentTrack = UGameplayStatics::CreateSound2D(GetWorld(), MusicTrack);
		bFadingCurrentTrackOut = false;
		if (CurrentTrack)
		{
			PlayingCinematicMusic = true;
			CurrentTrack->Play();
			CurrentTrack->OnAudioFinished.AddDynamic(this, &USCMusicComponent::CinematicKillMusicFinished);
		}
	}
}

void USCMusicComponent::CinematicKillMusicFinished()
{
	if (CurrentTrack)
	{
		CurrentTrack->DestroyComponent();
		CurrentTrack = nullptr;
		bFadingCurrentTrackOut = false;
	}

	PlayingCinematicMusic = false;

	HighPriorityStinger = false;

	// Check to see if we need to restart the 2 minute warning music.
	if (!IsPlaying2MinuteWarning)
	{
		PostTrackFinished();
	}
	// put back the 2 minute warning music (most likely for Jason only because a counselor would be dead).
	else if(IsAlive() && IsPlaying2MinuteWarning && TwoMinuteWarningTrack)
	{
		TwoMinuteWarningTrack->SetVolumeMultiplier(1.0f);
	}
}

void USCMusicComponent::HandleHighPriorityStingerPlaying()
{
}

void USCMusicComponent::PostInitMusicComponenet()
{
}

void USCMusicComponent::PostTrackFinished()
{
}

void USCMusicComponent::PlayAbilitiesWarning()
{
	if (MusicObject)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), MusicObject->JasonAbilityUnlockWarning);
	}
}

void USCMusicComponent::ShutdownMusicComponent()
{
	SetComponentTickEnabled(false);

	PrimaryComponentTick.bCanEverTick = false;

	CleanupMusic();
}
