// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCRadio.h"

#include "SCAudioSettingsSaveGame.h"
#include "SCCounselorCharacter.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCLocalPlayer.h"
#include "SCSpecialMoveComponent.h"

ASCRadio::ASCRadio(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = false;
	NetUpdateFrequency = 40.0f;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Destroy_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("Destroy_SpecialMoveHandler"));
	Destroy_SpecialMoveHandler->SetupAttachment(RootComponent);
	Destroy_SpecialMoveHandler->bTakeCameraControl = false;

	DestructionDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DestructionDriver"));
	DestructionDriver->SetNotifyName("DestroyRadio");

	CounselorUse_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("CounselorUse_SpecialMoveHandler"));
	CounselorUse_SpecialMoveHandler->SetupAttachment(RootComponent);
	CounselorUse_SpecialMoveHandler->bTakeCameraControl = false;

	UseRadioDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("UseRadioDriver"));
	UseRadioDriver->SetNotifyName("RadioUse");

	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicComponent"));
	MusicComponent->SetupAttachment(RootComponent);

	SoundEffectsComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundEffectsComponent"));
	SoundEffectsComponent->SetupAttachment(RootComponent);

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Press | (int32)EILLInteractMethod::Hold;
	InteractComponent->bCanHighlight = true;
	InteractComponent->HoldTimeLimit = 1.5f;
	InteractComponent->SetupAttachment(RootComponent);

	PerkStaminaModifierVolume = CreateDefaultSubobject<USphereComponent>(TEXT("PerkModifierVolume"));
	PerkStaminaModifierVolume->InitSphereRadius(1500.0f);
	PerkStaminaModifierVolume->SetupAttachment(RootComponent);
}

void ASCRadio::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCRadio, bIsPlaying);
	DOREPLIFETIME(ASCRadio, bIsBroken);
	DOREPLIFETIME(ASCRadio, bIsChangingTracks);
}

void ASCRadio::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCRadio::OnInteract);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCRadio::CanInteractWith);
	}

	if (Destroy_SpecialMoveHandler)
	{
		Destroy_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCRadio::DestructionStarted);
		Destroy_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCRadio::DestructionComplete);
	}

	if (DestructionDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCRadio::OnBreakRadio);
		DestructionDriver->InitializeBegin(BeginDelegate);
	}

	if (CounselorUse_SpecialMoveHandler)
	{
		CounselorUse_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCRadio::OnUseRadioStarted);
		CounselorUse_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCRadio::OnUseRadioComplete);
	}

	if (UseRadioDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCRadio::OnUseRadio);
		UseRadioDriver->InitializeBegin(BeginDelegate);
	}

	if (PerkStaminaModifierVolume)
	{
		PerkStaminaModifierVolume->OnComponentBeginOverlap.AddDynamic(this, &ASCRadio::OnBeginPerkVolumeOverlap);
		PerkStaminaModifierVolume->OnComponentEndOverlap.AddDynamic(this, &ASCRadio::OnEndPerkVolumeOverlap);
	}
}

void ASCRadio::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority)
	{
		// cbrungardt - I changed it to Songs.Num() - 1 because we use the last slot of the radio
		// for the Streamer Mode track.
		// nfaletra - NO! BAD CHUCK! Setup a StreamerModeSong because this is too easy fuck up in the blueprint
		if ((Songs.Num()) > 0)
		{
			CurrentSong = FMath::RandHelper(Songs.Num());
		}
	}
}

void ASCRadio::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timers
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_NextTrack);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCRadio::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (Character->IsKiller())
		{
			Destroy_SpecialMoveHandler->ActivateSpecial(Character);
		}
		else
		{
			bIsChangingTracks = false;
			CounselorUse_SpecialMoveHandler->ActivateSpecial(Character);

			// Score event for turning the radio on
			if (Role == ROLE_Authority && !bIsPlaying)
			{
				if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
					GM->HandleScoreEvent(Character->PlayerState, TurnRadioOnScoreEvent);
			}
		}
		break;

	case EILLInteractMethod::Hold:
		bIsChangingTracks = true;
		CounselorUse_SpecialMoveHandler->ActivateSpecial(Character);
		break;
	}
}

int32 ASCRadio::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (bIsBroken)
		return 0;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (Killer->IsAttemptingGrab() || Killer->IsGrabKilling() || Killer->GetGrabbedCounselor() != nullptr)
			return 0;
	}

	if (Character->IsInSpecialMove())
		return 0;

	int32 Flags = (int32)EILLInteractMethod::Press;

	if (bIsPlaying && Character->IsA<ASCCounselorCharacter>())
		Flags |= (int32)EILLInteractMethod::Hold;

	return Flags;
}

void ASCRadio::UpdateStreamerMode(bool bStreamerMode)
{
	if (MusicComponent && MusicComponent->IsPlaying())
	{
		MusicComponent->Stop();

		if (!bStreamerMode)
		{
			CurrentSong = FMath::RandHelper(Songs.Num());
		}

		if (GetWorldTimerManager().IsTimerActive(TimerHandle_NextTrack))
		{
			GetWorldTimerManager().ClearTimer(TimerHandle_NextTrack);
		}

		PlaySong();
	}
}

void ASCRadio::OnBeginPerkVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		Counselor->EnteredRadioPerkModifierVolume(this);
	}
}

void ASCRadio::OnEndPerkVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		Counselor->LeftRadioPerkModifierVolume(this);
	}
}

void ASCRadio::OnUseRadioStarted(ASCCharacter* Interactor)
{
	UseRadioDriver->SetNotifyOwner(Interactor);
}

void ASCRadio::OnUseRadioComplete(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}
}

void ASCRadio::OnUseRadio(FAnimNotifyData NotifyData)
{
	if (bIsChangingTracks)
		NextTrack();
	else
		ToggleMusic();
}

void ASCRadio::ToggleMusic()
{
	if (Role == ROLE_Authority)
	{
		bIsPlaying = !bIsPlaying;
		OnRep_IsPlaying();
	}
}

void ASCRadio::NextTrack()
{
	if (Songs.Num() <= 0)
		return;

	if ((Songs.Num()) > 1)
	{
		int32 NextSong = CurrentSong;
		do
		{
			NextSong = FMath::RandHelper(Songs.Num());
		} while (NextSong == CurrentSong);

		CurrentSong = NextSong;
	}

	// Clear our timer so it starts at the beginning
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_NextTrack))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_NextTrack);
	}

	PlaySong();
}

void ASCRadio::DestructionStarted(ASCCharacter* Interactor)
{
	DestructionDriver->SetNotifyOwner(Interactor);
}

void ASCRadio::DestructionComplete(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
	}
}

void ASCRadio::OnBreakRadio(FAnimNotifyData NotifyData)
{
	DestructionDriver->SetNotifyOwner(nullptr);

	if (Role == ROLE_Authority)
		BreakRadio();
}

void ASCRadio::BreakRadio()
{
	check(Role == ROLE_Authority);

	bIsBroken = true;
	OnRep_Broken();
	bIsPlaying = false;
	OnRep_IsPlaying();
}

void ASCRadio::OnRep_IsPlaying()
{
	bIsPlaying ? PlaySong() : StopMusic();
}

void ASCRadio::OnRep_Broken()
{
	if (InteractComponent)
	{
		ForceNetUpdate();
		InteractComponent->bIsEnabled = !bIsBroken;
	}

	if (bIsBroken)
	{
		OnRadioDestroyed();

		SoundEffectsComponent->SetSound(BreakSound);
		SoundEffectsComponent->Play();
	}
}

void ASCRadio::PlaySong()
{
	TAssetPtr<USoundCue> SongToPlay = IsStreamerMode() ? StreamerModeSong : Songs[CurrentSong];
	if (SongToPlay.Get())
	{
		// Already loaded
		if (GetWorldTimerManager().IsTimerPaused(TimerHandle_NextTrack))
		{
			GetWorldTimerManager().UnPauseTimer(TimerHandle_NextTrack);
			StartMusic(SongToPlay, GetWorldTimerManager().GetTimerElapsed(TimerHandle_NextTrack));
		}
		else
		{
			GetWorldTimerManager().SetTimer(TimerHandle_NextTrack, this, &ThisClass::NextTrack, SongToPlay.Get()->Duration);
			StartMusic(SongToPlay, 0.f);
		}
	}
	else if (!SongToPlay.IsNull())
	{
		// Load and play when it's ready
		USCGameInstance* GameInstance = Cast<USCGameInstance>(GetGameInstance());
		GameInstance->StreamableManager.RequestAsyncLoad(SongToPlay.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::PlaySong));
	}
}

void ASCRadio::StartMusic(const TAssetPtr<USoundCue>& SongToPlay, float StartTime)
{
	if (ensure(SongToPlay.Get()))
	{
		MusicComponent->SetSound(SongToPlay.Get());
		MusicComponent->Play(IsStreamerMode() ? 0.f : StartTime);
	}

	SoundEffectsComponent->SetSound(TurnOnSound);
	SoundEffectsComponent->Play();
}

void ASCRadio::StopMusic()
{
	GetWorldTimerManager().PauseTimer(TimerHandle_NextTrack);
	MusicComponent->Stop();

	if (!bIsBroken)
	{
		SoundEffectsComponent->SetSound(TurnOffSound);
		SoundEffectsComponent->Play();
	}
}

bool ASCRadio::IsStreamerMode()
{
	// Check to see if the player is currently in streaming mode.
	if (UWorld* World = GetWorld())
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(World->GetFirstLocalPlayerFromController()))
		{
			if (USCAudioSettingsSaveGame* AudioSettings = LocalPlayer->GetLoadedSettingsSave<USCAudioSettingsSaveGame>())
			{
				return AudioSettings->bStreamingMode;
			}
		}
	}
	return false;
}
