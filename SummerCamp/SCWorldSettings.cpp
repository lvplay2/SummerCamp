// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWorldSettings.h"

#include "SCCabin.h"
#include "SCEscapePod.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameNetworkManager.h"
#include "SCGameSession.h"
#include "SCGameState_Hunt.h"
#include "SCInGameHUD.h"
#include "SCKillerCharacter.h"
#include "SCKillZDamageType.h"
#include "SCPlayerController.h"
#include "SCVoiceOverComponent.h"

#include "SCGameState_SPChallenges.h"
#include "SCChallengeData.h"
#include "SCDossier.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "LevelSequenceActor.h"

#if WITH_EDITOR
TAutoConsoleVariable<int32> CVarEmulateMatchStartInPIE(TEXT("sc.EmulateMatchStartInPIE"), 0,
	TEXT("Emulate normal match flow in PIE? Can be tedious.\n")
);
#endif

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarSCRainForceState(TEXT("sc.RainForceState"), -1,
	TEXT("0 to force off, 1 to force on, -1 for no preference\n")
);
#endif

#define INTRO_OUTRO_COOLDOWN 1.0f

ASCWorldSettings::ASCWorldSettings(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TotalFuseCount(1)
, RainChance(0.5f) // 50% chance by default
, bOverrideMatchTime(false) // By default do NOT override the game time per level
, LevelMatchTimeOverride(600) // Default custom time to 10 minutes.
{
	GameNetworkManagerClass = ASCGameNetworkManager::StaticClass();
	KillZDamageType = USCKillZDamageType::StaticClass();

	RandomLevelIndex = INDEX_NONE;

	// Default the hero character to Tommy Jarvis
	static ConstructorHelpers::FClassFinder<ASCCounselorCharacter> HunterPawnObject(TEXT("Pawn'/Game/Characters/Counselors/Hunter/Hunter_Counselor.Hunter_Counselor_C'"));
	HeroCharacterClass = HunterPawnObject.Class;
}

void ASCWorldSettings::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCWorldSettings, RandomLevelIndex);
	DOREPLIFETIME(ASCWorldSettings, bIsRaining);
	DOREPLIFETIME(ASCWorldSettings, RandomizedPods);
}

void ASCWorldSettings::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_OutroCompleted);
		World->GetTimerManager().ClearTimer(TimerHandle_IntroVideoCompleted);
		World->GetTimerManager().ClearTimer(TimerHandle_OutroVideoCompleted);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCWorldSettings::NotifyBeginPlay()
{
	Super::NotifyBeginPlay();

	if (Role != ROLE_Authority)
	{
		// Check if we were told to play a level intro before BeginPlay and start it now
		if (LevelIntroSequence != nullptr || bSkipIntroPendingBeginPlay)
		{
			// Load the weapon and mesh
			UStaticMesh* WeaponMesh = nullptr;
			const ASCWeapon* const KillerWeaponRef = CachedKillerWeapon ? Cast<ASCWeapon>(CachedKillerWeapon->GetDefaultObject()) : nullptr;
			if (IsValid(KillerWeaponRef))
			{
				// Grab that mesh!
				WeaponMesh = KillerWeaponRef->GetMesh<UStaticMeshComponent>()->GetStaticMesh();
			}

			IntroStart(bSkipIntroPendingBeginPlay, CachedKillerSkin, WeaponMesh);
			bSkipIntroPendingBeginPlay = false;
		}
	}

	// Single player challenge override is for editor only.
#if WITH_EDITOR
	if (IsValid(ChallengeOverride))
	{
		if (UWorld* World = GetWorld())
		{
			if (World->IsPlayInEditor())
			{
				if (ASCGameState_SPChallenges* SP_Game = Cast<ASCGameState_SPChallenges>(World->GetGameState()))
				{
					SP_Game->Dossier->InitializeChallengeObjectivesFromClass(ChallengeOverride);
				}
			}
		}
	}
#endif
}

void ASCWorldSettings::NotifyMatchStarted()
{
	Super::NotifyMatchStarted();

	// Unload Intro, Prep Outro
	SetLocalVisibilityForLevels(IntroLevels, false, false);
	SetLocalVisibilityForLevels(OutroLevels, false, true);
}

ASCWorldSettings* ASCWorldSettings::GetSCWorldSettings(UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		return Cast<ASCWorldSettings>(WorldContextObject->GetWorld()->GetWorldSettings());
	}

	return nullptr;
}

void ASCWorldSettings::SettingsLoaded()
{
	check(Role == ROLE_Authority);

	UE_LOG(LogSC, Log, TEXT("SettingsLoaded"));

	if (LevelIntroSequence && !bIntroPlaying)
	{
		// we're waiting to play the level intro?
		StartLevelIntro(false);
	}
	else if (LevelOutroSequence && !bOutroPlaying)
	{
		// we're waiting to play the level outro?
		StartLevelOutro();
	}
}

void ASCWorldSettings::SetLocalVisibilityForLevels(const TArray<FName>& LevelNames, const bool bShouldBeVisible, const bool bShouldBeLoaded)
{
	UWorld* World = GetWorld();
	for (FName LevelName : LevelNames)
	{
		if (ULevelStreaming* LevelStreamingObject = UGameplayStatics::GetStreamingLevel(World, LevelName))
		{
			LevelStreamingObject->bShouldBeLoaded = bShouldBeLoaded;
			LevelStreamingObject->bShouldBeVisible = bShouldBeVisible;
		}
		else
		{
			UE_LOG(LogSC, Error, TEXT("Could not find level %s during ASCWorldSettings::SetLocalVisibilityForLevels"), *LevelName.ToString());
		}
	}
}

void ASCWorldSettings::SkipLevelIntroOutro()
{
	check(Role == ROLE_Authority);

	// Stop the intro/outro
	if (bIntroPlaying)
	{
		// Notify the level
		LevelIntroSkippedDelegate.Broadcast(KillerClass, nullptr, nullptr);

		StopLevelIntro();
	}
	else if (bOutroPlaying)
	{
		// Notify the level
		LevelOutroSkippedDelegate.Broadcast(KillerClass, nullptr, nullptr);

		StopLevelOutro();
	}

	// Immediately cut to black on everybody
	UWorld* World = GetWorld();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(*It))
		{
			PC->PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
		}
	}
}

ALevelSequenceActor* ASCWorldSettings::GetIntroVideo() const
{
	for (int i = 0; i < IntroSequences.Num(); ++i)
	{
		if (IntroSequences[i].KillerClass.Get() == KillerClass)
		{
			return IntroSequences[i].Sequence;
		}
	}
	return nullptr;
}

void ASCWorldSettings::StartLevelIntro(bool bSkipToEnd)
{
	check(Role == ROLE_Authority);

	UWorld* World = GetWorld();

#if WITH_EDITOR
	// Skip the intro for PIE by default
	if (World->IsPlayInEditor() && CVarEmulateMatchStartInPIE.GetValueOnGameThread() == 0)
		bSkipToEnd = true;
#endif

	// cache these off while we have the killer player state.
	TSubclassOf<USCJasonSkin> JasonSkin = nullptr;
	TSoftClassPtr<ASCWeapon> KillerWeaponClass = nullptr;

	ASCPlayerState* KillerPlayerState = nullptr;
	if (ASCGameState_Hunt* HuntGS = Cast<ASCGameState_Hunt>(World->GetGameState()))
	{
		if (HuntGS->CurrentKillerPlayerState)
		{
			KillerPlayerState = HuntGS->CurrentKillerPlayerState;

			HuntGS->KillerClass.LoadSynchronous(); // TODO: Make Async
			KillerClass = HuntGS->KillerClass.Get();
			KillerClassSet.Broadcast();

			// Load the Jason skin
			JasonSkin = HuntGS->CurrentKillerPlayerState->GetKillerSkin();

			// Load the weapon
			if (!HuntGS->CurrentKillerPlayerState->GetKillerWeapon().IsNull())
			{
				KillerWeaponClass = HuntGS->CurrentKillerPlayerState->GetKillerWeapon();
				KillerWeaponClass.LoadSynchronous();
			}
		}
	}
	else if (ASCGameState* GS = World->GetGameState<ASCGameState>())
	{
		if (GS->CurrentKillerPlayerState)
		{
			KillerPlayerState = GS->CurrentKillerPlayerState;

			KillerClass = GS->CurrentKillerPlayerState->GetActiveCharacterClass();
			if (KillerClass)
				KillerClassSet.Broadcast();

			// Load the Jason skin
			JasonSkin = GS->CurrentKillerPlayerState->GetKillerSkin();

			// Load the weapon
			if (!GS->CurrentKillerPlayerState->GetKillerWeapon().IsNull())
			{
				KillerWeaponClass = GS->CurrentKillerPlayerState->GetKillerWeapon();
				KillerWeaponClass.LoadSynchronous();
			}
		}
	}

	if (IntroSequences.Num() > 0 && !LevelIntroSequence)
	{
		LevelIntroSequence = GetIntroVideo();
	}

	UE_LOG(LogSC, Log, TEXT("ServerPlayIntro %s %s"), *KillerWeaponClass.ToString(), *GetNameSafe(KillerClass));

	// We have some null data, if we haven't already attempted to load it then lets try that.
	if (!bSkipToEnd && !WaitingOnSettingsLoad && KillerPlayerState && (!JasonSkin || KillerWeaponClass.IsNull()))
	{
		WaitingOnSettingsLoad = true;
		KillerPlayerState->CLIENT_RequestLoadPlayerSettings();
		return;
	}

	WaitingOnSettingsLoad = false;
	ClientsPlayIntro(LevelIntroSequence, KillerClass, bSkipToEnd, JasonSkin, KillerWeaponClass);
	ForceNetUpdate();
}

void ASCWorldSettings::StopLevelIntro(const bool bSkipCooldown/* = false*/)
{
	if (bIntroPlaying && LevelIntroSequence)
	{
		LevelIntroSequence->SequencePlayer->Stop();
		LevelIntroSequence = nullptr;
		bSkipIntroPendingBeginPlay = false;

		if (bSkipCooldown)
		{
			// Force completion now, skipping the cool down
			IntroCompleted();
		}
	}
}

void ASCWorldSettings::ClientsPlayIntro_Implementation(ALevelSequenceActor* Sequence, TSubclassOf<ASCKillerCharacter> SelectedKillerClass, const bool bSkipToEnd, TSubclassOf<USCJasonSkin> KillerSkin, const TSoftClassPtr<ASCWeapon>& KillerWeapon)
{
	UWorld* World = GetWorld();

	// Show the intro levels when we're not skipping it
	SetLocalVisibilityForLevels(IntroLevels, !bSkipToEnd, true);

	// Flush level streaming to ensure it's good to go
	World->FlushLevelStreaming();

	UE_LOG(LogSC, Log, TEXT("ClientPlayIntro %s %s"), *KillerWeapon.ToString(), *GetNameSafe(SelectedKillerClass));

	KillerWeapon.LoadSynchronous();
	CachedKillerSkin = KillerSkin;
	CachedKillerWeapon = KillerWeapon.Get();

	// Load the weapon and mesh
	UStaticMesh* WeaponMesh = nullptr;

	const ASCWeapon* const KillerWeaponRef = CachedKillerWeapon ? Cast<ASCWeapon>(CachedKillerWeapon->GetDefaultObject()) : nullptr;
	if (IsValid(KillerWeaponRef))
	{
		// Grab that mesh!
		WeaponMesh = KillerWeaponRef->GetMesh<UStaticMeshComponent>()->GetStaticMesh();
	}

	// Let level Blueprints know
	if (KillerClass != SelectedKillerClass)
	{
		KillerClass = SelectedKillerClass;
		KillerClassSet.Broadcast();
	}

	// Start the intro
	if (Role == ROLE_Authority || World->bBegunPlay)
	{
		LevelIntroSequence = Sequence;
		IntroStart(bSkipToEnd, KillerSkin, WeaponMesh);
	}
	else
	{
		// Defer this until we finish BeginPlay locally
		if (bSkipToEnd)
		{
			bSkipIntroPendingBeginPlay = true;
		}
		else
		{
			LevelIntroSequence = Sequence;
		}
	}
}

void ASCWorldSettings::IntroStart(const bool bSkipToEnd, TSubclassOf<USCJasonSkin> KillerSkin, UStaticMesh* KillerWeapon)
{
	// Do not stack
	if (bIntroPlaying)
		return;

	bIntroPlaying = true;

	UWorld* World = GetWorld();
	if (!bSkipToEnd && LevelIntroSequence)
	{
		// Notify the level
		LevelIntroStartingDelegate.Broadcast(KillerClass, KillerSkin, KillerWeapon);

		// Start the IntroSequence
		LevelIntroSequence->SequencePlayer->OnStop.AddDynamic(this, &ThisClass::IntroStopped);

		TArray<AActor*> AdditionalReceivers;
		AdditionalReceivers.Add(this);
		
		LevelIntroSequence->SetEventReceivers(AdditionalReceivers);
		LevelIntroSequence->SequencePlayer->Play();

		// Notify all players
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
				PC->NotifyLevelIntroSequenceStarted();
		}

		// Notify the level
		LevelIntroStartedDelegate.Broadcast(KillerClass, KillerSkin, KillerWeapon);
	}
	else
	{
		bIntroPlaying = true;

		// Notify the level
		LevelIntroSkippedDelegate.Broadcast(KillerClass, KillerSkin, KillerWeapon);

		// No valid intro found, skip to completion
		GetWorld()->GetTimerManager().SetTimerForNextTick( this, &ThisClass::IntroCompleted);
	}
}

void ASCWorldSettings::CheckClientsIntroState()
{
	if (Role == ROLE_Authority &&
		LevelIntroSequence != nullptr)
	{
		// Immediately cut to black on everybody
		UWorld* World = GetWorld();
		bool bAllDoneWithIntro = true;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = Cast<APlayerController>(*It);
			const ASCPlayerState* PS = PC ? Cast<ASCPlayerState>(PC->PlayerState) : nullptr;

			if (PS && !PS->bIntroSequenceCompleted)
			{
				bAllDoneWithIntro = false;
				break;
			}
		}

		if (bAllDoneWithIntro)
		{
			// Kill the intro now
			IntroCompleted();
		}
	}
}

void ASCWorldSettings::IntroSequenceCompleted()
{
	// Get our player state
	UWorld* World = GetWorld();
	ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	APlayerController* PC = LocalPlayer ? LocalPlayer->GetPlayerController(World) : nullptr;
	ASCPlayerState* PS = PC ? Cast<ASCPlayerState>(PC->PlayerState) : nullptr;

	// Inform that the intro is completed
	if (PS)
	{
		PS->InformIntroCompleted();
	}
}

void ASCWorldSettings::IntroStopped()
{
	// If the server gets to the end of the intro, call it done for everyone
	if (Role == ROLE_Authority)
	{
		if (bIntroPlaying)
		{
			IntroCompleted();
		}
	}
}

void ASCWorldSettings::IntroCompleted()
{
	// Ensure we don't do this twice
	if (!bIntroPlaying)
	{
		return;
	}

	// No longer playing
	bIntroPlaying = false;

	// Make sure these are cleaned up
	if (LevelIntroSequence != nullptr)
	{
		LevelIntroSequence->SequencePlayer->Stop();
		LevelIntroSequence = nullptr;
	}

	bSkipIntroPendingBeginPlay = false;

	UWorld* World = GetWorld();

	// Hide/unload the intro levels
	SetLocalVisibilityForLevels(IntroLevels, false, false);

	if (Role == ROLE_Authority)
	{
		// Start the actual match
		World->GetAuthGameMode<AGameMode>()->StartMatch();

		if (bOverrideMatchTime)
		{
			if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
			{
				GS->RemainingTime = LevelMatchTimeOverride;
			}
		}

		// Notify all players
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
			{
				PC->NotifyLevelIntroSequenceCompleted();
			}
		}
	}

	// Notify the level
	LevelIntroCompletedDelegate.Broadcast(KillerClass, nullptr, nullptr);
}

ALevelSequenceActor* ASCWorldSettings::GetOutroVideo() const
{
	for (int i = 0; i < OutroSequences.Num(); ++i)
	{
		if (OutroSequences[i].KillerClass.Get() == KillerClass)
		{
			return OutroSequences[i].Sequence;
		}
	}
	return nullptr;
}

void ASCWorldSettings::StartLevelOutro()
{
	check(Role == ROLE_Authority);

	UWorld* World = GetWorld();

	// cache these off while we have the killer player state.
	TSubclassOf<USCJasonSkin> JasonSkin = nullptr;
	TSoftClassPtr<ASCWeapon> KillerWeaponClass = nullptr;

	ASCPlayerState* KillerPlayerState = nullptr;

	// Only play the outro if the killer is still alive
	bool bShouldPlayOutro = false; //Default to no outro for safety.
	if (ASCGameState* GS = Cast<ASCGameState>(World->GetGameState()))
	{
		if (GS->CurrentKiller)
			bShouldPlayOutro = GS->CurrentKiller->IsAlive();

		if (GS->CurrentKillerPlayerState)
		{
			// Load the Jason skin
			KillerPlayerState = GS->CurrentKillerPlayerState;
			JasonSkin = GS->CurrentKillerPlayerState->GetKillerSkin();

			// Load the weapon
			if (!GS->CurrentKillerPlayerState->GetKillerWeapon().IsNull())
			{
				KillerWeaponClass = GS->CurrentKillerPlayerState->GetKillerWeapon();
			}
		}
	}

	KillerWeaponClass.LoadSynchronous();

	// Pick an outro video based on the KillerClass
	if (OutroSequences.Num() > 0 && !LevelOutroSequence && bShouldPlayOutro)
	{
		// Search for the outro sequence to play
		LevelOutroSequence = GetOutroVideo();
	}

	// We have some null data, if we haven't already attempted to load it then lets try that.
	if (LevelOutroSequence && !WaitingOnSettingsLoad && KillerPlayerState && (!JasonSkin || KillerWeaponClass.IsNull()))
	{
		WaitingOnSettingsLoad = true;
		KillerPlayerState->CLIENT_RequestLoadPlayerSettings();
		return;
	}
	

	WaitingOnSettingsLoad = false;
	// Now tell everyone to play it
	ClientsPlayOutro(LevelOutroSequence, KillerClass, JasonSkin, KillerWeaponClass);
	ForceNetUpdate();
}

void ASCWorldSettings::StopLevelOutro()
{
	if (bOutroPlaying && LevelOutroSequence)
	{
		LevelOutroSequence->SequencePlayer->Stop();
		LevelOutroSequence = nullptr;
	}
}

void ASCWorldSettings::ClientsPlayOutro_Implementation(ALevelSequenceActor* Sequence, TSubclassOf<ASCKillerCharacter> SelectedKillerClass, TSubclassOf<USCJasonSkin> KillerSkin, const TSoftClassPtr<ASCWeapon>& KillerWeapon)
{
	UWorld* World = GetWorld();

	UE_LOG(LogSC, Log, TEXT("ClientPlayOutro"));

	// Show the outro levels
	SetLocalVisibilityForLevels(OutroLevels, true, true);

	// Load the weapon and mesh
	UStaticMesh* WeaponMesh = nullptr;

	KillerWeapon.LoadSynchronous();
	const ASCWeapon* const KillerWeaponRef = KillerWeapon.Get() ? Cast<ASCWeapon>(KillerWeapon.Get()->GetDefaultObject()) : nullptr;
	if (IsValid(KillerWeaponRef))
	{
		// Grab that mesh!
		WeaponMesh = KillerWeaponRef->GetMesh<UStaticMeshComponent>()->GetStaticMesh();
	}

	// Flush level streaming to ensure it's good to go
	World->FlushLevelStreaming();

	// Notify level Blueprints
	if (KillerClass != SelectedKillerClass)
	{
		KillerClass = SelectedKillerClass;
		KillerClassSet.Broadcast();
	}

	LevelOutroSequence = Sequence;
	if (LevelOutroSequence && ensure(LevelOutroSequence->SequencePlayer))
	{
		// Notify the level
		LevelOutroStartingDelegate.Broadcast(KillerClass, KillerSkin, WeaponMesh);

		// Start the sequence
		LevelOutroSequence->SequencePlayer->OnStop.AddDynamic(this, &ThisClass::OutroStopCooldown);
		LevelOutroSequence->SequencePlayer->Play();
		bOutroPlaying = true;

		// Notify all players
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
				PC->NotifyLevelOutroSequenceStarted();
		}

		// Notify the level
		LevelOutroStartedDelegate.Broadcast(KillerClass, KillerSkin, WeaponMesh);
	}
	else
	{
		// Notify the level
		LevelOutroSkippedDelegate.Broadcast(KillerClass, KillerSkin, WeaponMesh);

		// Wait a frame, this is being called from a MatchState change so can not change it again
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::OutroCompleted);
	}
}

void ASCWorldSettings::OutroStopCooldown()
{
	// Make sure these are cleaned up
	LevelOutroSequence = nullptr;

	// Wait a short time before ending it so that clients who received the message to start it late don't get the sequence cut off
	GetWorldTimerManager().SetTimer(TimerHandle_OutroCompleted, this, &ThisClass::OutroCompleted, INTRO_OUTRO_COOLDOWN);
}

void ASCWorldSettings::OutroCompleted()
{
	// No longer playing
	bOutroPlaying = false;

	// Make sure these are cleaned up
	LevelOutroSequence = nullptr;

	UWorld* World = GetWorld();

	// Notify all players
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(*It))
			PC->NotifyLevelOutroSequenceCompleted();
	}

	if (Role == ROLE_Authority)
	{
		// Notify the game mode
		World->GetAuthGameMode<ASCGameMode>()->OnPostMatchOutroComplete();
	}

	// Notify the level
	LevelOutroCompletedDelegate.Broadcast(KillerClass, nullptr, nullptr);
}

void ASCWorldSettings::LoadRandomLevel()
{
	check(RandomLevelIndex == INDEX_NONE); // Should initialize to INDEX_NONE
	check(Role == ROLE_Authority);

	if (RandomLevels.Num() > 0)
	{
		// Pick a random level, update immediately and simulate the rep handler
		RandomLevelIndex = FMath::RandRange(0, RandomLevels.Num() - 1);
		ForceNetUpdate();

		// Stream in the random level
		const FLatentActionInfo LatentInfo(0, (int32)'RNDM', TEXT("OnRandomLevelLoaded"), this);
		UGameplayStatics::LoadStreamLevel(this, RandomLevels[RandomLevelIndex], true, true, LatentInfo);
	}
	else
	{
		// No random levels, but still simulate completion
		OnRandomLevelLoaded();
	}

	// Get our rain selection from the travel URL
	ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
	bool bShouldRain = (FMath::FRand() <= RainChance); // Defaults to random
	FString RainOption;
	if (FParse::Value(*GameMode->OptionsString, TEXT("rain="), RainOption))
	{
		if (RainOption.Compare(TEXT("on"), ESearchCase::IgnoreCase) == 0)
			bShouldRain = true;
		else if (RainOption.Compare(TEXT("off"), ESearchCase::IgnoreCase) == 0)
			bShouldRain = false;
	}

#if !UE_BUILD_SHIPPING
	// Optional CVar override in dev builds (trumps URL)
	const int32 ForcedRainState = CVarSCRainForceState.GetValueOnGameThread();
	if (ForcedRainState != -1)
	{
		bShouldRain = (ForcedRainState == 1);
	}
#endif

	SetRaining(bShouldRain);

	// Find the escape pods if any exist
	for (TActorIterator<ASCEscapePod> It(GetWorld()); It; ++It)
	{
		RandomizedPods.AddUnique(*It);
	}

	// shuffle that bitch
	for (int SwapAt = ActivePods.Num() - 1; SwapAt >= 0; --SwapAt)
	{
		RandomizedPods.SwapMemory(SwapAt, FMath::RandRange(0, ActivePods.Num() -1));
	}
	OnRep_RandomizedPods();
}

void ASCWorldSettings::UnloadRandomLevel()
{
	if (RandomLevelIndex != INDEX_NONE)
	{
		// Stream in the random level
		FLatentActionInfo LatentInfo;
		LatentInfo.UUID = 1;
		LatentInfo.Linkage = 0;

		UGameplayStatics::UnloadStreamLevel(this, RandomLevels[RandomLevelIndex], LatentInfo);
	}
}

void ASCWorldSettings::OnRandomLevelLoaded()
{
	check(Role == ROLE_Authority);
	if (UWorld* World = GetWorld())
	{
		// Spawn in random items
		World->GetAuthGameMode<ASCGameMode>()->LoadRandomItems();
	}
}

void ASCWorldSettings::OnRep_RandomizedPods()
{
	ActivePods = RandomizedPods;
	for ( ; ActivePods.Num() > NumActiveEscapePods; )
	{
		ActivePods[NumActiveEscapePods]->BreakPod();
		ActivePods.RemoveAt(NumActiveEscapePods);
	}
}

USkeletalMesh* ASCWorldSettings::GetKillerMesh() const
{
	if (KillerClass)
	{
		if (const ASCKillerCharacter* const DefaultKiller = Cast<ASCKillerCharacter>(KillerClass->GetDefaultObject()))
		{
			return DefaultKiller->GetPawnMesh()->SkeletalMesh;
		}
	}

	return nullptr;
}

USkeletalMesh* ASCWorldSettings::GetKillerMaskMesh() const
{
	if (KillerClass)
	{
		if (const ASCKillerCharacter* const DefaultKiller = Cast<ASCKillerCharacter>(KillerClass->GetDefaultObject()))
		{
			return DefaultKiller->GetKillerMaskMesh()->SkeletalMesh;
		}
	}

	return nullptr;
}

void ASCWorldSettings::SetRaining(bool bShouldRain)
{
	check(Role == ROLE_Authority);
	if (RainEnabledLevel.IsNone() || RainDisabledLevel.IsNone())
	{
#if !UE_BUILD_SHIPPING
		FMessageLog("PIE").Warning(FText::FromString(FString::Printf(TEXT("Could not set raining as either RainEnabledLevel (%s) or RainDisabledLevel (%s) is unset!"), *RainEnabledLevel.ToString(), *RainDisabledLevel.ToString())));
#endif
		return;
	}

	// Steam in the level
	const FLatentActionInfo NoCallback(0,  (int32)'UNLD', TEXT(""), nullptr);

	if (bShouldRain)
	{
		const FLatentActionInfo RainLevelLoaded(0, (int32)'Wet!', TEXT("OnRainLevelLoaded"), this);
		UGameplayStatics::UnloadStreamLevel(this, RainDisabledLevel, NoCallback);
		UGameplayStatics::LoadStreamLevel(this, RainEnabledLevel, true, true, RainLevelLoaded);
	}
	else
	{
		const FLatentActionInfo DryLevelLoaded(0,  (int32)'Dry!', TEXT("OnDryLevelLoaded"), nullptr);
		UGameplayStatics::UnloadStreamLevel(this, RainEnabledLevel, NoCallback);
		UGameplayStatics::LoadStreamLevel(this, RainDisabledLevel, true, true, DryLevelLoaded);
	}

	bIsRaining = bShouldRain;
	OnRep_IsRaining();
}

void ASCWorldSettings::OnRainLevelLoaded()
{
}

void ASCWorldSettings::OnDryLevelLoaded()
{
}

void ASCWorldSettings::OnRep_IsRaining()
{
	// Set material params
	if (RainMaterialParameterColleciton)
	{
		static const FName NAME_RainParam(TEXT("Rain"));
		UKismetMaterialLibrary::SetScalarParameterValue(this, RainMaterialParameterColleciton, NAME_RainParam, GetIsRaining() ? 1.f : 0.f);
	}

	OnRainingStateChanged.Broadcast(bIsRaining);
}
