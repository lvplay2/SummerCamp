// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVoiceOverComponent_Counselor.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Sound/SoundNodeWavePlayer.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameBlueprintLibrary.h"
#include "SCGameInstance.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"

static FName NAME_OneShotVO(TEXT("OneShotVO"));

USCVoiceOverComponent_Counselor::USCVoiceOverComponent_Counselor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, KillerChaseVODistance(300.0f)
, CurrentBreath(EBreathType::INHALE)
, CurrentBreathingState(EBreathingState::NONE)
, NextBreathingState(EBreathingState::NONE)
, DeadFriendlyVODistance(1000.f)
, KillerHidingVODistance(300.f)
, KillerSpottedDistance(2300.f)
, HidingFromKillerCooldownTime(20.f)
, SpottedKillerCooldownTime(30.0f)
, GettingChasedCooldown(15.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	AudioComponent->bAutoActivate = false;

	if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		AudioComponent->SetupAttachment(CounselorCharacter->GetRootComponent());
	}
}

void USCVoiceOverComponent_Counselor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		// Check to see if we are hiding and can see the killer.
		if (ASCKillerCharacter* Killer = Counselor->GetKillerInView())
		{
			const bool bShouldPlay = Counselor->Controller && Counselor->Controller->IsA<ASCCounselorAIController>() ? Cast<ASCCounselorAIController>(Counselor->Controller)->ShouldFleeKiller() : true;
			if (bShouldPlay)
			{
				const float JasonDistanceSq = Counselor->GetSquaredDistanceTo(Killer);
				if (Counselor->IsInHiding())
				{
					// Allow a small delay after releasing the "hold breath" button before we attempt to play the audio.
					if (!Counselor->IsHoldingBreath() && Counselor->CanHoldBreath() && Counselor->GetFear() >= 70.f) // FIXME: Hard-coded 70
					{
						PlayKillerInRangeWhileHidingVO(JasonDistanceSq);
					}
				}
				else
				{
					PlayKillerInRangeVO(JasonDistanceSq);
				}
			}
		}
		else
		{
			KillerOutOfView();
		}
	}
}

void USCVoiceOverComponent_Counselor::InitVoiceOverComponent(TSubclassOf<USCVoiceOverObject> VOClass)
{
	Super::InitVoiceOverComponent(VOClass);

	CheckBreathing();
}

void USCVoiceOverComponent_Counselor::CheckBreathing()
{
	// check to see if we are sprinting or jogging.
	if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		// No more breathing if dead.
		if (!CounselorCharacter->IsAlive())
			return;

		// Reset the next breathing state
		NextBreathingState = EBreathingState::NONE;

		// Get the counselors stamina percentage.
		float StaminaPercent = CounselorCharacter->GetStamina() / CounselorCharacter->GetMaxStamina();

		// First lets check to see if we are in a hiding spot
		if (CounselorCharacter->IsInHiding())
		{
			NextBreathingState = EBreathingState::HIDING;
		}
		// finally check stamina and then speed levels.
		else
		{	
			// Stamina is 10% or less and you recently ran out of stanima
			if (StaminaPercent <= 0.10f && RanOutOfStamina)
			{
				NextBreathingState = EBreathingState::LOWSTAMINA;
			}
			else if (RanOutOfStamina)
			{
				RanOutOfStamina = false;
			}

			if (NextBreathingState != EBreathingState::LOWSTAMINA)
			{
				// Get the next breathing state based on the players action.
				const float Speed = CounselorCharacter->GetVelocity().Size();
				if (Speed > 0.f)
				{
					if (CounselorCharacter->IsSprinting())
					{
						NextBreathingState = EBreathingState::SPRITING;
					}
					else if (CounselorCharacter->IsRunning())
					{
						NextBreathingState = EBreathingState::RUNNING;
					}
				}
			}
		}

		// Cue up the next breathing state if its anything but none and different then the current breathing state using the Warmup.
		if (StaminaPercent <= 0.20f && CurrentBreathingState != NextBreathingState && NextBreathingState != EBreathingState::NONE && GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_BreathingWarmupTimer) == -1.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BreathingCooldownTimer);

			float WarmupTime = GetWarmupTime(NextBreathingState);
			if (WarmupTime <= 0.f)
				ProcessNextBreath();
			else
				GetWorld()->GetTimerManager().SetTimer(TimerHandle_BreathingWarmupTimer, this, &USCVoiceOverComponent_Counselor::ProcessNextBreath, WarmupTime);
		}
		else
		{
			// Check to see if we need to turn off the Current Breathing State using the Cool Down
			if (CurrentBreathingState != EBreathingState::NONE && NextBreathingState == EBreathingState::NONE && GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_BreathingCooldownTimer) == -1.f)
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BreathingWarmupTimer);

				float CooldownTime = GetCoolDownTime(CurrentBreathingState);
				if (CooldownTime <= 0.f)
					ProcessNextBreath();
				else
					GetWorld()->GetTimerManager().SetTimer(TimerHandle_BreathingCooldownTimer, this, &USCVoiceOverComponent_Counselor::ProcessNextBreath, CooldownTime);
			}
		}

		// We are currently breathing and there now is a Next Breathing State cue'd up lets reset our cooldown time if its been activated.
		if (CurrentBreathingState != EBreathingState::NONE && NextBreathingState != EBreathingState::NONE && GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_BreathingCooldownTimer) > 0.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BreathingCooldownTimer);
		}

		// Play the actual breathing then "Check Breathing" again once the breath is done playing.
		if (CounselorCharacter->IsAlive() && CurrentBreathingState != EBreathingState::NONE)
		{
			PlayBreath(CounselorCharacter->GetRootComponent());
		}
		// There is not breathing that is suppose to happen lets just check again.
		else if (CounselorCharacter->IsAlive())
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_BreathTimer, this, &USCVoiceOverComponent_Counselor::CheckBreathing, 0.1);
		}
	}
}

void USCVoiceOverComponent_Counselor::ProcessNextBreath()
{
	CurrentBreathingState = NextBreathingState;
	NextBreathingState = EBreathingState::NONE;
}

void USCVoiceOverComponent_Counselor::PlayBreath(USceneComponent* ActorRootComponent)
{
	PendingBreathVoiceOver = GetCurrentBreathSoundCue(CurrentBreathingState);
	if (PendingBreathVoiceOver.Get())
	{
		DeferredPlayBreath();
	}
	else if (!PendingBreathVoiceOver.IsNull())
	{
		// Load and play when it's ready
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(PendingBreathVoiceOver.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayBreath));
	}

	// Switch breaths
	CurrentBreath == EBreathType::INHALE ? CurrentBreath = EBreathType::EXHALE : CurrentBreath = EBreathType::INHALE;
}

void USCVoiceOverComponent_Counselor::CleanupVOComponent()
{
	if (AudioComponent)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
		AudioComponent = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_BreathTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_BreathingWarmupTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_BreathingCooldownTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_HidingFromKillerCooldown);
		World->GetTimerManager().ClearTimer(TimerHandle_SpottedKillerCooldownTime);
	}

	PrimaryComponentTick.bCanEverTick = false;
}

float USCVoiceOverComponent_Counselor::GetCoolDownTime(EBreathingState CurrentState)
{
	float CooldownTime = 0.f;
	const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
	if (VoiceOverDefault)
	{
		switch (CurrentState)
		{
		case EBreathingState::RUNNING:
			CooldownTime = VoiceOverDefault->JoggingCoolDownTime;
			break;
		case EBreathingState::SPRITING:
			CooldownTime = VoiceOverDefault->SprintCoolDownTime;
			break;
		case EBreathingState::LOWSTAMINA:
			CooldownTime = VoiceOverDefault->LowStaminaCoolDownTime;
			break;
		case EBreathingState::HIDING:
			CooldownTime = VoiceOverDefault->HideCoolDownTime;
			break;
		}
	}

	return CooldownTime;
}

float USCVoiceOverComponent_Counselor::GetWarmupTime(EBreathingState CurrentState)
{
	float WarmupTime = 0.f;
	const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
	if (VoiceOverDefault)
	{
		switch (CurrentState)
		{
		case EBreathingState::RUNNING:
			WarmupTime = VoiceOverDefault->JoggingWarmUpTime;
			break;
		case EBreathingState::SPRITING:
			WarmupTime = VoiceOverDefault->SprintWarmUpTime;
			break;
		case EBreathingState::LOWSTAMINA:
			WarmupTime = VoiceOverDefault->LowStaminaWarmUpTime;
			break;
		case EBreathingState::HIDING:
			WarmupTime = VoiceOverDefault->HideWarmUpTime;
			break;
		}
	}

	return WarmupTime;
}

TAssetPtr<USoundCue> USCVoiceOverComponent_Counselor::GetCurrentBreathSoundCue(EBreathingState CurrentState)
{
	TAssetPtr<USoundCue> SoundCue = nullptr;
	const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
	if (VoiceOverDefault)
	{
		switch (CurrentState)
		{
		case EBreathingState::RUNNING:
			if (CurrentBreath == EBreathType::INHALE)
				SoundCue = VoiceOverDefault->Jogging_Inhale;
			else
				SoundCue = VoiceOverDefault->Jogging_Exhale;
			break;
		case EBreathingState::SPRITING:
			if (CurrentBreath == EBreathType::INHALE)
				SoundCue = VoiceOverDefault->Sprinting_Inhale;
			else
				SoundCue = VoiceOverDefault->Sprinting_Exhale;
			break;
		case EBreathingState::LOWSTAMINA:
			if (CurrentBreath == EBreathType::INHALE)
				SoundCue = VoiceOverDefault->LowStamina_Inhale;
			else
				SoundCue = VoiceOverDefault->LowStamina_Exhale;
			break;
		case EBreathingState::HIDING:
			if (CurrentBreath == EBreathType::INHALE)
				SoundCue = VoiceOverDefault->Hiding_Inhale;
			else
				SoundCue = VoiceOverDefault->Hiding_Exhale;
			break;
		}
	}
	return SoundCue;
}

FString USCVoiceOverComponent_Counselor::DebugBreathingStateString(EBreathingState CurrentState)
{
	FString BreathingStateStr;
	const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
	if (VoiceOverDefault)
	{
		switch (CurrentState)
		{
		case EBreathingState::RUNNING:
			BreathingStateStr = FString(TEXT("RUNNING"));
			break;
		case EBreathingState::SPRITING:
			BreathingStateStr = FString(TEXT("SPRITING"));
			break;
		case EBreathingState::LOWSTAMINA:
			BreathingStateStr = FString(TEXT("LOWSTAMINA"));
			break;
		case EBreathingState::HIDING:
			BreathingStateStr = FString(TEXT("HIDING"));
			break;
		default:
			BreathingStateStr = FString(TEXT("NONE"));
			break;
		}
	}

	return BreathingStateStr;
}

void USCVoiceOverComponent_Counselor::PlayVoiceOver(const FName VoiceOverName, bool CinematicKill/*=false*/, bool Queue /*= false*/)
{
	// Don't play VO that's on cooldown
	if (IsOnCooldown(VoiceOverName))
		return;

	if (IsInCinematicKill)
		return;

	if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		static FName NAME_Died(TEXT("Died"));
		static FName NAME_HoldBreath(TEXT("HoldBreath"));
		if ((CounselorCharacter->IsAlive() || (VoiceOverName == NAME_Died && !CounselorCharacter->IsInContextKill())) && (!CounselorCharacter->IsHoldingBreath() || VoiceOverName == NAME_HoldBreath))
		{
			const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
			if (VoiceOverDefault && AudioComponent)
			{
				FVoiceOverData VoiceOver = VoiceOverDefault->GetVoiceOverDataByVOName(VoiceOverName);

				if (!CurrentPlayingVOEvent.IsNone() && AudioComponent->IsPlaying() && Queue)
				{
					if (!VoiceOver.VoiceOverFile.IsNull())
						NextVoiceOverName = VoiceOverName;
					else
						NextVoiceOverName = NAME_None;
				}
				else
				{
					PendingVoiceOver = nullptr;
					PendingCinematicVoiceOver = nullptr;
					PendingBreathVoiceOver = nullptr;
					CurrentPlayingVOEvent = NAME_None;
					if (!VoiceOver.VoiceOverName.IsNone() && !VoiceOver.VoiceOverFile.IsNull())
					{
						PendingVoiceOver = VoiceOver.VoiceOverFile;
						CurrentPlayingVOEvent = VoiceOverName;
					}

					if (PendingVoiceOver.Get())
					{
						DeferredPlayVoiceOver();
					}
					else if (!PendingVoiceOver.IsNull())
					{
						// Load and play when it's ready
						USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
						GameInstance->StreamableManager.RequestAsyncLoad(PendingVoiceOver.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayVoiceOver));
					}
				}
			}
		}
	}
}

void USCVoiceOverComponent_Counselor::PlayVoiceOverBySoundCue(USoundCue *SoundCue)
{
	if (IsInCinematicKill)
		return;

	if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		if ((CounselorCharacter->IsAlive() || !CounselorCharacter->IsInContextKill()))
		{
			const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
			if (VoiceOverDefault && AudioComponent)
			{
				PendingVoiceOver = nullptr;
				PendingCinematicVoiceOver = nullptr;
				PendingBreathVoiceOver = nullptr;
				CurrentPlayingVOEvent = NAME_None;
				if (SoundCue)
				{
					PendingVoiceOver = SoundCue;

					CurrentPlayingVOEvent = NAME_OneShotVO;
				}

				if (PendingVoiceOver.Get())
				{
					DeferredPlayVoiceOver();
				}
				else if (!PendingVoiceOver.IsNull())
				{
					// Load and play when it's ready
					USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
					GameInstance->StreamableManager.RequestAsyncLoad(PendingVoiceOver.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayVoiceOver));
				}
			}
		}
	}
}

void USCVoiceOverComponent_Counselor::PlayCinematicVoiceOver(const FName VoiceOverName)
{
	const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
	if (VoiceOverDefault && AudioComponent)
	{
		IsInCinematicKill = true;

		AudioComponent->Stop();

		FVoiceOverData VoiceOver = VoiceOverDefault->GetVoiceOverDataByVOName(VoiceOverName);

		PendingVoiceOver = nullptr;
		PendingCinematicVoiceOver = nullptr;
		PendingBreathVoiceOver = nullptr;
		if (!VoiceOver.VoiceOverName.IsNone() && !VoiceOver.VoiceOverFile.IsNull())
		{
			PendingCinematicVoiceOver = VoiceOver.VoiceOverFile;
		}

		if (PendingCinematicVoiceOver.Get())
		{
			DeferredPlayCinematicVoiceOver();
		}
		else if (!PendingCinematicVoiceOver.IsNull())
		{
			// Load and play when it's ready
			USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
			GameInstance->StreamableManager.RequestAsyncLoad(PendingCinematicVoiceOver.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlayCinematicVoiceOver));
		}
	}
}

void USCVoiceOverComponent_Counselor::StopVoiceOver()
{
	if (!IsInCinematicKill)
	{
		if (AudioComponent && AudioComponent->IsPlaying())
		{
			AudioComponent->Stop();
		}
	}
}

bool USCVoiceOverComponent_Counselor::PlayKillerInRangeWhileHidingVO(float KillerDistanceSq)
{
	if (KillerDistanceSq <= FMath::Square(KillerHidingVODistance))
	{
		static FName NAME_KillerIsNearWhileHiding(TEXT("KillerIsNearWhileHiding"));
		if (CurrentPlayingVOEvent != NAME_KillerIsNearWhileHiding && !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_HidingFromKillerCooldown))
		{
			PlayVoiceOver(NAME_KillerIsNearWhileHiding);

			// Set a cool down timer so that we don't constantly play the VO.
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_HidingFromKillerCooldown, HidingFromKillerCooldownTime, false);
		}
		return true;
	}
	return false;
}

bool USCVoiceOverComponent_Counselor::PlayKillerInRangeVO(float KillerDistanceSq)
{
	if (!KillerSpottedAndInRange && KillerDistanceSq <= FMath::Square(KillerSpottedDistance))
	{
		static FName NAME_Screams(TEXT("Screams"));
		if (CurrentPlayingVOEvent != NAME_Screams && !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_SpottedKillerCooldownTime))
		{
			PlayVoiceOver(NAME_Screams);

			KillerSpottedAndInRange = true;
		}
		return true;
	}
	return false;
}

void USCVoiceOverComponent_Counselor::KillerOutOfView()
{
	if (!KillerSpottedAndInRange)
		return;

	if (ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		if (ASCKillerCharacter* KillerCharacter = State->CurrentKiller)
		{
			if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
			{
				const float KillerDistanceSq = (KillerCharacter->GetActorLocation() - CounselorCharacter->GetActorLocation()).SizeSquared();
				if (KillerDistanceSq > FMath::Square(KillerSpottedDistance))
				{
					KillerSpottedAndInRange = false;

					// Set a cool down timer so that we don't constantly play the VO.
					GetWorld()->GetTimerManager().SetTimer(TimerHandle_SpottedKillerCooldownTime, SpottedKillerCooldownTime, false);
				}
			}
		}
	}
}

void USCVoiceOverComponent_Counselor::DeferredPlayVoiceOver()
{
	UWorld* World = GetWorld();
	if (!World || !PendingVoiceOver.Get())
		return;

	// Clear the current breathing timer.
	World->GetTimerManager().ClearTimer(TimerHandle_BreathTimer);

	// Stop breathing if it was playing
	if (AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	// Add new Voice Over to Play
	AudioComponent->Sound = PendingVoiceOver.Get();

#if PLATFORM_PS4
	if (USCGameBlueprintLibrary::IsJapaneseSKU(this))
	{
		// Grab what language culture this is to then set the sound wave path to.
		FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
		FString CultureString = Culture->GetTwoLetterISOLanguageName().ToUpper();

		// English is our default culture lets play the sound and exit.
		if (CultureString.Equals("JA"))
		{
			// Japanese lets start the translation.
			USoundCue* SoundCue = Cast<USoundCue>(AudioComponent->Sound);
			if (SoundCue)
			{
				// grabs the wave player nodes.
				TArray<USoundNodeWavePlayer*> WavePlayers;
				SoundCue->RecursiveFindNode(SoundCue->FirstNode, WavePlayers);
				bool LocalizedWaves = false;
				for (int32 WaveIndex = 0; WaveIndex < WavePlayers.Num(); ++WaveIndex)
				{
					// Sound isn't localized yet
					if (!WavePlayers[WaveIndex]->bSoundLocalized)
					{
						// Get the sound wave
						if (USoundWave* SoundWave = WavePlayers[WaveIndex]->GetSoundWave())
						{
							// Grab and parse the path.
							FString SoundPathName = SoundWave->GetPathName();
							TArray<FString> SplitPath;
							SoundPathName.ParseIntoArray(SplitPath, TEXT("."));
							FString LocalizedPath;
							if (SplitPath.Num() > 1)
							{
								// Add the Language extention
								LocalizedPath = SplitPath[0] + "_" + CultureString;
								LocalizedPath = LocalizedPath + "." + SplitPath[1] + "_" + CultureString;
								// Load the new sound wave

								USoundWave* LocalizedSoundWave = LoadObject<USoundWave>(nullptr, *LocalizedPath);
								if (LocalizedSoundWave)
								{
									// Add it to the list of loaded localized waves
									LocalizedSoundWaves.Add(LocalizedSoundWave);
								}
								// there is the chance that this localized wave was already loaded lets search the list.
								else
								{
									for (int i = 0; i < LocalizedSoundWaves.Num(); ++i)
									{
										if (LocalizedSoundWaves[i]->GetPathName() == LocalizedPath)
										{
											LocalizedSoundWave = LocalizedSoundWaves[i];
											break;
										}
									}
								}
								// We found or loaded a new localized wave lets load it into the sound cue.
								if (LocalizedSoundWave)
								{
									// Set the localized wave in the sound cue
									WavePlayers[WaveIndex]->SetSoundWave(LocalizedSoundWave);
									// Mark the sound wave as localized so we don't do it again.
									WavePlayers[WaveIndex]->bSoundLocalized = true;
									LocalizedWaves = true;
								}
							}
						}
						else
						{
							// sound wave isn't loaded yet, lets wait for it.
							FTimerHandle TimerHandle_WaitForWaveToLoad;
							World->GetTimerManager().SetTimer(TimerHandle_WaitForWaveToLoad, this, &USCVoiceOverComponent_Counselor::DeferredPlayVoiceOver, 0.05f, false);
							return;
						}
					}
					else
					{
						LocalizedWaves = true;
					}
				}

				// The sound cue is now localized for Japanese lets play it.
				if (LocalizedWaves)
				{
					AudioComponent->Play();
				}
			}
		}
		else // !CultureString.Equals("JA")
		{
			AudioComponent->Play();
		}
	}
	else // OSS->GetAppId() != TEXT("CUSA11523_00")
	{
		AudioComponent->Play();
	}
#else // !PLATFORM_PS4
	AudioComponent->Play();
#endif

	// if we are playing over another VO currently playing lets remove its binding.
	if (AudioComponent->OnAudioFinished.IsBound())
	{
		AudioComponent->OnAudioFinished.RemoveDynamic(this, &ThisClass::OnCurrentPlayingVOEventFinished);
	}

	// if we played the died VO we need to clean up when its done.
	if (CurrentPlayingVOEvent.IsEqual(TEXT("Died")))
	{
		AudioComponent->OnAudioFinished.AddDynamic(this, &ThisClass::CleanupVOComponent);
	}
	else
	{
		AudioComponent->OnAudioFinished.AddDynamic(this, &ThisClass::OnCurrentPlayingVOEventFinished);
	}
	
	if (ASCCounselorCharacter* CounselorCharacter = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
	{
		float StaminaPercent = CounselorCharacter->GetStamina() / CounselorCharacter->GetMaxStamina();

		if (StaminaPercent <= 0.0f)
		{
			RanOutOfStamina = true;
		}

		if (!CounselorCharacter->IsHoldingBreath() && CounselorCharacter->IsAlive() && !CounselorCharacter->IsInContextKill())
		{
			// Restart the breathing check when this voice over is done.
			World->GetTimerManager().SetTimer(TimerHandle_BreathTimer, this, &ThisClass::CheckBreathing, AudioComponent->Sound->Duration);
		}
	}

	// Reset this
	PendingVoiceOver = nullptr;
}

void USCVoiceOverComponent_Counselor::DeferredPlayCinematicVoiceOver()
{
	// Protect against getting garbage collected while waiting for the Async callback
	if (!IsValid(this) || !IsValid(AudioComponent))
		return;

	if (!PendingCinematicVoiceOver.Get())
		return;
	
	if (AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	UWorld* World = GetWorld();
	if (!World)
		return;

	AudioComponent->Sound = PendingCinematicVoiceOver.Get();

#if PLATFORM_PS4
	if (USCGameBlueprintLibrary::IsJapaneseSKU(this))
	{
		// Grab what language culture this is to then set the sound wave path to.
		FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
		FString CultureString = Culture->GetTwoLetterISOLanguageName().ToUpper();

		// English is our default culture lets play the sound and exit.
		if (CultureString.Equals("JA"))
		{
			// Japanese lets start the translation.
			USoundCue* SoundCue = Cast<USoundCue>(AudioComponent->Sound);
			if (SoundCue)
			{
				// grabs the wave player nodes.
				TArray<USoundNodeWavePlayer*> WavePlayers;
				SoundCue->RecursiveFindNode(SoundCue->FirstNode, WavePlayers);
				bool LocalizedWaves = false;
				for (int32 WaveIndex = 0; WaveIndex < WavePlayers.Num(); ++WaveIndex)
				{
					// Sound isn't localized yet
					if (!WavePlayers[WaveIndex]->bSoundLocalized)
					{
						// Get the sound wave
						if (USoundWave* SoundWave = WavePlayers[WaveIndex]->GetSoundWave())
						{
							// Grab and parse the path.
							FString SoundPathName = SoundWave->GetPathName();
							TArray<FString> SplitPath;
							SoundPathName.ParseIntoArray(SplitPath, TEXT("."));
							FString LocalizedPath;
							if (SplitPath.Num() > 1)
							{
								// Add the Language extention
								LocalizedPath = SplitPath[0] + "_" + CultureString;
								LocalizedPath = LocalizedPath + "." + SplitPath[1] + "_" + CultureString;
								// Load the new sound wave

								USoundWave* LocalizedSoundWave = LoadObject<USoundWave>(nullptr, *LocalizedPath);
								if (LocalizedSoundWave)
								{
									// Add it to the list of loaded localized waves
									LocalizedSoundWaves.Add(LocalizedSoundWave);
								}
								// there is the chance that this localized wave was already loaded lets search the list.
								else
								{
									for (int i = 0; i < LocalizedSoundWaves.Num(); ++i)
									{
										if (LocalizedSoundWaves[i]->GetPathName() == LocalizedPath)
										{
											LocalizedSoundWave = LocalizedSoundWaves[i];
											break;
										}
									}
								}
								// We found or loaded a new localized wave lets load it into the sound cue.
								if (LocalizedSoundWave)
								{
									// Set the localized wave in the sound cue
									WavePlayers[WaveIndex]->SetSoundWave(LocalizedSoundWave);
									// Mark the sound wave as localized so we don't do it again.
									WavePlayers[WaveIndex]->bSoundLocalized = true;
									LocalizedWaves = true;
								}
							}
						}
						else
						{
							// sound wave isn't loaded yet, lets wait for it.
							FTimerHandle TimerHandle_WaitForWaveToLoad;
							World->GetTimerManager().SetTimer(TimerHandle_WaitForWaveToLoad, this, &USCVoiceOverComponent_Counselor::DeferredPlayCinematicVoiceOver, 0.05f, false);
							return;
						}
					}
					else
					{
						LocalizedWaves = true;
					}
				}

				// The sound cue is now localized for Japanese lets play it.
				if (LocalizedWaves)
				{
					AudioComponent->Play();
				}
			}
		}
		else // !CultureString.Equals("JA")
		{
			AudioComponent->Play();
		}
	}
	else // OSS->GetAppId() != TEXT("CUSA11523_00")
	{
		AudioComponent->Play();
	}
#else // !PLATFORM_PS4
	AudioComponent->Play();
#endif

	AudioComponent->OnAudioFinished.RemoveAll(this);
	AudioComponent->OnAudioFinished.AddDynamic(this, &ThisClass::CleanupVOComponent);

	// Reset this
	PendingCinematicVoiceOver = nullptr;
}

void USCVoiceOverComponent_Counselor::DeferredPlayBreath()
{
	UWorld* World = GetWorld();
	if (!World || !PendingBreathVoiceOver.Get())
		return;

	if (AudioComponent)
	{
		if (AudioComponent->IsPlaying())
		{
			AudioComponent->Stop();
		}

		AudioComponent->Sound = PendingBreathVoiceOver.Get();
		AudioComponent->Play();
	}
	
	// Set a timer for the next breath
	World->GetTimerManager().SetTimer(TimerHandle_BreathTimer, this, &ThisClass::CheckBreathing, PendingBreathVoiceOver.Get()->Duration + 0.1f);
}

void USCVoiceOverComponent_Counselor::OnCurrentPlayingVOEventFinished()
{
	CurrentPlayingVOEvent = TEXT("");

	if (!NextVoiceOverName.IsNone())
	{
		// Defer playing the voice over for one tick, because this delegate can be hit by garbage collection causing a crash
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::PlayVoiceOver, NextVoiceOverName, false, false));
		}
		NextVoiceOverName = NAME_None;
	}
}

bool USCVoiceOverComponent_Counselor::IsVoiceOverPlaying(const FName VoiceOverName)
{
	if (VoiceOverName == CurrentPlayingVOEvent)
		return true;
	
	return false;
}

bool USCVoiceOverComponent_Counselor::IsCounselorTalking() const
{
	return CurrentPlayingVOEvent == NAME_OneShotVO;
}

void USCVoiceOverComponent_Counselor::HoldBreath(bool bHoldBreath)
{
	if (bHoldBreath)
	{
		static FName NAME_HoldBreath(TEXT("HoldBreath"));
		PlayVoiceOver(NAME_HoldBreath);
	}
	else
	{
		static FName NAME_HoldBreathEnd(TEXT("HoldBreathEnd"));
		PlayVoiceOver(NAME_HoldBreathEnd);
	}
}

void USCVoiceOverComponent_Counselor::OnKillerClosingInDuringChase(bool bGettingChased)
{
	if (bGettingChased)
	{
		if (!HasKillerClosingInDuringChase && !GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_GettingChasedCooldown))
		{
			static FName NAME_ChaseStart(TEXT("ChaseStart"));
			PlayVoiceOver(NAME_ChaseStart);

			HasKillerClosingInDuringChase = true;
		}
	}
	else
	{
		HasKillerClosingInDuringChase = false;

		// Set a cooldown timer so that we don't constantly play the VO.
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_GettingChasedCooldown, GettingChasedCooldown, false);
	}
}
