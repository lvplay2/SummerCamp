// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVoiceOverComponent_Killer.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Sound/SoundNodeWavePlayer.h"

#include "SCGameBlueprintLibrary.h"
#include "SCGameInstance.h"

USCVoiceOverComponent_Killer::USCVoiceOverComponent_Killer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCVoiceOverComponent_Killer::PlayVoiceOver(const FName VoiceOverName, bool CinematicKill /*= false*/, bool Queue /*= false*/)
{
	// Don't play VO that's on cooldown
	if (IsOnCooldown(VoiceOverName))
		return;

	const USCVoiceOverObject* const VoiceOverDefault = VoiceOverObjectClass ? VoiceOverObjectClass->GetDefaultObject<USCVoiceOverObject>() : nullptr;
	if (IsLocallyControlled() && VoiceOverDefault)
	{
		FVoiceOverData VoiceOver = VoiceOverDefault->GetVoiceOverDataByVOName(VoiceOverName);

		PendingVoiceOver = nullptr;
		if (!VoiceOver.VoiceOverName.IsNone() && !VoiceOver.VoiceOverFile.IsNull())
		{
			PendingVoiceOver = VoiceOver.VoiceOverFile;
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

void USCVoiceOverComponent_Killer::DeferredPlayVoiceOver()
{
	UWorld* World = GetWorld();
	if (!World || !PendingVoiceOver.Get())
		return;

#if PLATFORM_PS4
	if (USCGameBlueprintLibrary::IsJapaneseSKU(this)) // Japanese Disc SKU
	{
		// Grab what language culture this is to then set the sound wave path to.
		FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
		FString CultureString = Culture->GetTwoLetterISOLanguageName().ToUpper();

		// English is our default culture lets play the sound and exit.
		if (CultureString.Equals("JA"))
		{
			// Japanese lets start the translation.
			USoundCue* SoundCue = Cast<USoundCue>(PendingVoiceOver.Get());
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
							World->GetTimerManager().SetTimer(TimerHandle_WaitForWaveToLoad, this, &USCVoiceOverComponent_Killer::DeferredPlayVoiceOver, 0.05f, false);
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
					UGameplayStatics::PlaySound2D(World, PendingVoiceOver.Get());
				}
			}
		}
		else // !CultureString.Equals("JA")
		{
			UGameplayStatics::PlaySound2D(World, PendingVoiceOver.Get());
		}
	} // OSS->GetAppId() != TEXT("CUSA11523_00")
	else
	{
		UGameplayStatics::PlaySound2D(World, PendingVoiceOver.Get());
	}
#else // !PLATFORM_PS4
	UGameplayStatics::PlaySound2D(World, PendingVoiceOver.Get());
#endif
}
