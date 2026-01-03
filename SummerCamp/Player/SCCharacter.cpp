// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacter.h"

// UE4
#include "Animation/AnimMontage.h"
#include "Classes/Sound/SoundWaveProcedural.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "WidgetComponent.h"

// IGF
#include "ILLCrowdAgentInterfaceComponent.h"
#include "ILLDynamicCameraComponent.h"
#include "ILLInteractableManagerComponent.h"
#include "ILLPlayerInput.h"

// SC
#include "NavAreas/SCNavArea_JasonOnly.h"
#include "SCAfterimage.h"
#include "SCCabin.h"
#include "SCCabinet.h"
#include "SCCabinGroupVolume.h"
#include "SCCharacterAIController.h"
#include "SCCharacterMovement.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAnimInstance.h"
#include "SCDestructableWall.h"
#include "SCDoor.h"
#include "SCDriveableVehicle.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameMode_Offline.h"
#include "SCGameNetworkManager.h"
#include "SCGameState.h"
#include "SCGameState_Paranoia.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCIndoorComponent.h"
#include "SCInGameHUD.h"
#include "SCItem.h"
#include "SCMinimapIconComponent.h"
#include "SCMusicComponent.h"
#include "SCPamelaTape.h"
#include "SCTommyTape.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCPlayerController.h"
#include "SCPowerPlant.h"
#include "SCRepairPart.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSplineCamera.h"
#include "SCTrap.h"
#include "SCWaypoint.h"
#include "SCWeapon.h"
#include "SCWindow.h"
#include "SCWorldSettings.h"
#include "StatModifiedValue.h"


TAutoConsoleVariable<int32> CVarDebugCombat(TEXT("sc.DebugCombat"), 0,
	TEXT("Displays debug information for combat.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

TAutoConsoleVariable<int32> CVarDisableStun(TEXT("sc.DisableStun"), 0,
	TEXT("Enables or disables stun animations.\n")
	TEXT(" 0: Enabled\n")
	TEXT(" 1: Disabled\n"));

TAutoConsoleVariable<int32> CVarDisableDestroyWeapon(TEXT("sc.DisableDestroyWeapon"), 0,
	TEXT("Enables or disables Weapon destruction on hit.\n")
	TEXT(" 0: Enabled\n")
	TEXT(" 1: Disabled\n"));

TAutoConsoleVariable<int32> CVarDebugPerks(TEXT("sc.DebugPerks"), 0,
	TEXT("Shows what perks are set\n")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: Enabled\n"));

TAutoConsoleVariable<int32> CVarDebugHealth(TEXT("sc.DebugHealth"), 0,
	TEXT("Shows current health value\n")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: Enabled\n"));

TAutoConsoleVariable<int32> CVarDebugPowerPlant(TEXT("sc.DebugPowerPlant"), 0,
	TEXT("Shows power plant statistics\n")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: General Statics\n")
	TEXT(" 2: Arrows to orphaned actors/components\n")
	TEXT(" 3: Names/Locations of actors/components\n"));

TAutoConsoleVariable<int32> CVarDebugCabin(TEXT("sc.DebugCabin"), 0,
	TEXT("Shows what cabin the character is inside of\n"));

TAutoConsoleVariable<int32> CVarDebugStuck(TEXT("sc.DebugStuck"), 0,
	TEXT("Shows debug information to help track down players getting stuck\n")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: Enable\n"));

TAutoConsoleVariable<int32> CVarDebugWiggle(TEXT("sc.DebugWiggleGame"), 0,
	TEXT("Shows stun data for wiggle game\n")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: Enabled\n"));

TAutoConsoleVariable<int32> CVarInfiniteWiggle(TEXT("sc.InfiniteWiggle"), 0,
	TEXT("Makes counselors unable to wiggle free\n")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: Enabled\n"));

float ASCCharacter::DoublePressTime = 0.2f;

#if !UE_BUILD_SHIPPING
// There can only be one! (Or it doesn't work in PIE)
FSCDebugSpawners ASCCharacter::SpawnerDebugger;

TAutoConsoleVariable<int32> CVarShowSpawnedItemsInfo(TEXT("sc.ShowSpawnedItemsInfo"), 3,
	TEXT("Adjusts the verbosity of data shown when using sc.ShowSpawnedItems.\n")
	TEXT(" 0: Arrows only\n")
	TEXT(" 1: Totals and arrows\n")
	TEXT(" 2: Totals, arrows and locations\n")
	TEXT(" 3: Totals, arrows and names/classes (in world)\n")
);

void FSCDebugSpawners::RegisterCommands(UWorld* World)
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Register exec commands with the console manager for auto-completion if they havent been registered already by another net driver
	if (!ConsoleManager.IsNameRegistered(TEXT("sc.ShowSpawnedItems Reset")))
	{
		ConsoleManager.RegisterConsoleCommand(
			TEXT("sc.ShowSpawnedItems"),
			TEXT("Turns on debug info for spawned classes"),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FSCDebugSpawners::ParseCommand),
			ECVF_Default // Maybe switch this to ECVF_Cheat if it gets abused
		);

		ConsoleManager.RegisterConsoleCommand(TEXT("sc.ShowSpawnedItems Reset"), TEXT("Clears all active debug information about spawners"));

		auto AddClassToList = [this, &ConsoleManager](FName DisplayName, TSubclassOf<AActor> Class) -> void
		{
			if (Class)
			{
				ConsoleManager.RegisterConsoleCommand(*FString::Printf(TEXT("sc.ShowSpawnedItems %s"), *DisplayName.ToString()), *FString::Printf(TEXT("Displays debug stats for objects of the class %s"), *Class->GetName()));
				ClassNameMapping.Add(DisplayName, Class);
			}
		};

		// Some general test items
		AddClassToList(FName(TEXT("AllWeapons")), ASCWeapon::StaticClass());
		AddClassToList(FName(TEXT("AllRepairParts")), ASCRepairPart::StaticClass());
		AddClassToList(FName(TEXT("Counselors")), ASCCounselorCharacter::StaticClass());
		AddClassToList(FName(TEXT("Killers")), ASCKillerCharacter::StaticClass());
		AddClassToList(FName(TEXT("Doors")), ASCDoor::StaticClass());
		AddClassToList(FName(TEXT("Cabinets")), ASCCabinet::StaticClass());

		// Why is the phone such a pain to find?!
		ConsoleManager.RegisterConsoleCommand(TEXT("sc.ShowSpawnedItems PhoneAFriend"), TEXT("Calls sc.ShowSpawnedItems with PhoneBoxFuse, PhoneJunctionsBox and Phone"));
		AddClassToList(FName(TEXT("PhoneBoxFuse")), FindObject<UClass>(ANY_PACKAGE, TEXT("/Game/Blueprints/Items/Small/PhoneBoxParts/PhoneBoxFuse.PhoneBoxFuse_C")));
		AddClassToList(FName(TEXT("PhoneJunctionBox")), FindObject<UClass>(ANY_PACKAGE, TEXT("/Game/Blueprints/Props/PhoneJunctionBox_BP.PhoneJunctionBox_BP_C")));
		AddClassToList(FName(TEXT("Phone")), FindObject<UClass>(ANY_PACKAGE, TEXT("/Game/Blueprints/Props/PolicePhone_BP.PolicePhone_BP_C")));

		AddClassToList(FName(TEXT("ImposterOutfit")), FindObject<UClass>(ANY_PACKAGE, TEXT("/Game/Blueprints/Items/Special/ParanoiaOutfit.ParanoiaOutfit_C")));
		// Map specific items
		ASCWorldSettings* WorldSettings = World ? Cast<ASCWorldSettings>(World->GetWorldSettings()) : nullptr;
		if (WorldSettings)
		{
			for (FLargeItemSpawnData& ItemData : WorldSettings->LargeItemSpawnData)
			{
				if (*ItemData.ActorClass != nullptr)
				AddClassToList(FName(*(*ItemData.ActorClass)->GetName()), ItemData.ActorClass);
			}

			for (FSmallItemSpawnData& ItemData : WorldSettings->SmallItemSpawnData)
			{
				if (*ItemData.ItemClass != nullptr)
					AddClassToList(FName(*(*ItemData.ItemClass)->GetName()), ItemData.ItemClass);
			}

			if (*WorldSettings->PamelaTapeClass != nullptr)
				AddClassToList(FName(*(*WorldSettings->PamelaTapeClass)->GetName()), WorldSettings->PamelaTapeClass);

			if (*WorldSettings->TommyTapeClass != nullptr)
				AddClassToList(FName(*(*WorldSettings->TommyTapeClass)->GetName()), WorldSettings->TommyTapeClass);
		}
	}
}

void FSCDebugSpawners::UnregisterCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Don't unregister twice!
	if (!ConsoleManager.IsNameRegistered(TEXT("sc.ShowSpawnedItems Reset")))
		return;

	// Gather up all relevant console commands
	TArray<IConsoleObject*> DebugSpawnersConsoleCommands;
	ConsoleManager.ForEachConsoleObjectThatStartsWith(
		FConsoleObjectVisitor::CreateStatic<TArray<IConsoleObject*>&>(&FSCDebugSpawners::AddCommandToArray, DebugSpawnersConsoleCommands),
		TEXT("sc.ShowSpawnedItems"));

	// Unregister them all
	for (IConsoleObject*& CVar : DebugSpawnersConsoleCommands)
	{
		ConsoleManager.UnregisterConsoleObject(CVar);
	}

	// Clear out mappings
	ClassNameMapping.Empty();
	ActiveClasses.Empty();
	bHasClasses = false;
}

void FSCDebugSpawners::AddCommandToArray(const TCHAR* Name, IConsoleObject* CVar, TArray<IConsoleObject*>& Sink)
{
	Sink.Add(CVar);
}

void FSCDebugSpawners::ParseCommand(const TArray<FString>& Args)
{
	if (Args.Num() <= 0)
		return;

	// Special case for Reset
	if (Args[0] == FString(TEXT("Reset")))
	{
		ActiveClasses.Empty();
		bHasClasses = false;
		return;
	}

	TArray<FString> CustomArgs(Args);

	const int32 PhoneAFriendIndex = CustomArgs.Find(TEXT("PhoneAFriend"));
	if (PhoneAFriendIndex != INDEX_NONE)
	{
		CustomArgs.RemoveAt(PhoneAFriendIndex);
		CustomArgs.Add(TEXT("PhoneBoxFuse"));
		CustomArgs.Add(TEXT("PhoneJunctionBox"));
		CustomArgs.Add(TEXT("Phone"));
	}

	// This allows more than one item to be passed at a time and makes the following checks faster
	for (const FString& Param : CustomArgs)
	{
		// First look for the class in our map (pre-defined classes)
		if (const TSubclassOf<AActor>* MapActorClass = ClassNameMapping.Find(FName(*Param)))
		{
			// If we're already looking for it, remove it
			if (ActiveClasses.Contains(*MapActorClass))
				ActiveClasses.Remove(*MapActorClass);
			// Not looking for it, add it
			else
				ActiveClasses.Add(*MapActorClass);
		}
		else
		{
			// Class wasn't in the map, look it up
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, *Param);
			const TSubclassOf<AActor> ActorClass = Class;
			if (ActorClass)
			{
				// If we're already looking for it, remove it
				if (ActiveClasses.Contains(ActorClass))
					ActiveClasses.Remove(ActorClass);
				// Not looking for it, add it
				else
					ActiveClasses.Add(ActorClass);
			}
			else // Maybe just not actually a class
			{
				UE_LOG(LogEngine, Warning, TEXT("Could not find matching class with the name \"%s\""), *Param);
			}
		}
	}

	bHasClasses = ActiveClasses.Num() > 0;
}

void FSCDebugSpawners::Draw(UWorld* World, APawn* LocalPlayer) const
{
	// Different colors per class so we don't get confused by too many arrows
	static const uint8 NUM_COLORS = 10; // Make sure this matches the number in the array below, so we can loop if we need to
	static const FColor DEBUG_COLORS[NUM_COLORS] =
	{
		FColor::Emerald,
		FColor::Cyan,
		FColor::Red,
		FColor::White,
		FColor::Magenta,
		FColor::Orange,
		FColor::Silver,
		FColor::Purple,
		FColor::Turquoise,
		FColor::Yellow
	};

	if (bHasClasses == false || !IsValid(World))
		return;

	// DRAW!
	const FVector PlayerLocation = LocalPlayer->GetActorLocation();
	uint8 CurrColor = 0;
	for (const TSubclassOf<AActor>& Class : ActiveClasses)
	{
		if (!IsValid(Class))
			continue;

		// Loop through our colors
		const FColor DrawColor = DEBUG_COLORS[CurrColor++];
		if (CurrColor == NUM_COLORS)
			CurrColor = 0;

		// Display each actor
		int32 ActorCount = 0;
		for (TActorIterator<AActor> It(World, Class); It; ++It)
		{
			// Skip items someone else is holding
			const AActor* Actor = (*It);
			if (Actor->GetOwner() && Actor->GetOwner()->IsA(ACharacter::StaticClass()))
				continue;

			const FVector ObjectLocation = Actor->GetActorLocation();
			const FVector ToItem = ObjectLocation - PlayerLocation;
			const float Distance = ToItem.Size();

			// Debug message
			if (CVarShowSpawnedItemsInfo.GetValueOnGameThread() == 2)
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DrawColor, FString::Printf(TEXT("- %s: %s"), *Actor->GetName(), *ObjectLocation.ToString()));
			// Debug message displayed in world
			else if (CVarShowSpawnedItemsInfo.GetValueOnGameThread() >= 3)
				DrawDebugString(World, FVector(0.f, 0.f, 25.f), FString::Printf(TEXT("%s (%.2f m)"), *Actor->GetClass()->GetName(), Distance * 0.01f), (*It), DrawColor, 0.f, false);

			// Arrow (far)/sphere (near)
			if (Distance <= 150.f)
			{
				DrawDebugSphere(World, ObjectLocation, 25.f, 8, DrawColor, false, 0.f, 0, 1.f);
			}
			else
			{
				const FVector Direction = ToItem / Distance;
				const float DistanceScale = FMath::Lerp(50.f, 200.f, FMath::Clamp(Distance / 10000.f, 0.f, 1.f));
				DrawDebugDirectionalArrow(World, PlayerLocation, PlayerLocation + Direction * DistanceScale, 15.f, DrawColor, false, 0.f, 0, 1.f);
			}

			++ActorCount;
		}

		// Grand total for this class (prints at top of list)
		if (CVarShowSpawnedItemsInfo.GetValueOnGameThread() >= 1)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, DrawColor, FString::Printf(TEXT("%s: %d"), *Class->GetName(), ActorCount), false);
	}
}
#endif // !UE_BUILD_SHIPPING

ASCCharacter::ASCCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UILLInteractableManagerComponent>(TEXT("Interaction Manager"))
							.SetDefaultSubobjectClass<USCCharacterMovement>(ACharacter::CharacterMovementComponentName)
							.DoNotCreateDefaultSubobject(AILLCharacter::Mesh1PComponentName)
)
, CameraRotateRate(90.f, 90.f, 90.f)
, MinInteractHoldTime(0.2f)
, InteractSpeedModifier(1.f)
, bPauseAnimsOnDeath(true)
, Health(100.f)
, MaxHealth(100.f)
, InvulnerabilityTime(2.f)
, MaxWigglesPerSecond(10)
, MinWiggleDelta(0.01f)
, MaxVisibilityDistance(6000.f)
, MaxNearbyDistance(1500.f)
, LockOnDistance(2500.f)
, SoftLockDistance(250.f)
, SoftLockAngle(90.f)
, ThrowStrength(1000.f)
, LeftFootSocketName(FName(TEXT("leftFootSocket")))
, RightFootSocketName(FName(TEXT("rightFootSocket")))
{
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	static FName NAME_MinimapIconComponent(TEXT("USCMinimapIconComponent"));
	MinimapIconComponent = CreateDefaultSubobject<USCMinimapIconComponent>(NAME_MinimapIconComponent);
	MinimapIconComponent->SetupAttachment(RootComponent);

	static FName NAME_InteractableManagerComponent(TEXT("Interactable Manager"));
	InteractableManagerComponent = CreateDefaultSubobject<USCInteractableManagerComponent>(NAME_InteractableManagerComponent);
	InteractableManagerComponent->bUseCameraRotation = false;
	InteractableManagerComponent->bUseEyeLocation = true;

	NoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("NoiseEmitter"));
	NoiseEmitter->NoiseLifetime = .8f; // Twice ASCCharacterAIController::PawnSensingComponent->SensingInterval

	GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("DoubleClickTime"), DoublePressTime, GInputIni);

	bReplicates = true;
	bReplicateMovement = true;
	MinNetUpdateFrequency = 10.f;
	NetUpdateFrequency = 30.f; // pjackson: Had to halve this from default due to being bAlwaysRelevant, saturating the network

	static FName NAME_VoiceAudioComponent(TEXT("VoIPAudioComponent"));
	VoiceAudioComponent = CreateDefaultSubobject<UAudioComponent>(NAME_VoiceAudioComponent);
	VoiceAudioComponent->SetupAttachment(RootComponent);
	VoiceAudioComponent->bIsUISound = false;
	VoiceAudioComponent->bAllowSpatialization = true;
	VoiceAudioComponent->SetVolumeMultiplier(1.5f);

	GetMesh()->bUseRefPoseOnInitAnim = true;

	LookAtTransmitDelay = 1.f/5.f; // Send look-at updates at 5FPS
	VisibilityUpdateDelay = 1.f/3.f; // Update at 3FPS
	bIgnoreDeathRagdoll = false;

	bInStun = false;
	InvulnerableStunTimer = 0.0f;
	InvulnerableStunDuration = 3.0f;

	Health.SetMaxValue(MaxHealth);
}

void ASCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCCharacter, Health);
	DOREPLIFETIME(ASCCharacter, CurrentStance); // FIXME: pjackson: Some places do not simulate this properly, like grabbing
	DOREPLIFETIME(ASCCharacter, LockOntoCharacter);
	DOREPLIFETIME(ASCCharacter, BlockData);
	DOREPLIFETIME_CONDITION(ASCCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASCCharacter, bIsRunning, COND_SkipOwner);
	DOREPLIFETIME(ASCCharacter, PickingItem);
	DOREPLIFETIME(ASCCharacter, EquippedItem);
	DOREPLIFETIME(ASCCharacter, bIsInside);
	DOREPLIFETIME_CONDITION(ASCCharacter, HeadLookAtDirection, COND_SkipOwner);
	DOREPLIFETIME(ASCCharacter, bIsStruggling);
	DOREPLIFETIME(ASCCharacter, CurrentThrowable);
	DOREPLIFETIME(ASCCharacter, TriggeredTrap);
	DOREPLIFETIME(ASCCharacter, bInStun);
	DOREPLIFETIME(ASCCharacter, bDisabled);
}

void ASCCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Characters should be ignoring the AnimCameraBlocker trace channel.
	GetMesh()->SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);

	if (IsLocallyControlled())
	{
		CLIENT_SetSkeletonToGameplayMode(true);
	}

	InteractableManagerComponent->OnClientLockedInteractable.AddDynamic(this, &ASCCharacter::OnClientLockedInteractable);

	SpawnPostProcessVolume();

	// Start all cameras off, off
	TArray<UActorComponent*> DynamicCameras = GetComponentsByClass(UILLDynamicCameraComponent::StaticClass());
	for (UActorComponent* Camera : DynamicCameras)
	{
		bool bIsActive = false;
		for (const FILLCameraBlendInfo& StackCamera : ActiveCameraStack)
		{
			if (StackCamera.DynamicCamera == Camera)
			{
				bIsActive = true;
				break;
			}
		}

		if (!bIsActive)
			Camera->Deactivate();
	}

	// attempting to make sure the camera always always becomes active on the character
	ReturnCameraToCharacter(0.0f, VTBlend_Linear, 2.0f, true, false);

	// Make it rain! (or sound like it)
	UWorld* World = GetWorld();
	if (ASCWorldSettings* WorldSettings = World ? Cast<ASCWorldSettings>(World->GetWorldSettings()) : nullptr)
	{
		SetRainEnabled(WorldSettings->GetIsRaining());
		WorldSettings->OnRainingStateChanged.AddUniqueDynamic(this, &ThisClass::SetRainEnabled);
	}

#if !UE_BUILD_SHIPPING
	SpawnerDebugger.RegisterCommands(GetWorld());
#endif

	// Hook our Montage Ended delegate for catching interrupted animations
	if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
		AnimInstance->OnMontageEnded.AddDynamic(this, &ThisClass::OnMontageEnded);
}

void ASCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

#if !UE_BUILD_SHIPPING
	if (IsLocallyControlled() || EndPlayReason != EEndPlayReason::Destroyed)
		SpawnerDebugger.UnregisterCommands();
#endif

	// Clean up any leftover skeleton changes. -- Duplicate of Destroyed to try and prevent bones from switching back secretly again
	CLIENT_SetSkeletonToGameplayMode(true);

	if (GetWorld())
	{
		FTimerManager& TimerManager = GetWorldTimerManager();
		TimerManager.ClearTimer(TimerHandle_Stun);
		TimerManager.ClearTimer(TimerHandle_StunRecovery);
		TimerManager.ClearTimer(TimerHandle_Invulnerability);

		// Clear Sound Mixes as well
		UGameplayStatics::ClearSoundMixModifiers(GetWorld());
	}
}

void ASCCharacter::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	check(InInputComponent);
	InInputComponent->BindAxis(TEXT("GLB_Forward_PC"), this, &ASCCharacter::OnForwardPC);
	InInputComponent->BindAxis(TEXT("GLB_Forward_CTRL"), this, &ASCCharacter::OnForwardController);

	InInputComponent->BindAxis(TEXT("GLB_Right_PC"), this, &ASCCharacter::OnRightPC);
	InInputComponent->BindAxis(TEXT("GLB_Right_CTRL"), this, &ASCCharacter::OnRightController);

	InInputComponent->BindAxis(TEXT("LookUp"), this, &ASCCharacter::AddControllerPitchInput);
	InInputComponent->BindAxis(TEXT("Turn"), this, &ASCCharacter::AddControllerYawInput);

	InInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Pressed, this, &ASCCharacter::OnInteract1Pressed);
	InInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Released, this, &ASCCharacter::OnInteract1Released);

	InInputComponent->BindAction(TEXT("GLB_Combat_PC"), IE_Pressed, this, &ASCCharacter::ToggleCombatStance);
	InInputComponent->BindAction(TEXT("GLB_Combat_CTRL"), IE_Pressed, this, &ASCCharacter::ToggleCombatStance);

#if WITH_EDITOR
	InInputComponent->BindAction(TEXT("DEBUG_SkipCutscene"), IE_Pressed, this, &ASCCharacter::OnSkipCutscene);
#endif


	if (Controller)
	{
		if (!CombatStanceInputComponent)
		{
			CombatStanceInputComponent = NewObject<UInputComponent>(Controller, TEXT("CombatStanceInput"));
			CombatStanceInputComponent->bBlockInput = true;
		}

		CombatStanceInputComponent->ClearBindingValues();
		CombatStanceInputComponent->ClearActionBindings();
		CombatStanceInputComponent->BindAxis(TEXT("GLB_Forward_PC"), this, &ASCCharacter::OnForwardPC);
		CombatStanceInputComponent->BindAxis(TEXT("GLB_Forward_CTRL"), this, &ASCCharacter::OnForwardController);

		CombatStanceInputComponent->BindAxis(TEXT("GLB_Right_PC"), this, &ASCCharacter::OnRightPC);
		CombatStanceInputComponent->BindAxis(TEXT("GLB_Right_CTRL"), this, &ASCCharacter::OnRightController);

		CombatStanceInputComponent->BindAxis(TEXT("LookUp"), this, &ASCCharacter::AddControllerPitchInput);
		CombatStanceInputComponent->BindAxis(TEXT("Turn"), this, &ASCCharacter::AddControllerYawInput);

		CombatStanceInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Pressed, this, &ASCCharacter::OnInteract1Pressed);
		CombatStanceInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Released, this, &ASCCharacter::OnInteract1Released);

		CombatStanceInputComponent->BindAction(TEXT("GLB_Combat_PC"), IE_Pressed, this, &ASCCharacter::ToggleCombatStance);
		CombatStanceInputComponent->BindAction(TEXT("GLB_Combat_CTRL"), IE_Pressed, this, &ASCCharacter::ToggleCombatStance);

		if (!InteractMinigameInputComponent)
		{
			InteractMinigameInputComponent = NewObject<UInputComponent>(Controller, TEXT("InteractMinigameInput"));
			InteractMinigameInputComponent->bBlockInput = true;
		}

		InteractMinigameInputComponent->ClearBindingValues();
		InteractMinigameInputComponent->ClearActionBindings();
		InteractMinigameInputComponent->BindAction(TEXT("MG_InteractSkill1"), IE_Pressed, this, &ASCCharacter::OnInteractSkill1Pressed);
		InteractMinigameInputComponent->BindAction(TEXT("MG_InteractSkill2"), IE_Pressed, this, &ASCCharacter::OnInteractSkill2Pressed);
		InteractMinigameInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Released, this, &ASCCharacter::OnInteract1Released);

		if (!WiggleInputComponent)
		{
			WiggleInputComponent = NewObject<UInputComponent>(Controller, TEXT("WiggleInput"));
			WiggleInputComponent->bBlockInput = true;
		}

		WiggleInputComponent->ClearBindingValues();
		WiggleInputComponent->ClearActionBindings();
		WiggleInputComponent->BindAxis(TEXT("GLB_Forward_PC"), this, &ASCCharacter::OnForwardPC);
		WiggleInputComponent->BindAxis(TEXT("GLB_Forward_CTRL"), this, &ASCCharacter::OnForwardController);

		WiggleInputComponent->BindAxis(TEXT("GLB_Right_PC"), this, &ASCCharacter::OnRightPC);
		WiggleInputComponent->BindAxis(TEXT("GLB_Right_CTRL"), this, &ASCCharacter::OnRightController);

		WiggleInputComponent->BindAxis(TEXT("LookUp"), this, &ASCCharacter::AddControllerPitchInput);
		WiggleInputComponent->BindAxis(TEXT("Turn"), this, &ASCCharacter::AddControllerYawInput);
	}
}

void ASCCharacter::ModifyInputSensitivity(FKey InputKey, float SensitivityMod)
{
	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		PlayerController->ModifyInputSensitivity(InputKey, SensitivityMod);
	}
}

void ASCCharacter::RestoreInputSensitivity(FKey InputKey)
{
	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		PlayerController->RestoreInputSensitivity(InputKey);
	}
}

bool ASCCharacter::IsInteractGameActive() const
{
	return GetInteractable() ? GetInteractable()->Pips.Num() > 0 && bHoldInteractStarted : false; 
}

void ASCCharacter::ToggleInput(bool Enable)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (Enable)
		{
			EnableInput(PC);
			return;
		}

		DisableInput(PC);
	}
}

void ASCCharacter::OnInteract1Pressed()
{
	InteractPressed = true;
	InteractTimer = 0.f;
}

void ASCCharacter::OnInteract1Released()
{
	if (!InteractPressed)
		return;

	const int32 InteractMethodFlags = InteractableManagerComponent->GetBestInteractableMethodFlags();

	// See if the press/release qualifies for a Press interaction
	if (USCInteractComponent* Interactable = GetInteractable())
	{
		if (CVarILLDebugInteractables.GetValueOnGameThread() >= 1)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::White, FString::Printf(TEXT("Trying to press on %s:%s, T: %.1f / %.1f"), *Interactable->GetOwner()->GetName(), *Interactable->GetName(), InteractTimer, Interactable->GetHoldTimeLimit(this) > 0.f ? MinInteractHoldTime : 0.f));
		}

		bool bRelease = true;
		if (InteractMethodFlags & (int32)EILLInteractMethod::Hold)
		{
			bRelease = InteractTimer <= (Interactable->GetHoldTimeLimit(this) + MinInteractHoldTime);

			if (bRelease && InteractTimerPassedMinHoldTime())
			{
				OnHoldInteract(Interactable, EILLHoldInteractionState::Canceled);
			}
		}

		if (bRelease && !InteractTimerPassedMinHoldTime())
		{
			if (InteractMethodFlags & (int32)EILLInteractMethod::Press)
			{
				OnInteract(Interactable, EILLInteractMethod::Press);
			}
			else if (InteractMethodFlags & (int32)EILLInteractMethod::Hold)
			{
				OnHoldInteract(Interactable, EILLHoldInteractionState::Canceled);
			}
		}
	}

	// Make sure we don't have the interact minigame stuck on screen
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
	{
		if (PC->IsLocalController())
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
			{
				Hud->StopInteractMinigame();
			}

			PC->PopInputComponent(InteractMinigameInputComponent);
		}
	}

	// Some special moves can go fast! We'll only get here if we're already in an interaction
	if (USCSpecialMove_SoloMontage* SpecialMove = Cast<USCSpecialMove_SoloMontage>(ActiveSpecialMove))
	{
		if (SpecialMove->TrySpeedUp())
		{
			if (Role < ROLE_Authority)
				SERVER_SpeedUpSpecialMove();
			else
				MULTICAST_SpeedUpSpecialMove();
		}
	}

	InteractSpeedModifier = 1.f;
	InteractPressed = false;
	InteractTimer = 0.f;
	bHoldInteractStarted = false;
}

void ASCCharacter::HideHUD(bool bHideHUD)
{
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
	{
		if (PC->IsLocalController())
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
			{
				Hud->HideHUD(bHideHUD);
			}
		}
	}
}

bool ASCCharacter::SERVER_SpeedUpSpecialMove_Validate()
{
	return true;
}

void ASCCharacter::SERVER_SpeedUpSpecialMove_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	MULTICAST_SpeedUpSpecialMove();
}

void ASCCharacter::MULTICAST_SpeedUpSpecialMove_Implementation()
{
	if (USCSpecialMove_SoloMontage* SpecialMove = Cast<USCSpecialMove_SoloMontage>(ActiveSpecialMove))
	{
		SpecialMove->TrySpeedUp(true);
	}
}

void ASCCharacter::OnInteractSkill1Pressed()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* SCHud = PC->GetSCHUD())
		{
			SCHud->OnInteractSkill(0);
		}
	}
}

void ASCCharacter::OnInteractSkill2Pressed()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* SCHud = PC->GetSCHUD())
		{
			SCHud->OnInteractSkill(1);
		}
	}
}

void ASCCharacter::OnForwardPC(float Val)
{
	// Don't move if the special move doesn't allow it
	if (ActiveSpecialMove && !ActiveSpecialMove->CanMove())
		return;

	if (Controller && Val)
	{
		FRotator Rotation = Controller->GetControlRotation();
		FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetScaledAxis(EAxis::X);
		AddMovementInput(Direction, Val);
	}
}

void ASCCharacter::OnForwardController(float Val)
{
	OnForwardPC(Val);

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				if (FMath::Abs(Val) <= KINDA_SMALL_NUMBER && bIsSprinting)
					SetSprinting(false);
			}
		}
	}

}

void ASCCharacter::OnRightPC(float Val)
{
	// Don't move if the special move doesn't allow it
	if (ActiveSpecialMove && !ActiveSpecialMove->CanMove())
		return;

	if (Controller && Val)
	{
		FRotator Rotation = Controller->GetControlRotation();
		FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetScaledAxis(EAxis::Y);
		AddMovementInput(Direction, Val);
	}
}

void ASCCharacter::OnRightController(float Val)
{
	OnRightPC(Val);
}

void ASCCharacter::AddControllerYawInput(float Val)
{
	const float DeltaSeconds = [this]()
	{
		if (UWorld* World = GetWorld())
			return World->GetDeltaSeconds();
		return 0.f;
	}();

	if (CanUseHardLock() && LockOntoCharacter && Controller)
	{
		const FRotator Direction = (LockOntoCharacter->GetActorLocation() - GetActorLocation()).Rotation();
		FRotator ControlRotation = Controller->GetControlRotation();

		if (!IsDodging())
		{
			FRotator NewRot = ControlRotation;
			NewRot.Yaw = Direction.Yaw;
			ControlRotation = FMath::RInterpTo(ControlRotation, NewRot, DeltaSeconds, 4.0f);
		}
		else
		{
			ControlRotation.Yaw = Direction.Yaw;
		}

		Controller->SetControlRotation(ControlRotation);
		return;
	}

	if (IsUsingGamepad())
	{
		// Not sure why, but rate seems to be doubled, so we half it and scale it by time
		const float YawRotation = Val * (CameraRotateRate.Y * 0.5f * DeltaSeconds);
		Super::AddControllerYawInput(YawRotation);
	}
	else
	{
		// Don't smooth mouse input
		Super::AddControllerYawInput(Val);
	}
}

void ASCCharacter::AddControllerPitchInput(float Val)
{
	if (IsUsingGamepad())
	{
		const float DeltaSeconds = [this]()
		{
			if (UWorld* World = GetWorld())
				return World->GetDeltaSeconds();
			return 0.f;
		}();

		// Not sure why, but rate seems to be doubled, so we half it and scale it by time
		const float PitchRotation = Val * (CameraRotateRate.X * 0.5f * DeltaSeconds);
		Super::AddControllerPitchInput(PitchRotation);
	}
	else
	{
		// Don't smooth mouse input
		Super::AddControllerPitchInput(Val);
	}
}

void ASCCharacter::OnCombatPressed()
{
	SetCombatStance(true);
}

void ASCCharacter::OnCombatReleased()
{
	SetCombatStance(false);
}

void ASCCharacter::OnAttack()
{
	if (!CanAttack())
		return;

	if (BlockData.bIsBlocking)
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_Attack();
	}
	else
	{
		AttackTraceBegin();
		MULTICAST_Attack();
	}

	if (CurrentStance == ESCCharacterStance::Combat)
	{
		bAttackHeld = true;
		AttackHoldTimer = 0.0f;
	}
}

void ASCCharacter::OnHeavyAttack()
{
	if (Role < ROLE_Authority)
	{
		SERVER_Attack(true);
	}
	else
	{
		HitActors.Empty();
		MULTICAST_Attack(true);
	}
}

void ASCCharacter::OnStopAttack()
{
	bAttackHeld = false;
	AttackHoldTimer = 0.0f;
}

bool ASCCharacter::SERVER_Attack_Validate(bool HeavyAttack /*= false*/)
{
	return true;
}

void ASCCharacter::SERVER_Attack_Implementation(bool HeavyAttack /*= false*/)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	if (HeavyAttack)
		OnHeavyAttack();
	else
		OnAttack();

	// Notify nearby AI
	MakeStealthNoise(1.f, this);
}

void ASCCharacter::MULTICAST_Attack_Implementation(bool HeavyAttack /*= false*/)
{
	if (USkeletalMeshComponent* PawnMesh = GetPawnMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(PawnMesh->GetAnimInstance()))
		{
			AnimInstance->PlayAttackAnim(HeavyAttack);
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0 && Role == ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Blue, FString::Printf(TEXT("%s Attempted an Attack"), *GetClass()->GetName()));
	}

	bHeavyAttacking = HeavyAttack;
}

void ASCCharacter::PlayRecoil(const FHitResult& Hit)
{
	MULTICAST_PlayRecoil(Hit);
}


void ASCCharacter::MULTICAST_PlayRecoil_Implementation(const FHitResult& Hit)
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			AnimInstance->PlayRecoilAnim();
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0 && Role == ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Blue, FString::Printf(TEXT("%s recoiled"), *GetClass()->GetName()));
		if (ASCWeapon* Weapon = GetCurrentWeapon())
		{
			DrawDebugCapsule(GetWorld(), Weapon->KillVolume->GetComponentLocation(), Weapon->KillVolume->GetCollisionShape().GetCapsuleHalfHeight(), Weapon->KillVolume->GetCollisionShape().GetCapsuleRadius(), Weapon->KillVolume->GetComponentQuat(), FColor::Green, false, 5.f);
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 3.f, 9, FColor::Green, false, 5.f);
		}
	}
}

void ASCCharacter::PlayWeaponImpact(const FHitResult& Hit)
{
	MULTICAST_PlayWeaponImpact(Hit);
}

void ASCCharacter::MULTICAST_PlayWeaponImpact_Implementation(const FHitResult& Hit)
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			AnimInstance->PlayWeaponImpactAnim();
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0 && Role == ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Blue, FString::Printf(TEXT("%s\'s weapon is stuck"), *GetClass()->GetName()));
		if (ASCWeapon* Weapon = GetCurrentWeapon())
		{
			DrawDebugCapsule(GetWorld(), Weapon->KillVolume->GetComponentTransform().GetLocation(), Weapon->KillVolume->GetCollisionShape().GetCapsuleHalfHeight(), Weapon->KillVolume->GetCollisionShape().GetCapsuleRadius(), Weapon->KillVolume->GetComponentQuat(), FColor::Green, false, 5.f);
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 3.f, 9, FColor::Green, false, 5.f);
		}
	}
}

USCInteractComponent* ASCCharacter::GetInteractable() const
{
	return Cast<USCInteractComponent>(InteractableManagerComponent->GetBestInteractableComponent());
}

void ASCCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetCapsuleComponent()->OnComponentBeginOverlap.AddUniqueDynamic(this, &ASCCharacter::OnOverlapBegin);
	GetCapsuleComponent()->OnComponentEndOverlap.AddUniqueDynamic(this, &ASCCharacter::OnOverlapEnd);

	SetThirdPerson(true);

	if (Role == ROLE_Authority)
	{
		Health = GetMaxHealth();
	}

	if (MusicComponent)
	{
		MusicComponent->InitMusicComponent(MusicObjectClass);
	}
}

void ASCCharacter::OnInteract(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod)
{
	check(IsLocallyControlled());
	
	// Due to high lag, the server may already be attempting to lock an interaction for us
	if (bAttemptingInteract)
		return;

	// Set a flag indicating that we're attempting to interact with something
	bAttemptingInteract = true;

	// Notify the AI
	if (ASCCharacterAIController* AIC = Cast<ASCCharacterAIController>(Controller))
	{
		AIC->PawnInteractStarted(CastChecked<USCInteractComponent>(Interactable));
	}

	// Call to the server to enforce server-set limitations
	SERVER_OnInteract(Interactable, InteractMethod);
}

bool ASCCharacter::SERVER_OnInteract_Validate(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod)
{
	return true;
}

void ASCCharacter::SERVER_OnInteract_Implementation(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	// We're picking up an item, don't allow other interactions
	if (PickingItem)
	{
		CLIENT_CancelInteractAttempt();
		return;
	}

	if (Interactable)
	{
		InteractableManagerComponent->AttemptInteract(Interactable, InteractMethod, /*bAutoLock=*/true);
	}
}

void ASCCharacter::OnInteractAccepted(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod)
{
	CLIENT_CancelInteractAttempt();

	if (USCInteractComponent* SCInteractable = Cast<USCInteractComponent>(Interactable))
	{
		switch (InteractMethod)
		{
		case EILLInteractMethod::Press:
			PlayInteractAnim(SCInteractable);
			break;

		case EILLInteractMethod::Hold:
			OnHoldInteract(SCInteractable, EILLHoldInteractionState::Success);
			break;
		}
	}

	// Notify the AI
	if (ASCCharacterAIController* AIC = Cast<ASCCharacterAIController>(Controller))
	{
		AIC->PawnInteractAccepted(CastChecked<USCInteractComponent>(Interactable));
	}
}

bool ASCCharacter::CanInteractAtAll() const
{
	// Don't call super

	// Don't fail if we're really close to the end of a special move (bridges one frame gaps)
	if (ActiveSpecialMove)
	{
		const float TimeRemaining = ActiveSpecialMove->GetTimeRemaining();
		if (TimeRemaining == -1.f) // -1 means the special move is being manually controlled, can't test for it being at the end of the animation
			return false;

		if (TimeRemaining > 0.05f) // Magic number based on testing with 250ms lag and then buffered a little bit
			return false;
	}

	// Maybe we just can't do things at all
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return false;
	}

	// No interacting if blocking
	if (BlockData.bIsBlocking)
		return false;

	// You're stunned, stop it
	if (IsStunned())
		return false;

	if (InCombatStance())
		return false;

	// Nah, it's cool
	return true;
}

void ASCCharacter::SetVehicleCollision(bool bEnable)
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Vehicle, bEnable ? ECR_Block : ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_Boat, bEnable ? ECR_Block : ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, bEnable ? ECR_Block : ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_Killer, bEnable ? ECR_Block : ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECC_Vehicle, bEnable ? ECR_Block : ECR_Ignore);
	}
}

void ASCCharacter::CleanupLocalAfterDeath(ASCPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->PopInputComponent(InteractMinigameInputComponent);
		PlayerController->PopInputComponent(WiggleInputComponent);
		PlayerController->PopInputComponent(CombatStanceInputComponent);
	}

	InteractableManagerComponent->CleanupInteractions();

	// Only play camera shakes for local controllers, this should be simulated
	AILLPlayerController* PC = Cast<AILLPlayerController>(PlayerController);
	if (PC && PC->IsLocalController() && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StopAllInstancesOfCameraShake(UCameraShake::StaticClass());
	}

	// Handle this character being a view target for a local player too
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if ((*It)->IsLocalController() && (*It)->GetViewTarget() == this)
		{
			(*It)->PlayerCameraManager->StopAllInstancesOfCameraShake(UCameraShake::StaticClass());
		}
	}
}

void ASCCharacter::OnHoldInteract(USCInteractComponent* Interactable, EILLHoldInteractionState NewState)
{
	if (NewState != EILLHoldInteractionState::Interacting && IsLocallyControlled() && Interactable)
	{
		Interactable->TryAutoReturnCamera(this);

		// Called when the hold interact ends, auto-unlock if hold state changed isn't bound and we don't have a valid OnPress to call
		if (!Interactable->OnHoldStateChanged.IsBound() &&
			(!(Interactable->InteractMethods & (int32)EILLInteractMethod::Press) && !InteractTimerPassedMinHoldTime()))
		{
			InteractableManagerComponent->UnlockInteraction(Interactable);
		}
	}

	if (Role < ROLE_Authority)
	{
		SERVER_OnHoldInteract(Interactable, NewState);
	}
	else
	{
		if (Interactable)
		{
			Interactable->OnHoldStateChangedBroadcast(this, NewState);
		}
	}
}

bool ASCCharacter::SERVER_OnHoldInteract_Validate(USCInteractComponent* Interactable, EILLHoldInteractionState NewState)
{
	return true;
}

void ASCCharacter::SERVER_OnHoldInteract_Implementation(USCInteractComponent* Interactable, EILLHoldInteractionState NewState)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	OnHoldInteract(Interactable, NewState);
}

void ASCCharacter::CLIENT_CancelInteractAttempt_Implementation()
{
	bAttemptingInteract = false;
}

void ASCCharacter::CLIENT_CancelInteractAttemptFlag_Implementation() {
	bAttemptingInteract = false;
}

void ASCCharacter::OnClientLockedInteractable(UILLInteractableComponent* LockingInteractable, const EILLInteractMethod InteractMethod, const bool bSuccess)
{
	bAttemptingInteract = false;

	// We didn't get the lock, make sure interactions clean up correctly
	if (!bSuccess && InteractMethod == EILLInteractMethod::Hold)
	{
		const UILLInteractableComponent* LockedComponent = GetInteractableManagerComponent()->GetLockedInteractable();

		if (CVarDebugStuck.GetValueOnGameThread() >= 1)
		{
			auto GetInteractNameSafe = [](const UILLInteractableComponent* Component) -> FString
			{
				if (Component)
					return FString::Printf(TEXT("%s:%s"), *GetNameSafe(Component->GetOwner()), *Component->GetName());
				return TEXT("NONE");
			};
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s could not be locked, %s was locked locally"), *GetInteractNameSafe(LockingInteractable), *GetInteractNameSafe(LockedComponent)), false);
		}

		if (!LockedComponent || LockedComponent == LockingInteractable)
		{
			ClearAnyInteractions();
		}
	}
}

void ASCCharacter::ClearAnyInteractions()
{
	// No getting out of kills
	if (bInContextKill)
		return;

	if (CVarDebugStuck.GetValueOnGameThread() >= 1)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Orange, FString::Printf(TEXT("Clear Any Interactions Called")), false);
	}

	bAttemptingInteract = false;

	// Clear hold interactions
	if (InteractTimer > 0.f)
	{
		OnInteract1Released();
	}

	// Clear any item we were picking up
	if (PickingItem)
	{
		PickingItem->bIsPicking = false;
		PickingItem = nullptr;
	}

	// Clear our ACTUAL interaction
	if (UILLInteractableComponent* LockedInteraction = InteractableManagerComponent->GetLockedInteractable())
		InteractableManagerComponent->UnlockInteraction(LockedInteraction);

	// Clear the active special move
	if (USCSpecialMove_SoloMontage* Move = Cast<USCSpecialMove_SoloMontage>(ActiveSpecialMove))
		Move->CancelSpecialMove();

	// Stop any animation with root motion (some special moves (repair) are DUMB)
	if (USCAnimInstance* SCAnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		// Don't abort stun animations!
		const bool bStunActive = SCAnimInstance->StunMontage && SCAnimInstance->Montage_IsActive(SCAnimInstance->StunMontage);
		if (!bStunActive && SCAnimInstance->bIsMontageUsingRootMotion && !bPlayedDeathAnimation)
			SCAnimInstance->Montage_Stop(0.5f);
	}

	// Stop attack tracing
	bPerformAttackTrace = false;

	// Take our camera back
	ReturnCameraToCharacter(0.25f, VTBlend_EaseInOut, 2.0f, true, false);
}

void ASCCharacter::PlayInteractAnim(USCInteractComponent* Interactable)
{
	if (Role < ROLE_Authority)
	{
		SERVER_PlayInteractAnim(Interactable);
	}
	else
	{
		MULTICAST_PlayInteractAnim(Interactable);
	}
}

bool ASCCharacter::SERVER_PlayInteractAnim_Validate(USCInteractComponent* Interactable)
{
	return true;
}

void ASCCharacter::SERVER_PlayInteractAnim_Implementation(USCInteractComponent* Interactable)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	PlayInteractAnim(Interactable);
}

void ASCCharacter::MULTICAST_PlayInteractAnim_Implementation(USCInteractComponent* Interactable)
{
	if (Interactable)
	{
		UAnimMontage* InteractAnim = nullptr;
		if (IsA<ASCCounselorCharacter>())
		{
			InteractAnim = Interactable->CounselorInteractAnim;
		}
		else if (IsA<ASCKillerCharacter>())
		{
			InteractAnim = Interactable->KillerInteractAnim;
		}

		if (InteractAnim)
		{
			PlayAnimMontage(InteractAnim);
		}
	}
}

void ASCCharacter::FailureInteractAnim()
{
	if (Role < ROLE_Authority)
	{
		SERVER_FailureInteractAnim();
	}
	else
	{
		MULTICAST_FailureInteractAnim();
	}
}

bool ASCCharacter::SERVER_FailureInteractAnim_Validate()
{
	return true;
}

void ASCCharacter::SERVER_FailureInteractAnim_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	FailureInteractAnim();
}

void ASCCharacter::MULTICAST_FailureInteractAnim_Implementation()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
		return;

	UAnimInstance* AnimInst = CharacterMesh->GetAnimInstance();
	if (!AnimInst)
		return;

	UAnimMontage* InteractAnim = AnimInst->GetCurrentActiveMontage();
	if (!InteractAnim)
		return;

	static const FName FailureSection(TEXT("Failure"));

	// Make sure we have a failure section
	if (InteractAnim->GetSectionIndex(FailureSection) != INDEX_NONE)
	{
		if (AnimInst->Montage_GetCurrentSection(InteractAnim) != FailureSection)
			AnimInst->Montage_JumpToSection(FailureSection, InteractAnim); // Montage will handle getting back to Idle
	}
}

void ASCCharacter::CancelInteractAnim()
{
	if (Role < ROLE_Authority)
	{
		SERVER_CancelInteractAnim();
	}
	else
	{
		MULTICAST_CancelInteractAnim();
	}
}

bool ASCCharacter::SERVER_CancelInteractAnim_Validate()
{
	return true;
}

void ASCCharacter::SERVER_CancelInteractAnim_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	CancelInteractAnim();
}

void ASCCharacter::MULTICAST_CancelInteractAnim_Implementation()
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
		{
			if (UAnimMontage* InteractAnim = AnimInstance->GetCurrentActiveMontage())
			{
				static const FName EnterSection(TEXT("Enter"));
				static const FName ExitSection(TEXT("Exit"));
				static const FName CancelSection(TEXT("Cancel"));

				const bool bHasCancelSection = InteractAnim->GetSectionIndex(CancelSection) != INDEX_NONE;
				const FName CurrentSection = AnimInstance->Montage_GetCurrentSection(InteractAnim);
				if (CurrentSection == EnterSection)
				{
					AnimInstance->Montage_SetNextSection(CurrentSection, bHasCancelSection ? CancelSection : ExitSection, InteractAnim);
				}
				else // Could be in Idle or Failure section so we can just jump straight to the cancel
				{
					AnimInstance->Montage_JumpToSection(bHasCancelSection ? CancelSection : ExitSection, InteractAnim);
				}
			}
		}
	}
}

void ASCCharacter::FinishInteractAnim()
{
	if (Role < ROLE_Authority)
	{
		SERVER_FinishInteractAnim();
	}
	else
	{
		MULTICAST_FinishInteractAnim();
	}
}

bool ASCCharacter::SERVER_FinishInteractAnim_Validate()
{
	return true;
}

void ASCCharacter::SERVER_FinishInteractAnim_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	FinishInteractAnim();
}

void ASCCharacter::MULTICAST_FinishInteractAnim_Implementation()
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
		{
			if (UAnimMontage* InteractAnim = AnimInstance->GetCurrentActiveMontage())
			{
				static const FName ExitSection(TEXT("Exit"));
				static const FName SuccessSection(TEXT("Success"));

				const bool bHasSuccessSection = InteractAnim->GetSectionIndex(SuccessSection) != INDEX_NONE;
				AnimInstance->Montage_JumpToSection(bHasSuccessSection ? SuccessSection : ExitSection, InteractAnim);
			}
		}
	}
}

void ASCCharacter::ForceStopInteractAnim()
{
	if (Role < ROLE_Authority)
	{
		SERVER_ForceStopInteractAnim();
	}
	else
	{
		MULTICAST_ForceStopInteractAnim();
	}
}

void ASCCharacter::CLIENT_CancelRepairInteract_Implementation()
{
	ClearAnyInteractions();
	OnInteract1Released();
}

bool ASCCharacter::SERVER_ForceStopInteractAnim_Validate()
{
	return true;
}

void ASCCharacter::SERVER_ForceStopInteractAnim_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	ForceStopInteractAnim();
}

void ASCCharacter::MULTICAST_ForceStopInteractAnim_Implementation()
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
		{
			UAnimMontage* InteractAnim = AnimInstance->GetCurrentActiveMontage();
			StopAnimMontage(InteractAnim);
		}
	}
}

void ASCCharacter::Native_NotifyMinigameFail(USoundCue* FailSound)
{
	if (Role < ROLE_Authority)
	{
		SERVER_NotifyMinigameFail(FailSound);
	}
	else
	{
		CLIENT_NotifyMinigameFail(FailSound);
	}
}

bool ASCCharacter::SERVER_NotifyMinigameFail_Validate(USoundCue* FailSound)
{
	return true;
}

void ASCCharacter::SERVER_NotifyMinigameFail_Implementation(USoundCue* FailSound)
{
	Native_NotifyMinigameFail(FailSound);
}

void ASCCharacter::CLIENT_NotifyMinigameFail_Implementation(USoundCue* FailSound)
{
	UGameplayStatics::PlaySound2D(GetWorld(), FailSound);
}

void ASCCharacter::Destroyed()
{
	if (ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		State->PlayerCharacterList.Remove(this);
	}

	// Clean up any leftover skeleton changes.

	Super::Destroyed();
}

void ASCCharacter::UpdateCharacterVisibility()
{
	const float MaxDistSQ = MaxVisibilityDistance * MaxVisibilityDistance;
	float ClosestDistance = MaxDistSQ;
	ASCCharacter* ClosestPlayer = nullptr;
	bool bClosestPlayerIsFriendly = true;

	ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState());
	if (!GameState)
		return;
		
	bool Nearby = false;
	bool FriendlyNearby = false;
	const float NearbyDistSQ = MaxNearbyDistance * MaxNearbyDistance;

	for (ASCCharacter* Player : GameState->PlayerCharacterList)
	{
		if (Player == this)
			continue;

		if (!PlayerVisibilities.Contains(Player))
			PlayerVisibilities.Add(Player, false);

		const float Distance = (GetActorLocation() - Player->GetActorLocation()).SizeSquared();

		if (Distance > MaxDistSQ || Player->IsCharacterDisabled())
		{
			if (PlayerVisibilities[Player])
				SERVER_OnCharacterOutOfView(Player);

			PlayerVisibilities[Player] = false;
			continue;
		}

		const bool bIsFriendly = IsOnSameTeamAs(Player);
		if (Distance < NearbyDistSQ)
		{
			Nearby = true;
			FriendlyNearby |= bIsFriendly;
		}

		float FOV = 90.f;
		if (!Cast<AAIController>(Controller))
			FOV = GetCachedCameraInfo().FOV;
		if (LineOfSight(Player, FOV * 0.5f, false, true))
		{
			if (!PlayerVisibilities[Player])
			{
				SERVER_OnCharacterInView(Player);
				PlayerVisibilities[Player] = true;
			}

			if (Player->IsAlive())
			{
				// Let's lock on to whoever, but prioritize those who are on a different team than us
				const bool bIsCloser = Distance < ClosestDistance;
				if ((bIsFriendly == bClosestPlayerIsFriendly && bIsCloser) || (!bIsFriendly && bClosestPlayerIsFriendly))
				{
					ClosestDistance = Distance;
					ClosestPlayer = Player;
					bClosestPlayerIsFriendly = bIsFriendly;
				}
			}
		}
		else if (PlayerVisibilities[Player])
		{
			SERVER_OnCharacterOutOfView(Player);
			PlayerVisibilities[Player] = false;
		}
	}

	if (Nearby != bPlayersNearby || FriendlyNearby != bFriendlysNearby)
	{
		bPlayersNearby = Nearby;
		bFriendlysNearby = FriendlyNearby;
		SERVER_OnCharactersNearby(bPlayersNearby, bFriendlysNearby);
	}

	if (ClosestPlayer != nullptr)
	{
		const float LockDistance = GetLockOnDistance();
		const float LockOnSQ = LockDistance * LockDistance;
		if (ClosestDistance < LockOnSQ && ClosestPlayer != LockOntoCharacter)
		{
			// Switch to the character not on our team first, then do it based on distance difference
			bool bShouldSwitch = LockOntoCharacter ? !IsOnSameTeamAs(ClosestPlayer) && IsOnSameTeamAs(LockOntoCharacter) : true;
			if (!bShouldSwitch && LockOntoCharacter)
			{
				const float PreviousNearDist = FVector::Dist(LockOntoCharacter->GetActorLocation(), GetActorLocation());
				const float NewNearDist = FMath::Sqrt(LockOnSQ);

				// Add some stability to the locked on character (unless they're dead) by checking if the new one is more than 20% closer
				bShouldSwitch = !LockOntoCharacter->IsAlive() || PreviousNearDist > NewNearDist * 1.2f;
			}

			if (bShouldSwitch)
				SERVER_SetLockOnTarget(ClosestPlayer);
		}
	}
	else if (LockOntoCharacter != nullptr)
	{
		SERVER_SetLockOnTarget(nullptr);
	}
}

bool ASCCharacter::CanSoftLockOntoCharacter(const ASCCharacter* TargetCharacter) const
{
	// Early out
	if (!TargetCharacter || !CanUseSoftLock())
		return false;

	// Check direction and distance
	const FVector ToTargetCharacter = TargetCharacter->GetActorLocation() - GetActorLocation();
	const float SoftLockAngleAsDot = FMath::Acos(FMath::DegreesToRadians(SoftLockAngle));
	const bool bSoftLockRotationValid = (ToTargetCharacter.GetSafeNormal() | GetActorForwardVector()) >= SoftLockAngleAsDot;
	const bool bSoftLockDistance = ToTargetCharacter.SizeSquared() <= FMath::Square(SoftLockDistance);

	return bSoftLockRotationValid && bSoftLockDistance;
}

bool ASCCharacter::SERVER_OnCharacterInView_Validate(ASCCharacter* VisibleCharacter)
{
	return true;
}

void ASCCharacter::SERVER_OnCharacterInView_Implementation(ASCCharacter* VisibleCharacter)
{
	MULTICAST_OnCharacterInView(VisibleCharacter);
}

void ASCCharacter::MULTICAST_OnCharacterInView_Implementation(ASCCharacter* VisibleCharacter)
{
	OnCharacterInView(VisibleCharacter);
}

bool ASCCharacter::SERVER_OnCharacterOutOfView_Validate(ASCCharacter* VisibleCharacter)
{
	return true;
}

void ASCCharacter::SERVER_OnCharacterOutOfView_Implementation(ASCCharacter* VisibleCharacter)
{
	MULTICAST_OnCharacterOutOfView(VisibleCharacter);
}

void ASCCharacter::MULTICAST_OnCharacterOutOfView_Implementation(ASCCharacter* VisibleCharacter)
{
	OnCharacterOutOfView(VisibleCharacter);
}

bool ASCCharacter::SERVER_OnCharactersNearby_Validate(bool IsNearby, bool IsFriendlysNear)
{
	return true;
}

void ASCCharacter::SERVER_OnCharactersNearby_Implementation(bool IsNearby, bool IsFriendlysNear)
{
	MULTICAST_OnCharactersNearby(IsNearby, IsFriendlysNear);
}

void ASCCharacter::MULTICAST_OnCharactersNearby_Implementation(bool IsNearby, bool IsFriendlysNear)
{
	bPlayersNearby = IsNearby;
	bFriendlysNearby = IsFriendlysNear;
	OnCharactersNearby(bPlayersNearby, bFriendlysNearby);
}

void ASCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Make sure that players don't get stuck tilted.
	if (!IsInContextKill())
	{
		if (FMath::Abs(GetActorRotation().Roll) >= KINDA_SMALL_NUMBER)
		{
			FRotator CurrentRotation = GetActorRotation();
			const FRotator TargetRotation(0.f, CurrentRotation.Yaw, 0.f);
			CurrentRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.f);

			// Done interping, snap to null roll
			if (FMath::Abs(GetActorRotation().Roll) <= KINDA_SMALL_NUMBER)
				CurrentRotation = TargetRotation;

			SetActorRotation(CurrentRotation, ETeleportType::TeleportPhysics);
		}

		// Update view
		if (IsLocallyControlled())
		{
			// Need to guard the controller since it could be a vehicle controller at this point
			if (AController* LocalController = GetCharacterController())
			{
				FRotator CurrentControlRotation = LocalController->GetControlRotation();
				if (FMath::Abs(CurrentControlRotation.Roll) >= KINDA_SMALL_NUMBER)
				{
					CurrentControlRotation.Roll = 0.f;
					LocalController->SetControlRotation(CurrentControlRotation);
				}
			}
		}
	}

	UpdateLookAtRotation(DeltaSeconds);

	if (IsLocallyControlled())
	{
		ASCPlayerController* PC = Cast<ASCPlayerController>(Controller);
		ASCInGameHUD* Hud = PC ? PC->GetSCHUD() : nullptr;

		if (bAllowBlockingCorrection)
			CharacterBlockingUpdate();

		UpdateStunWiggleValues(DeltaSeconds);

		TimeUntilVisibilityUpdate -= DeltaSeconds;
		if (TimeUntilVisibilityUpdate <= 0.f)
		{
			UpdateCharacterVisibility();
			TimeUntilVisibilityUpdate = VisibilityUpdateDelay;
		}

		if (!InCombatStance() && OnHitTimer > 0.0f)
		{
			OnHitTimer -= DeltaSeconds;
			if (OnHitTimer <= 0.0f)
			{
				if (Hud)
				{
					Hud->OnCombatStanceChanged(false);
				}
			}
		}

		if (bAttackHeld && CurrentStance == ESCCharacterStance::Combat)
		{
			AttackHoldTimer += DeltaSeconds;
			if (AttackHoldTimer > 0.1f)
			{
				OnHeavyAttack();
				bAttackHeld = false;
			}
		}

		if (InteractPressed)
		{
			UpdateInteractHold(DeltaSeconds);
		}

		// This code could probably just be moved to the SCPlayerController
		if (PC)
		{
			// Adjust our combat stance input component
			if (InCombatStance() && !PC->ContainsInputComponent(CombatStanceInputComponent))
			{
				// Ensure the 
				PC->PushInputComponent(CombatStanceInputComponent);
				if (Hud)
				{
					Hud->OnCombatStanceChanged(true);
				}

			}
			else if (!InCombatStance() && PC->ContainsInputComponent(CombatStanceInputComponent))
			{
				PC->PopInputComponent(CombatStanceInputComponent);
				if (Hud)
				{
					Hud->OnCombatStanceChanged(false);
				}

				// Force block off if we are exiting combat stance
				OnEndBlock();
			}
		}
	}

#if 0
	// Always use the controller rotation for AI
	if (Cast<ASCCharacterAIController>(Controller))
		bUseControllerRotationYaw = true;
	else
#endif
		bUseControllerRotationYaw = (CurrentStance == ESCCharacterStance::Combat) ? GetVelocity().SizeSquared() > 0.f : false;

	if (Role == ROLE_Authority)
	{
		if (bPerformAttackTrace)
		{
			AttackTrace();
		}

		if (InvulnerableStunTimer > 0.0f)
			InvulnerableStunTimer -= DeltaSeconds;
	}

	if (LockOntoCharacter)
	{
		bool bLockToVictim = CanUseHardLock();

		float InterpSpeed = 4.f;

		// Test for soft lock
		if (!bLockToVictim)
		{
			if (CanSoftLockOntoCharacter(LockOntoCharacter))
			{
				// We're not using hard lock, so interp faster
				InterpSpeed = 6.f;
				bLockToVictim = true;

				// Debug values and visible soft lock angle
				if (CVarDebugCombat.GetValueOnGameThread() >= 1)
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Red, FString::Printf(TEXT("We %s soft locked onto %s"),
						bLockToVictim ? TEXT("are") : TEXT("are not"), *LockOntoCharacter->GetName()));

					const FVector ViewDirection = FRotator(0.f, GetActorRotation().Yaw, 0.f).RotateVector(FVector::ForwardVector);
					DrawDebugCone(GetWorld(), GetActorLocation(), ViewDirection, SoftLockDistance, FMath::DegreesToRadians(SoftLockAngle), 0.f, 8, FColor::Red, false, 0.f);
				}
			}
		}
		else // Using hard lock
		{
			if (CVarDebugCombat.GetValueOnGameThread() >= 1)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Red, FString::Printf(TEXT("We are hard locked onto %s"), *LockOntoCharacter->GetName()));
			}
		}

		// We have a hard lock or a soft lock (don't care which) -- interp our rotation torwards them
		if (bLockToVictim)
		{
			const FRotator RotToLockOnCharacter = (LockOntoCharacter->GetActorLocation() - GetActorLocation()).Rotation();

			FRotator TargetRot = GetActorRotation();
			TargetRot.Yaw = RotToLockOnCharacter.Yaw;
			TargetRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, InterpSpeed);
			SetActorRotation(TargetRot);

			// Draw the character we're locked onto
			if (CVarDebugCombat.GetValueOnGameThread() >= 1)
			{
				DrawDebugSphere(GetWorld(), LockOntoCharacter->GetActorLocation(), 32.f, 8, FColor::Red, false, 0.f);
			}
		}
	}

	// bump idle timer if player is moving
	if (Role == ROLE_Authority)
	{
		if (GetVelocity().SizeSquared() > KINDA_SMALL_NUMBER)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
			{
				PC->BumpIdleTime();
			}
		}
	}

#if !UE_BUILD_SHIPPING
	AGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AGameState>() : nullptr;
	if (GameState && GameState->IsMatchInProgress())
	{
		DrawDebug();
	}
#endif // !UE_BUILD_SHIPPING
}

void ASCCharacter::UpdateInteractHold(float DeltaSeconds)
{
	if (USCInteractComponent* Interactable = GetInteractable())
	{
		if (InteractableManagerComponent->GetBestInteractableMethodFlags() & (int32) EILLInteractMethod::Hold)
		{
			// Something disabled this interactable (like power going out)
			if (!Interactable->bIsEnabled)
			{
				ClearAnyInteractions();
				return;
			}

			// if the minigame (bHoldInteractStarted) is true, use the intelligence stat and the interact speed modifier
			InteractTimer += DeltaSeconds * (bHoldInteractStarted ? InteractSpeedModifier * (Interactable->Pips.Num() > 0 ? GetStatInteractMod() * GetPerkInteractMod() : 1.f) : 1.f);

			if (CVarILLDebugInteractables.GetValueOnGameThread() >= 1)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::White, FString::Printf(TEXT("Interact timer %.2f += %.2f * %.2f"), InteractTimer, DeltaSeconds, InteractSpeedModifier));
			}

			if (!bHoldInteractStarted && InteractTimerPassedMinHoldTime())
			{
				if (InteractableManagerComponent->LockInteraction(Interactable, EILLInteractMethod::Hold) || InteractableManagerComponent->GetLockedInteractable() == Interactable)
				{
					Interactable->PlayInteractionCamera(this);
					PlayInteractAnim(Interactable);
					OnHoldInteract(Interactable, EILLHoldInteractionState::Interacting);

					if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
					{
						// if we should enable the minigame, start it
						if (Interactable->Pips.Num() > 0)
						{
							PC->PushInputComponent(InteractMinigameInputComponent);
							if (ASCInGameHUD* Hud = PC->GetSCHUD())
							{
								Hud->StartInteractMinigame();
							}
						}
					}
					bHoldInteractStarted = true;
				}
			}

			// if we reached the hold time limit, enact the hold interaction
			if (InteractTimer >= Interactable->GetHoldTimeLimit(this) + MinInteractHoldTime)
			{
				Interactable->TryAutoReturnCamera(this);
				OnInteract(Interactable, EILLInteractMethod::Hold);
				InteractPressed = false;
				InteractTimer = 0.f;

				// if the minigame is active, end it
				if (bHoldInteractStarted)
				{
					if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
					{
						if (Interactable->Pips.Num() > 0)
						{
							if (ASCInGameHUD* Hud = PC->GetSCHUD())
							{
								Hud->StopInteractMinigame();
							}

							PC->PopInputComponent(InteractMinigameInputComponent);
						}
					}
					InteractSpeedModifier = 1.f;
					bHoldInteractStarted = false;
				}

			}

			if (CVarILLDebugInteractables.GetValueOnGameThread() >= 1 && bHoldInteractStarted)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.2f, FColor::White, FString::Printf(TEXT("Trying to hold on %s:%s, T: %.2f / %.2f"), *Interactable->GetOwner()->GetName(), *Interactable->GetName(), InteractTimer, Interactable->GetHoldTimeLimit(this)));
			}
		}
	}
}

void ASCCharacter::StartTalking()
{
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
		PC->StartTalking();
}

void ASCCharacter::StopTalking()
{
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
		PC->StopTalking();
}

void ASCCharacter::OnShowScoreboard()
{
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
		PC->OnShowScoreboard();
}

void ASCCharacter::OnHideScoreboard()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->OnHideScoreboard();
}

bool ASCCharacter::SERVER_SetActorRotation_Validate(FVector_NetQuantizeNormal InRotation, const bool bShouldTeleport /*= false*/)
{
	return true;
}

void ASCCharacter::SERVER_SetActorRotation_Implementation(FVector_NetQuantizeNormal InRotation, const bool bShouldTeleport /*= false*/)
{
	SetActorRotation(InRotation.Rotation(), bShouldTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None);
}

void ASCCharacter::ReturnCameraToCharacter(const float BlendTime, const EViewTargetBlendFunction BlendFunc, const float BlendExp, const bool bLockOutgoing, const bool bUpdateControlRotation)
{
	if (!IsPendingKill() && IsLocallyControlled())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerController->SetViewTargetWithBlend(this, BlendTime, BlendFunc, BlendExp, bLockOutgoing);

			if (bUpdateControlRotation)
			{
				if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
				{
					FRotator DesiredNewControlRot = FRotator::ZeroRotator;
					DesiredNewControlRot.Yaw = CameraManager->GetCameraRotation().Yaw;
					PlayerController->SetControlRotation(DesiredNewControlRot);
				}
			}

			// Null our roll
			FRotator CurrentControlRotation(PlayerController->GetControlRotation());
			CurrentControlRotation.Roll = 0.f;
			PlayerController->SetControlRotation(CurrentControlRotation);

			// Don't smooth out our camera any more, reset any active cameras (resets spring arms and camera lag)
			ResetCameras();
		}
	}
}

void ASCCharacter::ResetCameras()
{
	for (auto& CameraBlend : ActiveCameraStack)
	{
		CameraBlend.DynamicCamera->Activate(true);
	}
}

void ASCCharacter::TakeCameraControl(AActor* ViewFromActor, const float BlendTime, const EViewTargetBlendFunction BlendFunc, const float BlendExp, const bool bLockOutgoing)
{
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			PC->SetViewTargetWithBlend(ViewFromActor, BlendTime, BlendFunc, BlendExp, bLockOutgoing);
		}
	}
}

void ASCCharacter::MoveIgnoreActorChainAdd(AActor* ActorChild)
{
	if(!ActorChild)
		return;

	do
	{
		MoveIgnoreActorAdd(ActorChild);
		ActorChild = ActorChild->GetOwner() == nullptr ? ActorChild->GetAttachParentActor() : ActorChild->GetOwner();
	} while(ActorChild);
}

void ASCCharacter::MoveIgnoreActorChainRemove(AActor* ActorChild)
{
	if(!ActorChild)
		return;

	do
	{
		MoveIgnoreActorRemove(ActorChild);
		ActorChild = ActorChild->GetOwner() == nullptr ? ActorChild->GetAttachParentActor() : ActorChild->GetOwner();
	} while(ActorChild);
}

float ASCCharacter::GetBuoyancy()
{
	if (CurrentStance == ESCCharacterStance::Swimming)
	{
		return (GetVelocity().Size() / GetCharacterMovement()->MaxSwimSpeed) * 0.06f + 1.f;
	}

	return 0;
}

float ASCCharacter::GetWaterDepth() const
{
	if (CurrentWaterVolume == nullptr)
		return 0.f;

	const float CollisionHalfHeight = GetSimpleCollisionHalfHeight();

	UBrushComponent* VolumeBrushComp = CurrentWaterVolume->GetBrushComponent();
	FHitResult Hit(1.f);

	if (VolumeBrushComp)
	{
		const FVector ActorLocation = GetActorLocation();
		const FVector MeshLocation = GetMesh()->GetComponentLocation();
		const FVector HalfHeightVector = FVector(0.f, 0.f, CollisionHalfHeight);

		static const FName NAME_DepthCheck(TEXT("DepthCheck"));
		FCollisionQueryParams NewTraceParams(NAME_DepthCheck, true);
		VolumeBrushComp->LineTraceComponent(Hit, ActorLocation + HalfHeightVector, ActorLocation - HalfHeightVector, NewTraceParams);

		if (Hit.Time != 1.f)
		{
			// If we're fully submerged return our foot location.
			if (Hit.Time <= KINDA_SMALL_NUMBER)
			{
				return GetWaterZ() - MeshLocation.Z;
			}
			return Hit.Location.Z - MeshLocation.Z;
		}
	}

	return 0.f;
}

void ASCCharacter::AttackTraceBegin()
{
	if (Role != ROLE_Authority)
		return;

	if (const ASCWeapon* CurrentWeapon = GetCurrentWeapon())
	{
		PreviousTraceRotation = CurrentWeapon->KillVolume->GetComponentRotation();
		PreviousTraceLocation = CurrentWeapon->KillVolume->GetComponentLocation();
	}

	// Clear the hit actors every time we start attacking again
	HitActors.Empty();
	bHitAnObstaclePrior = false;
}

void ASCCharacter::AttackTrace()
{
	// Only process attacks on the server
	if (Role != ROLE_Authority)
		return;

	// Paranoid check
	const ASCWeapon* CurrentWeapon = GetCurrentWeapon();
	if (!CurrentWeapon)
		return;

	// Perform a sweep using the weapon's collision shape (KillVolume) between last frame's position/rotation and this frame's position/rotation
	TArray<FHitResult> WeaponHits;
	CurrentWeapon->GetSweepInfo(WeaponHits, PreviousTraceLocation, PreviousTraceRotation);

	PreviousTraceLocation = CurrentWeapon->KillVolume->GetComponentLocation();
	PreviousTraceRotation = CurrentWeapon->KillVolume->GetComponentRotation();

	bWeaponHitProcessed = false;

	// If the sweep is good, let the attack animation update the weapons position and test again on the next frame
	for (const FHitResult& Hit : WeaponHits)
	{
		if (!Hit.bBlockingHit)
			continue;

		AActor* HitActor = Hit.GetActor();

		// This ignores BSP, that's ok.
		if (HitActor &&
			!HitActors.Contains(HitActor))
		{
			// Check to see if hit goes through an obstacle BEFORE hitting its intended target. 
			if (HitActor->IsA<ASCDoor>() || HitActor->IsA<ASCWindow>())
			{
				bHitAnObstaclePrior = true;
			}

			HitActors.Add(HitActor);

			// Unless you're Jason (he's powerful and all that) don't process the hit if we hit an obstacle first. Otherwise you'll be able to 
			// hit through doors, windows, etc. 
			if (!bHitAnObstaclePrior || IsKiller())
			{
				ProcessWeaponHit(Hit);
			}
		}

		if (bWeaponHitProcessed)
			break;
	}

	if (bWeaponHitProcessed)
		bPerformAttackTrace = false;
}

void ASCCharacter::ProcessWeaponHit(const FHitResult& Hit)
{
	ASCWeapon* Weapon = GetCurrentWeapon();
	if (!Weapon)
		return;

	// If we hit a character, check if they're blocking (also always recoil against Killers
	if (ASCCharacter* HitCharacter = Cast<ASCCharacter>(Hit.GetActor()))
	{
		if (Hit.GetComponent() && Hit.GetComponent()->IsA<USkeletalMeshComponent>())
		{
			if (HitCharacter->IsBlocking() || HitCharacter->IsA<ASCKillerCharacter>())
			{
				PlayRecoil(Hit);
				bWeaponHitProcessed = true;
			}
		}
	}
	else if (Hit.GetComponent() && (Hit.GetComponent()->IsA<UStaticMeshComponent>() || Hit.GetComponent()->IsA<USkeletalMeshComponent>()))
	{
		// Get the angle between the impact point and the character's forward
		FVector ActorLocation = GetActorLocation();
		ActorLocation.Z = Hit.ImpactPoint.Z; // Throw out Z
		FVector ActorToImpact = Hit.ImpactPoint - ActorLocation;
		ActorToImpact.Normalize();
		const float ImpactAngle = FMath::RadiansToDegrees(FMath::Acos(GetActorForwardVector() | ActorToImpact));

		if (!bWeaponHitProcessed)
		{
			if (Weapon->CanGetStuckIn(Hit.GetActor(), Hit.PenetrationDepth, ImpactAngle, UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get())))
			{
				PlayWeaponImpact(Hit);
			}
			else if (ImpactAngle <= Weapon->GetMaximumRecoilAngle())
			{
				PlayRecoil(Hit);
			}

			bWeaponHitProcessed = true;
		}
	}
	
	if (bWeaponHitProcessed)
		Weapon->SpawnHitEffect(Hit);
}

void ASCCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	if (Role == ROLE_Authority)
	{
		ForceKillCharacter(this, FDamageEvent(DamageType.GetClass()));
	}
}

float ASCCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	check(Role == ROLE_Authority);

	if (!CanTakeDamage())
		Damage = 0.f;

	if (!IsAlive())
		Damage = 0.f;

	MULTICAST_DamageTaken(Damage, DamageEvent, EventInstigator, DamageCauser);
	
	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (USkeletalMeshComponent* PawnMesh = GetPawnMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(PawnMesh->GetAnimInstance()))
		{
			const FVector HitDir = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() : FVector::ZeroVector;
			const ESCCharacterHitDirection HitDirection = GetHitDirection(HitDir);

			const USCBaseDamageType* const BaseDamage = DamageEvent.DamageTypeClass ? Cast<const USCBaseDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;
			const int32 SelectedHit = (BaseDamage && BaseDamage->bShouldPlayHitReaction == false) ? -1 : AnimInstance->ChooseHitAnim(HitDirection, BlockData.bIsBlocking);
			MULTICAST_OnHit(ActualDamage, HitDirection, SelectedHit, BlockData.bIsBlocking);
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s took %.2f damage from %s"), *GetName(), ActualDamage, DamageCauser ? *DamageCauser->GetName() : TEXT("Suicide")));
	}

	return ActualDamage;
}

void ASCCharacter::MULTICAST_DamageTaken_Implementation(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	const USCBaseDamageType* const BaseDamage = DamageEvent.DamageTypeClass ? Cast<const USCBaseDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;
	if (BaseDamage && !BaseDamage->bShouldPlayHitReaction)
		return;

	ClearAnyInteractions();
}

void ASCCharacter::BeginStun(TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, ESCCharacterHitDirection HitDirection/* = ESCCharacterHitDirection::MAX*/)
{
	if (Role < ROLE_Authority)
	{
		SERVER_BeginStun(StunClass, StunData, HitDirection);
	}
	else
	{
		const USCStunDamageType* const StunDefaults = Cast<const USCStunDamageType>(StunClass->GetDefaultObject());

		// Pick the stun animation to play
		UAnimMontage* StunMontage = nullptr;

		// No stun stacking
		if (bInStun)
			return;

		if (IsBlocking())
			OnEndBlock();

		// enough time hasn't passed since our last stun, ignore this one
		if (IsRecoveringFromStun()/* || (InvulnerableStunTimer > 0.0f && StunDefaults->StunData->bCanBeBlocked)*/) // Temporarily removed per request from GUN
			return;

		bInStun = true;
		bool bIsValidStunArea = true;

		if (IsKiller())
		{
			bIsValidStunArea = StunDefaults->bShouldCheckStunArea ? IsValidStunArea(StunData.StunInstigator) : true;
			StunMontage = bIsValidStunArea ? StunDefaults->JasonStunAnimMontage : nullptr;
			if (bIsValidStunArea && StunDefaults->JasonStunAnimMontage2H && GetCurrentWeapon() && GetCurrentWeapon()->bIsTwoHanded)
			{
				StunMontage = StunDefaults->JasonStunAnimMontage2H;
			}

			if (!StunMontage && HitDirection < ESCCharacterHitDirection::MAX && StunDefaults->JasonDirectionalStunAnimMontages[(int32)HitDirection])
			{
				StunMontage = StunDefaults->JasonDirectionalStunAnimMontages[(int32)HitDirection];
			}

			if (StunMontage == nullptr)
			{
#if WITH_EDITOR
				FMessageLog("PIE").Error()
					->AddToken(FUObjectToken::Create(StunDefaults->GetClass()))
					->AddToken(FTextToken::Create(FText::FromString(TEXT(" has no valid stun animation for the Killer!"))));
#endif
			}
		}
		else
		{
			if (StunDefaults->CounselorStunAnimMontage)
			{
				StunMontage = StunDefaults->CounselorStunAnimMontage;
			}
		}


		MULTICAST_BeginStun(StunMontage, StunClass, StunData, bIsValidStunArea);
		SERVER_OnBlock(false); // in the event we are hit with a non blocking stun, make sure we clear the block flag

		if ((!StunDefaults->bPlayMinigame || (StunDefaults->bPlayMinigame && !bIsValidStunArea)) && !bWiggleGameActive)
		{
			ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(StunData.StunInstigator);
			float StunTime = Counselor ? StunDefaults->TotalStunTime.Get(Counselor) : StunDefaults->TotalStunTime.BaseValue;
			StunTime *= StunData.StunTimeModifier;

			GetWorldTimerManager().SetTimer(TimerHandle_Stun, this, &ASCCharacter::EndStun, StunTime);
		}
	}
}

bool ASCCharacter::SERVER_BeginStun_Validate(TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, ESCCharacterHitDirection HitDirection/* = ESCCharacterHitDirection::MAX*/)
{
	return true;
}

void ASCCharacter::SERVER_BeginStun_Implementation(TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, ESCCharacterHitDirection HitDirection/* = ESCCharacterHitDirection::MAX*/)
{
	BeginStun(StunClass, StunData, HitDirection);
}

void ASCCharacter::MULTICAST_BeginStun_Implementation(UAnimMontage* StunMontage, TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, bool bIsValidStunArea)
{
	if (CVarDebugStuck.GetValueOnGameThread() >= 1 && IsLocallyControlled())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Orange, FString::Printf(TEXT("Stun Started: %s @ %.3f"), StunMontage ? *StunMontage->GetName() : TEXT("NONE"), CurrentTime), false);
	}

	ClearAnyInteractions();

	const USCStunDamageType* const StunDefaults = Cast<const USCStunDamageType>(StunClass->GetDefaultObject());

	if (IsLocallyControlled())
	{
		SetSprinting(false);

		if (StunDefaults && StunDefaults->bPlayMinigame && bIsValidStunArea)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			{
				PC->PushInputComponent(WiggleInputComponent);
				if (!PC->IsMoveInputIgnored())
					PC->SetIgnoreMoveInput(true);
				if (ASCInGameHUD* Hud = PC->GetSCHUD())
				{
					StunText = StunDefaults->StunText;
					Hud->OnBeginWiggleMinigame();
				}
			}

			bWiggleGameActive = true;
			InitStunWiggleGame(StunDefaults, StunData);
		}
	}

	if (CVarDisableStun.GetValueOnGameThread() >= 1)
		return;

	ActiveStunDefaults = StunDefaults;

	// Play stun animation
	if (StunMontage)
	{
		if (USkeletalMeshComponent* PawnMesh = GetPawnMesh())
		{
			if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(PawnMesh->GetAnimInstance()))
			{
				AnimInstance->PlayStunAnimation(StunMontage);
			}
		}
	}
}

bool ASCCharacter::SERVER_EndStun_Validate()
{
	return true;
}

void ASCCharacter::SERVER_EndStun_Implementation()
{
	EndStun();
}

void ASCCharacter::MULTICAST_EndStun_Implementation()
{
	if (CVarDebugStuck.GetValueOnGameThread() >= 1 && IsLocallyControlled())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Orange, FString::Printf(TEXT("Stun Finished: %s @ %.3f"), AnimInstance->StunMontage ? *AnimInstance->StunMontage->GetName() : TEXT("NONE"), CurrentTime), false);
	}

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		PC->PopInputComponent(WiggleInputComponent);
		PC->SetIgnoreMoveInput(false);

		if (bWiggleGameActive)
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
			{
				Hud->OnEndWiggleMinigame();
			}
		}
	}

	bWiggleGameActive = false;

	// stun ended, make sure player cant be stunned again for a set length of time
	if (Role == ROLE_Authority)
		InvulnerableStunTimer = InvulnerableStunDuration;

	if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		if (AnimInstance->StunMontage && ActiveStunDefaults && ActiveStunDefaults->bHasStunRecovery)
		{
			// How long the recover from stun animation is
			const float EndStunLength = AnimInstance->StunMontage->GetSectionLength(AnimInstance->StunMontage->GetSectionIndex(TEXT("EndStun")));

			// Make sure our timer isn't <= 0 or it wont start.
			if (EndStunLength > 0.f)
			{
				// This character is recovering from the stun
				bIsRecoveringFromStun = true;
				FTimerDelegate StunRecoveryDelegate;
				StunRecoveryDelegate.BindLambda([this]() { if (IsValid(this)) { bIsRecoveringFromStun = false; }});
				GetWorldTimerManager().SetTimer(TimerHandle_StunRecovery, StunRecoveryDelegate, EndStunLength, false);
			}

			// Make sure our timer isn't <= 0 or it wont start.
			if (EndStunLength + InvulnerabilityTime > 0)
			{
				// This character is invulnerable for a little
				bInvulnerable = true;
				FTimerDelegate InvulnerabilityDelegate;
				InvulnerabilityDelegate.BindLambda([this]() { if (IsValid(this)) { bInvulnerable = false; }});
				GetWorldTimerManager().SetTimer(TimerHandle_Invulnerability, InvulnerabilityDelegate, EndStunLength + InvulnerabilityTime, false);
			}

			// make sure the player cant be stunned again for the invulnerable duration plus the stun recovery animation time
			if (Role == ROLE_Authority)
				InvulnerableStunTimer += EndStunLength;
		}

		AnimInstance->EndStunAnimation();
	}

	if (TriggeredTrap)
	{
		EscapeTrap();
	}

	ActiveStunDefaults = nullptr;
}

void ASCCharacter::EndStun()
{
	if (Role != ROLE_Authority && Role != ROLE_AutonomousProxy)
		return;
	
	bInStun = false;

	// Force combat stance off. If we were in combat stance before getting stunned, we get into a weird input state.
	SetCombatStance(false);
	SetStance(ESCCharacterStance::Standing);

	if (Role == ROLE_AutonomousProxy)
	{
		SERVER_EndStun();
	}
	else if (Role == ROLE_Authority)
	{
		MULTICAST_EndStun();
	}
}

bool ASCCharacter::IsStunned() const
{
	bool bIsAnimStunned = false;
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			bIsAnimStunned = AnimInstance->IsStunned();
		}
	}

	return bIsAnimStunned || bInStun || bIsRecoveringFromStun;
}

void ASCCharacter::InitStunWiggleGame(const USCStunDamageType* StunDefaults, const FSCStunData& StunData)
{
	ResetStunWiggleValues();

	// Jason values
	DeltaPerWiggle = StunDefaults->PercentPerWiggle.BaseValue * 0.01f;
	StaminaPerWiggle = StunDefaults->StaminaPerWiggle.BaseValue;
	DepletionRate = StunDefaults->DepletionRate.BaseValue;
	MaxStunTime = StunDefaults->TotalStunTime.BaseValue;
	const float BaseStunTime = MaxStunTime;
	MaxStunTime *= StunData.StunTimeModifier;

	if (CVarDebugWiggle.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, MaxStunTime + 5.f, FColor::Cyan, FString::Printf(TEXT("Stun Max Time: %.1f -- Modified Max Time: %.1f  Modifier: %.1f \n Stamina per wiggle: %.1f \n Completion per wiggle: %.1f"),
			BaseStunTime, MaxStunTime, StunData.StunTimeModifier, StaminaPerWiggle, DeltaPerWiggle));

	}
}

void ASCCharacter::UpdateStunWiggleValues(float DeltaTime)
{
	if (!bWiggleGameActive)
		return;

	RollingTime += DeltaTime;

	if (RollingTime >= 1.f)
	{
		RollingTime -= 1.f;
		NumWiggles = 0;
		SetStruggling(false);
	}

	TotalTime += DeltaTime;

	const float MoveAxisValue = [&]() -> float
	{
		// Check with the AI controller if they are wiggling
		if (PlayerState && PlayerState->bIsABot)
		{
			if (ASCCharacterAIController* BotController = Cast<ASCCharacterAIController>(Controller))
			{
				return BotController->IsWiggling() ? -1.0f : 0.f; // Simulate a key press
			}

			return 0.f;
		}

		// Player input
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// FInputActionKeyMapping
			for (auto Action : PC->PlayerInput->GetKeysForAction("GLB_Wiggle"))
			{
				if (PC->PlayerInput->GetKeyValue(Action.Key) > 0)
					return -1.f;
			}
		}

		return 0.f;
	}();

	float Delta = -DepletionRate * DeltaTime;
	if (signbit(MoveAxisValue) != signbit(LastMoveAxisValue) && MoveAxisValue != 0.f && NumWiggles < MaxWigglesPerSecond)
	{
		++NumWiggles;
		SetStruggling(true);

		Delta = DeltaPerWiggle;

		Delta = FMath::Max(MinWiggleDelta, Delta);
	}

	const float MinAlpha = MaxStunTime > 0.f ? TotalTime / MaxStunTime : 0.f;
	WiggleValue = FMath::Clamp(WiggleValue + Delta, MinAlpha, 1.f);

	LastMoveAxisValue = MoveAxisValue;

	if (CVarDebugWiggle.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.0f, FColor::Cyan, FString::Printf(TEXT("Elapsed Time: %.1f --- Completion With Input: %.1f --- Completion: %.1f"),
			TotalTime, WiggleValue, MinAlpha));

	}

	if (CVarInfiniteWiggle.GetValueOnGameThread() > 0)
	{
		// Ignore finishing the wiggle
	}
	else
	{
		if (WiggleValue >= 1.f)
		{
			// Wiggle mini game completed
			EndStun();
			SetStruggling(false);

			// Reset values
			ResetStunWiggleValues();
		}
	}
}

void ASCCharacter::ResetStunWiggleValues()
{
	WiggleValue = 0.f;
	LastMoveAxisValue = 0.f;
	NumWiggles = 0;
	RollingTime = 0.f;
	TotalTime = 0.f;
}

void ASCCharacter::AttachToCharacter(ASCCharacter* AttachTo)
{
	AttachToComponent(AttachTo->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachTo->CharacterAttachmentSocket);
	SetActorRelativeLocation(-GetMesh()->RelativeLocation);
	SetActorRelativeRotation(FRotator::ZeroRotator);
}

void ASCCharacter::DetachFromCharacter()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ASCCharacter::BreakGrab(bool bStunWithPocketKnife/* = false*/)
{
	if (GrabBreakSpecialMove)
	{
		if (!bStunWithPocketKnife)
			StartSpecialMove(GrabBreakSpecialMove, this);

		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh() ? GetMesh()->GetAnimInstance() : nullptr))
		{
			AnimInstance->AllowRootMotion();
		}
	}

	BP_BreakGrab(bStunWithPocketKnife);
}

void ASCCharacter::ActivateSplineCamera(USCSplineCamera* ToActivate)
{
	if (!ToActivate)
		return;

	ActiveCameraStack.Empty();

	MakeOnlyActiveCamera(this, ToActivate);
	ToActivate->ActivateCamera();
}

void ASCCharacter::ReactivateDynamicCams()
{
	TArray<UActorComponent*> ChildComponents = GetComponentsByClass(UCameraComponent::StaticClass());

	// Deactivate all cameras on the actor
	for (UActorComponent* comp : ChildComponents)
	{
		if (USCSplineCamera* cam = Cast<USCSplineCamera>(comp))
		{
			cam->SetActive(false);
		}
		else
		{
			comp->SetActive(true);
		}
	}
}

bool ASCCharacter::SetStruggling_Validate(bool InStruggling)
{
	return true;
}

void ASCCharacter::SetStruggling_Implementation(bool InStruggling)
{
	bIsStruggling = InStruggling;
}

FRotator ASCCharacter::GetBaseAimRotation() const
{
	FRotator POVRot = GetActorRotation();

	if (POVRot.Pitch == 0.f)
	{
		POVRot.Pitch = RemoteViewPitch;
		POVRot.Pitch = POVRot.Pitch * 360.f / 255.f;
	}

	return POVRot;
}

void ASCCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
	if (NewController->IsA<ASCPlayerController>() && GameMode && GameMode->IsA<ASCGameMode_Offline>())
	{
		// Add an ILLCrowdAgentInterfaceComponent to this character so AI properly avoids this player
		UILLCrowdAgentInterfaceComponent* CrowdInterfaceComp = NewObject<UILLCrowdAgentInterfaceComponent>(this, TEXT("AIAvoidance"));
		if (CrowdInterfaceComp)
		{
			CrowdInterfaceComp->RegisterComponent();
		}
		
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
			Capsule->AreaClass = USCNavArea_JasonOnly::StaticClass();
		SetCanAffectNavigationGeneration(true);
		
	}

	if (ASCCharacterAIController* AIController = Cast<ASCCharacterAIController>(NewController))
	{
		// Try to force navmesh generation when we are possessed
		UNavigationSystem::UpdateActorAndComponentsInNavOctree(*this);

		// Set our starting waypoint on our controller
		if (StartingWaypoint)
			AIController->SetStartingWaypoint(StartingWaypoint);
	}

	// Make sure our capsule's overlap functions are bound and update our overlaps.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		if (!Capsule->OnComponentBeginOverlap.IsAlreadyBound(this, &ASCCharacter::OnOverlapBegin))
			Capsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ASCCharacter::OnOverlapBegin);
		if (!Capsule->OnComponentEndOverlap.IsAlreadyBound(this, &ASCCharacter::OnOverlapEnd))
			Capsule->OnComponentEndOverlap.AddUniqueDynamic(this, &ASCCharacter::OnOverlapEnd);

		// This is necessary for placed characters since they can register overlaps without notifying about them when APawn::Restart() is called
		Capsule->UpdateOverlaps();
	}

	if (NewController->IsA(ASCPlayerController::StaticClass()) || NewController->IsA(ASCCharacterAIController::StaticClass()))
	{
		bIsPossessed = true; // CLEANUP: What is the point of this?
	}
	else
	{
		// CLEANUP: Somebody explain this
		NewController->UnPossess();
	}
}

void ASCCharacter::UnPossessed()
{
	Super::UnPossessed();

	bIsPossessed = false;
}

bool ASCCharacter::CanTakeDamage() const
{
	if (Role != ROLE_Authority)
		return false;

	if (IsStunned())
		return false;

	if (IsCharacterDisabled())
		return false;

	return true;
}

void ASCCharacter::OnReceivedController()
{
	Super::OnReceivedController();

	if (IsLocallyControlled())
	{
		InteractableManagerComponent->SetComponentTickEnabled(true);

		SpawnPostProcessVolume();

		// Make sure that all item shimmers are updated correctly based on our character class
		UWorld* World = GetWorld();
		for (TActorIterator<ASCItem> It(World, ASCItem::StaticClass()); It; ++It) // OPTIMIZE: pjackson
		{
			(*It)->SetShimmerEnabled(false);
			(*It)->SetShimmerEnabled(true);
		}
	}
}

void ASCCharacter::OnReceivedPlayerState()
{
	Super::OnReceivedPlayerState();

	// Voice audio component
	if (ASCPlayerState* PS = GetPlayerState())
	{
		USoundWaveProcedural* SoundStreaming = Cast<USoundWaveProcedural>(VoiceAudioComponent->Sound);
		if (!SoundStreaming)
		{
			SoundStreaming = NewObject<USoundWaveProcedural>();
			SoundStreaming->SampleRate = 16000;
			SoundStreaming->NumChannels = 1;
			SoundStreaming->Duration = INDEFINITELY_LOOPING_DURATION;
			SoundStreaming->SoundGroup = SOUNDGROUP_Voice;
			SoundStreaming->bLooping = false;

			VoiceAudioComponent->SetSound(SoundStreaming);
		}

		PS->VoiceAudioComponent = VoiceAudioComponent;
		if (VoiceAudioComponent && VoiceAudioComponent->GetAttenuationSettingsToApply())
		{
			PS->MonoAttenuationRadius = VoiceAudioComponent->GetAttenuationSettingsToApply()->OmniRadius;
			PS->MonoAttenuationFalloff = VoiceAudioComponent->GetAttenuationSettingsToApply()->FalloffDistance;
		}

		PS->CharacterSpawned();
	}
}

void ASCCharacter::UpdateCharacterMeshVisibility()
{
	Super::UpdateCharacterMeshVisibility();

	// make sure animations always play on the dedicated server so things like melee attacks work
	GetMesh()->MeshComponentUpdateFlag =  EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
}

void ASCCharacter::Suicide()
{
	KilledBy(this);
}

void ASCCharacter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = nullptr;
		if (EventInstigator != nullptr)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = nullptr;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, nullptr);
	}
}

bool ASCCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (bIsDying																		// already dying
		|| IsPendingKill()																// already destroyed
		|| Role != ROLE_Authority														// not authority
		|| GetWorld()->GetAuthGameMode() == nullptr
		|| GetWorld()->GetAuthGameMode<AGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}
	
	return true;
}

bool ASCCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	ensure(Role == ROLE_Authority);

	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
		return false;

	Health = 0.0f;

	const UDamageType* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	if (!DamageType->bCausedByWorld)
		Killer = GetDamageInstigator(Killer, *DamageType);

	// Notify the game mode
	AController* const KilledPlayer = Controller ? Controller : Cast<AController>(GetOwner());

	if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		GameMode->CharacterKilled(Killer, KilledPlayer, this, DamageType);
	}

	// Disable navigation invoking for AI
	if (ASCCharacterAIController* CAIC = Cast<ASCCharacterAIController>(Controller))
	{
		CAIC->OnPawnDied();
	}

	// Force a movement replication update
	GetCharacterMovement()->ForceReplicationUpdate();

	// Call to the simulated path for death
	OnDeath(KillingDamage, DamageEvent, Killer ? Cast<ASCPlayerState>(Killer->PlayerState) : nullptr, DamageCauser);
	
	return true;
}

void ASCCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, AActor* DamageCauser)
{
	if (bIsDying)
		return;

	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		// Make sure we don't lock interaction with an item as we go to our grave
		if (UILLInteractableComponent* LockedComponent = InteractableManagerComponent->GetLockedInteractable())
			InteractableManagerComponent->UnlockInteraction(LockedComponent);

		ClientsOnDeath(KillingDamage, DamageEvent, KillerPlayerState, DamageCauser);
	}

	// Cancel any current interactions.
	ClearAnyInteractions();

	// We're dying, get rid of interaction prompts.
	InteractableManagerComponent->CleanupInteractions();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	// We need some way to opt out of this (or in?)
	if (!bIgnoreDeathRagdoll)
	{
		SetRagdollPhysics(NAME_None, true, 1.f, false, false);
	}

	if (bPauseAnimsOnDeath)
	{
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;
		GetMesh()->bPauseAnims = true;
	}

	// Shutdown and destroy the music system when the player dies.
	if (MusicComponent)
	{
		MusicComponent->ShutdownMusicComponent();
		MusicComponent->DestroyComponent();
		MusicComponent = nullptr;
	}

	// Catch all to remove screen FX
	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		for (auto Blendable : PPV->Settings.WeightedBlendables.Array)
		{
			Blendable.Weight = 0.f;
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, FString::Printf(TEXT("%s killed by %s"), *GetClass()->GetName(), DamageCauser ? *DamageCauser->GetClass()->GetName() : TEXT("Suicide")));
	}

	if (OnCharacterDeath.IsBound())
		OnCharacterDeath.Broadcast();

//	SetActorTickEnabled(false);
}

ESCCharacterHitDirection ASCCharacter::GetHitDirection(const FVector& VectorToDamageCauser)
{
	// Determine where we were hit from
	const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), VectorToDamageCauser);
	const float RightDot = FVector::DotProduct(GetActorRightVector(), VectorToDamageCauser);

	const ESCCharacterHitDirection FrontBack = ForwardDot > 0.0f ? ESCCharacterHitDirection::Back : ESCCharacterHitDirection::Front;
	const ESCCharacterHitDirection LeftRight = RightDot > 0.0f ? ESCCharacterHitDirection::Left : ESCCharacterHitDirection::Right;
	const ESCCharacterHitDirection HitDirection = FMath::Abs(ForwardDot) > FMath::Abs(RightDot) ? FrontBack : LeftRight;

	return HitDirection;
}

void ASCCharacter::ClientsOnDeath_Implementation(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, AActor* DamageCauser)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
	{
		PC->OnHideEmoteSelect();
	}

	OnDeath(KillingDamage, DamageEvent, KillerPlayerState, DamageCauser);
}

void ASCCharacter::MULTICAST_OnHit_Implementation(float ActualDamage, ESCCharacterHitDirection HitDirection, int32 SelectedHit, bool bBlocking)
{
	if (bInStun)
		return;

	// Don't play a hit reaction if we get a -1
	if (SelectedHit >= 0)
	{
		if (USkeletalMeshComponent* PawnMesh = GetPawnMesh())
		{
			if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(PawnMesh->GetAnimInstance()))
			{
				AnimInstance->PlayHitAnim(HitDirection, SelectedHit, bBlocking);
			}
		}
	}

	// Notify the local HUD
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->IsLocalController())
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
			{
				Hud->OnCharacterTookDamage(this, ActualDamage, HitDirection);
			}
		}
	}

	if (IsLocallyControlled())
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
				Hud->OnCombatStanceChanged(true);
		}

		OnHitTimer = 3.0f;
	}
}

void ASCCharacter::SetRagdollPhysics(FName BoneName /* = NAME_None*/, bool bSimulate /* = true*/, float BlendWeight /* = 1.f*/, bool SkipCustomPhys /* = false*/, bool MakeServerCall /* = false*/)
{
	// Jason should never ragdoll, specifically when he disconnects from the game. 
	bSimulate = IsKiller() ? false : bSimulate;

	// If marked as a server call make sure we call to the server first.
	if (MakeServerCall)
	{
		if (Role < ROLE_Authority)
		{
			SERVER_SetRagdollPhysics(BoneName, bSimulate, BlendWeight, SkipCustomPhys);
		}
		else
		{
			MULTICAST_SetRagdollPhysics(BoneName, bSimulate, BlendWeight, SkipCustomPhys);
		}
		return;
	}

	// If unmarked as a server call or we've already run through the multicast this will execute.
	if (IsPendingKill() || !GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		return;
	}
	else
	{
		// Initialize physics
		if (BoneName != NAME_None)
		{
			GetMesh()->SetAllBodiesBelowSimulatePhysics(BoneName, bSimulate);
			GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(BoneName, BlendWeight, SkipCustomPhys);
		}
		else
		{
			GetMesh()->SetAllBodiesSimulatePhysics(bSimulate);
			GetMesh()->SetSimulatePhysics(bSimulate);
		}
	}
}

bool ASCCharacter::SERVER_SetRagdollPhysics_Validate(FName BoneName /* = NAME_None*/, bool Simulate /* = true*/, float BlendWeight /* = 1.f*/, bool SkipCustomPhys /* = false*/)
{
	return true;
}

void ASCCharacter::SERVER_SetRagdollPhysics_Implementation(FName BoneName /* = NAME_None*/, bool Simulate /* = true*/, float BlendWeight /* = 1.f*/, bool SkipCustomPhys /* = false*/)
{
	SetRagdollPhysics(BoneName, Simulate, BlendWeight, SkipCustomPhys, true);
}

void ASCCharacter::MULTICAST_SetRagdollPhysics_Implementation(FName BoneName /* = NAME_None*/, bool Simulate /* = true*/, float BlendWeight /* = 1.f*/, bool SkipCustomPhys /* = false*/)
{
	SetRagdollPhysics(BoneName, Simulate, BlendWeight, SkipCustomPhys);
}


bool ASCCharacter::IsRagdoll() const
{
	return GetMesh()->bBlendPhysics;
}

float ASCCharacter::GetSpeedModifier() const
{
	return 1.f;
}

void ASCCharacter::SetSprinting(bool bSprinting)
{
	if (Role < ROLE_Authority)
	{
		ServerSetSprinting(bSprinting);
	}

	bIsSprinting = bSprinting;
}

bool ASCCharacter::ServerSetSprinting_Validate(bool bSprinting)
{
	return true;
}

void ASCCharacter::ServerSetSprinting_Implementation(bool bSprinting)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	SetSprinting(bSprinting);
}

void ASCCharacter::SetRunning(bool bRunning)
{
	const bool ShouldRun = bRunning && (CurrentStance != ESCCharacterStance::Combat);
	if (ShouldRun == bIsRunning)
		return;

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bRunning);
	}

	bIsRunning = bRunning;
}

bool ASCCharacter::ServerSetRunning_Validate(bool bRunning)
{
	return true;
}

void ASCCharacter::ServerSetRunning_Implementation(bool bRunning)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	SetRunning(bRunning);
}

////////////////////////////////////////////////////////////////////////////////
// Weapon Usage

bool ASCCharacter::CanAttack() const
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return false;
	}

	// Dead people don't attack
	if (!IsAlive())
		return false;

	// Can't attack if already attacking
	if (IsAttacking())
		return false;

	if (IsRecoiling())
		return false;

	if (IsWeaponStuck())
		return false;

	// We're using something
	if (InteractableManagerComponent->GetLockedInteractable())
		return false;

	if (CurrentStance == ESCCharacterStance::Swimming)
		return false;

	// Can't attack if stunned or recovering from a stun
	if (IsStunned() || IsRecoveringFromStun())
		return false;

	// Can't attack if you don't have a weapon
	if (GetCurrentWeapon() == nullptr)
		return false;

	// Can't attack if we're doing something cool
	if (ActiveSpecialMove)
		return false;

	// Can't attack if we're picking something up
	if (PickingItem)
		return false;

	return true;
}

bool ASCCharacter::CanBlock() const
{
	if (!IsAlive())
		return false;

	if (IsInSpecialMove())
		return false;

	// We're using something
	if (InteractableManagerComponent->GetLockedInteractable())
		return false;

	if (IsBlocking())
		return false;

	if (!GetCurrentWeapon())
		return false;

	if (IsAttacking())
		return false;

	if (IsStunned() || IsRecoveringFromStun())
		return false;

	return true;
}

bool ASCCharacter::AddOrSwapPickingItem()
{
	// We should only be modifying inventory on the server
	check(Role == ROLE_Authority);

	if (!PickingItem)
		return false;

	if (PickingItem->PickupScoreEvent && GetWorld())
	{
		if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
		{
			GM->HandleScoreEvent(PlayerState, PickingItem->PickupScoreEvent);
		}
	}

	// Tracking picking up this item
	if (ASCPlayerState* PS = GetPlayerState())
	{
		PS->TrackItemUse(PickingItem->GetClass(), (uint8)ESCItemStatFlags::PickedUp);
	}

	if (PickingItem->bIsLarge)
	{
		// Character implementation assumes large item
		DropEquippedItem();
		EquipPickingItem();
		return true;
	}

	return false;
}

void ASCCharacter::EquipPickingItem()
{
	check(Role == ROLE_Authority);

	if (PickingItem)
	{
		EquippedItem = PickingItem;
		OnRep_EquippedItem();
		EquippedItem->SetOwner(this);
		EquippedItem->AttachToCharacter(false);
		EquippedItem->bIsPicking = false;
		InteractableManagerComponent->UnlockInteraction(EquippedItem->GetInteractComponent());
		PickingItem = nullptr;
	}
}

void ASCCharacter::DropEquippedItem(const FVector& DropLocationOverride /*= FVector::ZeroVector*/, bool bOverrideDroppingCharacter /*= false*/)
{
	check(Role == ROLE_Authority);

	if (EquippedItem)
	{
		FVector DropLocation = DropLocationOverride;
		if (PickingItem && PickingItem->CurrentCabinet == nullptr)
			DropLocation = PickingItem->GetActorLocation();

		EquippedItem->OnDropped(bOverrideDroppingCharacter ? nullptr : this, DropLocation);
		EquippedItem = nullptr;
		OnRep_EquippedItem();

		// If we drop our current item (might be a weapon!) make sure we don't stay in combat stance
		if (CurrentStance == ESCCharacterStance::Combat)
			SetCombatStance(false);
	}
}

void ASCCharacter::SimulatePickupItem(ASCItem* Item)
{
	if (Role < ROLE_Authority)
	{
		SERVER_SimulatePickupItem(Item);
	}
	else
	{
		MULTICAST_SimulatePickupItem(Item);
	}
}

bool ASCCharacter::SERVER_SimulatePickupItem_Validate(ASCItem* Item)
{
	return true;
}

void ASCCharacter::SERVER_SimulatePickupItem_Implementation(ASCItem* Item)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	SimulatePickupItem(Item);
}

void ASCCharacter::MULTICAST_SimulatePickupItem_Implementation(ASCItem* Item)
{
	if (Item == nullptr)
		return;

	if (Role == ROLE_Authority)
	{
		PickingItem = Item;
		PickingItem->bIsPicking = true;
	}

	// Rotate the character toward the item and kill movement
	GetCharacterMovement()->StopMovementImmediately();
	FRotator NewRotation = (Item->GetActorLocation() - GetActorLocation()).ToOrientationRotator();
	NewRotation.Pitch = 0.f;
	NewRotation.Roll = 0.f;
	SetActorRotation(NewRotation);

	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCCounselorAnimInstance* AnimInstance = Cast<USCCounselorAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			const float MontageLength = AnimInstance->PlayPickupItem(Item->GetActorLocation(), Item->bIsLarge);
		}
	}

	OnItemPickedUp(Item);
}

bool ASCCharacter::CanTakeItem(const ASCItem* Item) const
{
	if (Item)
	{
		const int32 Max = Item->GetInventoryMax();
		if (Max <= 0 || Max > NumberOfItemTypeInInventory(Item->GetClass()))
			return true;
	}

	return false;
}

bool ASCCharacter::HasItemInInventory(TSubclassOf<ASCItem> ItemClass) const
{
	if (EquippedItem && EquippedItem->IsA(ItemClass))
		return true;

	return false;
}

int32 ASCCharacter::NumberOfItemTypeInInventory(TSubclassOf<ASCItem> ItemClass) const
{
	if (EquippedItem && EquippedItem->IsA(ItemClass))
		return 1;

	return 0;
}

void ASCCharacter::GiveItem(TSubclassOf<ASCItem> ItemClass)
{
	if (!ItemClass)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = nullptr;
	SpawnParams.Instigator = nullptr;

	if (ASCItem* NewItem = GetWorld()->SpawnActor<ASCItem>(ItemClass, SpawnParams))
	{
		PickingItem = NewItem;
		PickingItem->bIsPicking = true;
		PickingItem->EnableInteraction(false);
		AddOrSwapPickingItem();
	}
}

void ASCCharacter::GiveItem(TSoftClassPtr<ASCItem> SoftItemClass)
{
	if (SoftItemClass.IsNull())
		return;

	if (SoftItemClass.Get())
	{
		GiveItem(TSubclassOf<ASCItem>(SoftItemClass.Get()));
	}
	else if (UWorld* World = GetWorld())
	{
		if (USCGameInstance* GameInstance = Cast<USCGameInstance>(World->GetGameInstance()))
		{
			FStreamableDelegate Delegate;
			Delegate.BindUObject(this, &ThisClass::GiveItem, SoftItemClass);
			GameInstance->StreamableManager.RequestAsyncLoad(SoftItemClass.ToSoftObjectPath(), Delegate);
		}
	}
}

void ASCCharacter::SetMaxHealth(float _Health)
{
	MaxHealth = _Health;
}

void ASCCharacter::OnRep_CurrentStance()
{
	// Make sure clients keep running turned off while waiting for the round trip of the stance change
	if (CurrentStance == ESCCharacterStance::Combat)
		SetRunning(false);

	// If we exit combat stance but we're still blocking stop it, just stop it.
	if (CurrentStance != ESCCharacterStance::Combat && BlockData.bIsBlocking)
	{
		BlockData.bIsBlocking = false;
		OnRep_Block();
	}

	OnStanceChanged(CurrentStance);
}

void ASCCharacter::OnEndBlockReation()
{
	OnRep_Block();
}

///////////////////////////////////////////////////////////////////////////////////
// Replication

ASCWeapon* ASCCharacter::GetCurrentWeapon() const
{
	return Cast<ASCWeapon>(EquippedItem);
}

void ASCCharacter::DestroyCurrentWeapon()
{
	if (CVarDisableDestroyWeapon.GetValueOnGameThread() >= 1)
		return;

	if (ASCWeapon* CurrentWeapon = GetCurrentWeapon())
	{
		if (Role == ROLE_Authority)
		{
			CurrentWeapon->Destroy();
		}

		DropEquippedItem();
	}
}

ASCItem* ASCCharacter::GetEquippedItem() const
{
	return EquippedItem;
}

bool ASCCharacter::IsEquippedItemNonWeapon() const
{
	if (EquippedItem)
	{
		if (EquippedItem->IsA(ASCWeapon::StaticClass()))
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

float ASCCharacter::GetRunningSpeedModifier() const
{
	if (CurrentStance == ESCCharacterStance::Standing && CurrentWaterVolume)
	{
		return 0.7f;
	}
	return 1.f;
}

void ASCCharacter::CharacterBlockingUpdate()
{
	// Don't move if the special move doesn't allow it.
	if (ActiveSpecialMove && !ActiveSpecialMove->CanMove())
		return;

	// AI can't push each other, that's mean.
	if (Controller && Controller->IsA(ASCCharacterAIController::StaticClass()))
		return;

	// Don't push Jason around
	if (IsKiller())
		return;

	FVector MovementVector = FVector::ZeroVector;

	const FVector CurrentLocation = GetActorLocation();
	const float MinDistance = GetCapsuleComponent()->GetScaledCapsuleRadius() + BlockingBufferDistance;

	if (const ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		for (const ASCCharacter* Character : State->PlayerCharacterList)
		{
			// Don't get pushed around by dead guys.
			if (!Character->IsAlive())
				continue;

			// Jason shouldn't push counselors and we shouldn't push ourselves.
			if (Character == this || Character->IsKiller())
				continue;

			// Grabbed counselors can't push us.
			if (Character->CurrentStance == ESCCharacterStance::Grabbed)
				continue;

			// Anyone in a special move can't push us.
			if (Character->IsInSpecialMove())
				continue;

			// Counselors in vehicles can't push.
			if (Character->GetVehicle())
				continue;

			const FVector OtherLocation = Character->GetActorLocation();
			if (FVector::DistSquared(CurrentLocation, OtherLocation) > FMath::Square(MinDistance + Character->GetCapsuleComponent()->GetScaledCapsuleRadius()))
				continue;

			MovementVector += CurrentLocation - OtherLocation;
		}

		if (!MovementVector.IsNearlyZero())
		{
			AddMovementInput(MovementVector.GetSafeNormal(), CorrectionStrength);
		}
	}
}

bool ASCCharacter::IsAttacking() const
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			return AnimInstance->IsAttacking();
		}
	}

	return false;
}

bool ASCCharacter::IsHeavyAttacking() const
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			return AnimInstance->IsHeavyAttack();
		}
	}

	return false;
}

bool ASCCharacter::IsRecoiling() const
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			return AnimInstance->IsRecoiling();
		}
	}

	return false;
}

bool ASCCharacter::IsWeaponStuck() const
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			return AnimInstance->IsWeaponStuck();
		}
	}

	return false;
}

int32 ASCCharacter::GetMaxHealth() const
{
	return MaxHealth;
}

bool ASCCharacter::IsAlive() const
{
	return Health > SMALL_NUMBER;
}

ASCPlayerState* ASCCharacter::GetPlayerState() const
{
	return Cast<ASCPlayerState>(PlayerState);
}

float ASCCharacter::GetInteractTimePercent() const
{
	USCInteractComponent* Interactable = GetInteractable();
	if (!Interactable || Interactable->GetHoldTimeLimit(this) == 0.f)
	{
		return 0.f;
	}

	if ((InteractableManagerComponent->GetBestInteractableMethodFlags() & (int32)EILLInteractMethod::Hold) == 0)
	{
		return 0.f;
	}

	return (InteractTimer - MinInteractHoldTime) / Interactable->GetHoldTimeLimit(this);
}

void ASCCharacter::SetInteractTimePercent(float NewInteractTime)
{
	USCInteractComponent* Interactable = GetInteractable();
	if (!Interactable || Interactable->GetHoldTimeLimit(this) == 0.f)
		return;

	InteractTimer = NewInteractTime * Interactable->GetHoldTimeLimit(this) + MinInteractHoldTime;
}

void ASCCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	check(OverlappedComponent == GetCapsuleComponent());

	if (APhysicsVolume* OtherPhysicsVolume = Cast<APhysicsVolume>(OtherActor))
	{
		if (OtherPhysicsVolume->bWaterVolume && OtherPhysicsVolume != CurrentWaterVolume)
		{
			CurrentWaterVolume = OtherPhysicsVolume;
			bInWater = true;
			OnInWaterChanged(bInWater);
			UpdateSwimmingState();
		}
	}
	else if (USCIndoorComponent* IndoorComp = Cast<USCIndoorComponent>(OtherComp))
	{
		if (OverlappingIndoorComponents.AddUnique(IndoorComp) != INDEX_NONE)
		{
			// Update bIsInside and simulate OnRep_IsInside on the server
			if (Role == ROLE_Authority)
			{
				const bool bWasInside = bIsInside;
				bIsInside = (OverlappingIndoorComponents.Num() > 0);
				if (bWasInside != bIsInside)
				{
					OnRep_IsInside();
				}
			}
		}
	}
}

void ASCCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APhysicsVolume* Volume = Cast<APhysicsVolume>(OtherActor))
	{
		if (Volume == CurrentWaterVolume && !IsInContextKill())
		{
			CurrentWaterVolume = nullptr;
			bInWater = false;
			OnInWaterChanged(bInWater);
			UpdateSwimmingState();
		}
	}
	else if (USCIndoorComponent* IndoorComp = Cast<USCIndoorComponent>(OtherComp))
	{
		OverlappingIndoorComponents.Remove(IndoorComp);

		// Update bIsInside and simulate OnRep_IsInside on the server
		if (Role == ROLE_Authority)
		{
			const bool bWasInside = bIsInside;
			bIsInside = (OverlappingIndoorComponents.Num() > 0);
			if (bWasInside != bIsInside)
			{
				OnRep_IsInside();
			}
		}
	}
}

bool ASCCharacter::IsIndoors() const
{
	return bIsInside;
}

bool ASCCharacter::IsIndoorsWith(ASCCharacter* OtherCharacter) const
{
	if (IsIndoors() && OtherCharacter && OtherCharacter->IsIndoors())
	{
		for (USCIndoorComponent* IndoorComp : OverlappingIndoorComponents)
		{
			if (OtherCharacter->OverlappingIndoorComponents.Contains(IndoorComp))
				return true;
		}
	}

	return false;
}

bool ASCCharacter::IsInsideCabin(ASCCabin* Cabin) const
{
	for (USCIndoorComponent* IndoorComp : OverlappingIndoorComponents)
	{
		if (ASCCabin* InsideCabin = Cast<ASCCabin>(IndoorComp->GetOwner()))
		{
			if (InsideCabin == Cabin)
				return true;
		}
	}

	return false;
}

void ASCCharacter::ForceIndoors(bool IsInside)
{
	if (Role == ROLE_Authority)
	{
		if (bIsInside != IsInside)
		{
			bIsInside = IsInside;
			OnRep_IsInside();
		}
	}
	else
	{
		SERVER_ForceIndoors(IsInside);
	}
}

bool ASCCharacter::SERVER_ForceIndoors_Validate(bool IsInside)
{
	return true;
}

void ASCCharacter::SERVER_ForceIndoors_Implementation(bool IsInside)
{
	ForceIndoors(IsInside);
}

void ASCCharacter::OnRep_IsInside()
{
	OnIndoorsChanged(bIsInside);

	if (OnIsInsideChanged.IsBound())
		OnIsInsideChanged.Broadcast(bIsInside);

	if (Role == ROLE_Authority)
	{
		if (ASCCharacterAIController* AIC = Cast<ASCCharacterAIController>(Controller))
		{
			AIC->PawnIndoorsChanged(bIsInside);
		}
	}
}

void ASCCharacter::ToggleCombatStance()
{
	if (IsAttacking() || IsBlocking())
		return;

	const bool InCombat = CurrentStance == ESCCharacterStance::Combat;
	SetCombatStance(!InCombat);
}

void ASCCharacter::SetCombatStance(const bool bEnable)
{
	const bool bInCombat = CurrentStance == ESCCharacterStance::Combat;
	if (bEnable == bInCombat)
		return;

	// Can't go into combat without a weapon
	if (bEnable && GetCurrentWeapon() == nullptr)
		return;

	// Can't go into combat while in a special move
	if (bEnable && IsInSpecialMove())
		return;

	// Can't go into combat stance while stunned
	if (IsStunned())
		return;

	if (bEnable)
	{
		SetSprinting(false);
		SetRunning(false);
		SetStance(ESCCharacterStance::Combat);
	}
	else
	{
		if (Role < ROLE_Authority)
		{
			ServerSetStance(PreviousStance);
		}

		// Simulate stance changing effects when client-authorative position is enabled to make high latency + high packet-loss feel better
		if (Role == ROLE_Authority || (Role == ROLE_AutonomousProxy && GetDefault<AGameNetworkManager>()->ClientAuthorativePosition))
		{
			const ESCCharacterStance NewStance = PreviousStance;
			PreviousStance = ESCCharacterStance::Standing;
			CurrentStance = NewStance;
			OnRep_CurrentStance();

			// Just in case we're client authoritive
			if (Role != ROLE_Authority)
				SERVER_SetLockOnTarget(nullptr);
			LockOntoCharacter = nullptr;
			CLIENT_SetCombatStance(bEnable);
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0 && Role == ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Blue, FString::Printf(TEXT("%s %s combat stance"), *GetClass()->GetName(), bEnable ? TEXT("entered") : TEXT("exited")));
	}

	// notify blueprint
	OnCombatModeChanged(CurrentStance == ESCCharacterStance::Combat);
}

void ASCCharacter::SetStance(ESCCharacterStance NewStance)
{
	if (Role != ROLE_Authority && Role != ROLE_AutonomousProxy)
		return;

	if (CurrentStance == NewStance)
		return;

	if (CurrentStance == ESCCharacterStance::Combat)
	{
		// Special case handling for if we are leaving combat stance
		SetCombatStance(false);
	}

	if (Role == ROLE_AutonomousProxy)
	{
		ServerSetStance(NewStance);
	}

	// Simulate stance changing effects when client-authorative position is enabled to make high latency + high packet-loss feel better
	if (Role == ROLE_Authority || (Role == ROLE_AutonomousProxy && GetDefault<AGameNetworkManager>()->ClientAuthorativePosition))
	{
		if (CurrentStance == NewStance)
			return;

		PreviousStance = CurrentStance;
		CurrentStance = NewStance;
		OnRep_CurrentStance();
	}
}

void ASCCharacter::CLIENT_SetCombatStance_Implementation(const bool bEnable)
{
	SetCombatStance(bEnable);
}

bool ASCCharacter::ServerSetStance_Validate(ESCCharacterStance NewStance)
{
	return true;
}

void ASCCharacter::ServerSetStance_Implementation(ESCCharacterStance NewStance)
{
	// Bump idle timer if we're actually changing the stance.
	if (CurrentStance != NewStance)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			PC->BumpIdleTime();
	}

	SetStance(NewStance);
}

bool ASCCharacter::LineOfSight(ASCCharacter* OtherActor, float MinFacingAngle, bool AllowFromMesh, bool AllowFromCamera)
{
	check(IsLocallyControlled());
	if (!OtherActor->IsVisible())
		return false;

	// Distance culling to save on very long ray casts being sent to physics
	const FVector OurActorLocation = GetActorLocation();
	const FVector OtherActorLocation = OtherActor->GetActorLocation();
	const float MaxDistSQ = FMath::Square(MaxVisibilityDistance);
	if (FVector::DistSquared(OurActorLocation, OtherActorLocation) >= MaxDistSQ)
	{
		return false;
	}

	// Make sure the AI pawn sensor agrees
	if (ASCCharacterAIController* AIController = Cast<ASCCharacterAIController>(Controller))
	{
		if (!AIController->CanSeePawn(OtherActor))
			return false;
	}

	// Ray trace from us towards OtherActor, checking for visibility
	static FName NAME_VisiblityTrace(TEXT("VisiblityTrace"));
	FCollisionQueryParams SC_TraceParams = FCollisionQueryParams(NAME_VisiblityTrace, true, this);
	SC_TraceParams.bTraceComplex = true;
	SC_TraceParams.bTraceAsyncScene = true;
	SC_TraceParams.bReturnPhysicalMaterial = false;

	// Ignore us and OtherActor in the trace
	SC_TraceParams.AddIgnoredActor(this);
	SC_TraceParams.AddIgnoredActor(OtherActor);

	// Also ignore our vehicle
	ASCDriveableVehicle* Vehicle = GetVehicle();
	if (Vehicle)
		SC_TraceParams.AddIgnoredActor(Vehicle);

	// Perform the ray trace
	FHitResult SC_Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(SC_Hit, OurActorLocation, OtherActorLocation, ECC_Visibility, SC_TraceParams);

	// Did we hit something and is Jason facing the counselor?
	bool MeToOther = AllowFromMesh;
	if (SC_Hit.bBlockingHit || FVector::DotProduct(GetActorForwardVector(), OtherActorLocation - OurActorLocation) < MinFacingAngle)
	{
		// There was something in the way...
		MeToOther = false;
	}

	// Grab our camera orientation
	FVector CameraLocation;
	FRotator CameraRotation;
	if (UCameraComponent* CarCamera = Vehicle ? Vehicle->GetFirstPersonCamera() : nullptr)
	{
		CameraLocation = CarCamera->GetComponentLocation();
		CameraRotation = CarCamera->GetComponentRotation();
	}
	else if (Cast<AAIController>(Controller))
	{
		GetActorEyesViewPoint(CameraLocation, CameraRotation);
	}
	else
	{
		CameraLocation = GetCachedCameraInfo().Location;
		CameraRotation = GetCachedCameraInfo().Rotation;
	}

	// Ray trace from our camera to their center of mass
	SC_Hit = FHitResult(ForceInit);
	GetWorld()->LineTraceSingleByChannel(SC_Hit, CameraLocation, OtherActorLocation, ECC_Visibility, SC_TraceParams);

	// Did we hit something and is the camera facing the counselor?
	bool CameratoOther = AllowFromCamera;
	if (SC_Hit.bBlockingHit || FVector::DotProduct(CameraRotation.Vector(), OtherActorLocation - CameraLocation) < MinFacingAngle)
	{
		// There was something in the way...
		CameratoOther = false;
	}

	return CameratoOther || MeToOther;
}

bool ASCCharacter::SERVER_SetLockOnTarget_Validate(ASCCharacter* Target)
{
	return true;
}

void ASCCharacter::SERVER_SetLockOnTarget_Implementation(ASCCharacter* Target)
{
	LockOntoCharacter = Target;
}

void ASCCharacter::OnStartBlock()
{
	if (CanBlock())
		SERVER_OnBlock(true);
}

void ASCCharacter::OnEndBlock()
{
	SERVER_OnBlock(false);
}

bool ASCCharacter::SERVER_OnBlock_Validate(bool ShouldBlock)
{
	return true;
}

void ASCCharacter::SERVER_OnBlock_Implementation(bool ShouldBlock)
{
	BlockData.bIsBlocking = ShouldBlock;

	if (ShouldBlock)
	{
		if (USkeletalMeshComponent* CurrentMesh = GetPawnMesh())
		{
			if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
			{
				BlockData.BlockAnimSlot = AnimInstance->ChooseBlockAnim();
			}
		}
	}

	OnRep_Block();
}

void ASCCharacter::OnRep_Block()
{
	if (USkeletalMeshComponent* CurrentMesh = GetPawnMesh())
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			if (BlockData.bIsBlocking)
			{

				AnimInstance->PlayBlockAnim(BlockData.BlockAnimSlot);
			}
			else
			{
				AnimInstance->EndBlockAnim(BlockData.BlockAnimSlot);
			}
		}
	}
}

void ASCCharacter::ForceKillCharacter(ASCCharacter* Killer, FDamageEvent DamageEvent/* = FDamageEvent()*/)
{
	if (!IsAlive())
		return;

	// If we're a counselor and we've already escaped please don't kill us again.
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(this))
	{
		if (Counselor->HasEscaped())
			return;
	}

	AController* KillerController = Killer ? Killer->Controller : nullptr;
	if (Killer == this)
	{
		Die(FMath::Square(MaxHealth), DamageEvent, KillerController, Killer);
	}
	else
	{
		// Make sure the amount of damage well exceeds the amount of health we have.
		TakeDamage(FMath::Square(MaxHealth), DamageEvent, KillerController, Killer);
	}
	GetWorldTimerManager().ClearTimer(TimerHandle_ForceKill);
}

TSubclassOf<ASCAfterimage> ASCCharacter::GetAfterImageClass()
{
	return AfterImageClass;
}

void ASCCharacter::EnablePawnCollision_Implementation(bool bEnable)
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		FCollisionResponseContainer CollisionResponse = Capsule->GetCollisionResponseToChannels();
		CollisionResponse.Pawn =				bEnable ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore;
		CollisionResponse.GameTraceChannel13 =	bEnable ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore;
		CollisionResponse.Camera =				bEnable ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore;
		Capsule->SetCollisionResponseToChannels(CollisionResponse);

		if (bEnable)
			Capsule->WakeAllRigidBodies();
	}

	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		FCollisionResponseContainer CollisionResponse = CurrentMesh->GetCollisionResponseToChannels();
		CollisionResponse.Pawn =				bEnable ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore;
		CollisionResponse.GameTraceChannel13 =	bEnable ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore;
		CollisionResponse.Camera =				bEnable ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore;
		CurrentMesh->SetCollisionResponseToChannels(CollisionResponse);

		if (bEnable)
			CurrentMesh->WakeAllRigidBodies();
	}
}

float ASCCharacter::GetForceKillTimerElapsed()
{
	return GetWorldTimerManager().GetTimerElapsed(TimerHandle_ForceKill);
}


void ASCCharacter::BeginSpecialMoveWithCallback(TSubclassOf<UILLSpecialMoveBase> SpecialMove, FSCSpecialMoveCreationDelegate Callback)
{
	// Lets not start another if we're already in one plox.
	if (!SpecialMove || ActiveSpecialMove)
		return;

	StartSpecialMove(SpecialMove);
	if (ActiveSpecialMove)
	{
		Callback.ExecuteIfBound(this);
	}
}

bool ASCCharacter::SERVER_FinishedSpecialMove_Validate(const bool bWasAborted /*= false*/)
{
	return true;
}

void ASCCharacter::SERVER_FinishedSpecialMove_Implementation(const bool bWasAborted /*= false*/)
{
	if (!bClientInSpecialMove)
		return;

	bClientInSpecialMove = false;

	// Sometimes a client can decide to stop a special move and won't tell the server. Make sure the server knows!
	if (USCSpecialMove_SoloMontage* SpecialMove = GetActiveSCSpecialMove())
	{
		if (bWasAborted)
		{
			MULTICAST_AbortSpecialMove(SpecialMove->GetClass());
		}
		else
		{
			SpecialMove->ForceDestroy();
		}
	}

	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		PC->BumpIdleTime();
	}
	// Notify the AI
	else if (ASCCharacterAIController* AIC = Cast<ASCCharacterAIController>(Controller))
	{
		AIC->PawnSpecialMoveFinished(bWasAborted);
	}
}

void ASCCharacter::MULTICAST_AbortSpecialMove_Implementation(TSubclassOf<USCSpecialMove_SoloMontage> MoveClass)
{
	USCSpecialMove_SoloMontage* SpecialMove = GetActiveSCSpecialMove();
	if (!IsValid(SpecialMove))
		return;

	// Special moves aren't replicating, so make sure we've got the right one
	if (SpecialMove->IsA(MoveClass))
	{
		SpecialMove->CancelSpecialMove();
	}
}

USCSpecialMove_SoloMontage* ASCCharacter::GetActiveSCSpecialMove() const
{
	return Cast<USCSpecialMove_SoloMontage>(ActiveSpecialMove);
}

void ASCCharacter::ContextKillStarted()
{
	bInContextKill = true;
	bInvulnerable = true;

	if (USCSpecialMove_SoloMontage* ContextKillMove = GetActiveSCSpecialMove())
	{
		bPlayedDeathAnimation = ContextKillMove->DieOnFinish();
	}

	if (GetAttachParentActor())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		}
	}

	GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = true;
	// We don't want to apply any smoothing to movement during context kills. Let the animation handle it.
	GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
}

void ASCCharacter::ContextKillEnded()
{
	bInContextKill = false;
	bInvulnerable = false;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = false;
	// We don't want to apply any smoothing to movement during context kills. We need to turn it back on afterward.
	GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
}

void ASCCharacter::UpdateLookAtRotation(float DeltaSeconds)
{
	if (IsLocallyControlled())
	{
		FVector EyeLoc;
		FRotator EyeRot;
		GetActorEyesViewPoint(EyeLoc, EyeRot);

		FVector Direction;
		if (AAIController* AIC = Cast<AAIController>(Controller))
		{
			// Stare at the focal point if present
			const FVector FocalPoint = AIC->GetFocalPoint();
			if (FAISystem::IsValidLocation(FocalPoint))
			{
				Direction = (FocalPoint - EyeLoc).GetSafeNormal();
			}
			else
			{
				Direction = GetActorForwardVector();
			}
		}
		else
		{
			Direction = (GetCachedCameraInfo().Location - EyeLoc).GetSafeNormal();
		}
		const FVector Forward = GetActorForwardVector();
		const float ForwardDot = FVector::DotProduct(Direction.GetSafeNormal2D(), Forward);
		const bool bInFront = (ForwardDot >= 0.f);
		const bool bInDeadZone = (FMath::Abs(ForwardDot) < 0.2f);
		const bool bHasTarget = LockOntoCharacter != nullptr && (CanUseHardLock() || CanSoftLockOntoCharacter(LockOntoCharacter));

		FVector TargetLocation;
		FRotator TargetRotation;

		if (bHasTarget)
			LockOntoCharacter->GetActorEyesViewPoint(TargetLocation, TargetRotation);
	
		const FVector ToCam = Direction;
		const FVector FromCam = (-Direction).GetSafeNormal2D();
		const FVector ToTarget = TargetLocation - EyeLoc;
		const FRotator LookAt = FRotationMatrix::MakeFromX( bHasTarget ? ToTarget : (bInFront ? ToCam : FromCam) ).Rotator();

		FRotator FinalRot = GetActorRotation().GetInverse() + LookAt;
		FinalRot.Normalize();
	
		FinalRot.Roll = 0.0f;
		FinalRot.Yaw = bInDeadZone ? 0.f : FMath::ClampAngle(FinalRot.Yaw, -55.0f, 55.0f);
		FinalRot.Pitch = bInDeadZone ? 0.f : (FMath::ClampAngle(FinalRot.Pitch, bInFront || bHasTarget ? -25.0f : 0.0f, 25.0f) * -1.0f);

		FinalRot = FMath::RInterpTo(HeadLookAtRotation, FinalRot, DeltaSeconds, 3.f);

		// Store it off
		HeadLookAtRotation = FinalRot;
		HeadLookAtDirection = HeadLookAtRotation.Vector();

		// Tell the server periodically
		if (Role < ROLE_Authority)
		{
			if (TimeUntilSendLookAtUpdate <= 0.f)
			{
				// Delta-check to avoid superfluous updates
				if (!LastSentHeadLookAtDirection.Equals(HeadLookAtDirection, KINDA_SMALL_NUMBER))
				{
					SERVER_SetHeadLookAtDirection(HeadLookAtDirection);
					TimeUntilSendLookAtUpdate = LookAtTransmitDelay;
					LastSentHeadLookAtDirection = HeadLookAtDirection;
				}
			}
			else
			{
				TimeUntilSendLookAtUpdate -= DeltaSeconds;
			}
		}
	}
	else
	{
		// Remote client, simply lerp the HeadLookAtRotation towards the replicated HeadLookAtDirection
		HeadLookAtRotation = FMath::RInterpTo(HeadLookAtRotation, HeadLookAtDirection.Rotation(), DeltaSeconds, 3.f);
	}
}

bool ASCCharacter::SERVER_SetHeadLookAtDirection_Validate(FVector_NetQuantizeNormal LookAtDirection)
{
	return true;
}

void ASCCharacter::SERVER_SetHeadLookAtDirection_Implementation(FVector_NetQuantizeNormal LookAtDirection)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	HeadLookAtDirection = LookAtDirection;
}

FVector2D ASCCharacter::GetRequestedMoveInput() const
{
	UCharacterMovementComponent* CurrentCharacterMovement = GetCharacterMovement();
	if (IsLocallyControlled())
	{
		if (!CurrentCharacterMovement->RequestedVelocity.IsZero())
			return FVector2D(CurrentCharacterMovement->RequestedVelocity); // Needed for AI
		return CurrentCharacterMovement->IsMoveInputIgnored() ? FVector2D::ZeroVector : FVector2D(CurrentCharacterMovement->GetLastInputVector());
	}

	if (CurrentCharacterMovement->IsFalling())
		return FVector2D::ZeroVector;

	FVector2D RemoteInput = FVector2D(CurrentCharacterMovement->GetCurrentAcceleration());
	const float Length = RemoteInput.Size();

	// If our length is small enough, we're not actually moving.
	if (Length <= KINDA_SMALL_NUMBER)
		return FVector2D::ZeroVector;

	// Otherwise we are, and it needs to be normalized to act like stick deflection
	return RemoteInput * (1.f / Length);
}

void ASCCharacter::OnSkipCutscene()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ConsoleCommand(TEXT("CANCELMATINEE"), true);
	}
}

void ASCCharacter::SetGravityEnabled(bool InEnable)
{
	if (Role < ROLE_Authority)
	{
		// Don't tell me how to live my life!
		if (IsLocallyControlled())
		{
			SERVER_SetGravityEnabled(InEnable);
			GetCharacterMovement()->SetMovementMode(InEnable ? EMovementMode::MOVE_Walking : EMovementMode::MOVE_Flying);
		}
	}
	else
	{
		MULTI_SetGravityEnabled(InEnable);
	}
}

bool ASCCharacter::SERVER_SetGravityEnabled_Validate(bool InEnable)
{
	return true;
}

void ASCCharacter::SERVER_SetGravityEnabled_Implementation(bool InEnable)
{
	SetGravityEnabled(InEnable);
}

void ASCCharacter::MULTI_SetGravityEnabled_Implementation(bool InEnable)
{
	// Set the character to flying to ignore gravity.
	GetCharacterMovement()->SetMovementMode(InEnable ? EMovementMode::MOVE_Walking : EMovementMode::MOVE_Flying);
}

void ASCCharacter::SpawnPolice()
{
	if (Role < ROLE_Authority)
	{
		SERVER_SpawnPolice();
	}
	else if (ASCGameMode* GM = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		GM->SpawnPolice();
	}
}

bool ASCCharacter::SERVER_SpawnPolice_Validate()
{
	return true;
}

void ASCCharacter::SERVER_SpawnPolice_Implementation()
{
	SpawnPolice();
}

void ASCCharacter::ShowHealthBar(bool bShow)
{
	if (IsLocallyControlled())
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
			{
				Hud->OnShowHealthBar(bShow);
			}
		}
	}
}

void ASCCharacter::ShowRandomLevel(bool bShow)
{
	if (IsLocallyControlled())
	{
		bShowRandomLevel = bShow;
	}
}

void ASCCharacter::DrawDebug()
{
#if !UE_BUILD_SHIPPING
	// Displays our spawner locations with arrows and locations
	if (IsLocallyControlled())
		SpawnerDebugger.Draw(GetWorld(), this);

	// Power plant debug pass through
	if (CVarDebugPowerPlant.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		if (ASCGameState* SCGameState = GetWorld() ? Cast<ASCGameState>(GetWorld()->GetGameState()) : nullptr)
			SCGameState->GetPowerPlant()->DebugDraw(GetWorld(), GetActorLocation(), CVarDebugPowerPlant.GetValueOnGameThread());
	}

	// Debug selected sublevel
	if (bShowRandomLevel)
	{
		if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings()))
		{
			if (WorldSettings->RandomLevels.Num() > 0 && WorldSettings->RandomLevels.IsValidIndex(WorldSettings->RandomLevelIndex))
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, TEXT("Random Level: ") + WorldSettings->RandomLevels[WorldSettings->RandomLevelIndex].ToString());
			else
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, TEXT("No random levels set!"));
		}
	}

	// Find the name of the cabin we're inside of
	if (CVarDebugCabin.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		if (OverlappingIndoorComponents.Num() > 0)
		{
			for (const USCIndoorComponent* Component : OverlappingIndoorComponents)
			{
				if (ASCCabin* Cabin = Cast<ASCCabin>(Component->GetOwner()))
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Emerald, FString::Printf(TEXT("%s is inside of %s"), *GetName(), *Cabin->GetName()));
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Emerald, FString::Printf(TEXT("%s is inside of unknown cabin (%s)"), *GetName(), *Component->GetName()));
				}
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, IsIndoors() ? FColor::Red : FColor::Emerald, FString::Printf(TEXT("%s is outside %s"), *GetName(), IsIndoors() ? TEXT("but the game thinks we're inside!") : TEXT("")));
		}
	}

	// Health
	if (CVarDebugHealth.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Health: %.2f"), (float)Health), false);
	}

	// Player perks
	if (CVarDebugPerks.GetValueOnGameThread() > 0)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
		{
			if (PS->GetActivePerks().Num() > 0)
			{
				for (int32 iPerk(0); iPerk < PS->GetActivePerks().Num(); ++iPerk)
				{
					USCPerkData* Perk = PS->GetActivePerks()[iPerk];
					check(Perk);

					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Magenta, FString::Printf(TEXT("Perk %d: %s"), iPerk, *Perk->PerkName.ToString()), false);
				}
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Red, TEXT("ERROR: NO PERKS SET"), false);
			}
		}
	}

	// Stuck issues
	if (CVarDebugStuck.GetValueOnGameThread() >= 1)
	{
		float ZPos = 50.f;
		DrawDebugStuck(ZPos);
	}
#endif //!UE_BUILD_SHIPPING
}

void ASCCharacter::DrawDebugStuck(float& ZPos)
{
#if !UE_BUILD_SHIPPING
	const auto DisplayMessageFunc = [&](const FColor& Color, const FString& Message)
	{
		if (IsLocallyControlled())
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, Color, Message, false);
		}
		else
		{
			DrawDebugString(GetWorld(), FVector(0.f, 0.f, ZPos), Message, this, Color, 0.f, false);
			ZPos -= 15.f;
		}
	};

	USCAnimInstance* SCAnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance());
	DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("DebugStuck: %s"), *GetName()));

	if (!InputEnabled())
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Input Ignored")));

	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		if (PlayerController->IsMoveInputIgnored())
			DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Move Input Ignored")));

		if (IsLocallyControlled())
			PlayerController->DebugDrawInputStack(0.f, FColor::Orange);
	}

	const UEnum* MoveModeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EMovementMode"), true);
	EMovementMode MoveMode = GetCharacterMovement()->MovementMode;

	DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Movement Mode: %s"), *MoveModeEnum->GetNameStringByValue((int32)MoveMode)));

	const UEnum* StanceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ESCCharacterStance"), true);
	DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Stance: %s"), *StanceEnum->GetNameStringByValue((int32)CurrentStance)));

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(SkelMesh->GetAnimInstance()))
		{
			DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Anim Is In Car: %s"), AnimInst->bIsInCar ? TEXT("TRUE") : TEXT("FALSE")));
			DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Anim Is In Boat: %s"), AnimInst->bIsInBoat ? TEXT("TRUE") : TEXT("FALSE")));
		}
	}

	DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Attach Parent: %s"), GetAttachParentActor() ? *GetAttachParentActor()->GetName() : TEXT("NONE")));

	DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Owner : %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("NONE")));

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		for (const auto IgnoredActor : RootPrimitive->GetMoveIgnoreActors())
		{
			if (IgnoredActor)
				DisplayMessageFunc(FColor::Orange, FString::Printf(TEXT("Ignored Actor: %s"), *IgnoredActor->GetName()));
		}
	}

	if (IsStunned())
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Stunned: %s"), SCAnimInstance->StunMontage ? *SCAnimInstance->StunMontage->GetName() : TEXT("NONE")));
	}

	if (ActiveSpecialMove)
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Special Move: %s (%.2fs remaining)"), *ActiveSpecialMove->GetName(), ActiveSpecialMove->GetTimeRemaining()));
	}

	if (IsInContextKill())
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("In Context Kill")));
	}

	if (PickingItem)
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Picking up item: %s"), *PickingItem->GetName()));
	}

	if (SCAnimInstance->bIsMontageUsingRootMotion)
	{
		DisplayMessageFunc(FColor::Red, FString(TEXT("Playing animation with root motion")));
	}

	if (bAttemptingInteract)
	{
		DisplayMessageFunc(FColor::Red, FString(TEXT("Attempting to interact")));
	}

	if (UILLInteractableComponent* LockedInteractable = GetInteractableManagerComponent()->GetLockedInteractable())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		InteractableManagerComponent->GetViewLocationRotation(ViewLocation, ViewRotation);
		const int32 Flags = LockedInteractable->CanInteractWithBroadcast(this, ViewLocation, ViewRotation);

		FString Interactions;
		if (Flags)
		{
			const UEnum* InteractionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EILLEditorInteractMethod"), true);
			for (int32 iMethod(0); iMethod < (int32)EILLEditorInteractMethod::EIM_MAX; ++iMethod)
			{
				if (Flags & (1 << iMethod))
				{
					Interactions += InteractionEnum->GetNameStringByValue(iMethod);
					Interactions += TEXT(" ");
				}
			}

			Interactions.TrimEnd();
		}
		else
		{
			// No flags, no available interactions
			Interactions = TEXT("None");
		}

		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Interaction locked with %s:%s (%s)"),
			*LockedInteractable->GetName(), *LockedInteractable->GetOwner()->GetName(), *Interactions));

		// WHAT THE FUCK MAN?
		if (ASCCabinet* LockedCabinet = Cast<ASCCabinet>(LockedInteractable->GetOwner()))
		{
			LockedCabinet->DebugDraw(this);
		}
	}
#endif //!UE_BUILD_SHIPPING
}

bool ASCCharacter::IsPerformingThrow() const
{
	return CurrentThrowable != nullptr;
}

int32 ASCCharacter::TriggerTrap(ASCTrap* InTriggeredTrap, FVector& FootLocation)
{
	check(Role == ROLE_Authority);

	TriggeredTrap = InTriggeredTrap;
	OnRep_TriggeredTrap();

	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (SkelMesh == nullptr)
		return -1;

	FVector LeftFootLocation = SkelMesh->GetSocketLocation(LeftFootSocketName);
	FVector RightFootLocation = SkelMesh->GetSocketLocation(RightFootSocketName);
	FVector TrapLocation = TriggeredTrap->GetActorLocation();

	if (FVector::DistSquared(TrapLocation, LeftFootLocation) < FVector::DistSquared(TrapLocation, RightFootLocation))
	{
		TriggeredTrap->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, LeftFootSocketName);
		FootLocation = LeftFootLocation;
		return 0;
	}
	else
	{
		TriggeredTrap->AttachToComponent(SkelMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, RightFootSocketName);
		FootLocation = RightFootLocation;
		return 1;
	}
}

void ASCCharacter::OnRep_TriggeredTrap()
{
	if (TriggeredTrap == nullptr)
	{
		bInStun = false;
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			PC->PopInputComponent(WiggleInputComponent);
			PC->SetIgnoreMoveInput(false);

			if (bWiggleGameActive)
			{
				if (ASCInGameHUD* Hud = PC->GetSCHUD())
				{
					Hud->OnEndWiggleMinigame();
				}
			}
		}

		bWiggleGameActive = false;
	}
}

void ASCCharacter::EscapeTrap(bool bPlayEscapeAnimation /*= true*/)
{
	TriggeredTrap->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	TriggeredTrap->ResetRotation();

	if (Role == ROLE_Authority)
	{
		if (!IsKiller() && bPlayEscapeAnimation)
			TriggeredTrap->MULTICAST_PlayEscape();

		TriggeredTrap = nullptr;
		OnRep_TriggeredTrap();
		bInStun = false;
	}
}

void ASCCharacter::SetRainEnabled(bool bRainEnabled)
{
	// Nothing to update
	if (bRainEnabled == bIsRaining)
		return;

	// Update our internal variable and our sounds
	bIsRaining = bRainEnabled;
}

void ASCCharacter::SpawnPostProcessVolume()
{
	if (!ForcedPostProcessVolume && Controller && Controller->IsLocalPlayerController())
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = this;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ForcedPostProcessVolume = GetWorld()->SpawnActor<APostProcessVolume>(SpawnInfo);

		if (ForcedPostProcessVolume)
		{
			ForcedPostProcessVolume->bUnbound = true;
			ForcedPostProcessVolume->Priority = 100.0f;
		}
	}
}

void ASCCharacter::StartSpecialMove(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor/* = nullptr*/, const bool bViaReplication/* = false*/)
{
	if (!SpecialMove)
		return;

	if (!bViaReplication)
	{
		if (CanStartSpecialMove(SpecialMove, TargetActor))
			ServerStartSpecialMove(SpecialMove, TargetActor);
		return;
	}

	TSubclassOf<UILLSpecialMoveBase> MatchingMove = nullptr;
	for (TSubclassOf<UILLSpecialMoveBase> MoveClass : AllowedSpecialMoves)
	{
		if (MoveClass == SpecialMove->GetClass())
		{
			MatchingMove = MoveClass;
			break;
		}
		else if (MoveClass->IsChildOf(SpecialMove))
		{
			MatchingMove = MoveClass;
		}
		else if (SpecialMove->IsChildOf(MoveClass))
		{
			MatchingMove = SpecialMove;
			break;
		}
	}

	if (MatchingMove)
	{
		// If we already had a special move, stop paying attention to the completion so we don't clear our NEW special move instead
		if (ActiveSpecialMove)
		{
			ActiveSpecialMove->RemoveCompletionDelegates(this);
			if (USCSpecialMove_SoloMontage* SCMove = Cast<USCSpecialMove_SoloMontage>(ActiveSpecialMove))
			{
				SCMove->CancelSpecialMoveDelegate.RemoveAll(this);
			}

			if (CVarDebugStuck.GetValueOnGameThread() >= 1 && IsLocallyControlled())
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Special move (%s -- %.2f remaining) still running! Will be orphaned!"), *ActiveSpecialMove->GetName(), ActiveSpecialMove->GetTimeRemaining()), false);
			}
		}

		ActiveSpecialMove = NewObject<UILLSpecialMoveBase>(this, MatchingMove);
		ActiveSpecialMove->BeginMove(TargetActor, FOnILLSpecialMoveCompleted::FDelegate::CreateUObject(this, &ASCCharacter::OnSpecialMoveCompleted));

		// If our special move gets aborted, we need to know about that too
		if (USCSpecialMove_SoloMontage* SCMove = Cast<USCSpecialMove_SoloMontage>(ActiveSpecialMove))
		{
			SCMove->CancelSpecialMoveDelegate.AddDynamic(this, &ASCCharacter::OnSpecialMoveCanceled);
		}

		OnSpecialMoveStarted();

		if (IsLocallyControlled() || Role == ROLE_Authority)
			bClientInSpecialMove = true;
	}
	else
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FUObjectToken::Create(GetClass()))
			->AddToken(FTextToken::Create(FText::FromString(TEXT(" cannot use special move "))))
			->AddToken(FUObjectToken::Create(SpecialMove->GetClass()));
#endif
	}
}

void ASCCharacter::ServerStartSpecialMove_Implementation(TSubclassOf<UILLSpecialMoveBase> SpecialMove, AActor* TargetActor)
{
	Super::ServerStartSpecialMove_Implementation(SpecialMove, TargetActor);

	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();
}

void ASCCharacter::OnSpecialMoveStarted()
{
	Super::OnSpecialMoveStarted();

	// Exit combat when starting a special move so that we don't get stuck locking onto someone
	if (CurrentStance == ESCCharacterStance::Combat)
	{
		SetCombatStance(false);
	}

	// Notify the AI
	if (ASCCharacterAIController* AIC = Cast<ASCCharacterAIController>(Controller))
	{
		AIC->PawnSpecialMoveStarted();
	}

	if (CVarDebugStuck.GetValueOnGameThread() >= 1 && IsLocallyControlled())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Orange, FString::Printf(TEXT("Special Move Started: %s @ %.3f"), *GetNameSafe(ActiveSpecialMove), CurrentTime), false);
	}
}

void ASCCharacter::OnSpecialMoveCompleted()
{
	if (CVarDebugStuck.GetValueOnGameThread() >= 1 && IsLocallyControlled())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Orange, FString::Printf(TEXT("Special Move Completed: %s (%.2f remaining) @ %.3f"), *GetNameSafe(ActiveSpecialMove), ActiveSpecialMove ? ActiveSpecialMove->GetTimeRemaining() : 0.f, CurrentTime), false);
	}

	// If we ain't got no world or we gonna die, don't worry about clearing the active special move
	if (GetWorld() && !IsPendingKill())
	{
		// Delay clearing the special move for a tick so that we can use it in other complete/cancel callbacks (SLOP ASS HACK)
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::SpecialMoveSlopAssHack);
	}

	if (IsLocallyControlled())
	{
		bClientInSpecialMove = false;
	}

	if (IsLocallyControlled() || Role == ROLE_Authority)
	{
		SERVER_FinishedSpecialMove(/*bWasAborted=*/false);
	}
}

void ASCCharacter::OnSpecialMoveCanceled()
{
	if (CVarDebugStuck.GetValueOnGameThread() >= 1 && IsLocallyControlled())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Special Move Canceled: %s (%.2f remaining) @ %.3f"), *GetNameSafe(ActiveSpecialMove), ActiveSpecialMove ? ActiveSpecialMove->GetTimeRemaining() : 0.f, CurrentTime), false);
	}

	// If we ain't got no world, don't worry about clearing the active special move
	if (GetWorld())
	{
		// Delay clearing the special move for a tick so that we can use it in other complete/cancel callbacks (SLOP ASS HACK)
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::SpecialMoveSlopAssHack);
	}

	if (IsLocallyControlled())
	{
		bClientInSpecialMove = false;
	}
	
	if (IsLocallyControlled() || Role == ROLE_Authority)
	{
		SERVER_FinishedSpecialMove(/*bWasAborted=*/true);
	}
}

void ASCCharacter::SpecialMoveSlopAssHack()
{
	Super::OnSpecialMoveCompleted();
}

void ASCCharacter::CLIENT_SetSkeletonToCinematicMode_Implementation(bool bForce /*= false*/)
{
	// If we're already evaluting, do this next frame (prevents a recursion assert)
	if (GetMesh()->IsPostEvaluatingAnimation())
	{
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &ThisClass::CLIENT_SetSkeletonToCinematicMode, bForce);
			World->GetTimerManager().SetTimerForNextTick(Delegate);
		}

		return;
	}

	if (IsLocallyControlled() || bForce)
	{
		// Force all bones to follow animation exactly so things line up 100%
		GetMesh()->SkeletalMesh->Skeleton->SetBoneTranslationRetargetingMode(0, EBoneTranslationRetargetingMode::Animation, true);
		GetMesh()->RefreshBoneTransforms();
		GetMesh()->RecalcRequiredBones(0);
	}
}

void ASCCharacter::CLIENT_SetSkeletonToGameplayMode_Implementation(bool bForce /*= false*/)
{
	// If we're already evaluting, do this next frame (prevents a recursion assert)
	if (GetMesh()->IsPostEvaluatingAnimation())
	{
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &ThisClass::CLIENT_SetSkeletonToGameplayMode, bForce);
			World->GetTimerManager().SetTimerForNextTick(Delegate);
		}

		return;
	}

	if (GetMesh()->IsRegistered() && (IsLocallyControlled() || bForce))
	{
		const int32 NumBones = GetMesh()->GetNumBones();

		for (int32 index = 0; index < NumBones; ++index)
		{
			const int32 SkelIndex = GetMesh()->SkeletalMesh->Skeleton->GetSkeletonBoneIndexFromMeshBoneIndex(GetMesh()->SkeletalMesh, index);
			if (ForcedAnimationBones.Contains(GetMesh()->GetBoneName(index)))
			{
				GetMesh()->SkeletalMesh->Skeleton->SetBoneTranslationRetargetingMode(SkelIndex, EBoneTranslationRetargetingMode::Animation, false);
				continue;
			}

			GetMesh()->SkeletalMesh->Skeleton->SetBoneTranslationRetargetingMode(SkelIndex, EBoneTranslationRetargetingMode::Skeleton, false);
		}
		GetMesh()->RefreshBoneTransforms();
		GetMesh()->RecalcRequiredBones(0);
	}
}

void ASCCharacter::MULTICAST_PlayPoliceArriveSFX_Implementation()
{
	if (GetWorld() && IsLocallyControlled())
	{
		UGameplayStatics::PlaySound2D(GetWorld(), PoliceArriveCue);

		if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
		{
			if (ASCInGameHUD* Hud = PC->GetSCHUD())
			{
				Hud->OnShowPoliceHasBeenCalled(true);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// RPCs for other actors

// -- Doors
bool ASCCharacter::SERVER_OpenDoor_Validate(ASCDoor* Door, const bool bOpen)
{
	return true;
}

void ASCCharacter::SERVER_OpenDoor_Implementation(ASCDoor* Door, const bool bOpen)
{
	Door->ForceNetUpdate();
	Door->bIsOpen = bOpen;
	Door->OnRep_IsOpen();
}

bool ASCCharacter::SERVER_DestroyDoor_Validate(ASCDoor* Door, const float Impulse)
{
	return true;
}

void ASCCharacter::SERVER_DestroyDoor_Implementation(ASCDoor* Door, const float Impulse)
{
	if (Door->bIsDestroyed)
		return;

	Door->MULTICAST_DestroyDoor(this, Impulse);

	// Track number of doors destroyed
	if (ASCPlayerState* PS = GetPlayerState())
	{
		PS->BrokeDoorDown();
		PS->EarnedBadge(Door->BrokeDoorDownBadge);
	}
}

void ASCCharacter::SetStartingWaypoint(ASCWaypoint* Waypoint)
{
	StartingWaypoint = Waypoint;

	if (ASCCharacterAIController* AIController = Cast<ASCCharacterAIController>(Controller))
		AIController->SetStartingWaypoint(StartingWaypoint);
}

void ASCCharacter::SetOverrideNextWaypoint(ASCWaypoint* Waypoint)
{
	if (ASCCharacterAIController* AIController = Cast<ASCCharacterAIController>(Controller))
		AIController->SetOverrideNextWaypoint(Waypoint);
}

ASCWaypoint* ASCCharacter::GetOverrideNextWaypoint() const
{
	if (ASCCharacterAIController* AIController = Cast<ASCCharacterAIController>(Controller))
		return Cast<ASCWaypoint>(AIController->GetOverrideNextWaypoint());

	return nullptr;
}

void ASCCharacter::SetCharacterDisabled(bool Disable)
{
	if (Role == ROLE_Authority)
	{
		bDisabled = Disable;
		OnRep_Disabled();
	}
}

bool ASCCharacter::SERVER_SetCharacterDisabled_Validate(bool Disable)
{
	return true;
}

void ASCCharacter::SERVER_SetCharacterDisabled_Implementation(bool Disable)
{
	SetCharacterDisabled(Disable);
}

void ASCCharacter::OnRep_Disabled()
{
	SetActorHiddenInGame(bDisabled);

	SetActorEnableCollision(!bDisabled);

	GetCharacterMovement()->GravityScale = bDisabled ? 0.0f : 1.0f;

	if (!GetWorld()->GetGameState()->IsA(ASCGameState_Paranoia::StaticClass()))
	{
		if (MinimapIconComponent)
			MinimapIconComponent->SetVisibility(!bDisabled);
	}
}

void ASCCharacter::OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted)
{
	// If we got here and we think we were playing a pickup animation, it was probably interrupted
	// Go ahead and pretend we finished now
	if (PickingItem && Role == ROLE_Authority)
	{
		AddOrSwapPickingItem();
	}
}