// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMusicComponent_Killer.h"

#include "Sound/SoundNodeMixer.h"

#include "SCCounselorCharacter.h"
#include "SCGameMode_SPChallenges.h"
#include "SCKillerCharacter.h"

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarDebugKillerMusic(TEXT("sc.DebugKillerMusic"), 0,
												 TEXT("Displays debug information for music.\n")
												 TEXT(" 0: None\n")
												 TEXT(" 1: Display debug info"));
#endif

USCMusicComponent_Killer::USCMusicComponent_Killer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void USCMusicComponent_Killer::PostInitMusicComponenet()
{
	// return out of this logic if we are in SP challenges.
	UWorld *World = GetWorld();
	if (World && World->GetAuthGameMode<ASCGameMode_SPChallenges>())
	{
		bIsSinglePlayerChallenges = true;
	}
}

void USCMusicComponent_Killer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (CVarDebugKillerMusic.GetValueOnGameThread() > 0 && IsLocalPlayerController())
	{
		const TCHAR* CurrentLayerStr = TEXT("UNKNOWN");
		switch (CurrentLayer)
		{
		case ESCKillerMusicLayer::Searching: CurrentLayerStr = TEXT("Searching"); break;
		case ESCKillerMusicLayer::SpotTarget: CurrentLayerStr = TEXT("SpotTarget"); break;
		case ESCKillerMusicLayer::TargetGone: CurrentLayerStr = TEXT("TargetGone"); break;
		case ESCKillerMusicLayer::ClosingIn: CurrentLayerStr = TEXT("ClosingIn"); break;
		case ESCKillerMusicLayer::Panic: CurrentLayerStr = TEXT("Panic"); break;
		}

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("Current Music Layer: %s"), CurrentLayerStr), true, FVector2D(1.2f, 1.2f));

		if (CurrentMixerNode)
		{
			for (int i = 0; i < CurrentMixerNode->InputVolume.Num(); ++i)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("Track %d - Volume Multiplier: %f"), i, CurrentMixerNode->InputVolume[i]), true, FVector2D(1.2f, 1.2f));
			}
		}
	}
#endif

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
			PostTrackFinished();
		}
	}

	if (CurrentMixerNode)
	{
		if (bIsSinglePlayerChallenges)
		{
			HandleCounselorAIAlerted();
		}
		else
		{
			HandleCounselorInRange();
		}
	}
}

void USCMusicComponent_Killer::PostTrackFinished()
{
	if (!CurrentTrack)
	{
		PlayMusic("JasonsMusic");
		ChangeDefferedLayer();
	}
}

void USCMusicComponent_Killer::CounselorInView(class ASCCounselorCharacter* VisibleCharacter)
{
	// return out of this logic if we are in SP challenges.
	if(bIsSinglePlayerChallenges)
		return;

	bool CounselorNotAlreadyInView = (CounselorsInView.Find(VisibleCharacter) == INDEX_NONE);
	if (CounselorNotAlreadyInView)
	{
		CounselorsInView.Add(VisibleCharacter);
	}
}

void USCMusicComponent_Killer::CounselorOutOfView(class ASCCounselorCharacter* VisibleCharacter)
{
	// return out of this logic if we are in SP challenges.
	if (bIsSinglePlayerChallenges)
		return;

	int index = CounselorsInView.Find(VisibleCharacter);
	if (index != INDEX_NONE)
	{
		CounselorsInView.RemoveAt(index);
	}
}

void USCMusicComponent_Killer::HandleCounselorAIAlerted()
{
	// TODO - cbrungardt
}

void USCMusicComponent_Killer::HandleCounselorInRange()
{
	UWorld* World = GetWorld();
	ASCKillerCharacter* Killer = CastChecked<ASCKillerCharacter>(GetOwner());

	float DesiredDistance = 4000.f;
	const float HalfDistance = DesiredDistance * .5f;

	// Find the closest counselor distance
	bool bAnyCounselorsInRange = false;
	float ClosestCounselorDistSq = FLT_MAX;
	for (int i = 0; i < CounselorsInView.Num(); ++i)
	{
		ASCCounselorCharacter* InViewCounselor = CounselorsInView[i];
		if (InViewCounselor && InViewCounselor->IsAlive())
		{
			bAnyCounselorsInRange = true;

			const float CounselorDistSq = InViewCounselor->GetSquaredDistanceTo(GetOwner());
			if (CounselorDistSq <= FMath::Square(DesiredDistance))
			{
				if (ClosestCounselorDistSq > CounselorDistSq)
				{
					ClosestCounselorDistSq = CounselorDistSq;
				}
			}
		}
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	ESCKillerMusicLayer NewLayer = CurrentLayer;
	if (bAnyCounselorsInRange)
	{
		if (ClosestCounselorDistSq <= FMath::Square(DesiredDistance))
		{
			if (ClosestCounselorDistSq <= FMath::Square(HalfDistance))
			{
				if (CurrentLayer == ESCKillerMusicLayer::Searching || CurrentLayer == ESCKillerMusicLayer::SpotTarget || CurrentLayer == ESCKillerMusicLayer::ClosingIn)
				{
					NewLayer = ESCKillerMusicLayer::Panic;
				}
			}
			else if (CurrentLayer == ESCKillerMusicLayer::Searching || CurrentLayer == ESCKillerMusicLayer::SpotTarget || CurrentLayer == ESCKillerMusicLayer::Panic)
			{
				NewLayer = ESCKillerMusicLayer::ClosingIn;
			}
		}
		else
		{
			if (CurrentLayer == ESCKillerMusicLayer::Searching || CurrentLayer == ESCKillerMusicLayer::TargetGone)
			{
				NewLayer = ESCKillerMusicLayer::SpotTarget;
			}

			if (CurrentLayer == ESCKillerMusicLayer::ClosingIn)
			{
				NewLayer = ESCKillerMusicLayer::SpotTarget;
			}
		}
	}
	else
	{
		if (CurrentLayer == ESCKillerMusicLayer::ClosingIn || CurrentLayer == ESCKillerMusicLayer::SpotTarget || CurrentLayer == ESCKillerMusicLayer::Panic)
		{
			NewLayer = ESCKillerMusicLayer::TargetGone;

			TimerManager.SetTimer(TimerHandle_LostTargetCooldownTimer, this, &USCMusicComponent_Killer::GoToSearchingLayer, 2.f);
		}
	}

	if (CurrentLayer < NewLayer)
	{
		if (NewLayer != ESCKillerMusicLayer::TargetGone)
		{
			TimerManager.ClearTimer(TimerHandle_LostTargetCooldownTimer);
		}

		CurrentLayer = NewLayer;

		TimerManager.ClearTimer(TimerHandle_RemoveDefferedTimer);

		if (!TimerManager.IsTimerActive(TimerHandle_AddDefferedTimer))
		{
			TimerManager.SetTimer(TimerHandle_AddDefferedTimer, this, &USCMusicComponent_Killer::ChangeDefferedLayer, 2.f);
		}
	}
	else if (CurrentLayer > NewLayer)
	{
		if (NewLayer != ESCKillerMusicLayer::TargetGone)
		{
			TimerManager.ClearTimer(TimerHandle_LostTargetCooldownTimer);
		}

		CurrentLayer = NewLayer;

		TimerManager.ClearTimer(TimerHandle_AddDefferedTimer);

		if (!TimerManager.IsTimerActive(TimerHandle_RemoveDefferedTimer))
		{
			TimerManager.SetTimer(TimerHandle_RemoveDefferedTimer, this, &USCMusicComponent_Killer::ChangeDefferedLayer, 2.f);
		}
	}
}

void USCMusicComponent_Killer::ChangeDefferedLayer()
{
	ChangeMusicLayer(CurrentLayer);
}

void USCMusicComponent_Killer::GoToSearchingLayer()
{
	ChangeMusicLayer(CurrentLayer = ESCKillerMusicLayer::Searching);
}

void USCMusicComponent_Killer::CleanupMusic(bool SkipTwoMinuteWarningMusic /* = false */)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_AddDefferedTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_RemoveDefferedTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_LostTargetCooldownTimer);

		CurrentLayer = ESCKillerMusicLayer::Searching;
		World->GetTimerManager().ClearTimer(TimerHandle_AddDefferedTimer);
		World->GetTimerManager().ClearTimer(TimerHandle_RemoveDefferedTimer);
	}
	
	Super::CleanupMusic(SkipTwoMinuteWarningMusic);
}