// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorCharacter.h"

// UE4
#include "Animation/AnimMontage.h"
#include "AssetRegistryModule.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "OnlineKeyValuePair.h"
#include "Engine/TextureRenderTarget2D.h"
#include "WidgetComponent.h"

#if WITH_EDITOR
# include "MessageLog.h"
# include "UObjectToken.h"
#endif

// IGF
#include "ILLGameBlueprintLibrary.h"
#include "ILLIKFinderComponent.h"
#include "IllMinimapWidget.h"
#include "ILLPlayerInput.h"

// SC
#include "SCDoubleCabinet.h"
#include "SCCharacterBodyPartComponent.h"
#include "SCCharacterMovement.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAIController.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorWardrobe.h"
#include "SCDriveableVehicle.h"
#include "SCEmoteData.h"
#include "SCFlareGun.h"
#include "SCGameInstance.h"
#include "SCGameMode_Hunt.h"
#include "SCGameMode_Paranoia.h"
#include "SCGameState_Hunt.h"
#include "SCGameState_Sandbox.h"
#include "SCGameState_Paranoia.h"
#include "SCGameNetworkManager.h"
#include "SCGun.h"
#include "SCHidingSpot.h"
#include "SCIndoorComponent.h"
#include "SCInGameHUD.h"
#include "SCInteractableManagerComponent.h"
#include "SCInventoryComponent.h"
#include "SCItem.h"
#include "SCKillerCharacter.h"
#include "SCKillerMask.h"
#include "SCMinimapIconComponent.h"
#include "SCMusicComponent_Counselor.h"
#include "SCNameplateComponent.h"
#include "SCPamelaSweater.h"
#include "SCPerkData.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCPocketKnife.h"
#include "SCRadio.h"
#include "SCSpecialItem.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSpecialMoveComponent.h"
#include "SCStunDamageType.h"
#include "SCTrap.h"
#include "SCVehicleStarterComponent.h"
#include "SCVoiceOverComponent_Counselor.h"
#include "SCWeapon.h"
#include "SCWindow.h"
#include "SCGameSession.h"
#include "SCImposterOutfit.h"
#include "SCPlayerController_Paranoia.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCCounselorCharacter"

TAutoConsoleVariable<int32> CVarDebugStumbling(TEXT("sc.DebugStumbling"), 0,
	TEXT("Displays debug information for stumbling.\n")
	TEXT(" 0: None\n")
	TEXT(" 1: Display debug info"));

TAutoConsoleVariable<int32> CVarDebugFirstAid(TEXT("sc.DebugFirstAid"), 0,
	TEXT("Displays debug information for firt aid healing (others)\n")
	TEXT(" 0: None\n")
	TEXT(" 1: Display debug info"));

TAutoConsoleVariable<int32> CVarDebugInfAmmo(TEXT("sc.InfiniteAmmo"), 0,
	TEXT("Enable Infinite ammo for the shotgun and flare gun\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On"));

TAutoConsoleVariable<int32> CVarDebugStamina(TEXT("sc.DebugStamina"), 0,
	TEXT("Display debug info for Stamina\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On"));

TAutoConsoleVariable<int32> CVarDebugRepair(TEXT("sc.DebugRepair"), 0,
	TEXT("Display debug info for Repair Speed\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On"));

static const FName NAME_root(TEXT("root"));
static const FName NAME_calf_r(TEXT("calf_r"));
static const FName NAME_calf_l(TEXT("calf_l"));
static const FName NAME_hand_r(TEXT("hand_r"));
static const FName NAME_hand_l(TEXT("hand_l"));
static const FName NAME_head(TEXT("head"));
static const FName NAME_spine_01(TEXT("spine_01"));
static const FName NAME_upperarm_r(TEXT("upperarm_r"));
static const FName NAME_upperarm_l(TEXT("upperarm_l"));

static const FName NAME_KillerHandSocket(TEXT("handGrabSocket"));
static const FName NAME_NeckSocket(TEXT("NeckGrabSocket"));

const float DEFAULT_CLOTHING_DELAY = 1.f;

ASCCounselorCharacter::ASCCounselorCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
// Health
, WoundedThreshold(0.5f)

// Stamina
, CurrentStamina(100.f, 100.f)
, StaminaMax(100.f)
, StaminaMinThreshold(10.f)
, StaminaDepleteRate(1.f)
//, StaminaRefillRate(1.f)
, StaminaIdleModifier(1.f)
, StaminaHidingModifier(1.5f)
, StaminaWalkingModifier(0.f)
, StaminaRunningModifier(1.2f)
, StaminaSprintingModifier(2.f)
, StaminaSwimmingModifier(3.2f)
, StaminaCrouchingModifier(1.2f)

// UNSORTED :(
, HidingSensitivityMod(0.5f)
, MinLightStrength(0.3f)
, MaxSmallItems(3)
, bIsFemale(true)
, DodgingSpeedMod(2.0f)
, StumblingSpeedMod(0.75f)
, WaterSpeedMod(0.7f)
, MinWaterDepthForSwimming(125.f)
, CrouchCapsuleRadius(45.f)
, FirstAidDistance(200.f)
, FirstAidAngle(75.f)
, MaxVehicleCameraYawAngle(70.f)
, MaxVehicleCameraPitchAngle(40.f)
, CameraRotateScale(0.5f, 0.5f)
, PocketKnifeDetachDelay(0.5f)
, MinKillerViewAngle(0.55f)

// Chase
, StartChaseKillerAngle(45.f)
, EndChaseKillerAngle(90.f)
, StartChaseDistance(1500.f)
, EndChaseDistance(3000.f)
, ChaseTimeout(2.f)
, LastChased_Timestamp(-FLT_MAX)

// Fear
, JasonNearMaxRange(300.f)
, JasonNearMinRange(50.f)
, MaxCorpseSpottedJumpDistance(1000.f)

// Combat
, HeavyAttackModifier(2.0f)
, WoundedStunMod(0.75f)
, RageGrabStunMod(0.5f)


// Noise Making
, CrouchingNoiseMod(1.f)
, WalkingNoiseMod(2.f)
, RunningNoiseMod(3.f)
, SprintingNoiseMod(4.f)
, SwimmingNoiseMod(10.f)

// Stumbling
, StumbleWoundedChance(3.f)
, StumbleCooldown(15.f)
, bForceStumbleOnStaminaDepletion(true)
, DropItemHoldTime(1.5f)

// Dead friendlies
, DeadFriendliesCooldownTime(10.f)

// Hiding|Breathing
, MaxBreath(100.f)
, MinHoldBreathThreshold(50.f)
, MinHoldBreathTimer(1.5f)

// Clothing
, AlphaMaskMaterialParameter(TEXT("AlphaMask"))
, SkinTextureMaterialParameter(TEXT("Skin Diffuse"))
, SkinMaterialIndex(0)
, CurrentOutfitLoadingHandle(INDEX_NONE)
, bAppliedOutfit(false)

// Emotes
, EmoteHoldDelayController(0.15f)
, EmoteHoldDelayKeyboard(0.f)
{
	// Keep in sync with ASCCounselorPlayerStart's capsule size!
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// Set our default mesh so we can attach to sockets/bones
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultMesh(TEXT("/Game/Characters/Counselors/Athlete/Mesh/Skeletal/Counselor_Female_Athlete_SM.Counselor_Female_Athlete_SM"));
	GetMesh()->SetSkeletalMesh(DefaultMesh.Object);

	static const FName NAME_LightMesh(TEXT("LightMesh"));
	LightMesh = CreateDefaultSubobject<UStaticMeshComponent>(NAME_LightMesh);
	LightMesh->SetRelativeLocationAndRotation(FVector(3.5f, 8.f, 9.f), FQuat(FRotator(0.f, 90.f, -90.f)));
	LightMesh->SetupAttachment(GetMesh(), TEXT("pelvis"));

	static const FName NAME_WalkieTalkie(TEXT("WalkieTalkie"));
	WalkieTalkie = CreateDefaultSubobject<UStaticMeshComponent>(NAME_WalkieTalkie);
	WalkieTalkie->SetupAttachment(GetMesh(), TEXT("walkieTalkie_Socket"));

	// Flashlight Component
	static const FName NAME_Flashlight(TEXT("Flashlight"));
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(NAME_Flashlight);
	Flashlight->SetupAttachment(GetMesh()); // Attach to ROOT
	Flashlight->SetRelativeLocationAndRotation(FVector(-3.5f, 50.f, 88.f), FQuat(FRotator(0.f, 90.f, 0.f)));
	FlashlightLerpSpeed = 15.f;

	// Create the Voice Over Component for the counselor
	static const FName NAME_VoiceOverComponent(TEXT("VoiceOverComponent_Counselor"));
	VoiceOverComponent = CreateDefaultSubobject<USCVoiceOverComponent_Counselor>(NAME_VoiceOverComponent);

	// Fear Component
	static const FName NAME_FearManager(TEXT("FearComponent"));
	FearManager = CreateDefaultSubobject<USCFearComponent>(NAME_FearManager);

	// Create the Music Component.
	static const FName NAME_MusicComponent(TEXT("MusicComponent_Counselor"));
	MusicComponent = CreateDefaultSubobject<USCMusicComponent_Counselor>(NAME_MusicComponent);

	// Special Inventory Component
	static const FName NAME_SpecialItemInventory(TEXT("SpecialItemInventory"));
	SpecialItemInventory = CreateDefaultSubobject<USCInventoryComponent>(NAME_SpecialItemInventory);
	SpecialItemInventory->bAutoEquip = false;

	// Small Inventory Component
	static const FName NAME_SmallItemInventory(TEXT("SmallItemInventory"));
	SmallItemInventory = CreateDefaultSubobject<USCInventoryComponent>(NAME_SmallItemInventory);
	SmallItemInventory->bAutoEquip = false;

	static const FName NAME_NameplateWidget(TEXT("PlayerNameplate"));
	NameplateWidget = CreateDefaultSubobject<USCNameplateComponent>(NAME_NameplateWidget);
	NameplateWidget->SetupAttachment(GetMesh(), NAME_head);
	NameplateWidget->bOutOfRangeWhenMatchNotInProgress = true;

	// Stamina
	GrabbedStaminaRefillRate.BaseValue = 2.5f;
	GrabbedStaminaRefillRate.StatModifiers[(uint8)ECounselorStats::Strength] = 15.f;
	GrabbedStaminaRefillRate.StatModifiers[(uint8)ECounselorStats::Composure] = 15.f;

	StaminaRefillRate.BaseValue = 2.0f;
	StaminaRefillRate.StatModifiers[(uint8)ECounselorStats::Stamina] = .2f;
	StaminaRefillRate.StatModifiers[(uint8)ECounselorStats::Composure] = .2f;

	// Combat
	DodgeStaminaCost.BaseValue = 20.f;
	DodgeStaminaCost.StatModifiers[(uint8)ECounselorStats::Strength] = 35.f;
	BlockStaminaCost.BaseValue = 10.f;
	BlockStaminaCost.StatModifiers[(uint8)ECounselorStats::Strength] = 35.f;
	HitKillerStaminaBoost.BaseValue = 50.f;
	HitKillerStaminaBoost.StatModifiers[(uint8)ECounselorStats::Composure] = 25.f;
	BreakFreeStaminaBoost.BaseValue = 50.f;
	BreakFreeStaminaBoost.StatModifiers[(uint8)ECounselorStats::Composure] = 25.f;
	WeaponDurabilityMod.BaseValue = 1.f;
	BlockingDamageMod.BaseValue = 1.f;
	StunnedDamageMod.BaseValue = 1.f;

	// Stumble
	StumbleStatMod.BaseValue = 1.f;
	StumbleStatMod.StatModifiers[(uint8)ECounselorStats::Composure] = 15.f;
	StumbleStatMod.StatModifiers[(uint8)ECounselorStats::Luck] = 5.f;

	// Face is special case limb
	static const FName NAME_FaceLimb(TEXT("Face"));
	FaceLimb = CreateDefaultSubobject<USCCharacterBodyPartComponent>(NAME_FaceLimb);
	FaceLimb->BoneToHide = NAME_head;
	FaceLimb->SetupAttachment(GetMesh());
	FaceLimb->SetLimbType(ELimb::HEAD);

	// All other lesser limbs
	const auto SetupLimb = [this](USCCharacterBodyPartComponent*& Limb, const FName& ObjectName, const FName& AttachSocket, USkeletalMeshComponent* ParentMesh, const ELimb& LimbType)
	{
		Limb = CreateDefaultSubobject<USCCharacterBodyPartComponent>(ObjectName);
		Limb->SetupAttachment(ParentMesh, AttachSocket);
		Limb->SetMasterPoseComponent(ParentMesh);
		Limb->SetVisibility(false, true);
		Limb->SetLimbType(LimbType);
	};

	USkeletalMeshComponent* CharMesh = GetMesh();
	SetupLimb(TorsoLimb, TEXT("Torso"), NAME_root, CharMesh, ELimb::TORSO);
	SetupLimb(RightArmLimb, TEXT("RightArm"), NAME_upperarm_r, CharMesh, ELimb::RIGHT_ARM);
	SetupLimb(LeftArmLimb, TEXT("LeftArm"), NAME_upperarm_l, CharMesh, ELimb::LEFT_ARM);
	SetupLimb(RightLegLimb, TEXT("RightLeg"), NAME_calf_r, CharMesh, ELimb::RIGHT_LEG);
	SetupLimb(LeftLegLimb, TEXT("LeftLeg"), NAME_calf_l, CharMesh, ELimb::LEFT_LEG);

	static const FName NAME_Hair(TEXT("Hair"));
	Hair = CreateDefaultSubobject<USkeletalMeshComponent>(NAME_Hair);
	Hair->SetupAttachment(FaceLimb);

	CounselorOutfitComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CounselorOutfitComponent"));
	CounselorOutfitComponent->SetMasterPoseComponent(CharMesh);
	CounselorOutfitComponent->SetupAttachment(CharMesh);

	BreathHoldModifier.BaseValue = 10.f;
	BreathHoldModifier.StatModifiers[(uint8)ECounselorStats::Stealth] = 1.f;

	BreathRechargeModifier.BaseValue = 10.f;
	BreathRechargeModifier.StatModifiers[(uint8)ECounselorStats::Composure] = 1.f;

	BreathNoiseMod.BaseValue = 1;
	BreathNoiseMod.StatModifiers[(uint8)ECounselorStats::Stealth] = .2f;

	OriginalCapsuleRadius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	AimPitch.SetValueWidth(180.0f, -180.0f);

	SpawnText = LOCTEXT("DefaultSpawnText", "5 minutes later... Survive!");
}

void ASCCounselorCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCCounselorCharacter, CurrentStamina);
	DOREPLIFETIME(ASCCounselorCharacter, GrabbedBy);
	DOREPLIFETIME(ASCCounselorCharacter, bIsStumbling);
	DOREPLIFETIME(ASCCounselorCharacter, SmallItemIndex);
	DOREPLIFETIME(ASCCounselorCharacter, bIsFlashlightOn);
	DOREPLIFETIME(ASCCounselorCharacter, bDroppingSmallItem);
	DOREPLIFETIME(ASCCounselorCharacter, bDroppingLargeItem);
	DOREPLIFETIME_CONDITION(ASCCounselorCharacter, AimPitch, COND_SkipOwner);
	DOREPLIFETIME(ASCCounselorCharacter, bLargeItemPressHeld);
	DOREPLIFETIME(ASCCounselorCharacter, bIsHoldingBreath);
	DOREPLIFETIME(ASCCounselorCharacter, CurrentBreath);
	DOREPLIFETIME(ASCCounselorCharacter, bCanHoldBreath);
	DOREPLIFETIME(ASCCounselorCharacter, bLockStamina);
	DOREPLIFETIME(ASCCounselorCharacter, bIsWounded);
	DOREPLIFETIME(ASCCounselorCharacter, bWearingSweater);
	DOREPLIFETIME(ASCCounselorCharacter, bHideLargeItemOverride);
	DOREPLIFETIME(ASCCounselorCharacter, bInKillerHand);
	DOREPLIFETIME(ASCCounselorCharacter, bInHiding);

	DOREPLIFETIME(ASCCounselorCharacter, CurrentOutfit);

#if !UE_BUILD_SHIPPING
	DOREPLIFETIME(ASCCounselorCharacter, bUnlimitedSweaterStun);
#endif
}

void ASCCounselorCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (FaceLimb->SkeletalMesh)
	{
		FaceLimb->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("headSocket"));
		Hair->AttachToComponent(FaceLimb,  FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("headSocket"));
	}

	if (VoiceOverObjectClass)
	{
		VoiceOverComponent->InitVoiceOverComponent(VoiceOverObjectClass);
	}

	if (ActiveAbilityClass)
	{
		ActiveAbility = NewObject<USCCounselorActiveAbility>(this, ActiveAbilityClass);
		ActiveAbility->SetAbilityOwner(this);
	}
}

void ASCCounselorCharacter::SetActorHiddenInGame(bool bNewHidden)
{
	// In general you should rely on the IGF and use ShouldShowCharacter() and UpdateCharacterMeshVisibility()
	// but.... we really don't want to show naked characters
	if (!bAppliedOutfit)
		bNewHidden = true;

	Super::SetActorHiddenInGame(bNewHidden);

	// make sure the hair doesnt cast shadows
	if (Hair)
		Hair->SetCastShadow(!bNewHidden);

	if (FaceLimb)
		FaceLimb->SetCastShadow(!bNewHidden);

	if (GetEquippedItem())
		GetEquippedItem()->SetActorHiddenInGame(bNewHidden);
}

void ASCCounselorCharacter::BeginPlay()
{
	Super::BeginPlay();

	FearManager->PushFearEvent(DarkFearEvent);

	FlashlightMeshBaseRotation = LightMesh->GetComponentRotation() - GetMesh()->GetComponentRotation();
	FlashlightMeshBaseRotation.Normalize();
	FlashlightCurrentRotation = FRotator::ZeroRotator;
	FlashlightBaseRotation = Flashlight->GetComponentRotation() - GetMesh()->GetComponentRotation();
	FlashlightBaseRotation.Normalize();

	FlashlightBaseLocation = Flashlight->RelativeLocation;
	FlashlightDesiredLocation = FlashlightBaseLocation;
	FlashlightLerpStartLocation = FlashlightBaseLocation;
	FlashlightLerpStartRotation = FRotator::ZeroRotator;
	FlashlightDesiredRotationOffset = FRotator::ZeroRotator;

	SetSenseEffects(false);

	CurrentBreath = MaxBreath;
	bCanHoldBreath = true;

#if !UE_BUILD_SHIPPING
	RegisterPerkCommands();
#endif

	// shut off collision for all components on the actor except the primary mesh and capsule component
	TArray<UActorComponent*> Components = GetComponentsByClass(UPrimitiveComponent::StaticClass());
	for (UActorComponent* Prim : Components)
	{
		if (Prim == GetMesh() || Prim == GetCapsuleComponent() || Prim == FaceLimb)
			continue;

		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Prim))
			PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorTransform(GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);

	// Fallback for not getting any clothes
	if (!bAppliedOutfit)
	{
		if (IsRunningDedicatedServer())
		{
			bAppliedOutfit = true;
			UpdateCharacterMeshVisibility();
		}
		else if (!CurrentOutfit.ClothingClass)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_RevertToDefaultOutfit, this, &ThisClass::RevertToDefaultOutfit, DEFAULT_CLOTHING_DELAY);
		}
		else // we had an outfit set from blueprint, probably for ai. Make sure we load this outfit
		{
			// Set our new outfit and never go back to default
			GetWorldTimerManager().ClearTimer(TimerHandle_RevertToDefaultOutfit);

			// Make sure we don't load any old pending outfit
			USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
			USCCounselorWardrobe* Wardrobe = GameInstance->GetCounselorWardrobe();
			if (CurrentOutfitLoadingHandle != INDEX_NONE)
				Wardrobe->CancelLoadingOutfit(CurrentOutfitLoadingHandle);

			// Hide our mesh
			bAppliedOutfit = false;
			UpdateCharacterMeshVisibility();

			// Finally, start our async load of our new outfit
			FSCClothingLoaded Callback;
			Callback.BindDynamic(this, &ThisClass::DeferredLoadOutfit);
			TArray<TAssetPtr<UObject>> AdditionalDependancies;
			AdditionalDependancies.Add(AlphaMaskApplicationMaterial);
			CurrentOutfitLoadingHandle = Wardrobe->StartLoadingOutfit(CurrentOutfit, Callback, AdditionalDependancies);
		}
	}
}

void ASCCounselorCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(TimerHandle_SeeDeadFriendliesCooldown);
		TimerManager.ClearTimer(TimerHandle_VOCooldown);
		TimerManager.ClearTimer(TimerHandle_Escape);
		TimerManager.ClearTimer(TimerHandle_BreakOut);
		TimerManager.ClearTimer(TimerHandle_Stumble);
		TimerManager.ClearTimer(TimerHandle_DropSmallItem);
		TimerManager.ClearTimer(TimerHandle_DelayVO);
		TimerManager.ClearTimer(TimerHandle_RevertToDefaultOutfit);
		TimerManager.ClearTimer(TimerHandle_Emote);
		TimerManager.ClearTimer(TimerHandle_EmoteBlockInput);
		TimerManager.ClearTimer(TimerHandle_ShowEmoteSelect);
		TimerManager.ClearTimer(TimerHandle_ResetBuoyancy);
	}

	CleanupOutfit();

#if !UE_BUILD_SHIPPING
	UnregisterPerkCommands();

	if (IsLocallyControlled())
	{
		IConsoleManager& ConsoleManager = IConsoleManager::Get();
		ConsoleManager.UnregisterConsoleObject(ConsoleManager.FindConsoleObject(TEXT("sc.PopLimbs")));
		ConsoleManager.UnregisterConsoleObject(ConsoleManager.FindConsoleObject(TEXT("sc.RemoveGore")));
	}
#endif
}

void ASCCounselorCharacter::OnReceivedController()
{
	Super::OnReceivedController();

	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		if (PPV->Settings.ColorGradingLUT == nullptr)
		{
			PPV->Settings.bOverride_ColorGradingLUT = true;
			PPV->Settings.bOverride_ColorGradingIntensity = true;
			PPV->Settings.ColorGradingIntensity = 0.0f;
			PPV->Settings.ColorGradingLUT = FearColorGrade;

			if (!FearScreenEffectInst)
			{
				FearScreenEffectInst = UMaterialInstanceDynamic::Create(FearScreenEffect, this);
				PPV->Settings.AddBlendable(FearScreenEffectInst, 1.0f);

				if (FearScreenEffectInst)
				{
					FearScreenEffectInst->SetScalarParameterValue(TEXT("AlphaDelta"), 0.0f);
					FearScreenEffectInst->SetScalarParameterValue(TEXT("DistortionDelta"), 0.0f);
				}
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (IsLocallyControlled())
	{
		IConsoleManager& ConsoleManager = IConsoleManager::Get();

		// If you add any functions to this list, make sure you unregister them in EndPlay!

		ConsoleManager.RegisterConsoleCommand(
			TEXT("sc.PopLimbs"),
			TEXT("Pops off both arms, both legs and head on the local player (not replicated). Will reattach if called again."),
			FConsoleCommandDelegate::CreateUObject(this, &ASCCounselorCharacter::PopLimbs),
			ECVF_Default
		);

		ConsoleManager.RegisterConsoleCommand(
			TEXT("sc.RemoveGore"),
			TEXT("Gets rid of blood masks, same code path as using health spray (not replicated)"),
			FConsoleCommandDelegate::CreateUObject(this, &ASCCounselorCharacter::RemoveGore),
			ECVF_Default
		);
	}
#endif
}

void ASCCounselorCharacter::StopCameraShakeInstances(TSubclassOf<UCameraShake> Shake)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
	{
		if (PC->IsLocalController() && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopAllInstancesOfCameraShake(Shake);
		}
	}

	if (AILLPlayerController* LocalController = UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld()))
	{
		if (LocalController->GetViewTarget() == this)
		{
			if (LocalController->PlayerCameraManager)
			{
				LocalController->PlayerCameraManager->StopAllInstancesOfCameraShake(Shake);
			}
		}
	}
}

void ASCCounselorCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	FearManager->RegisterLockFearCMD();
}

void ASCCounselorCharacter::ContextKillStarted()
{
	// Kill the flashlight
	ShutOffFlashlight();

	// Stop showing the breath indicator
	if (IsInHiding() && IsLocallyControlled())
	{
		if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
		{
			if (PlayerController->GetSCHUD())
			{
				if (ASCInGameHUD* HUD = PlayerController->GetSCHUD())
				{
					HUD->SetBreathVisible(false);
				}
			}
		}
	}

	// Revert to our normal clothing if this is a murder
	if (USCSpecialMove_SoloMontage* ContextKillMove = GetActiveSCSpecialMove())
	{
		if (ContextKillMove->DieOnFinish())
		{
			if (bWearingSweater)
				TakeOffPamelasSweater();
		}
	}

	Super::ContextKillStarted();
}

extern TAutoConsoleVariable<int32> CVarDebugWiggle;
void ASCCounselorCharacter::InitStunWiggleGame(const USCStunDamageType* StunDefaults, const FSCStunData& StunData)
{
	ResetStunWiggleValues();

	DeltaPerWiggle = StunDefaults->PercentPerWiggle.Get(this) * 0.01f;
	StaminaPerWiggle = StunDefaults->StaminaPerWiggle.Get(this, true);
	DepletionRate = StunDefaults->DepletionRate.Get(this, true);
	MaxStunTime = StunDefaults->TotalStunTime.Get(this, true);
	const float BaseStunTime = MaxStunTime;
	MaxStunTime *= StunData.StunTimeModifier;

	if (IsWounded())
	{
		DeltaPerWiggle *= WoundedStunMod;
	}

	if (bIsGrabbed && GrabbedBy != nullptr)
	{
		if (GrabbedBy->IsRageActive())
			DeltaPerWiggle *= RageGrabStunMod;

		DeltaPerWiggle *= GrabbedBy->GetGrabModifier();
	}

	if (CVarDebugWiggle.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, MaxStunTime, FColor::Cyan, FString::Printf(TEXT("Stun Max Time: %.1f -- Modified Max Time: %.1f  Modifier: %.1f \n Stamina per wiggle: %.1f \n Completion per wiggle: %.1f"),
			BaseStunTime, MaxStunTime, StunData.StunTimeModifier, StaminaPerWiggle, DeltaPerWiggle));

	}
}

void ASCCounselorCharacter::DrawDebug()
{
#if !UE_BUILD_SHIPPING
	Super::DrawDebug();

	// Stumbling
	if (CVarDebugStumbling.GetValueOnGameThread() > 0)
	{
		const float Frequency = EvaluateStumbleFrequency();
		const float Chance = EvaluateStumbleChance();
		const float AverageTimeBetweenStumbles = Chance == 0.f ? 0.f : Frequency * (1.f / (Chance / 100.f));

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, bIsStumbling ? FColor::Red : FColor::Cyan, FString::Printf(TEXT("Rolling every %.1f second(s) with a %.2f chance averaging at a stumble every %.1f second(s)"), Frequency, Chance, AverageTimeBetweenStumbles));
	}

	// First Aid
	if (CVarDebugFirstAid.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		DrawDebugCone(GetWorld(), GetActorLocation(), GetActorForwardVector(), FirstAidDistance, FMath::DegreesToRadians(FirstAidAngle), 0.f, 8, FColor::Purple, false, 0.f, 0, 1.5f);
	}

	// Output weapon stats
	if (CVarDebugCombat.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		if (ASCWeapon* Weapon = GetCurrentWeapon())
		{
			const FWeaponStats& WeaponStats = Weapon->WeaponStats;
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("%s Stats"), *Weapon->GetName()), false);
			const float BaseDamage = WeaponStats.Damage;
			const float ModifiedDamage = BaseDamage * StrengthAttackMod.Get(this);
			const float HeavyDamage = ModifiedDamage * HeavyAttackModifier;
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Base Damage: %.2f  Modified (x %.2f): %.2f  Heavy (x %.1f): %.2f"), BaseDamage, StrengthAttackMod.Get(this), ModifiedDamage, HeavyAttackModifier, HeavyDamage), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Base Stun: %d%%  Base Stuck: %d%%"), WeaponStats.StunChance, WeaponStats.StuckChance), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Durability: %.1f / %.1f  Attack Wear: %.1f  Block Wear: %.1f Durability Modifier: %.1f"), Weapon->CurrentDurability, WeaponStats.Durability, WeaponStats.AttackingWear, WeaponStats.BlockingWear, GetWeaponDurabilityModifier()), false);

			float StunChanceModifier = 0.f;
			float StunDurationModifier = 0.f;
			float BaseDamageModifier = 0.f;

			if (ASCPlayerState* PS = GetPlayerState())
			{
				for (const USCPerkData* Perk : PS->GetActivePerks())
				{
					check(Perk);

					// Check if they are using the correct weapon to apply this perk modifier
					if (Perk->RequiredWeapon && Weapon && Perk->RequiredWeapon != Weapon->GetClass())
						continue;

					StunChanceModifier += Perk->WeaponStunChanceModifier;
					StunDurationModifier += Perk->WeaponStunDurationModifier;
					BaseDamageModifier += Perk->WeaponDamageModifier;
				}
			}

			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Perk Stun Chance Modifier: %.2f"), StunChanceModifier), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Perk Stun Duration Modifier: %.2f"), StunDurationModifier), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Perk Damage Modifier: %.2f"), BaseDamageModifier), false);
		}
	}

	// Debug Repair
	if (CVarDebugRepair.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Stat Repair Modifier: %.2f"), GetStatInteractMod() - 1.f), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Perk Repair Modifier: %.2f"), GetPerkInteractMod() - 1.f), false);
	}
#endif //!UE_BUILD_SHIPPING
}

void ASCCounselorCharacter::DrawDebugStuck(float& ZPos)
{
#if !UE_BUILD_SHIPPING
	Super::DrawDebugStuck(ZPos);

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

	if (GrabbedBy)
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Grabbed by: %s"), *GrabbedBy->GetName()));
	}
#endif //!UE_BUILD_SHIPPING
}

void ASCCounselorCharacter::SetupPlayerInputComponent(class UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	check(InInputComponent);
	InInputComponent->BindAction(TEXT("CSLR_Sprint_PC"), IE_Pressed, this, &ASCCounselorCharacter::OnStartSprint);
	InInputComponent->BindAction(TEXT("CSLR_Sprint_PC"), IE_Released, this, &ASCCounselorCharacter::OnStopSprint);
	InInputComponent->BindAction(TEXT("CSLR_Sprint_CTRL"), IE_Pressed, this, &ASCCounselorCharacter::OnToggleSprint);
	InInputComponent->BindAction(TEXT("CSLR_Sprint_CTRL"), IE_Released, this, &ASCCounselorCharacter::OnToggleSprintStop);
	InInputComponent->BindAction(TEXT("CSLR_Run"), IE_Pressed, this, &ASCCounselorCharacter::OnRunToggle);
	InInputComponent->BindAction(TEXT("CSLR_Crouch"), IE_Released, this, &ASCCounselorCharacter::OnCrouchToggle);
	InInputComponent->BindAction(TEXT("CSLR_Flashlight"), IE_Pressed, this, &ASCCounselorCharacter::ToggleFlashlight);
	InInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ASCCounselorCharacter::ToggleMap);
	InInputComponent->BindAction(TEXT("CSLR_LargeItem"), IE_Pressed, this, &ASCCounselorCharacter::OnLargeItemPressed);
	InInputComponent->BindAction(TEXT("CSLR_LargeItem"), IE_Released, this, &ASCCounselorCharacter::OnLargeItemReleased);
	InInputComponent->BindAction(TEXT("CSLR_SmallItem"), IE_Pressed, this, &ASCCounselorCharacter::OnSmallItemPressed);
	InInputComponent->BindAction(TEXT("CSLR_SelectSmallItemNext"), IE_Pressed, this, &ASCCounselorCharacter::NextSmallItem);
	InInputComponent->BindAction(TEXT("CSLR_SelectSmallItemPrevious"), IE_Pressed, this, &ASCCounselorCharacter::PrevSmallItem);
	InInputComponent->BindAction(TEXT("CSLR_SmallItem_1"), IE_Pressed, this, &ASCCounselorCharacter::OnUseSmallItemOne);
	InInputComponent->BindAction(TEXT("CSLR_SmallItem_2"), IE_Pressed, this, &ASCCounselorCharacter::OnUseSmallItemTwo);
	InInputComponent->BindAction(TEXT("CSLR_SmallItem_3"), IE_Pressed, this, &ASCCounselorCharacter::OnUseSmallItemThree);
	InInputComponent->BindAction(TEXT("CSLR_UseAbility"), IE_Released, this, &ASCCounselorCharacter::UseAbility);
	InInputComponent->BindAction(TEXT("CSLR_DropItem"), IE_Pressed, this, &ASCCounselorCharacter::OnDropItemPressed);
	InInputComponent->BindAction(TEXT("CSLR_DropItem"), IE_Released, this, &ASCCounselorCharacter::OnDropItemReleased);
	InInputComponent->BindAction(TEXT("CSLR_RearView"), IE_Pressed, this, &ASCCounselorCharacter::OnRearCamPressed);
	InInputComponent->BindAction(TEXT("CSLR_RearView"), IE_Released, this, &ASCCounselorCharacter::OnRearCamReleased);
	InInputComponent->BindAction(TEXT("CSLR_ShowEmote"), IE_Pressed, this, &ASCCounselorCharacter::OnRequestShowEmoteSelect);
	InInputComponent->BindAction(TEXT("CSLR_ShowEmote"), IE_Released, this, &ASCCounselorCharacter::OnHideEmoteSelect);
	InInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ASCCounselorCharacter::OnShowScoreboard);
	InInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ASCCounselorCharacter::OnHideScoreboard);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Add bindings to parent class input components
	if (CombatStanceInputComponent)
	{
		CombatStanceInputComponent->BindAction(TEXT("CSLR_SmallItem"), IE_Pressed, this, &ASCCounselorCharacter::OnSmallItemPressed);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_SmallItemNext"), IE_Pressed, this, &ASCCounselorCharacter::NextSmallItem);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_PreviousItemNext"), IE_Pressed, this, &ASCCounselorCharacter::PrevSmallItem);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_SmallItem_1"), IE_Pressed, this, &ASCCounselorCharacter::OnUseSmallItemOne);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_SmallItem_2"), IE_Pressed, this, &ASCCounselorCharacter::OnUseSmallItemTwo);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_SmallItem_3"), IE_Pressed, this, &ASCCounselorCharacter::OnUseSmallItemThree);

		CombatStanceInputComponent->BindAction(TEXT("CMBT_CSLR_Dodge"), IE_Pressed, this, &ASCCounselorCharacter::OnDodge);
		CombatStanceInputComponent->BindAction(TEXT("GLB_DodgeLeft"), IE_DoubleClick, this, &ASCCounselorCharacter::DodgeLeft);
		CombatStanceInputComponent->BindAction(TEXT("GLB_DodgeRight"), IE_DoubleClick, this, &ASCCounselorCharacter::DodgeRight);
		CombatStanceInputComponent->BindAction(TEXT("GLB_DodgeBack"), IE_DoubleClick, this, &ASCCounselorCharacter::DodgeBack);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_CSLR_Attack"), IE_Pressed, this, &ASCCounselorCharacter::OnAttack);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_CSLR_Attack"), IE_Released, this, &ASCCounselorCharacter::OnStopAttack);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_CSLR_Block"), IE_Pressed, this, &ASCCounselorCharacter::OnStartBlock);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_CSLR_Block"), IE_Released, this, &ASCCounselorCharacter::OnEndBlock);

		CombatStanceInputComponent->BindAction(TEXT("CSLR_Sprint_PC"), IE_Pressed, this, &ASCCounselorCharacter::OnStartSprint);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_Sprint_PC"), IE_Released, this, &ASCCounselorCharacter::OnStopSprint);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_Sprint_CTRL"), IE_Pressed, this, &ASCCounselorCharacter::OnToggleSprint);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_Sprint_CTRL"), IE_Released, this, &ASCCounselorCharacter::OnToggleSprintStop);

		CombatStanceInputComponent->BindAction(TEXT("CSLR_DropItem"), IE_Pressed, this, &ASCCounselorCharacter::OnDropItemPressed);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_DropItem"), IE_Released, this, &ASCCounselorCharacter::OnDropItemReleased);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_Crouch"), IE_Released, this, &ASCCounselorCharacter::OnCrouchToggle);
		CombatStanceInputComponent->BindAction(TEXT("CSLR_Map"), IE_Pressed, this, &ASCCounselorCharacter::ToggleMap);

		CombatStanceInputComponent->BindAction(TEXT("CSLR_Flashlight"), IE_Pressed, this, &ASCCounselorCharacter::ToggleFlashlight);

		CombatStanceInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ASCCounselorCharacter::OnShowScoreboard);
		CombatStanceInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ASCCounselorCharacter::OnHideScoreboard);
	}

	if (WiggleInputComponent)
	{
		WiggleInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ASCCounselorCharacter::ToggleMap);

		WiggleInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ASCCounselorCharacter::OnShowScoreboard);
		WiggleInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ASCCounselorCharacter::OnHideScoreboard);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (!AimInputComponent)
	{
		AimInputComponent = NewObject<UInputComponent>(Controller, TEXT("AimInput"));
	}

	AimInputComponent->ClearBindingValues();
	AimInputComponent->ClearActionBindings();
	AimInputComponent->BindAction(TEXT("CSLR_Aim"), IE_Pressed, this, &ASCCounselorCharacter::OnAimPressed);
	AimInputComponent->BindAction(TEXT("CSLR_Aim"), IE_Released, this, &ASCCounselorCharacter::OnAimReleased);

	if (!PassengerInputComponent)
	{
		PassengerInputComponent = NewObject<UInputComponent>(Controller, TEXT("PassengerInput"));
		PassengerInputComponent->bBlockInput = true;
	}

	PassengerInputComponent->ClearBindingValues();
	PassengerInputComponent->ClearActionBindings();
	PassengerInputComponent->BindAxis(TEXT("LookUp"), this, &ASCCounselorCharacter::AddVehiclePitchInput);
	PassengerInputComponent->BindAxis(TEXT("Turn"), this, &ASCCounselorCharacter::AddVehicleYawInput);

	PassengerInputComponent->BindAction(TEXT("GLB_Interact1"), EInputEvent::IE_Pressed, this, &ASCCounselorCharacter::OnInteract1Pressed);
	PassengerInputComponent->BindAction(TEXT("GLB_Interact1"), EInputEvent::IE_Released, this, &ASCCounselorCharacter::OnInteract1Released);

	PassengerInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ASCCounselorCharacter::OnShowScoreboard);
	PassengerInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ASCCounselorCharacter::OnHideScoreboard);

	PassengerInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ASCCounselorCharacter::ToggleMap);

	if (!HidingInputComponent)
	{
		HidingInputComponent = NewObject<UInputComponent>(Controller, TEXT("HidingInput"));
		HidingInputComponent->bBlockInput = true;
	}

	HidingInputComponent->ClearBindingValues();
	HidingInputComponent->ClearActionBindings();
	HidingInputComponent->BindAction(TEXT("GLB_Interact1"), EInputEvent::IE_Pressed, this, &ASCCounselorCharacter::OnExitHidingPressed);
	HidingInputComponent->BindAxis(TEXT("Turn"), this, &ASCCounselorCharacter::AddControllerYawInput);
	HidingInputComponent->BindAxis(TEXT("LookUp"), this, &ASCCounselorCharacter::AddControllerPitchInput);
	HidingInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ASCCounselorCharacter::ToggleMap);

	HidingInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ASCCounselorCharacter::OnShowScoreboard);
	HidingInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ASCCounselorCharacter::OnHideScoreboard);

	HidingInputComponent->BindAction(TEXT("CSLR_HoldBreath"), IE_Pressed, this, &ASCCounselorCharacter::OnHoldBreath);
	HidingInputComponent->BindAction(TEXT("CSLR_HoldBreath"), IE_Released, this, &ASCCounselorCharacter::OnReleaseBreath);

	if (!EmoteInputComponent)
	{
		EmoteInputComponent = NewObject<UInputComponent>(Controller, TEXT("EmoteInput"));
		EmoteInputComponent->bBlockInput = true;
	}

	EmoteInputComponent->ClearBindingValues();
	EmoteInputComponent->ClearActionBindings();
	InInputComponent->BindAction(TEXT("CSLR_ShowEmote"), IE_Released, this, &ASCCounselorCharacter::OnHideEmoteSelect);
}

void ASCCounselorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive())
		return;

	// We're in grab stance but not grabbed, fix it
	if (CurrentStance == ESCCharacterStance::Grabbed && !GrabbedBy)
	{
		SetStance(ESCCharacterStance::Standing);
	}

	UpdateSwimmingState();

	if (EquippedItem && Role == ROLE_Authority && !IsCharacterDisabled())
	{
		const bool bIsSwimming = CurrentStance == ESCCharacterStance::Swimming && !EquippedItem->IsA(ASCGun::StaticClass());
		EquippedItem->SetActorHiddenInGame(bIsSwimming || IsInVehicle() || bInHiding || bHideLargeItemOverride || CurrentEmote || PropSpawnerHiddingItem > 0);
	}

	UWorld* World = GetWorld();
	bool bIsMatchActive = true; // default this to true just in case we are using a weird game mode that doesnt use SCGameState
	if (ASCGameState* GameState = World ? Cast<ASCGameState>(World->GetGameState()) : nullptr)
	{
		if (Cast<ASCGameState_Paranoia>(GameState))
		{
			// We don't want any pips for paranoia mode
			for (ASCCharacter* Character : GameState->PlayerCharacterList)
			{
				Character->MinimapIconComponent->SetVisibility(Character->IsLocallyControlled());
			}
		}
		else
		{
			// Check if we can see the killer
			if (Controller && Controller->IsLocalPlayerController() && !bIsKillerOnMap)
			{
				// Add the killer to the mini map if spotting ability is active
				if (IsAbilityActive(EAbilityType::Spotting))
				{
					for (ASCCharacter* Character : GameState->PlayerCharacterList)
					{
						if (Character->IsKiller())
							Character->MinimapIconComponent->SetVisibility(true);
					}
				}
				else if (OutOfSightTime > 0.0f)
				{
					OutOfSightTime -= DeltaSeconds;
					if (OutOfSightTime <= 0.0f)
					{
						for (ASCCharacter* Character : GameState->PlayerCharacterList)
						{
							if (Character->IsKiller())
								Character->MinimapIconComponent->SetVisibility(false);
						}
					}
				}
			}
		}
		// cache off if the match is active for use later in this function
		bIsMatchActive = GameState->IsMatchInProgress();
	}

	if (Role == ROLE_Authority)
	{
		UpdateStumbling_SERVER(DeltaSeconds);

		UpdateBreath(DeltaSeconds);

		bIsWounded = IsAlive() && Health <= WoundedThreshold;

		// just don't kick people who are idle while in a hiding spot for now
		if (bInHiding)
		{
			// Bump idle timer
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
				PC->BumpIdleTime();
		}
	}

	UpdateStamina(DeltaSeconds);

	UpdateFearEvents();

	UpdateActiveAbilities();

	UpdateChase();

	// update the grab transform if the counselor is in the killers hand or in a context kill
	if (bInKillerHand || IsInContextKill())
		UpdateGrabTransform();

	FollowExitSpline(DeltaSeconds);

	bool bShouldHideFearForContextKill = false;
	if (USCSpecialMove_SoloMontage* ContextKillMove = GetActiveSCSpecialMove())
	{
		bShouldHideFearForContextKill = ContextKillMove->DieOnFinish();
	}

	// Fear Effects
	static const FName AlphaDelta_NAME(TEXT("AlphaDelta"));
	static const FName DistortionDelta_NAME(TEXT("DistortionDelta"));
	const float CosmeticFear = (IsAlive() && bIsMatchActive && !bShouldHideFearForContextKill && !bHasEscaped) ? FearManager->GetCosmeticFear() : 0.0f;
	if (VisualFearCurve != nullptr)
	{
		const float FearMod = VisualFearCurve->GetFloatValue(CosmeticFear);

		if (APostProcessVolume* PPV = /*PC->*/GetPostProcessVolume())
			PPV->Settings.ColorGradingIntensity = FearMod;
	}

	if (FearVignetteCurve != nullptr)
	{
		const float VignetteMod = FearVignetteCurve->GetFloatValue(CosmeticFear);

		if (FearScreenEffectInst)
			FearScreenEffectInst->SetScalarParameterValue(AlphaDelta_NAME, VignetteMod);
	}

	if (FearDistortionCurve != nullptr)
	{
		const float DistortionMod = FearDistortionCurve->GetFloatValue(CosmeticFear);

		if (FearScreenEffectInst)
			FearScreenEffectInst->SetScalarParameterValue(DistortionDelta_NAME, DistortionMod);
	}

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
	{
		if (HUDOpacityFearCurve != nullptr && PC->GetSCHUD() != nullptr)
		{
			const float NewOpacity = bInHiding ? 0.0f : HUDOpacityFearCurve->GetFloatValue(CosmeticFear);
			const float CurOpacity = PC->GetSCHUD()->GetHUDWidgetOpacity();
			PC->GetSCHUD()->SetHUDWidgetOpacity(FMath::FInterpTo(CurOpacity, NewOpacity, DeltaSeconds, 1.0f));
		}
	}
	// ~FearEffects

	if (IsLocallyControlled() || Role == ROLE_Authority)
	{
		if (ASCGun* Gun = Cast<ASCGun>(EquippedItem))
		{
			if (Gun->HasAmmunition() && CurrentStance != ESCCharacterStance::Swimming)
			{
				if (!bHasAimingCapability)
				{
					if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
						PC->PushInputComponent(AimInputComponent);

					bHasAimingCapability = true;
				}
			}

			bIsAiming = Gun->IsAiming();
		}
		else if (bHasAimingCapability || CurrentStance == ESCCharacterStance::Swimming || bIsGrabbed)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
				PC->PopInputComponent(AimInputComponent);

			bHasAimingCapability = false;
			bIsAiming = false;
		}

		if (bIsAiming)
		{
			AimPitch = GetControlRotation().Pitch > 180.f ? GetControlRotation().Pitch - 360.f : GetControlRotation().Pitch;
		}
		else if (!FMath::IsNearlyZero((float)AimPitch))
		{
			AimPitch = FMath::FInterpTo((float)AimPitch, 0.f, DeltaSeconds, 5.f);
		}

#if 0
		if (Cast<ASCCharacterAIController>(Controller))
		{
			// Always use the controller rotation for AI
			GetCharacterMovement()->bOrientRotationToMovement = false;
			bUseControllerRotationYaw = true;
		}
		else
#endif
		if (!InCombatStance())
		{
			// Combat stance handles character rotation differently, so don't mess with that here
			if ((bIsAiming || CurrentStance == ESCCharacterStance::Swimming) && !IsInSpecialMove())
			{
				GetCharacterMovement()->bOrientRotationToMovement = false;
				bUseControllerRotationYaw = true;
			}
			else
			{
				GetCharacterMovement()->bOrientRotationToMovement = true;
				bUseControllerRotationYaw = false;
			}
		}
	}

	if (IsLocallyControlled())
	{
		UpdateDropItemTimer(DeltaSeconds);
	}

	ASCPlayerController* SCPlayerController = Cast<ASCPlayerController>(GetCharacterController());
	if (CurrentStance == ESCCharacterStance::Driving && SCPlayerController)
	{
		if (Role == ROLE_Authority)
		{
			SCPlayerController->BumpIdleTime();
		}

		// Clamp camera rotation when in a vehicle (only for passengers, not the driver)
		if (IsLocallyControlled() && Controller)
		{
			FRotator NewRotation(FRotator::ZeroRotator);
			if (CurrentSeat && CurrentSeat->ParentVehicle)
			{
				NewRotation = CurrentSeat->ParentVehicle->GetActorRotation();
			}

			NewRotation.Yaw += LocalVehicleYaw;
			NewRotation.Pitch += LocalVehiclePitch;
			NewRotation.Roll = 0.f;
			Controller->SetControlRotation(NewRotation);
		}
	}

	if (IsLocallyControlled())
	{
		CheckForDeadFriendliesInRange();
	}

	// Flashlight! So pretty!
	if (bIsFlashlightOn)
	{
		const FRotator CurrentBoneRotation = (LightMesh->GetComponentRotation() - GetMesh()->GetComponentRotation()).GetNormalized();
		FRotator FullDeltaRotation = (CurrentBoneRotation - FlashlightMeshBaseRotation).GetNormalized();
		FVector InterpLocation = Flashlight->RelativeLocation;

		if (FlashlightCurrentLerpTime < FlashlightLocationLerpTime)
		{
			FlashlightCurrentLerpTime += DeltaSeconds;

			const float InterpAlpha = FMath::InterpCircularInOut(0.f, 1.f, FMath::Clamp(FlashlightCurrentLerpTime / FlashlightLocationLerpTime, 0.f, 1.f));
			const FRotator InterpRotation = FMath::Lerp<FRotator>(FlashlightLerpStartRotation, FlashlightDesiredRotationOffset, InterpAlpha);
			InterpLocation = FMath::Lerp<FVector>(FlashlightLerpStartLocation, FlashlightDesiredLocation, InterpAlpha);
			FullDeltaRotation += InterpRotation;

		}
		else
		{
			FullDeltaRotation += FlashlightDesiredRotationOffset;
		}

		FlashlightCurrentRotation = FMath::RInterpTo(FlashlightCurrentRotation, FullDeltaRotation, DeltaSeconds, FlashlightLerpSpeed);

		Flashlight->SetRelativeLocationAndRotation(InterpLocation, FlashlightBaseRotation + FlashlightCurrentRotation);

		// Raycast check
		FHitResult HitResult;
		FCollisionQueryParams QueryParams(TEXT("FlashlightTrace"), true, this);
		QueryParams.AddIgnoredActor(EquippedItem);
		if (World->LineTraceSingleByChannel(HitResult, GetActorLocation(), Flashlight->GetComponentLocation(), ECollisionChannel::ECC_WorldDynamic, QueryParams))
		{
			if (HitResult.bBlockingHit)
				Flashlight->SetVisibility(false);
			else
				Flashlight->SetVisibility(true);
		}
		else
		{
			Flashlight->SetVisibility(true);
		}
	}

	if (!IsInSpecialMove())
	{
		if (CurrentSeat)
		{
			FTransform ActorTransform = GetActorTransform();
			if (CurrentSeat->ParentVehicle)
			{
				if (USkeletalMeshComponent* SkelMesh = CurrentSeat->ParentVehicle->GetMesh())
				{
					ActorTransform = SkelMesh->GetSocketTransform(CurrentSeat->AttachSocket);
				}
			}

			GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -85.f), FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetActorTransform(ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	if (SenseEffectComponent)
	{
		SenseEffectComponent->SetRelativeRotation(FQuat::Identity);
		SenseEffectComponent->SetWorldRotation(FQuat::Identity, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

bool ASCCounselorCharacter::ShouldShowInventory() const
{
	if (CurrentSeat)
		return false;

	if (SmallItemInventory->GetItemList().Num() <= 0)
		return false;

	return true;
}

int32 ASCCounselorCharacter::GetIndexOfSmallItem(TSubclassOf<ASCItem> ItemClass) const
{
	const TArray<AILLInventoryItem*> SmallItemList = SmallItemInventory->GetItemList();
	for (int i = 0; i < SmallItemList.Num(); ++i)
	{
		if (IsValid(SmallItemList[i]) && SmallItemList[i]->IsA(ItemClass))
			return i;
	}

	return -1;
}

void ASCCounselorCharacter::SetFlashlightDesiredOffset(const FVector NewLocationOffset, const FRotator NewRotationOffset, float InterpTime)
{
	const float currentAlpha = FMath::InterpCircularInOut(0.f, 1.f, FMath::Clamp(FlashlightCurrentLerpTime / FlashlightLocationLerpTime, 0.f, 1.f));

	// Cache current values so we can lerp from where we are to the new dest.
	FlashlightLerpStartRotation = FMath::Lerp(FRotator::ZeroRotator, FlashlightDesiredRotationOffset, currentAlpha);
	FlashlightLerpStartLocation = Flashlight->RelativeLocation;
	
	FlashlightLocationLerpTime = FMath::Max(InterpTime, KINDA_SMALL_NUMBER);
	FlashlightCurrentLerpTime = 0.f;
	FlashlightDesiredLocation = FlashlightBaseLocation + NewLocationOffset;
	FlashlightDesiredRotationOffset = NewRotationOffset;
}

bool ASCCounselorCharacter::ToggleFlashlight_Validate()
{
	return true;
}

void ASCCounselorCharacter::ToggleFlashlight_Implementation()
{
	bIsFlashlightOn = !bIsFlashlightOn;
	OnRep_FlashlightOn(!bIsFlashlightOn);
}

void ASCCounselorCharacter::OnRep_FlashlightOn(bool bWasFlashlightOn)
{
	// play on or off flashlight sound effect.
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), bIsFlashlightOn ? FlashlightOffSound : FlashlightOnSound, GetActorLocation());

	// turn on flashlight
	Flashlight->SetVisibility(bIsFlashlightOn);
	OnFlashlightToggle(bIsFlashlightOn);
}

void ASCCounselorCharacter::TurnOnFlashlight()
{
	if (Role == ROLE_Authority && !bIsFlashlightOn)
		ToggleFlashlight();
}

void ASCCounselorCharacter::ShutOffFlashlight()
{
	if (Role == ROLE_Authority && bIsFlashlightOn)
		ToggleFlashlight();
}

void ASCCounselorCharacter::UpdateChase()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	// Check if we have any killers chasing us
	bool bIsBeingChasedThisFrame = false;
	float NearestChaserSq = FLT_MAX;

	if (ASCGameState* SCGameState = Cast<ASCGameState>(World->GetGameState()))
	{
		for (ASCCharacter* Character : SCGameState->PlayerCharacterList)
		{
			if (IsValid(Character) && Character->IsKiller() && IsBeingChasedBy(Character))
			{
				bIsBeingChasedThisFrame = GetVelocity().SizeSquared() > 0.0f;
				const float ChaseDistanceSq = FVector::DistSquaredXY(Character->GetActorLocation(), GetActorLocation());
				if (ChaseDistanceSq < NearestChaserSq)
					NearestChaserSq = ChaseDistanceSq;

				break;
			}
		}
	}

	// Update our chase status
	const float FrameTime = World->GetTimeSeconds();
	if (bIsBeingChasedThisFrame && !IsInContextKill())
	{
		const bool WasChased = bIsBeingChased;

		ChasingKillerDistance = FMath::Sqrt(NearestChaserSq);
		bIsBeingChased = true;
		LastChased_Timestamp = FrameTime;

		// Let the VO system know that the killer is closing in.
		if (USCVoiceOverComponent_Counselor *VoiceOverComponent_Counselor = Cast<USCVoiceOverComponent_Counselor>(VoiceOverComponent))
		{
			if (ChasingKillerDistance <= VoiceOverComponent_Counselor->KillerChaseVODistance)
			{
				VoiceOverComponent_Counselor->OnKillerClosingInDuringChase(true);
			}
		}

		if (WasChased == false)
		{
			ChaseStart_Timestamp = FrameTime;
			OnChaseStart();
		}
	}
	else if (bIsBeingChased && FrameTime - LastChased_Timestamp >= ChaseTimeout)
	{
		bIsBeingChased = false;

		// Total length of our chase
		OnChaseEnd(FrameTime - ChaseStart_Timestamp);

		// Let the VO system know the killer is pulling away.
		if (USCVoiceOverComponent_Counselor *VoiceOverComponent_Counselor = Cast<USCVoiceOverComponent_Counselor>(VoiceOverComponent))
		{
			VoiceOverComponent_Counselor->OnKillerClosingInDuringChase(false);
		}
	}
}

bool ASCCounselorCharacter::IsBeingChasedBy(const ASCCharacter* ChasingCharacter) const
{
	if (ChasingCharacter->IsCharacterDisabled())
		return false;

	FVector ToChaser = ChasingCharacter->GetActorLocation() - GetActorLocation();
	if (ChasingKillerMustBeMoving)
	{
		// Are they moving?
		const FVector ChaserVelocity = ChasingCharacter->GetVelocity();
		if (ChaserVelocity.SizeSquared() <= 0.f)
			return false;

		// Are they moving towards us?
		if ((ChaserVelocity | ToChaser) >= 0.f)
			return false;
	}

	if (ASCCounselorAIController* AIController = Cast<ASCCounselorAIController>(Controller))
	{
#if WITH_EDITOR
		// Handle bAIIgnorePlayers
		if (AIController->bAIIgnorePlayers && ChasingCharacter->Controller && ChasingCharacter->Controller->IsPlayerController())
			return false;
#endif
		// We aren't being chased if we are using scripted behavior
		if (!AIController->IsUsingOfflineBotsLogic())
			return false;
	}

	// Do not think we're being chased while the chasing killer is using stalk or shift
	if (const ASCKillerCharacter* ChasingKiller = Cast<ASCKillerCharacter>(ChasingCharacter))
	{
		if (ChasingKiller->IsStalkActive())
			return false;
		if (ChasingKiller->IsShiftActive())
			return false;
	}

	// Are they close?
	const float DistanceSq = ToChaser.SizeSquared();
	const float MaxDistanceSq = FMath::Square(bIsBeingChased ? EndChaseDistance : StartChaseDistance);
	if (DistanceSq > MaxDistanceSq)
		return false;

	// Are they behind us?
	ToChaser.Normalize();
	const float MaxAngleDot = FMath::Acos(FMath::DegreesToRadians(bIsBeingChased ? EndChaseKillerAngle : StartChaseKillerAngle));
	const FVector PlayerBackwardVector = (FRotator(0.f, 180.f, 0.f) + GetActorRotation()).Vector().GetSafeNormal();
	if ((PlayerBackwardVector | ToChaser) < MaxAngleDot)
		return false;

	// Well. Shit.
	return true;
}

void ASCCounselorCharacter::UpdateFearEvents()
{
	const bool InLight = TestLights();
	if (InLight != bInLight)
	{
		if (!InLight)
			FearManager->PushFearEvent(DarkFearEvent);
		else
			FearManager->PopFearEvent(DarkFearEvent);
		bInLight = InLight;
	}

	if (IsWounded())
		FearManager->PushFearEvent(WoundedFearEvent);
	else
		FearManager->PopFearEvent(WoundedFearEvent);

	if (IsIndoors())
		FearManager->PushFearEvent(IndoorFearEvent);
	else
		FearManager->PopFearEvent(IndoorFearEvent);

	if (CurrentWaterVolume)
		FearManager->PushFearEvent(InWaterFearEvent);
	else
		FearManager->PopFearEvent(InWaterFearEvent);

	if (bFriendlysNearby)
		FearManager->PushFearEvent(NearFriendlyFearEvent);
	else
		FearManager->PopFearEvent(NearFriendlyFearEvent);

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GameState = World->GetGameState<ASCGameState>())
		{
			if (GameState->CurrentKiller)
			{
				bool bIgnoreJason = false;
				if (AAIController* AIC = Cast<AAIController>(Controller))
				{
					bIgnoreJason = AIC->bAIIgnorePlayers && (GameState->CurrentKiller->Controller && GameState->CurrentKiller->Controller->IsPlayerController());
				}

				if (!bIgnoreJason)
				{
					const FVector KillerLoc = GameState->CurrentKiller->GetActorLocation();
					const FVector MyLoc = GetActorLocation();
					const float HorizontalDistance = (KillerLoc - MyLoc).Size2D();
					const float FearMaxDistance = JasonNearMaxRange; // TODO: pjackson: Scale by stealth?

					if (HorizontalDistance < FearMaxDistance)
					{
						const float NormalizedDistance = FMath::Clamp((HorizontalDistance - JasonNearMinRange) / (FearMaxDistance - JasonNearMinRange), 0.f, 1.f);
						FearManager->PushFearEvent(JasonNearFearEvent, NormalizedDistance);
					}
					else
					{
						FearManager->PopFearEvent(JasonNearFearEvent);
					}
				}
			}
		}
	}
}

void ASCCounselorCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (bInWheelchair && bInWater && !bIsGrabbed && IsAlive())
	{
		// Kill the wheelchair jock if they enter the water and they aren't grabbed.
		// If they are grabbed, ASCKillerCharacter::MULTICAST_DropCounselor handles killing this counselor
		ForceKillCharacter(this);
	}

	if (!InShackWarningToJasonPlayed || !SeenPamelasHead)
	{
		if (ASCGameState* SCGameState = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (USCIndoorComponent* IndoorComp = Cast<USCIndoorComponent>(OtherComp))
			{
				// Alert Jason that a counselor has found his shack.
				if (!InShackWarningToJasonPlayed && IndoorComp->IsJasonsShack && !IndoorComp->IsPamelasHeadInHere && !HasItemInInventory(ASCKillerMask::StaticClass()))
				{
					if (SCGameState->CurrentKiller && SCGameState->CurrentKiller->IsLocallyControlled())
					{
						SCGameState->CurrentKiller->VoiceOverComponent->PlayVoiceOver(TEXT("CounselorInLair"));
						InShackWarningToJasonPlayed = true;
					}
				}
				// Play the see dead body stinger stuff when the counselor sees Pamela's head.
				else if (!SeenPamelasHead && IndoorComp->IsJasonsShack && IndoorComp->IsPamelasHeadInHere)
				{ 
					// Play the I've seen a dead body music stinger
					if (USCMusicComponent_Counselor* MusicComponent_Counselor = Cast<USCMusicComponent_Counselor>(MusicComponent))
					{
						MusicComponent_Counselor->PlayDeadFriendlyStinger();
					}

					// Emphasize the spotted dead body with jump scare anim and camera rotation
					PlayAnimMontage(CorpseSpottedJumpMontage);

					if (IsLocallyControlled())
					{
						PlayCameraShake(DeadBodyShake, 1.f, ECameraAnimPlaySpace::CameraLocal, FRotator::ZeroRotator);
					}

					SeenPamelasHead = true;
				}

				// Give this player a badge for entering Jason's shack
				if (Role == ROLE_Authority && IndoorComp->IsJasonsShack)
				{
					if (ASCPlayerState* PS = GetPlayerState())
						PS->EarnedBadge(EnterJasonsShackBadge);
				}
			}
		}
	}
}

void ASCCounselorCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	if (InShackWarningToJasonPlayed)
	{
		if (ASCGameState* SCGameState = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (SCGameState->CurrentKiller && SCGameState->CurrentKiller->IsLocallyControlled())
			{
				if (USCIndoorComponent* IndoorComp = Cast<USCIndoorComponent>(OtherComp))
				{
					if (IndoorComp->IsJasonsShack)
					{
						GetWorldTimerManager().SetTimer(TimerHandle_VOCooldown, this, &ASCCounselorCharacter::OnShackWarningToJasonCooldownDone, 30.f);
					}
				}
			}
		}
	}
}

void ASCCounselorCharacter::OnShackWarningToJasonCooldownDone()
{
	InShackWarningToJasonPlayed = false;
}

void ASCCounselorCharacter::ProcessWeaponHit(const FHitResult& Hit)
{
	// Paranoid check
	ASCWeapon* Weapon = GetCurrentWeapon();
	if (!Weapon)
		return;

	if (ASCCharacter* HitCharacter = Cast<ASCCharacter>(Hit.GetActor()))
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(HitCharacter))
		{
			if (Counselor->IsInHiding() || Counselor->IsInContextKill())
				return;
		}
		//if (HitCharacter->CanTakeDamage())
		{
			if (Hit.GetComponent() && Hit.GetComponent()->IsA<USkeletalMeshComponent>())
			{
				// Lambda for calculating damage
				const float Damage = [&]()
				{
					float WeaponDamage = Weapon->WeaponStats.Damage * StrengthAttackMod.Get(this) * (bHeavyAttacking ? HeavyAttackModifier : 1.f);
					if (ASCPlayerState* PS = GetPlayerState())
					{
						float DamageMod = 1.f;
						for (const USCPerkData* Perk : PS->GetActivePerks())
						{
							if (!Perk)
								continue;

							// Check if we're using the correct weapon to apply a damage modifier
							if (Perk->RequiredWeapon && Perk->RequiredWeapon != Weapon->GetClass())
								continue;

							// Check if the perk applies to the character we hit
							if (Perk->RequiredCharacterClassToHit.Get() && (!HitCharacter->GetClass()->IsChildOf(Perk->RequiredCharacterClassToHit.Get()) && Perk->RequiredCharacterClassToHit.Get() != HitCharacter->GetClass()))
								continue;

							if (Perk->GetClass() == PotentRangerPerk && bIsHunter)
								DamageMod += Perk->TommyJarvisWeaponDamageModifier;
							else
								DamageMod += Perk->WeaponDamageModifier * (Perk->GetClass() == FriendshipPerk ? GetNumCounselorsAroundMe() : 1.f);
						}

						WeaponDamage *= DamageMod;
					}

					return WeaponDamage;
				}();

				// Lambda for calculating stun chance
				const int StunChance = [&]()
				{
					float StunChanceModifier = StunChanceMod.Get(this);
					if (ASCPlayerState* PS = GetPlayerState())
					{
						for (const USCPerkData* Perk : PS->GetActivePerks())
						{
							check(Perk);

							if (!Perk)
								continue;

							// Check if we are using the correct weapon to apply this perk modifier
							if (Perk->RequiredWeapon && Perk->RequiredWeapon != Weapon->GetClass())
								continue;

							// Check if we can apply this perk modifier against the hit character
							if (Perk->RequiredCharacterClassToHit.Get() && (!HitCharacter->GetClass()->IsChildOf(Perk->RequiredCharacterClassToHit.Get()) && Perk->RequiredCharacterClassToHit.Get() != HitCharacter->GetClass()))
								continue;

							StunChanceModifier += Perk->WeaponStunChanceModifier * 100.f;
						}
					}
					return FMath::RandRange(0, 100 - (int)StunChanceModifier);
				}();

				bool bDidDamage = false;
				ASCKillerCharacter* HitKiller = Cast<ASCKillerCharacter>(HitCharacter);
				const bool bShouldStun = HitKiller && ((!HitKiller->IsRecoveringFromStun() && !HitKiller->IsStunned() && !HitKiller->IsInvulnerable()) || HitKiller->IsStunnedBySweater()) && StunChance < Weapon->WeaponStats.StunChance;
				if (bShouldStun) // We stunned Jason!
				{
					bDidDamage = HitCharacter->TakeDamage(Damage, FDamageEvent(Weapon->WeaponStats.WeaponStunClass), GetCharacterController(), this) > 0.f;
				}
				else // Nah, we just did lame damage
				{
					bDidDamage = HitCharacter->TakeDamage(Damage, FDamageEvent(Weapon->WeaponStats.WeaponDamageClass), GetCharacterController(), this) > 0.f;
				}

				// Tracking for player stats
				if (bDidDamage || bShouldStun)
				{
					if (ASCPlayerState* PS = GetPlayerState())
					{
						uint8 StatFlags = HitCharacter->IsA<ASCKillerCharacter>() ? (uint8)ESCItemStatFlags::HitJason : (uint8)ESCItemStatFlags::HitCounselor;
						if (bShouldStun)
						{
							MULTICAST_PlayCounselorStunnedJasonVO();
							StatFlags |= (uint8)ESCItemStatFlags::Stun;
							PS->EarnedBadge(MeleeStunnedJasonBadge);
						}

						PS->TrackItemUse(Weapon->GetClass(), StatFlags);
					}

					// Reduce durability when hitting a character
					Weapon->WearDurability(true);

					ReturnStamina(HitKillerStaminaBoost.Get(this));
				}

				bWeaponHitProcessed = true;
			}
		}
	}

	Super::ProcessWeaponHit(Hit);
}

void ASCCounselorCharacter::MULTICAST_PlayCounselorStunnedJasonVO_Implementation()
{
	static FName NAME_JasonStunVO(TEXT("JasonStun"));
	VoiceOverComponent->PlayVoiceOver(NAME_JasonStunVO);
}

void ASCCounselorCharacter::MULTICAST_PlayCounselorKnockedJasonsMaskOffVO_Implementation()
{
	static FName NAME_KnockJasonsMaskOffVO(TEXT("KnockJasonsMaskOff"));
	VoiceOverComponent->PlayVoiceOver(NAME_KnockJasonsMaskOffVO);
}

void ASCCounselorCharacter::MULTICAST_PlayWoundedVO_Implementation()
{
	// Play the wounded screams first (will also be heard by everyone in the distance)
	static FName NAME_WoundedScreamsVO(TEXT("WoundedScreams"));
	VoiceOverComponent->PlayVoiceOver(NAME_WoundedScreamsVO);

	// Now play the wounded VO for a little bit extra flavor. We queue this up with the VO system.
	static FName NAME_WoundedVO(TEXT("Wounded"));
	VoiceOverComponent->PlayVoiceOver(NAME_WoundedVO, false, true);
}

void ASCCounselorCharacter::MULTICAST_PlayHitImpactVO_Implementation()
{
	static FName NAME_ImpactVO(TEXT("Impact"));
	VoiceOverComponent->PlayVoiceOver(NAME_ImpactVO);
}

void ASCCounselorCharacter::MULTICAST_PlayDiedVO_Implementation()
{
	static FName NAME_DiedVO(TEXT("Died"));
	VoiceOverComponent->PlayVoiceOver(NAME_DiedVO);
}

float ASCCounselorCharacter::GetWeaponDurabilityModifier() const
{
	float BaseDurabilityMod = WeaponDurabilityMod.Get(this);
	// Loop through perks and add to the modifier
	float PerkDurabilityMod = 0.f;
	if (ASCPlayerState* PS = GetPlayerState())
	{
		float DurabilityModifier = 0.0f;
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			PerkDurabilityMod += Perk->WeaponDurabilityModifier;
		}
	}

	return BaseDurabilityMod + PerkDurabilityMod;
}

void ASCCounselorCharacter::UseAbility()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	// If the timer wasn't active, we held long enough to show the emote menu and don't want to now use our ability
	if (!World->GetTimerManager().IsTimerActive(TimerHandle_ShowEmoteSelect) && IsUsingGamepad())
		return;

	World->GetTimerManager().ClearTimer(TimerHandle_ShowEmoteSelect);

	//const bool bCanUseActiveAbility = ActiveAbility && ActiveAbility->CanUseAbility(); // NOTE: Disabled since we're not handling counselor abilities right now, RIP
	const bool bCanUseSweaterAbility = SweaterAbility && SweaterAbility->CanUseAbility();
	if (bCanUseSweaterAbility)
	{
		SERVER_UseAbility();
	}
	else if (HasSpecialItem(ASCImposterOutfit::StaticClass()))
	{
		if (IsInHiding() || IsInSpecialMove() || CurrentStance == ESCCharacterStance::Swimming)
			return;

		if (ASCPlayerController_Paranoia* PC = Cast<ASCPlayerController_Paranoia>(Controller))
		{
			if (PC->CanPlayerTransform())
				PC->SERVER_TransformIntoKiller();
		}
	}
}

bool ASCCounselorCharacter::IsUsingSweaterAbility() const
{
	if (!SweaterAbility)
		return false;

	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			if (AnimInst->SweaterAbilityMontage && AnimInst->Montage_IsPlaying(AnimInst->SweaterAbilityMontage))
				return true;
		}
	}

	return false;
}

void ASCCounselorCharacter::SERVER_UseAbility_Implementation()
{
	// The server needs to validate if these should happen.
	if ((SweaterAbility && SweaterAbility->CanUseAbility())) 
		//(ActiveAbility && ActiveAbility->CanUseAbility())) // NOTE: Disabled since we're not handling counselor abilities right now, RIP
	{
		// Bump idle timer
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			PC->BumpIdleTime();

		MULTICAST_UseAbility();
	}
}

bool ASCCounselorCharacter::SERVER_UseAbility_Validate()
{
	return true;
}

void ASCCounselorCharacter::MULTICAST_UseAbility_Implementation()
{
	if (SweaterAbility && SweaterAbility->CanUseAbility())
	{
		SweaterAbility->Activate(this);

		// Play the pamela sweater VO.
		static FName NAME_PamelaSweaterActivate = TEXT("PamelaSweaterActivate");
		VoiceOverComponent->PlayVoiceOver(NAME_PamelaSweaterActivate);

		if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
		{
			if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(SkeletalMesh->GetAnimInstance()))
			{
				const float MontageTime = AnimInst->Montage_Play(AnimInst->SweaterAbilityMontage) * 2.0f;
				GetWorldTimerManager().SetTimer(TimerHandle_SweaterTimer, this, &ASCCounselorCharacter::TakeOffPamelasSweater, MontageTime > 0.0f ? MontageTime : 0.1f, false);
			}
		}

#if !UE_BUILD_SHIPPING
		// In dev builds, check to see if we have the cheat enabled
		if (!bUnlimitedSweaterStun)
		{
#endif
		if (IsLocallyControlled())
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			{
				PC->GetSCHUD()->SetSweaterAbilityAvailable(false);
			}
		}
#if !UE_BUILD_SHIPPING
		}
		else // if (bUnlimitedSweaterStun)
		{
			SweaterAbility->SetUsed(false);
		}
#endif
	}
	else if (ActiveAbility && ActiveAbility->CanUseAbility())
	{
		ActiveAbility->Activate(this);
	}
}

void ASCCounselorCharacter::ActivateAbility(USCCounselorActiveAbility* Ability)
{
	CurrentActiveAbilities.Add(FDateTime::Now().GetTicks(), Ability);
}

float ASCCounselorCharacter::GetAbilityModifier(EAbilityType Type) const
{
	float Modifier = 0.f;

	for (auto& Ability : CurrentActiveAbilities)
	{
		if (Ability.Value->AbilityType == Type)
		{
			Modifier += Ability.Value->AbilityModifierValue;
		}
	}

	return Modifier;
}

bool ASCCounselorCharacter::IsAbilityActive(EAbilityType Type) const
{
	for (auto& Ability : CurrentActiveAbilities)
	{
		if (Ability.Value->AbilityType == Type)
		{
			return true;
		}
	}

	return false;
}

void ASCCounselorCharacter::RemoveAbility(EAbilityType Type)
{
	for (auto It = CurrentActiveAbilities.CreateConstIterator(); It; ++It)
	{
		if (It.Value()->AbilityType == Type)
		{
			CurrentActiveAbilities.Remove(It.Key());
		}
	}
}

void ASCCounselorCharacter::UpdateActiveAbilities()
{
	const int64 CurrentTime = FDateTime::Now().GetTicks();
	for (auto It = CurrentActiveAbilities.CreateConstIterator(); It; ++It)
	{
		const float TimeActive = (float)(CurrentTime - It.Key()) / ETimespan::TicksPerSecond;
		if (TimeActive >= It.Value()->AbilityDuration)
		{
			if (It.Value()->AbilityType == EAbilityType::Spotting)
			{
				bIsKillerOnMap = false;
				OutOfSightTime = 0.1f;
			}

			CurrentActiveAbilities.Remove(It.Key());
		}
	}
}

void ASCCounselorCharacter::KilledBy(APawn* EventInstigator)
{
	// If we suicide and we're currently attached to another actor it probably means we dc'd like bitches.
	if (EventInstigator == this && GrabbedBy != nullptr)
	{
		EventInstigator = Cast<APawn>(GrabbedBy);
	}

	Super::KilledBy(EventInstigator);
}

float ASCCounselorCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	check(Role == ROLE_Authority);

	// if player already is dead, get the f out of here
	if (!IsAlive() || HasEscaped())
		return 0.0f;

	// Disables team killing for quick play matches
	if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		if (!GameMode->CanTeamKill())
		{
			if (DamageCauser && DamageCauser->IsA<ASCCounselorCharacter>())
				return 0.0f;
		}
	}

	// Don't take damage while getting out of a vehicle, it can put us in a bad spot and we'd rather be invulnerable
	if (bLeavingSeat)
		return 0.0f;

	if (bInStun && !DamageEvent.DamageTypeClass->IsChildOf(USCStunDamageType::StaticClass()))
		EndStun();

	// Cancel an emote animation
	if (CurrentEmote)
	{
		SERVER_CancelEmote();
	}

	MULTICAST_DamageTaken(Damage, DamageEvent, EventInstigator, DamageCauser);

	float BlockDamageModifier = 1.f;
	// attempt a block
	if (IsBlocking())
	{
		if (ASCWeapon* Weapon = GetCurrentWeapon())
		{
			Weapon->WearDurability(false);
			if (CVarDebugCombat.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s blocked attack from %s"), *GetClass()->GetName(), DamageCauser ? *DamageCauser->GetClass()->GetName() : TEXT("unknown")));
			}
			
			BlockDamageModifier = BlockingDamageMod.Get(this, true);
		}

		// If we are hit with a heavy attack, break the block
		ASCCharacter* Attacker = Cast<ASCCharacter>(DamageCauser);
		if (Attacker && Attacker->IsHeavyAttacking())
		{
			OnEndBlock();
		}
	}

	float PerkDamageModifier = 1.f;
	// See if the counselor has any perks to mitigate or increase damage taken
	if (ASCPlayerState* CounselorPS = GetPlayerState())
	{
		float DamageMod = 1.f;
		for (const USCPerkData* Perk : CounselorPS->GetActivePerks())
		{
			if (!Perk)
				continue;

			PerkDamageModifier += Perk->WeaponDamageTakenModifier;
		}
	}

	bool Stunned = false;
	float ActualStunDamage = 0.0f;
	// if the attack should stun, apply the stun
	const USCStunDamageType* const StunDefaults = DamageEvent.DamageTypeClass ? Cast<const USCStunDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;
	if (StunDefaults)
	{
		// Only get stunned by non counselors
		if (DamageCauser == nullptr || DamageCauser == this || !DamageCauser->IsA<ASCCounselorCharacter>())
		{
			Stunned = true;
			FSCStunData StunData;
			StunData.StunInstigator = DamageCauser;
			StunData.StunTimeModifier = 1.0f;

			// Apply perk modifiers
			if (ASCPlayerState* PS = GetPlayerState())
			{
				for (const USCPerkData* Perk : PS->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					if (DamageCauser)
					{
						// Check to see if we have a perk that lets us escape traps faster
						if (DamageCauser->IsA<ASCTrap>())
							StunData.StunTimeModifier += Perk->TrapStunTimeModifier;
						// Grab mini game stun
						else if (DamageCauser->IsA<ASCKillerCharacter>())
							StunData.StunTimeModifier += Perk->BreakGrabDurationModifier;
					}
				}
			}

			BeginStun(StunDefaults->GetClass(), StunData);

			ActualStunDamage = AILLCharacter::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser) * StunnedDamageMod.Get(this, true) * PerkDamageModifier;
		}
	}

	// Attempt to apply damage
	const bool WasWounded = IsWounded();
	const float ActualDamage = Stunned ? ActualStunDamage : (Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser) * BlockDamageModifier * PerkDamageModifier);
	if (ActualDamage > 0.0f)
	{
		if (ASCPlayerState* PS = GetPlayerState())
		{
			PS->TookDamage();
		}
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, (float)GetMaxHealth());

		// if this attack killed the player
		if (!IsAlive())
		{
			ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(DamageCauser);
			if (Killer)
			{
				if (Role == ROLE_Authority)
				{
					// If we're the hunter we should check to see if we sacrificed ourselves for any of the other counselors.
					if (bIsHunter && !GetWorld()->GetGameState()->IsA(ASCGameState_Paranoia::StaticClass()))
					{
						const FVector MyLocation = GetActorLocation();
						if (const ASCGameState* GS = GetWorld()->GetGameState<ASCGameState>())
						{
							for (const ASCCharacter* Character : GS->PlayerCharacterList)
							{
								if (Character == this)
									continue;

								if (!Character->IsAlive())
									continue;

								if (const ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
								{
									const float Distance = (MyLocation - Character->GetActorLocation()).Size();
									if (Distance <= ActiveSacrificeRadius)
									{
										if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
										{
											GameMode->HandleScoreEvent(GetPlayerState(), SacrificeScoreEvent);
											break;
										}
									}
								}
							}
						}
					}
				}

				// See if we should dismember a limb
				ASCWeapon* KillerWeapon = Killer->GetCurrentWeapon();
				const int DismembermentChance = FMath::RandRange(0, 100);
				if (!IsInContextKill() && KillerWeapon && DismembermentChance < Killer->CombatDismemberChance)
				{
					if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
					{
						const FPointDamageEvent& PointDamageEvent = (const FPointDamageEvent&)DamageEvent;
						if (CVarDebugCombat.GetValueOnGameThread() > 0)
						{
							GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s hit bone %s on %s"), DamageCauser ? *DamageCauser->GetClass()->GetName() : TEXT("unknown"), *PointDamageEvent.HitInfo.BoneName.ToString(), *GetClass()->GetName()));
						}

						if (USCCharacterBodyPartComponent* HitLimb = GetBodyPartWithBone(PointDamageEvent.HitInfo.BoneName))
						{
							if (HitLimb->IsAttachedTo(GetMesh()) && HitLimb != TorsoLimb) // Don't give XP for dismembering the torso since that can't be detached
							{
								SERVER_DetachLimb(HitLimb, FVector::ZeroVector);

								if (CVarDebugCombat.GetValueOnGameThread() > 0)
								{
									GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Detaching limb: %s on %s"), *HitLimb->GetName(), *GetClass()->GetName()));
								}
							}
						}
					}
				}

				// Play died VO on the counselor
				MULTICAST_PlayDiedVO();

				// Play Pamela VO when Jason kills a counselor
				Killer->MULTICAST_PlayPamelaVOFromCounselorDeath();
			}

			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
			if (CVarDebugCombat.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s Killed"), *GetClass()->GetName()));
			}

		}// else if this attack wounded the player
		else if (Health <= WoundedThreshold && !WasWounded)
		{
			OnWounded();

			// Track number of wounded counselors
			if (ASCGameState_Hunt* GS = GetWorld()->GetGameState<ASCGameState_Hunt>())
			{
				GS->CounselorsWoundedCount++;
			}

			if (ASCGameMode_Paranoia* GSP = GetWorld()->GetAuthGameMode<ASCGameMode_Paranoia>())
			{
				if (ASCCharacter* Character = Cast<ASCCharacter>(DamageCauser))
					GSP->HandlePlayerTakeDamage(Character->GetController(), GetController());
			}

			// Play wounded VO
			MULTICAST_PlayWoundedVO();

			if (CVarDebugCombat.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Blue, FString::Printf(TEXT("%s Wounded"), *GetClass()->GetName()));
			}
		}
		// just hit player normal impact VO.
		else
		{
			if (ASCGameMode_Paranoia* GSP = GetWorld()->GetAuthGameMode<ASCGameMode_Paranoia>())
			{
				if (ASCCharacter* Character = Cast<ASCCharacter>(DamageCauser))
					GSP->HandlePlayerTakeDamage(Character->GetController(), GetController());
			}

			// Don't play an impact VO after a window dive because we already played it before.
			if (!DamageEvent.DamageTypeClass || !DamageEvent.DamageTypeClass->IsChildOf(USCWindowDiveDamageType::StaticClass()))
			{
				MULTICAST_PlayHitImpactVO();
			}
		}

		// Fake sight on Jason so we flee
		if (ASCCounselorAIController* AIController = Cast<ASCCounselorAIController>(Controller))
		{
			if (IsAlive() && DamageCauser && (DamageCauser->IsA<ASCKillerCharacter>() || DamageCauser->IsA<ASCTrap>()))
			{
				ASCGameState* GameState = GetWorld()->GetGameState<ASCGameState>();
				if (GameState && GameState->CurrentKiller)
					AIController->ReceivedKillerStimulus(GameState->CurrentKiller, GameState->CurrentKiller->GetActorLocation(), true, 1.f);
			}
		}

		ApplyDamageGore(DamageEvent.DamageTypeClass);
	}

	return ActualDamage;
}

void ASCCounselorCharacter::MULTICAST_DamageTaken_Implementation(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	Super::MULTICAST_DamageTaken_Implementation(Damage, DamageEvent, EventInstigator, DamageCauser);

	// If we have an EquippedItem and it isn't attached to our character, re-attach it to our hand
	if (EquippedItem)
	{
		EquippedItem->AttachToCharacter(false);
	}

	// if we take damage we need to abort using items.
	if (bUsingSmallItem)
	{
		for (ASCItem* Item : GetSmallItems())
		{
			Item->EndItemUse();
		}
		bUsingSmallItem = false;
	}

	if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		// We got hit or something and the special move was interrupted, Make sure we reset our vehicle shitz.
		if (!CurrentSeat)
		{
			AnimInst->ForceOutOfVehicle();
		}

		AnimInst->CancelUseItem();
	}

	ApplyDamageGore(DamageEvent.DamageTypeClass);
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			HUD->CloseCounselorMap();
		}
	}
}

void ASCCounselorCharacter::CleanupLocalAfterDeath(ASCPlayerController* PlayerController)
{
	Super::CleanupLocalAfterDeath(PlayerController);

	if (PlayerController)
	{
		PlayerController->PopInputComponent(PassengerInputComponent);
		PlayerController->PopInputComponent(AimInputComponent);
		PlayerController->PopInputComponent(HidingInputComponent);
	}
}

AController* ASCCounselorCharacter::GetCharacterController() const
{
	if (CurrentSeat && CurrentSeat->IsDriverSeat())
	{
		if (CurrentSeat->ParentVehicle->Controller)
			return CurrentSeat->ParentVehicle->Controller;
	}

	if (PendingSeat && PendingSeat->IsDriverSeat())
	{
		if (PendingSeat->ParentVehicle->Controller)
			return PendingSeat->ParentVehicle->Controller;
	}

	// We aren't in a car or it won't cast, try the character controller instead
	return Super::GetCharacterController();
}

void ASCCounselorCharacter::ClearAnyInteractions()
{
	// Double checking this here (same as Super) to make sure we DIE and so we can leave our hiding spot before clearing interactions
	if (bInContextKill)
		return;

	// Leave our hiding spot
	if (CurrentHidingSpot)
		CurrentHidingSpot->CancelHiding();

	Super::ClearAnyInteractions();
}

void ASCCounselorCharacter::UpdateSwimmingState()
{
	// Don't update water stuff if we're grabbed please.
	if (CurrentStance == ESCCharacterStance::Grabbed)
		return;

	if (IsInSpecialMove())
		return;

	// We don't want to swim immediately, check our depth before swimming.
	if (CurrentWaterVolume && GetCharacterMovement()->NavAgentProps.bCanSwim)
	{
		const FVector CounselorLocation = GetActorLocation();
		const float WaterZ = GetWaterZ();

		const FVector Start(CounselorLocation.X, CounselorLocation.Y, WaterZ);

		FCollisionQueryParams TraceParams(TEXT("WaterDepthTrace"), false, this);
		TraceParams.AddIgnoredActor(EquippedItem);

		UWorld* World = GetWorld();
		if (ASCGameState* GameState = World->GetGameState<ASCGameState>())
		{
			// Add the Killer(s) to our list of ignored actors
			for (ASCCharacter* Character : GameState->PlayerCharacterList)
			{
				if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
					TraceParams.AddIgnoredActor(Killer);
			}
		}

		for (const AILLInventoryItem* SpecialItem : SpecialItemInventory->GetItemList())
			TraceParams.AddIgnoredActor(SpecialItem);

		for (const AILLInventoryItem* SmallItem : SmallItemInventory->GetItemList())
			TraceParams.AddIgnoredActor(SmallItem);

		TraceParams.bTraceAsyncScene = true;

		// If true, the water isn't deeper than MinWaterDepthForSwimming
		const bool bIsWaterShallow = (CounselorLocation.Z - MinWaterDepthForSwimming) > WaterZ || World->LineTraceTestByChannel(Start, Start - FVector::UpVector * MinWaterDepthForSwimming, ECollisionChannel::ECC_WorldDynamic, TraceParams);

		if (bIsWaterShallow == false)
		{
			if (GetCharacterMovement()->MovementMode != MOVE_Swimming || CurrentStance != ESCCharacterStance::Swimming)
			{
				OnAimReleased();

				if (CurrentStance == ESCCharacterStance::Combat)
					SetCombatStance(false);

				SetStance(ESCCharacterStance::Swimming);
				GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
				GetCharacterMovement()->bOrientRotationToMovement = true;

				// prevent people from getting back out of the water and crouching
				if (PreviousStance == ESCCharacterStance::Crouching)
					PreviousStance = ESCCharacterStance::Standing;

				FTransform trans = GetActorTransform();
				trans.SetLocation(Start);
				RippleComponent = UGameplayStatics::SpawnEmitterAtLocation(World, RippleParticle, trans, false);
			}

			if (RippleComponent)
			{
				FTransform trans = GetActorTransform();
				trans.SetLocation(Start + FVector(0, 0, 0.1f) + (GetActorForwardVector()*40.0f));
				RippleComponent->SetWorldTransform(trans, false, nullptr, ETeleportType::TeleportPhysics);
			}

			// Prevent the player camera from dipping bellow the surface of the water, even if we're not swimming
			if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
			{
				FRotator Rotation = PlayerController->GetControlRotation();
				Rotation.Normalize();
				if (Rotation.Pitch > 0.f)
				{
					Rotation.Pitch = 0.f;
					PlayerController->SetControlRotation(Rotation);
				}
			}

			// Cancel current emote, otherwise we dance and swim at the same time.
			if (CurrentEmote)
			{
				SERVER_CancelEmote();
			}
		}
		else if (bIsWaterShallow && CurrentStance == ESCCharacterStance::Swimming)
		{
			SetStance(PreviousStance);
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);

			if (RippleComponent)
			{
				RippleComponent->KillParticlesForced();
				RippleComponent->DestroyComponent();
				RippleComponent = nullptr;
			}
		}
	}
	else if (CurrentWaterVolume == nullptr && CurrentStance == ESCCharacterStance::Swimming)
	{
		SetStance(PreviousStance);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void ASCCounselorCharacter::OnForwardPC(float Val)
{
	if (IsDodging() || IsMoveInputIgnored())
		return;

	if (CurrentEmote)
	{
		if (bEmoteBlocksMovement)
		{
			return;
		}
		else if (FMath::Abs(Val) > KINDA_SMALL_NUMBER)
		{
			SERVER_CancelEmote();
		}
	}

	if (CurrentStance == ESCCharacterStance::Swimming)
		Val = FMath::Max(Val, 0.0f);

	Super::OnForwardPC(Val);
}

void ASCCounselorCharacter::OnRightPC(float Val)
{
	if (IsDodging() || IsMoveInputIgnored())
		return;

	if (CurrentEmote)
	{
		if (bEmoteBlocksMovement)
		{
			return;
		}
		else if (FMath::Abs(Val) > KINDA_SMALL_NUMBER)
		{
			SERVER_CancelEmote();
		}
	}

	if (CurrentStance == ESCCharacterStance::Swimming)
		return;

	Super::OnRightPC(Val);
}


void ASCCounselorCharacter::OnForwardController(float Val)
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (!Input->IsUsingGamepad())
				return;
		}
	}

	if (IsMoveInputIgnored())
		return;

	if (CurrentStance == ESCCharacterStance::Swimming)
		Val = FMath::Max(Val, 0.0f);

	CurrentInputVector.X = Val;

	if (CurrentEmote)
	{
		if (bEmoteBlocksMovement)
		{
			if (bLoopingEmote && CurrentInputVector.IsZero())
			{
				bEmoteBlocksMovement = false;
			}

			return;
		}
		else if (FMath::Abs(Val) > KINDA_SMALL_NUMBER)
		{
			SERVER_CancelEmote();
		}
	}

	const bool ShouldRun = CurrentInputVector.Size() >= 0.85f;

	if (ShouldRun != IsRunning())
		SetRunning(ShouldRun);

	Super::OnForwardController(CurrentInputVector.GetSafeNormal().X);
}

void ASCCounselorCharacter::OnRightController(float Val)
{
	if (CurrentStance == ESCCharacterStance::Swimming)
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (!Input->IsUsingGamepad())
				return;
		}
	}

	if (IsMoveInputIgnored())
		return;

	CurrentInputVector.Y = Val;

	if (CurrentEmote)
	{
		if (bEmoteBlocksMovement)
		{
			if (bLoopingEmote && CurrentInputVector.IsZero())
			{
				bEmoteBlocksMovement = false;
			}

			return;
		}
		else if (FMath::Abs(Val) > KINDA_SMALL_NUMBER)
		{
			SERVER_CancelEmote();
		}
	}

	const bool ShouldRun = CurrentInputVector.Size() >= 0.85f;

	if (ShouldRun != IsRunning())
		SetRunning(ShouldRun);

	Super::OnRightController(CurrentInputVector.GetSafeNormal().Y);
}

void ASCCounselorCharacter::OnDodge()
{
	if (CurrentInputVector.X < -0.5f)
	{
		DodgeBack();
	}
	else if (CurrentInputVector.Y > 0.0f)
	{
		DodgeRight();
	}
	else if (CurrentInputVector.Y < 0.0f)
	{
		DodgeLeft();
	}
}

void ASCCounselorCharacter::ToggleMap()
{
	// Don't open the map if we're interacting with something.
	if (bHoldInteractStarted)
		return;

	// Don't allow opening the map while in a special move.
	if (IsInSpecialMove())
		return;

	OnAimReleased();

	OnHideScoreboard();

	OnHideEmoteSelect();

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			if (bIsGrabbed)
				HUD->CloseCounselorMap();
			else
				HUD->OnShowCounselorMap();
		}
	}
}

void ASCCounselorCharacter::OnAimPressed()
{
	// No rooty tooty point and shooty if we're spawning.
	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		if (!PlayerController->IsGameInputAllowed())
			return;
	}

	if (CurrentStance == ESCCharacterStance::Swimming)
		return;

	if (InteractPressed)
		return;

	if (bAttemptingInteract)
		return;

	if (PickingItem)
		return;

	if (IsInSpecialMove())
		return;

	// cant aim if you are getting in or out of a vehicle
	if (IsInVehicle() || PendingSeat || bLeavingSeat)
		return;

	if (InteractableManagerComponent->GetLockedInteractable())
		return;

	// Can't use items while escaping
	if (IsEscaping() || HasEscaped())
		return;

	if (CurrentEmote)
	{
		if (bEmoteBlocksMovement)
			return;
		else
			SERVER_CancelEmote();
	}

	// No using the sweater AND aiming!
	if (IsUsingSweaterAbility())
		return;

	if (ASCGun* Gun = Cast<ASCGun>(EquippedItem))
	{
		Gun->AimGun(true);
		if (Gun->HasAmmunition())
		{
			if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
			{
				if (!PlayerController->IsMoveInputIgnored())
					PlayerController->SetIgnoreMoveInput(true);

				if (ASCInGameHUD* HUD = PlayerController->GetSCHUD())
				{
					HUD->OnStartAiming();
				}
			}
		}
	}
}

void ASCCounselorCharacter::OnAimReleased()
{
	// No rooty tooty point and shooty if we're spawning.
	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		if (!PlayerController->IsGameInputAllowed())
			return;
	}

	if (ASCGun* Gun = Cast<ASCGun>(EquippedItem))
	{
		Gun->AimGun(false);

		if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
		{
			PlayerController->SetIgnoreMoveInput(false);

			if (ASCInGameHUD* HUD = PlayerController->GetSCHUD())
			{
				HUD->OnEndAiming();
			}
		}
	}
}

bool ASCCounselorCharacter::TryUseStamina(float StaminaToUse)
{
	if (CanUseStamina(StaminaToUse) == false)
		return false;

	// Use the stamina
	const float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Max(CurrentStamina - StaminaToUse, 0.f);

	const float StaminaDelta = OldStamina - CurrentStamina;
	if (StaminaDelta >= 1.f)
		OnStaminaChunkUsed(StaminaDelta);

	if (bLockStamina)
		CurrentStamina = OldStamina;

	return true;
}

void ASCCounselorCharacter::ReturnStamina(float StaminaToGain)
{
	const float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Min(CurrentStamina + StaminaToGain, StaminaMax);
	const float StaminaDelta = OldStamina - CurrentStamina;
	if (StaminaDelta >= 1.f)
		OnStaminaChunkGained(StaminaDelta);

	if (bLockStamina)
		CurrentStamina = OldStamina;
}

void ASCCounselorCharacter::UpdateStamina(float DeltaSeconds)
{
	if (bLockStamina)
		return;

	if (IsAttacking())
		return;

	const float Velocity = GetVelocity().SizeSquared2D();

	// Get perk modifiers
	const float RadioModifier = [this]()
	{
		float Modifier = 1.0f;

		if (IsInRangeOfRadioPlaying())
		{
			if (ASCPlayerState* PS = GetPlayerState())
			{
				for (const USCPerkData* Perk : PS->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					Modifier += Perk->RadioStaminaModifier;
				}
			}
		}

		return Modifier;
	}();

	if (CVarDebugStamina.GetValueOnGameThread() > 0 && IsInRangeOfRadioPlaying())
	{
		if (ASCPlayerState* CounselorState = GetPlayerState())
		{
			float PerkEasyListeningMod = 1.f;
			for (const USCPerkData* Perk : CounselorState->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				PerkEasyListeningMod += Perk->RadioStaminaModifier;
			}

			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Yellow, FString::Printf(TEXT("Easy Listening Stamina Perk Mod: %.2f"), PerkEasyListeningMod), false);
		}
	}

	const float StaminaRefill = StaminaRefillRate.Get(this) * StaminaFearRefillModifier->GetFloatValue(FearManager->GetFear());
	// Swimming is a special case for stamina
	if (CurrentStance == ESCCharacterStance::Swimming)
	{
		const float RequestedVelocity = GetRequestedMoveInput().SizeSquared();

		// Only drain stamina while we're actively swimming, and only restore it when we're at a full stop
		if (Velocity > KINDA_SMALL_NUMBER)
		{
			if (RequestedVelocity > KINDA_SMALL_NUMBER)
			{
				CurrentStamina -= StaminaDepleteRate * GetStaminaModifier() * DeltaSeconds;
			}
		}
		else
		{
			CurrentStamina += StaminaRefill * GetStaminaModifier() * RadioModifier * DeltaSeconds;
		}
	}
	else
	{
		if (Velocity > KINDA_SMALL_NUMBER && !IsInSpecialMove() && !IsStunned() && !bHoldInteractStarted)
		{
			// Don't drain stamina while we're stumbling
			if (bIsStumbling == false && !IsAbilityActive(EAbilityType::Sprint))
				CurrentStamina -= StaminaDepleteRate * GetStaminaModifier() * DeltaSeconds; 
		}
		else
		{
			if (bIsGrabbed)
			{
				CurrentStamina += GrabbedStaminaRefillRate.Get(this) * GetStaminaModifier() * RadioModifier * DeltaSeconds;
			}
			else
				CurrentStamina += StaminaRefill * GetStaminaModifier() * RadioModifier * DeltaSeconds;
		}
	}

	// Force Sprinting off and Stumble when out of stamina
	if (CurrentStamina <= 0.f)
	{
		if (IsSprinting() && bForceStumbleOnStaminaDepletion && CurrentStance == ESCCharacterStance::Standing)
			SetStumbling(true, true);

		// if out of stamina, stop running or sprinting
		if (IsSprinting() && IsLocallyControlled())
		{
			SetSprinting(false);
			if (!IsRunning())
				SetRunning(true);
		}


		bStaminaDepleted = true;
	}

	// See if we've regenerated enough stamina to use it again
	if (bStaminaDepleted && CurrentStamina >= StaminaMinThreshold)
		bStaminaDepleted = false;

	// Clamp CurrentStamina between 0 and 100
	CurrentStamina = FMath::Clamp((float)CurrentStamina, 0.f, 100.f);
}

void ASCCounselorCharacter::ForceModifyStamina(float InStaminaChange)
{
	if (bLockStamina)
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_ForceModifyStamina(InStaminaChange);
	}
	else
	{
		CurrentStamina = FMath::Clamp(CurrentStamina + InStaminaChange, 0.f, 100.f);
	}
}

bool ASCCounselorCharacter::SERVER_ForceModifyStamina_Validate(float InStaminaChange)
{
	return true;
}

void ASCCounselorCharacter::SERVER_ForceModifyStamina_Implementation(float InStaminaChange)
{
	ForceModifyStamina(InStaminaChange);
}

void ASCCounselorCharacter::UpdateGrabTransform()
{
	if (!IsValid(GrabbedBy))
		return;

	// We're simulating physics so stop trying to update location.
	if (GetMesh()->IsSimulatingPhysics() || GetMesh()->bPauseAnims)
		return;

	const FRotator GrabRotation = GrabbedBy->GetMesh()->GetSocketRotation(GrabbedBy->CharacterAttachmentSocket);// the rotation of the grab socket in world space
	// snap the rotation to the attachment socket
	SetActorRotation(GrabRotation, ETeleportType::TeleportPhysics);

	//	have to reset the camera every frame to smooth out the camera from rotations
	if (IsLocallyControlled())
		ResetCameras();

	// if we are grabbed and not in a context kill, align the character to the killers hand socket
	if (!IsInContextKill())
	{
		const FVector HandSocket = GrabbedBy->GetMesh()->GetSocketLocation(NAME_KillerHandSocket);// socket to jasons hand in world space
		const FVector NeckSocket = GetMesh()->GetSocketLocation(NAME_NeckSocket);// socket to neck in world space
		const FVector NeckOffset = NeckSocket - GetMesh()->GetComponentLocation(); // get the neck bone in local space of the mesh translated by the pivot location

		// Take the Killers hand socket location, subtract the NeckOffset to put the hand socket location to the neck pivot location, then subtract the relative transform of the mesh
		// to ensure it lines up
		const FVector WorldLocation = HandSocket - NeckOffset - GetMesh()->GetRelativeTransform().GetTranslation();
		SetActorLocation(WorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else // we are in a context kill, just snap to the attachment socket
	{
		// REMOVE ME!
		FVector RootLocation = GetActorLocation();
		const FVector GrabSocket = GrabbedBy->GetMesh()->GetSocketLocation(GrabbedBy->CharacterAttachmentSocket);// socket to attach location in world space
		SetActorLocation(GrabSocket-GetMesh()->RelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);// set the actor location to the grab socket minus the relative offset of the mesh
	}
}

void ASCCounselorCharacter::FollowExitSpline(float DeltaTime)
{
	if (!ExitSpline)
		return;

	if (!IsSprinting())
		SetSprinting(true);

	if (ExitSpline->GetNumberOfSplinePoints() > 0 && SplineDistanceTraveled < ExitSpline->GetSplineLength())
	{
		const float Length = ExitSpline->GetSplineLength();
		float MinSpeed = 100.f;
		if (USCCharacterMovement* Movement = Cast<USCCharacterMovement>(GetCharacterMovement()))
		{
			MinSpeed = Movement->MaxWalkSpeed;
		}
		const float Speed = FMath::Max(GetVelocity().Size(), MinSpeed);
		const FVector Direction = ExitSpline->GetDirectionAtDistanceAlongSpline(SplineDistanceTraveled, ESplineCoordinateSpace::World);
		AddMovementInput(Direction, Speed);
		SplineDistanceTraveled = SplineDistanceTraveled + Speed * DeltaTime;
	}
	else
	{
		GetMovementComponent()->StopMovementImmediately();
		SetActorLocation(ExitSpline->GetLocationAtDistanceAlongSpline(ExitSpline->GetSplineLength(), ESplineCoordinateSpace::World));
		ExitSpline = nullptr;
	}
}

void ASCCounselorCharacter::SetEscaped(USplineComponent* ExitPath, float EscapeDelay)
{
	bHasEscaped = true;
	ExitSpline = ExitPath;
	SplineDistanceTraveled = 0;
	
	if (bIsGrabbed && GrabbedBy)
		GrabbedBy->DropCounselor();

	if (!IsInVehicle())
		SetStance(ESCCharacterStance::Standing);

	LockStaminaTo(100.f, true);

	if (OnEscaped.IsBound())
		OnEscaped.Broadcast();

	if (IsLocallyControlled())
	{
		StopCameraShakeInstances(UCameraShake::StaticClass());
	}

	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		PPV->Settings.ColorGradingIntensity = 0.0f;
		PPV->Settings.RemoveBlendable(FearScreenEffectInst);
		PPV->Settings.RemoveBlendable(TeleportScreenEffectInst);
	}

	// Shutdown the music system.
	if (MusicComponent)
	{
		MusicComponent->ShutdownMusicComponent();
		MusicComponent->DestroyComponent();
		MusicComponent = nullptr;
	}

	if (Role == ROLE_Authority)
	{
		if (EscapeDelay > 0.f)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_Escape, this, &ASCCounselorCharacter::OnDelayedDropItem, EscapeDelay, false);
		}
		else
		{
			if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				DropItemsOnShore();

				GameMode->CharacterEscaped(Controller, this);
			}
		}

		const FVector MyLocation = GetActorLocation();
		if (const ASCGameState* GS = GetWorld()->GetGameState<ASCGameState>())
		{
			for (const ASCCharacter* Character : GS->PlayerCharacterList)
			{
				if (Character == this)
					continue;

				if (!Character->IsAlive())
					continue;

				if (const ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
				{
					/*if (!Counselor->bIsHunter)
						continue;*/ // 02/03/2021 - re-enable Hunter Selection

					const float Distance = (MyLocation - Character->GetActorLocation()).Size();
					if (Distance <= HunterEscortRadius)
					{
						if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
						{
							GameMode->HandleScoreEvent(Character->GetPlayerState(), EscortEscapeScoreEvent);
						}
					}
				}
			}
		}
	}
}

float ASCCounselorCharacter::GetStaminaModifier() const
{
	float Modifier = 1.f;
	const float Velocity = GetVelocity().SizeSquared2D();
	ASCPlayerState* CounselorState = GetPlayerState();

	bool bDrain = false;
	if (Velocity > 0.f && !IsInSpecialMove())
	{
		bDrain = true;
		switch (CurrentStance)
		{
		case ESCCharacterStance::Standing:
			if (IsSprinting())
				Modifier = StaminaSprintingModifier;
			else if (IsRunning())
				Modifier = StaminaRunningModifier;
			else
				Modifier = StaminaWalkingModifier * StaminaFearRefillModifier->GetFloatValue(FearManager->GetFear());;
			break;

		case ESCCharacterStance::Swimming:
			Modifier = StaminaSwimmingModifier;

			if (IsSprinting())
				Modifier += StaminaSprintingModifier;

			// Check perks for swimming stamina drain modifiers
			if (CounselorState)
			{
				float PerkSwimmingStaminaDrain = 0.f;
				for (const USCPerkData* Perk : CounselorState->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					PerkSwimmingStaminaDrain += Perk->SwimmingStaminaDrainModifier;
				}

				if (CVarDebugStamina.GetValueOnGameThread() > 0)
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Perk Swimming Stamina Drain Mod: %.2f"), PerkSwimmingStaminaDrain), false);
				}

				Modifier += Modifier * PerkSwimmingStaminaDrain;
			}

			break;

		case ESCCharacterStance::Crouching:
			Modifier = -StaminaCrouchingModifier; // Inverting because I think this is meant to be additive? TODO: Fix this ridiculous situation
			break;

		case ESCCharacterStance::Combat:
			Modifier = StaminaWalkingModifier;
			break;

			// lpederson - This seems a bit harsh.
		default:
			Modifier = 0.f;
			break;
		}

		// Check perks for an overall stamina drain modifier
		if (CounselorState)
		{
			float PerkStaminaDrain = 0.f;
			for (const USCPerkData* Perk : CounselorState->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				PerkStaminaDrain += Perk->StaminaDrainModifier;
			}

			if (CVarDebugStamina.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Perk Stamina Drain Mod: %.2f"), PerkStaminaDrain), false);
			}

			Modifier += Modifier * PerkStaminaDrain;
		}
	}
	else
	{
		switch (CurrentStance)
		{
		case ESCCharacterStance::Crouching:
			Modifier = StaminaCrouchingModifier;

			// Check perks for an increase to this modifier
			if (CounselorState)
			{
				float PerkCrouchingStaminaRegen = 0.f;
				for (const USCPerkData* Perk : CounselorState->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					PerkCrouchingStaminaRegen += Perk->CrouchingStaminaRegenModifier;
				}

				if (CVarDebugStamina.GetValueOnGameThread() > 0)
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Perk Crouching Stamina Regen Mod: %.2f"), PerkCrouchingStaminaRegen), false);
				}

				Modifier += Modifier * PerkCrouchingStaminaRegen;
			}
			break;

		default:
			Modifier = StaminaIdleModifier;

			if (IsInHiding())
				Modifier = StaminaHidingModifier;

			// Check perks for an increase to this modifier
			if (CounselorState)
			{
				float PerkStandingStaminaRegen = 0.f;
				for (const USCPerkData* Perk : CounselorState->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					PerkStandingStaminaRegen += Perk->StandingStaminaRegenModifier;
				}

				if (CVarDebugStamina.GetValueOnGameThread() > 0)
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Perk Standing Stamina Regen Mod: %.2f"), PerkStandingStaminaRegen), false);
				}

				Modifier += Modifier * PerkStandingStaminaRegen;
			}
			break;
		}
	}

	float TotalMod = Modifier * StaminaStatMod.Get(this, true);
	if (CVarDebugStamina.GetValueOnAnyThread() > 0)
	{
		if (bDrain)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Total Stamina Drain Modifier: %.2f"), TotalMod), false);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Green, FString::Printf(TEXT("Total Stamina Regen Modifier: %.2f"), TotalMod), false);
		}
	}

	return TotalMod;
}

float ASCCounselorCharacter::GetPerkInteractMod() const
{
	float PerkInteractMod = 1.f;
	if (ASCPlayerState* PC = GetPlayerState())
	{
		for (USCPerkData* Perk : PC->GetActivePerks())
		{
			check(Perk);
			if (!Perk)
				continue;

			PerkInteractMod += Perk->RepairSpeedModifier;
		}
	}

	return PerkInteractMod;
}

void ASCCounselorCharacter::Native_NotifyKillerOfMinigameFail(USoundCue* FailSound)
{
	if (Role < ROLE_Authority)
	{
		SERVER_NotifyKillerOfMinigameFail(FailSound);
	}
	else if (ASCGameState* GameState = GetWorld()->GetGameState<ASCGameState>())
	{
		for (ASCCharacter* Character : GameState->PlayerCharacterList)
		{
			if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
				Killer->CLIENT_NotifyMinigameFail(FailSound);
		}
	}
}

bool ASCCounselorCharacter::SERVER_NotifyKillerOfMinigameFail_Validate(USoundCue* FailSound)
{
	return true;
}

void ASCCounselorCharacter::SERVER_NotifyKillerOfMinigameFail_Implementation(USoundCue* FailSound)
{
	Native_NotifyKillerOfMinigameFail(FailSound);
}

bool ASCCounselorCharacter::IsSmallInventoryFull() const
{
	return SmallItemInventory->GetItemList().Num() >= MaxSmallItems;
}

TArray<ASCItem*> ASCCounselorCharacter::GetSmallItems() const
{
	TArray<ASCItem*> Items;
	for (auto Item : SmallItemInventory->GetItemList())
	{
		Items.Add(Cast<ASCItem>(Item));
	}

	return Items;
}

ASCItem* ASCCounselorCharacter::GetCurrentSmallItem() const
{
	if (SmallItemIndex >= SmallItemInventory->GetItemList().Num())
		return nullptr;

	if (SmallItemIndex < 0)
		return nullptr;

	return Cast<ASCItem>(SmallItemInventory->GetItemList()[SmallItemIndex]);
}

void ASCCounselorCharacter::RemoveSmallItemOfType(TSubclassOf<ASCItem> ItemClass)
{
	for (int32 i(0); i<SmallItemInventory->GetItemList().Num(); ++i)
	{
		AILLInventoryItem* Item = SmallItemInventory->GetItemList()[i];
		if (IsValid(Item) && Item->IsA(ItemClass))
		{
			SmallItemInventory->RemoveItem(Item);
			NextSmallItem();
			Item->Destroy();
			break;
		}
	}
}

ASCItem* ASCCounselorCharacter::HasSmallRepairPart(TSubclassOf<ASCRepairPart> PartClass) const
{
	for (auto Item : SmallItemInventory->GetItemList())
	{
		if (IsValid(Item) && Item->IsA(PartClass))
			return Cast<ASCItem>(Item);
	}

	return nullptr;
}

bool ASCCounselorCharacter::UseSmallRepairPart(ASCRepairPart* Part)
{
	for (int32 i(0); i < SmallItemInventory->GetItemList().Num(); ++i)
	{
		if (SmallItemInventory->GetItemList()[i] == Part)
		{
			SmallItemIndex = i;
			UseCurrentSmallItem(false);
			return true;
		}
	}

	return false;
}

TArray<ASCItem*> ASCCounselorCharacter::GetSpecialItems() const
{
	TArray<ASCItem*> Items;
	for (auto Item : SpecialItemInventory->GetItemList())
	{
		Items.Add(Cast<ASCItem>(Item));
	}

	return Items;
}

ASCItem* ASCCounselorCharacter::HasSpecialItem(TSubclassOf<ASCSpecialItem> ItemClass) const
{
	for (const auto& Item : SpecialItemInventory->GetItemList())
	{
		if (IsValid(Item) && Item->IsA(ItemClass))
			return Cast<ASCItem>(Item);
	}

	return nullptr;
}

bool ASCCounselorCharacter::RemoveSpecialItem_Validate(ASCSpecialItem* Item, bool bDestroyItem)
{
	return true;
}

void ASCCounselorCharacter::RemoveSpecialItem_Implementation(ASCSpecialItem* Item, bool bDestroyItem)
{
	SpecialItemInventory->RemoveItem(Item);

	if (bDestroyItem)
	{
		Item->Destroy();
	}
}

bool ASCCounselorCharacter::AddOrSwapPickingItem()
{
	check(Role == ROLE_Authority);

	if (Super::AddOrSwapPickingItem())
		return true;

	if (!PickingItem)
		return false;

	if (PickingItem->bIsSpecial)
	{
		SpecialItemInventory->AddItem(PickingItem);

		if (PickingItem->IsA<ASCPamelaSweater>())
		{
			PutOnPamelasSweater(Cast<ASCPamelaSweater>(PickingItem));
		}
	}
	else
	{
		// if our current small inventory is maxed out, empty a slot first
		if (IsSmallInventoryFull())
		{
			if (ASCItem* CurrentItem = GetCurrentSmallItem())
			{
				SmallItemInventory->RemoveItem(CurrentItem);

				// Grab the cabinet the item is in (if it's in one)
				if (PickingItem->CurrentCabinet)
				{
					ASCCabinet* Cabinet = Cast<ASCCabinet>(PickingItem->CurrentCabinet);
					Cabinet->AddItem(CurrentItem, false, this);
				}
				else
				{
					CurrentItem->OnDropped(this, PickingItem->GetActorLocation());
				}
			}
		}

		SmallItemInventory->AddItem(PickingItem);
		SmallItemIndex = SmallItemInventory->GetItemList().Num() - 1;
		OnRep_SmallItemIndex();
	}

	PickingItem->bIsPicking = false;
	PickingItem = nullptr;

	return true;
}

void ASCCounselorCharacter::OnSwapItemOnUse()
{
	if (Role == ROLE_Authority && EquippedItem && EquippedItem->SwapToOnUse)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;

		ASCItem* SwappedItem = Cast<ASCItem>(GetWorld()->SpawnActor(EquippedItem->SwapToOnUse, nullptr, SpawnParams));
		PickingItem = SwappedItem;
		PickingItem->bIsPicking = true;
		PickingItem->EnableInteraction(false);
		EquippedItem->Destroy();
		AddOrSwapPickingItem();
	}
}

void ASCCounselorCharacter::OnItemPickedUp(ASCItem* item)
{
	FearManager->PushFearEvent(item->PickedUpFearEvent);

	if (item->IsA<ASCPamelaSweater>())
	{
		PutOnPamelasSweater(Cast<ASCPamelaSweater>(item));
	}
}

void ASCCounselorCharacter::SERVER_OnInteract_Implementation(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod)
{
	// Server only variable
	if (bUsingSmallItem)
	{
		CLIENT_CancelInteractAttempt();
		return;
	}

	Super::SERVER_OnInteract_Implementation(Interactable, InteractMethod);
}

void ASCCounselorCharacter::OnInteract1Pressed()
{
	if (bIsAiming)
	{
		InteractPressed = false;
		InteractTimer = 0.0f;
		return;
	}

	if (bDropItemPressed)
		return;

	if (PendingSeat)
		return;

	if (bLargeItemPressHeld)
		return;

	OnHideEmoteSelect();
	OnAimReleased();

	Super::OnInteract1Pressed();
}

void ASCCounselorCharacter::OnInteract1Released()
{
	if (bIsAiming || bLargeItemPressHeld)
	{
		InteractPressed = false;
		InteractTimer = 0.0f;
		return;
	}

	if (bDropItemPressed)
		return;

	if (PendingSeat)
		return;

	// No interacting while the item is registered as picking (Such as dropping a gun or picking up a weapon
	if (EquippedItem && EquippedItem->bIsPicking)
		return;

	// No interacting while we are dropping items.
	if (bDroppingLargeItem || bDroppingSmallItem)
		return;

	Super::OnInteract1Released();
}

void ASCCounselorCharacter::OnAttack()
{
	if (CanAttack())
	{
		Super::OnAttack();

		if (CurrentEmote && !bEmoteBlocksMovement)
			SERVER_CancelEmote();
	}
}

void ASCCounselorCharacter::OnClientLockedInteractable(UILLInteractableComponent* LockingInteractable, const EILLInteractMethod InteractMethod, const bool bSuccess)
{
	Super::OnClientLockedInteractable(LockingInteractable, InteractMethod, bSuccess);

	if (!bSuccess && InteractMethod == EILLInteractMethod::Hold && LockingInteractable->IsA<USCVehicleSeatComponent>())
		PendingSeat = nullptr;
}

void ASCCounselorCharacter::UseCurrentSmallItem(bool bFromInput /* = true */)
{
	// We don't have a current small item, what do you think we're using?!
	ASCItem* SmallItem = GetCurrentSmallItem();
	if (!SmallItem)
		return;

	// Can't use an item while we're doing something else that's probably a lot cooler
	if (GetInteractableManagerComponent()->GetLockedInteractable())
		return;

	// Don't use items in the water!
	if (CurrentStance == ESCCharacterStance::Swimming)
		return;

	// Can't use items while in a hide spot
	if (CurrentHidingSpot)
		return;

	// Can't use items while escaping
	if (IsEscaping() || HasEscaped())
		return;

	// Using an item while getting out of a vehicle causes issues
	if (!SmallItem->IsA<ASCRepairPart>())
	{
		if (IsInVehicle() || PendingSeat)
			return;
	}

	// Pocket knife overrides most small item limitations
	if (!SmallItem->IsA<ASCPocketKnife>())
	{
		if (IsInSpecialMove() || bIsGrabbed)
			return;

		if (bUsingSmallItem)
			return;
	}

	// Make sure we can ACTUALLY use this item
	if (!SmallItem->CanUse(bFromInput))
		return;

	// Don't use an item while picking up another item
	if (PickingItem)
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_UseCurrentSmallItem(bFromInput);
	}
	else
	{
		bUsingSmallItem = true;

		bCachedFromInput = bFromInput;
		MULTICAST_UseCurrentSmallItem(SmallItem, bFromInput);
	}
}

bool ASCCounselorCharacter::SERVER_UseCurrentSmallItem_Validate(bool bFromInput)
{
	return true;
}

void ASCCounselorCharacter::SERVER_UseCurrentSmallItem_Implementation(bool bFromInput)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	UseCurrentSmallItem(bFromInput);
}

void ASCCounselorCharacter::MULTICAST_UseCurrentSmallItem_Implementation(ASCItem* Item, bool bFromInput)
{
	if (Item)
	{
		// HNNNGGGGG
		if (Item->IsA<ASCPocketKnife>() && IsInSpecialMove())
		{
			if (USCSpecialMove_SoloMontage* SM = GetActiveSCSpecialMove())
			{
				SM->CancelSpecialMove();
			}

			if (GrabbedBy)
			{
				if (USCSpecialMove_SoloMontage* KillerSM = GetActiveSCSpecialMove())
				{
					KillerSM->CancelSpecialMove();
				}
				AttachToKiller(true);
			}
		}

		if (USkeletalMeshComponent* CurrentMesh = GetMesh())
		{
			if (USCCounselorAnimInstance* AnimInstance = Cast<USCCounselorAnimInstance>(CurrentMesh->GetAnimInstance()))
			{
				if (Item->ShouldAttach(bFromInput))
				{
					Item->AttachToCharacter(false);
				}

				Item->BeginItemUse();
				AnimInstance->PlayUseItemAnim(Item);
			}
		}
	}
}

void ASCCounselorCharacter::OnLargeItemPressed()
{
	// don't do this if we don't have a controller for some reason.
	if (!IsLocallyControlled())
		return;

	if (CurrentStance == ESCCharacterStance::Swimming)
		return;

	if (InteractPressed)
		return;

	// can use large items if escaping
	if (IsEscaping() || HasEscaped())
		return;

	// Remove our movement lock once the shotgun has been fired.
	if (ASCGun* Gun = Cast<ASCGun>(EquippedItem))
	{
		if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
		{
			PlayerController->SetIgnoreMoveInput(false);

			if (ASCInGameHUD* HUD = PlayerController->GetSCHUD())
			{
				HUD->OnEndAiming();
			}
		}
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (Controller && Controller->IsA<AAIController>())
	{
		GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewLocation = GetCachedCameraInfo().Location;
		ViewRotation = GetCachedCameraInfo().Rotation;
	}

	SERVER_UseCurrentLargeItem(true, ViewLocation, ViewRotation);
}

void ASCCounselorCharacter::OnLargeItemReleased()
{
	SERVER_SetLargeItemPressHeld(false);
}

bool ASCCounselorCharacter::SERVER_SetLargeItemPressHeld_Validate(bool Held)
{
	return true;
}

void ASCCounselorCharacter::SERVER_SetLargeItemPressHeld_Implementation(bool Held)
{
	bLargeItemPressHeld = Held;
}

void ASCCounselorCharacter::OnSmallItemPressed()
{
	UseCurrentSmallItem();
}

void ASCCounselorCharacter::OnUseSmallItemOne()
{
	SERVER_OnUseSmallItem(0);
}

void ASCCounselorCharacter::OnUseSmallItemTwo()
{
	SERVER_OnUseSmallItem(1);
}

void ASCCounselorCharacter::OnUseSmallItemThree()
{
	SERVER_OnUseSmallItem(2);
}

bool ASCCounselorCharacter::SERVER_OnUseSmallItem_Validate(uint8 ItemIndex)
{
	return true;
}

void ASCCounselorCharacter::SERVER_OnUseSmallItem_Implementation(uint8 ItemIndex)
{
	if (bUsingSmallItem)
		return;

	// No using items if we're already using the sweater
	if (IsUsingSweaterAbility())
		return;

	if (SmallItemInventory->GetItemList().Num() > ItemIndex)
	{
		SmallItemIndex = ItemIndex;
		OnRep_SmallItemIndex();
		UseCurrentSmallItem();
	}
}

void ASCCounselorCharacter::OnUseItem()
{
	// This could potentially be called on EVERYONE and we only want it to happen when called
	// directly from the server
	if (Role == ROLE_Authority)
	{
		if (ASCItem* Item = GetCurrentSmallItem())
		{
			bShouldDestroyItem = Item->Use(bCachedFromInput);
			if (bShouldDestroyItem)
			{
				// Remove from inventory
				ItemToDestroy = Item;
			}
		}

		bCachedFromInput = false;
	}
}

void ASCCounselorCharacter::OnForceDestroyItem(ASCItem* Item)
{
	if (Role == ROLE_Authority && Item != nullptr)
	{
		bShouldDestroyItem = true;
		ItemToDestroy = Item;
		OnDestroyItem();
	}
}

void ASCCounselorCharacter::OnDestroyItem()
{
	// This could potentially be called on EVERYONE and we only want it to happen when called
	// directly from the server
	if (Role == ROLE_Authority)
	{
		bUsingSmallItem = false;
		if (ItemToDestroy)
		{
			ItemToDestroy->EndItemUse();
		}

		if (bShouldDestroyItem && ItemToDestroy)
		{
			SmallItemInventory->RemoveItem(ItemToDestroy);
			NextSmallItem();
			ItemToDestroy->Destroy();
			ItemToDestroy = nullptr;
		}
		else
		{
			if (ASCItem* Item = GetCurrentSmallItem())
			{
				// Hide the item
				Item->AttachToCharacter(true);
				Item->EndItemUse();
			}
		}

		bShouldDestroyItem = false;
	}
}

void ASCCounselorCharacter::OnDropItemPressed()
{
	if (!CanDropItem())
		return;

	bDropItemPressed = true;
}

void ASCCounselorCharacter::OnDropItemReleased()
{
	if (CurrentWaterVolume || !bDropItemPressed || ActiveSpecialMove || GetInteractableManagerComponent()->GetLockedInteractable())
	{
		// these should always be reset.
		bDropItemPressed = false;
		DropItemTimer = 0.f;
		return;
	}

	if ((!GetEquippedItem() || DropItemTimer < DropItemHoldTime) && !bDroppingSmallItem)
	{
		if (SmallItemInventory->GetItemList().Num() > 0)
		{
			SERVER_SetDroppingSmallItem(true);
		}
	}

	// these should always be reset.
	bDropItemPressed = false;
	DropItemTimer = 0.f;
}

bool ASCCounselorCharacter::SERVER_SetDroppingSmallItem_Validate(const bool bDropping)
{
	return true;
}

void ASCCounselorCharacter::SERVER_SetDroppingSmallItem_Implementation(const bool bDropping)
{
	// We are currently locked into interacting with something else DON'T DO IT!
	if (InteractableManagerComponent->GetLockedInteractable() || PickingItem != nullptr)
		return;

	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	bDroppingSmallItem = bDropping;
	OnRep_DroppingSmallItem(); // Listen server
}

void ASCCounselorCharacter::OnRep_DroppingSmallItem()
{
	if (bDroppingSmallItem)
	{
		if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
		{
			if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(SkeletalMesh->GetAnimInstance()))
			{
				// DropCurrentSmallItem() will be called from an AnimNotify
				const float MontageTime = AnimInst->Montage_Play(AnimInst->DropSmallItemMontage);

				if (Role == ROLE_Authority)
					GetWorldTimerManager().SetTimer(TimerHandle_DropSmallItem, this, &ASCCounselorCharacter::DropCurrentSmallItem, MontageTime, false);
			}
		}
	}
}

void ASCCounselorCharacter::DropCurrentSmallItem()
{
	check(Role == ROLE_Authority);

	GetWorldTimerManager().ClearTimer(TimerHandle_DropSmallItem);

	if (ASCItem* Item = GetCurrentSmallItem())
	{
		SmallItemInventory->RemoveItem(Item);
		SmallItemIndex = SmallItemInventory->GetItemList().Num() - 1;
		Item->OnDropped(this);
	}

	bDroppingSmallItem = false;
}

void ASCCounselorCharacter::UpdateDropItemTimer(float DeltaSeconds)
{
	if (CurrentWaterVolume || ActiveSpecialMove || GetInteractableManagerComponent()->GetLockedInteractable())
		return;

	if (bDropItemPressed && !bDroppingLargeItem)
	{
		DropItemTimer += DeltaSeconds;
		if (DropItemTimer >= DropItemHoldTime)
		{
			if (GetEquippedItem())
			{
				SERVER_SetDroppingLargeItem(true);
				bDropItemPressed = false;
				DropItemTimer = 0.f;
			}
		}
	}
}

bool ASCCounselorCharacter::SERVER_SetDroppingLargeItem_Validate(const bool bDropping)
{
	return true;
}

void ASCCounselorCharacter::SERVER_SetDroppingLargeItem_Implementation(const bool bDropping)
{
	// We are currently locked into interacting with something else DON'T DO IT!
	if (InteractableManagerComponent->GetLockedInteractable() || PickingItem != nullptr)
		return;

	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	bDroppingLargeItem = bDropping;
	OnRep_DroppingLargeItem(); // Listen server
}

void ASCCounselorCharacter::OnRep_DroppingLargeItem()
{
	if (bDroppingLargeItem)
	{
		// Make sure we stop aiming before dropping the item.
		OnAimReleased();

		if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
		{
			if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(SkeletalMesh->GetAnimInstance()))
			{
				// DropEquippedItem() called from an AnimNotify
				AnimInst->Montage_Play(AnimInst->DropLargeItemMontage);
			}
		}
	}
}

void ASCCounselorCharacter::DropEquippedItem(const FVector& DropLocationOverride /*= FVector::ZeroVector*/, bool bOverrideDroppingCharacter /*= false*/)
{
	// Super checks for ROLE_Authority, make sure this is only called from the server
	Super::DropEquippedItem(DropLocationOverride, bOverrideDroppingCharacter);

	bDroppingLargeItem = false;
}

bool ASCCounselorCharacter::SERVER_UseCurrentLargeItem_Validate(bool bFromInput /* = true*/, FVector CameraLocation/* = FVector::ZeroVector*/, FRotator CameraForward/* = FRotator::ZeroRotator*/)
{
	return true;
}

void ASCCounselorCharacter::SERVER_UseCurrentLargeItem_Implementation(bool bFromInput /* = true*/, FVector CameraLocation/* = FVector::ZeroVector*/, FRotator CameraForward/* = FRotator::ZeroRotator*/)
{
	// If we're interacting with something or doing something truly "Special" don't use our item (unless we've called it from code).
	if ((InteractableManagerComponent->GetLockedInteractable() || IsInSpecialMove()) && bFromInput)
		return;

	if (EquippedItem && !bLargeItemPressHeld)
	{
		if (bFromInput)
			bLargeItemPressHeld = true;

		// set aiming info for fire
		if (ASCGun* Gun = Cast<ASCGun>(EquippedItem))
			Gun->SetAimingInfo(CameraLocation, CameraForward);

		if (EquippedItem->Use(bFromInput))
		{
			DestroyEquippedItem();
		}
	}
}

void ASCCounselorCharacter::DestroyEquippedItem()
{
	if (EquippedItem)
	{
		EquippedItem->Destroy();
		DropEquippedItem();
	}
}

void ASCCounselorCharacter::OnRearCamPressed()
{
	if (bIsAiming)
		return;

	OnUseRearCam(true);
}

void ASCCounselorCharacter::OnRearCamReleased()
{
	if (bIsAiming)
		return;

	OnUseRearCam(false);
}

ASCPlayerState* ASCCounselorCharacter::GetPlayerState() const
{
	if (CurrentSeat && CurrentSeat->IsDriverSeat())
	{
		if (CurrentSeat->ParentVehicle && CurrentSeat->ParentVehicle->PlayerState)
		{
			return Cast<ASCPlayerState>(CurrentSeat->ParentVehicle->PlayerState);
		}
	}

	return Super::GetPlayerState();
}

bool ASCCounselorCharacter::CanTakeDamage() const
{
	if (Role == ROLE_Authority)
	{
		if (PendingSeat)
			return false;

		if (IsInVehicle())
			return false;
	}

	return Super::CanTakeDamage();
}

float ASCCounselorCharacter::GetSpeedModifier() const
{
	float Modifier = Super::GetSpeedModifier();

	if (IsDodging())
		Modifier = DodgingSpeedMod;

	if (CurrentStance == ESCCharacterStance::Standing && bIsStumbling)
		Modifier = StumblingSpeedMod;

	if (CurrentStance == ESCCharacterStance::Swimming)
		Modifier *= WaterSpeedMod;

	// Perk modifiers
	if (ASCPlayerState* PS = GetPlayerState())
	{
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			if (CurrentStance == ESCCharacterStance::Crouching)
				Modifier += Perk->CrouchMoveSpeedModifier;
			else if (CurrentWaterVolume && CurrentStance == ESCCharacterStance::Swimming)
				Modifier += Perk->SwimSpeedModifier;
			else if (IsSprinting())
				Modifier += Perk->SprintSpeedModifier;
		}
	}

	return Modifier * SpeedStatMod.Get(this);
}

bool ASCCounselorCharacter::HasItemInInventory(TSubclassOf<ASCItem> ItemClass) const
{
	// Check our small items
	for (const auto& Item : SmallItemInventory->GetItemList())
	{
		if (IsValid(Item) && Item->IsA(ItemClass))
			return true;
	}

	// Check our special items
	for (const auto& Item : SpecialItemInventory->GetItemList())
	{
		if (IsValid(Item) && Item->IsA(ItemClass))
			return true;
	}

	// Super handles the large item
	return Super::HasItemInInventory(ItemClass);
}

int32 ASCCounselorCharacter::NumberOfItemTypeInInventory(TSubclassOf<ASCItem> ItemClass) const
{
	int32 Count = 0;

	// Check our small items
	for (const auto& Item : SmallItemInventory->GetItemList())
	{
		if (IsValid(Item) && Item->IsA(ItemClass))
			++Count;
	}

	// Check our special items
	for (const auto& Item : SpecialItemInventory->GetItemList())
	{
		if (IsValid(Item) && Item->IsA(ItemClass))
			++Count;
	}

	// Super handles the large item
	Count += Super::NumberOfItemTypeInInventory(ItemClass);

	return Count;
}

void ASCCounselorCharacter::SetSprinting(bool bSprinting)
{
	// Sprinting should only be used to exit crouching, nothing else
	if (bSprinting && CurrentStance == ESCCharacterStance::Crouching)
	{
		SetStance(ESCCharacterStance::Standing);
	}

	Super::SetSprinting(bSprinting);
}

void ASCCounselorCharacter::SetSprintingWithButtonState(bool Sprinting, bool ButtonState)
{
	// if we are wounded or out of stamina, slow run instead of sprint
	if ((IsWounded() || bStaminaDepleted) && !IsAbilityActive(EAbilityType::Sprint) && !bIsSprinting)
	{
		SetRunning(Sprinting);
		return;
	}

	if (Role < ROLE_Authority)
	{
		SERVER_SetSprintingWithButtonState(Sprinting, ButtonState);
	}

	if (Sprinting && InCombatStance())
		SetCombatStance(false);

	SetSprinting(Sprinting);
	bSprintPressed = ButtonState;
}


bool ASCCounselorCharacter::SERVER_SetSprintingWithButtonState_Validate(bool Sprinting, bool ButtonState) 
{ 
	return true; 
}

void ASCCounselorCharacter::SERVER_SetSprintingWithButtonState_Implementation(bool Sprinting, bool ButtonState)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	SetSprintingWithButtonState(Sprinting, ButtonState);
}

void ASCCounselorCharacter::OnToggleSprint()
{
	SetSprintingWithButtonState(!IsSprinting(), true);
}

void ASCCounselorCharacter::OnToggleSprintStop()
{
	SetSprintingWithButtonState(IsSprinting(), false);
}

void ASCCounselorCharacter::OnStartSprint()
{
	SetSprintingWithButtonState(true, true);
}

void ASCCounselorCharacter::OnStopSprint()
{
	SetSprintingWithButtonState(false, false);
}

bool ASCCounselorCharacter::IsSprintPressed() const
{
	return bSprintPressed;
}

void ASCCounselorCharacter::OnRunToggle()
{
	if (IsCrouching())
		OnCrouchToggle();

	SetRunning(!IsRunning());
}

void ASCCounselorCharacter::OnCrouchToggle()
{
	ASCPlayerController* MyPC = Cast<ASCPlayerController>(Controller);
	if (!MyPC || MyPC->IsMoveInputIgnored())
		return;

	// Don't change stance while interacting with something, this could cause you to not be able
	//  to use it anymore and get locked to that interaction, which would be bad
	if (bAttemptingInteract || GetInteractableManagerComponent()->GetLockedInteractable())
		return;

	switch (CurrentStance)
	{
	case ESCCharacterStance::Combat:
		SetCombatStance(false);
		// Pass through
	case ESCCharacterStance::Standing:
		SetSprinting(false);
		SetRunning(false);
		SetStance(ESCCharacterStance::Crouching);
		break;

	case ESCCharacterStance::Crouching:
		SetStance(ESCCharacterStance::Standing);
		break;
	}

	// Cancelling the emote, otherwise the camera moves to a crouch positiion during the emote.
	if (CurrentEmote)
	{
		SERVER_CancelEmote();
	}
}

void ASCCounselorCharacter::OnRep_SmallItemIndex()
{
	UpdateInventoryWidget();
}

void ASCCounselorCharacter::BeginGrabMinigame()
{
	if (Role == ROLE_Authority)
	{
		// We've already been dropped or something else went wrong, either way, we're not grabbed yo.
		if (!GrabbedBy)
			return;

		// Counselors in wheelchairs can't break out of a grab, so don't start the mini game
		if (bInWheelchair)
			return;

		TakeDamage(0.f, FDamageEvent(GrabStunDamageType), GrabbedBy->Controller, GrabbedBy);

		// If the counselor has a pocket knife, break the grab immediately
		for (int32 i(0); i < SmallItemInventory->GetItemList().Num(); ++i)
		{
			if (SmallItemInventory->GetItemList()[i]->IsA(ASCPocketKnife::StaticClass()))
			{
				SmallItemIndex = i;
				SERVER_UseCurrentSmallItem(false);
				BP_BreakGrab(true);
				break;
			}
		}
	}
}

void ASCCounselorCharacter::OnGrabbed(ASCKillerCharacter* Killer)
{
	bIsGrabbed = true;

	if (TriggeredTrap)
	{
		EscapeTrap(false);
	}

	GrabbedBy = Killer;

	if (bIsAiming && IsLocallyControlled()) 
		OnAimReleased();

	// Detach the wheelchair from our counselor
	if (bInWheelchair)
		DetachWheelchair();


	PreviousStance = ESCCharacterStance::Grabbed; // HACK: Prevents leaving combat stance from taking us out of the grabbed stance
	SetStance(ESCCharacterStance::Grabbed);

	if (ROLE_Authority == Role)
	{
		if (ASCItem* CurrentEquippedItem = GetEquippedItem())
		{
			// if we're grabbed in water spawn our equipped item on shore.
			if (CurrentWaterVolume)
			{
				if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
				{
					FTransform ItemTransform(FTransform::Identity);
					if (GameMode->GetShoreLocation(GetActorLocation(), CurrentEquippedItem, ItemTransform))
					{
						DropEquippedItem(ItemTransform.GetLocation(), true);
					}
				}
			}
			else
				DropEquippedItem();
		}
	}

	FearManager->PushFearEvent(GrabbedFearEvent);

	// Are we trying to perform a special move?
	if (!bInContextKill)
	{
		ClearAnyInteractions();

		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInstance->PlayGrabAnim();
		}
	}

	if (IsLocallyControlled())
	{
		OnInteract1Released(); // Force Interaction to end for repairs, etc.
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			// remove cinematic mode for safety (if we're interacting with something that takes over the camera)
			PC->SetCinematicMode(false, false, false, true, true);

			if (ASCInGameHUD* HUD = PC->GetSCHUD())
			{
				HUD->CloseCounselorMap();
				HUD->ChangeRootMenu(nullptr, true);
			}
		}
		else if (ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(Controller))
		{
			BotController->ClearInteractables();
		}

		OnHideEmoteSelect();
	}

	BP_OnGrabbed(Killer);
	if (OnCounselorGrabbed.IsBound())
		OnCounselorGrabbed.Broadcast();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Escape, ECollisionResponse::ECR_Ignore);
}

void ASCCounselorCharacter::OnDropped(bool IsDead, bool bIgnoreTrace/* = false*/)
{
	if (!bIsGrabbed)
		return;

	// Make sure we don't get dropped into collision unless we specifically want to, like at the end of a context kill.
	if (Role == ROLE_Authority && GrabbedBy && GetWorld() && !bIgnoreTrace)
	{
		bool Searching = true;
		FVector DesiredLocation = GetActorLocation();
		float Angle = 0.0f;
		const float Distance = (GrabbedBy->GetActorLocation() - GetActorLocation()).Size2D();
		float CurDist = Distance;

		while (Searching)
		{
			const FVector Dir = GrabbedBy->GetActorForwardVector().RotateAngleAxis(Angle, FVector::UpVector);
			DesiredLocation = GrabbedBy->GetActorLocation() + Dir * CurDist;

			static const FName NAME_DropSweep(TEXT("DropSweep"));
			FCollisionQueryParams QueryParams(NAME_DropSweep);
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(GrabbedBy);
			
			bool ValidSpot = !GetWorld()->OverlapBlockingTestByChannel(DesiredLocation, GetActorRotation().Quaternion(), ECC_Visibility, GetCapsuleComponent()->GetCollisionShape(), QueryParams);

			if (ValidSpot)
			{
				// an additional check to ensure that the player is above ground, just in case
				FHitResult HitResult;
				ValidSpot = GetWorld()->LineTraceSingleByChannel(HitResult, DesiredLocation, DesiredLocation - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * 2.0f), ECC_Visibility, QueryParams);
			}

			if (!ValidSpot)
			{
				// this chunk of code will move out by 5 degrees and rotate around, so you will check angle, 0, 5, -5, 10, -10, etc
				// if angle is non zero, negate
				if (Angle)
					Angle *= -1.0f;

				// if the angle is positive, add 5
				if (Angle >= 0.0f)
					Angle += 5.0f;

				// if we wrap all the way around, reset and move out
				if (Angle > 135.0f)
				{
					Angle = 0.0f;
					CurDist += Distance;

					// if we couldnt find a spot, default back to the original location and stop searching, this is bad
					if (CurDist > 2000.0f)
					{
						DesiredLocation = GetActorLocation();
						// notify the testers on screen if this gets hit. It should never get hit
						GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("NO VALID DROP SPOT FOUND FOR COUNSELOR! DEFAULTING TO ORIGINAL GRAB LOCATION!!!! NOTIFY AN ENGINEER!!!!!"));
						break;
					}
				}
				continue;
			}

			Searching = false;
		}

		SetActorLocation(DesiredLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		// One last update before dropping, this should keep the player from sliding and flipping when finishing a context kill.
		UpdateGrabTransform();
	}

	// make sure to only set this on the server since its serialized
	if (Role == ROLE_Authority)
		GrabbedBy = nullptr;

	bIsGrabbed = false;
	// detach from the killers hand
	AttachToKiller(false);

	FearManager->PopFearEvent(GrabbedFearEvent);

	if (!IsDead)
	{
		if (bWiggleGameActive)
		{
			EndStun();
		}

		// Turn off jelly legs when dropped
		//SetRagdollPhysics(TEXT("thigh_r"), false, 0.f, false);
		//SetRagdollPhysics(TEXT("thigh_l"), false, 0.f, false);

		if (USCAnimInstance* AI = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AI->StopGrabAnim();
		}

		ESCCharacterStance NewStance = ESCCharacterStance::Standing;
		if (CurrentWaterVolume)
		{
			if ((GetActorLocation().Z - MinWaterDepthForSwimming) <= GetWaterZ())
				NewStance = ESCCharacterStance::Swimming;
		}

		SetStance(NewStance);
		PreviousStance = ESCCharacterStance::Standing;

		UpdateSwimmingState();

		SetVehicleCollision(true);

		ReturnCameraToCharacter(0.75f, VTBlend_EaseInOut, 2.f);
	}

	BP_OnDropped();
	if (OnCounselorDropped.IsBound())
		OnCounselorDropped.Broadcast();

	CLIENT_RemoveAllInputComponents();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Escape, ECollisionResponse::ECR_Overlap);
}

void ASCCounselorCharacter::BreakGrab(bool bStunWithPocketKnife)
{
	Super::BreakGrab(bStunWithPocketKnife);

	if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		GameMode->HandleScoreEvent(PlayerState, EscapeJasonGrabScoreEvent);
	}

	float BaseStaminaBoost = BreakFreeStaminaBoost.Get(this);
	// Apply perk modifiers for stamina
	if (ASCPlayerState* PS = Cast<ASCPlayerState>(PlayerState))
	{
		float BreakFreeStaminaMod = 0.0f;
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			BreakFreeStaminaMod += Perk->BreakGrabStaminaModifier;
		}

		ReturnStamina(BaseStaminaBoost + BaseStaminaBoost * BreakFreeStaminaMod);
	}

	// Award badge for escaping grab without a pocket knife
	if (!bStunWithPocketKnife && Role == ROLE_Authority)
	{
		if (ASCPlayerState* PS = GetPlayerState())
			PS->EarnedBadge(EscapeGrabNoKnifeBadge);
	}

	// Make sure we are no longer attached to any other actor (e.g. vehicles)
	if (!bStunWithPocketKnife && GetAttachParentActor())
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ASCCounselorCharacter::AttachToKiller(bool bAttachToHand)
{
	// bInKillerHand is replicated so make sure this is only ever set on the server
	if (Role != ROLE_Authority)
		return;

	if (bAttachToHand == bInKillerHand)// dont call to the rep function if this is already set
		return;

	bInKillerHand = bAttachToHand;
	OnRep_InKillerHand();
}

void ASCCounselorCharacter::OnRep_InKillerHand()
{
	if (bInKillerHand)
	{
		// shut off character buoyancy so that it doesnt accumulate if grabbed
		SavedBuoyancy = GetCharacterMovement()->Buoyancy;
		GetCharacterMovement()->Buoyancy = 0.0f;

		// deactivate replicated movement, if you dont then the rotation breaks
		bReplicateMovement = false;
		// also shut off character movement to prevent corrections
		GetCharacterMovement()->Deactivate();
		// shut off gravity
		SetGravityEnabled(false);

		// shut off replication driven animation ticking for the server only, if you shut it off on clients they dont animate for the kill
		if (Role == ROLE_Authority && !IsLocallyControlled())
		{
			AAIController* AIC = Cast<AAIController>(Controller);// only do this if they arent ai controlled
			if (AIC == nullptr)
				GetMesh()->bOnlyAllowAutonomousTickPose = false;
		}

		// disable visibility collision so that the killers raycasts for interaction points still work
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Ignore);
	}
	else
	{
		// wait a few frames before setting the buoyancy back to prevent weird accumulation
		FTimerDelegate ResetBuoyancyDelegate;
		ResetBuoyancyDelegate.BindLambda([this]()
		{
			if (!IsValid(this))
				return;

			// set buoyancy back on
			GetCharacterMovement()->bJustTeleported = true;
			GetCharacterMovement()->Buoyancy = SavedBuoyancy;
		});
		GetWorldTimerManager().SetTimer(TimerHandle_ResetBuoyancy, ResetBuoyancyDelegate, 0.25f, false);


		// re-enable replicated movement for this actor
		bReplicateMovement = true;
		// re-enable character movement
		GetCharacterMovement()->Activate();
		// re-enable gravity
		SetGravityEnabled(true);

		if (Role == ROLE_Authority && !IsLocallyControlled())
		{
			AAIController* AIC = Cast<AAIController>(Controller);// only do this if its not ai controlled
			if (AIC == nullptr)
			{
				if (IsAlive())// only re-enable this is we are still alive. It is disabled when you unpossess.
					GetMesh()->bOnlyAllowAutonomousTickPose = true;
			}
		}

		// re-enable visibility collision
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Block);
	}
}

void ASCCounselorCharacter::EndStun()
{
	Super::EndStun();

	if (Role == ROLE_Authority)
	{
		bool bAboutToDie = false;
		if (USCSpecialMove_SoloMontage* SM = GetActiveSCSpecialMove())
		{
			bAboutToDie = SM->DieOnFinish();
		}

		// Make sure we're grabbed and not being killed
		if (bIsGrabbed && !bAboutToDie)
		{
			const bool bHasPocketKnife = SmallItemInventory->FindItem(ASCPocketKnife::StaticClass()) != nullptr;
			if (GrabbedBy)
			{
				GrabbedBy->BreakGrab(bHasPocketKnife);

				if (PocketKnifeDetachDelay > 0.f)
				{
					FTimerDelegate DropCounselorDelegate;
					DropCounselorDelegate.BindLambda([this]()
					{
						if (!IsValid(this) || !IsValid(GrabbedBy))
							return;

						GrabbedBy->DropCounselor();
					});
					GetWorldTimerManager().SetTimer(TimerHandle_BreakOut, DropCounselorDelegate, PocketKnifeDetachDelay, false);
				}
				else
				{
					GrabbedBy->DropCounselor();
				}

				AttachToKiller(false);
				ReturnCameraToCharacter();
			}

			BreakGrab(bHasPocketKnife);
		}
	}
}

void ASCCounselorCharacter::MULTICAST_EndStun_Implementation()
{
	Super::MULTICAST_EndStun_Implementation();
}

void ASCCounselorCharacter::MULTICAST_EnterVehicle_Implementation(ASCDriveableVehicle* Vehicle, USCVehicleSeatComponent* Seat, USCSpecialMoveComponent* SpecialMove)
{
	check(Vehicle);
	check(Seat);
	check(SpecialMove);

	// Block vehicle movement when counselors are entering the vehicle
	if (Vehicle)
		Vehicle->CounselorsInteractingWithVehicle.AddUnique(this);

	// Bind completion delegate
	if (SpecialMove)
	{
		SpecialMove->DestinationReached.AddUniqueDynamic(this, &ASCCounselorCharacter::OnEnterVehicleDestinationReached);
		SpecialMove->SpecialComplete.AddUniqueDynamic(this, &ASCCounselorCharacter::OnEnterVehicleComplete);
		SpecialMove->SpecialAborted.AddUniqueDynamic(this, &ASCCounselorCharacter::OnEnterVehicleAborted);
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	SetVehicleCollision(false);
	
	// Server only
	if (Role == ROLE_Authority)
	{
		// Hide and disable collision on equipped item
		if (EquippedItem)
		{
			EquippedItem->SetActorEnableCollision(false);
			EquippedItem->SetActorHiddenInGame(true);
		}

		// Track where we started the special move
		EnterVehicleStartLocation = GetActorLocation();

		// Start the special move!
		if (SpecialMove)
			SpecialMove->ActivateSpecial(this);
	}
	// Track the seat we're getting into

	PendingSeat = Seat;
	if (PendingSeat)
		PendingSeat->CounselorInSeat = this;
}

void ASCCounselorCharacter::OnEnterVehicleDestinationReached(ASCCharacter* Interactor)
{
	// Only play the vehicle animation if the character reached their destination
	if (PendingSeat)
	{
		if (PendingSeat->ParentVehicle)
		{
			PendingSeat->ParentVehicle->PlayAnimMontage(PendingSeat->EnterVehicleMontage);
		}

		// Interactable is locked automagically, we'll re-lock the seat locally in SetCurrentSeat
		if (IsLocallyControlled())
			InteractableManagerComponent->UnlockInteraction(PendingSeat);
	}
}

void ASCCounselorCharacter::OnEnterVehicleComplete(ASCCharacter* Interactor)
{
	if (PendingSeat)
	{
		PendingSeat->CounselorInSeat = this;
		SetCurrentSeat(PendingSeat);
		PendingSeat = nullptr;
		CurrentSeat->ParentVehicle->CounselorsInteractingWithVehicle.Remove(this);

		// Paranoid... Stupid notifies
		if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInst->bIsInCar = true;
		}
	}
	else
	{
		OnEnterVehicleAborted(Interactor);
	}
}

void ASCCounselorCharacter::OnEnterVehicleAborted(ASCCharacter* Interactor)
{
	// Re-enable vehicle collision
	SetVehicleCollision(true);

	// Most of this shit only needs to happen on the server
	if (Role == ROLE_Authority)
	{
		// Unhide any large item
		if (EquippedItem)
		{
			EquippedItem->SetActorEnableCollision(true);
			EquippedItem->SetActorHiddenInGame(false);
		}
	}

	if (IsLocallyControlled())
		InteractableManagerComponent->UnlockInteraction(PendingSeat);

	// Reset our movement mode
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->NavAgentProps.bCanSwim = true;
	UpdateSwimmingState();

	// Tell the vehicle we're not interacting with it anymore
	if (PendingSeat)
	{
		if (PendingSeat->ParentVehicle)
		{
			PendingSeat->ParentVehicle->CounselorsInteractingWithVehicle.Remove(this);
		}

		PendingSeat->CounselorInSeat = nullptr;
	}

	// Reset the pending seat pointer
	PendingSeat = nullptr;

	// Find a good spot to place the counselor
	static const FName NAME_VehicleSweep(TEXT("VehicleSweep"));
	FCollisionQueryParams QueryParams(NAME_VehicleSweep);
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	if (UWorld* World = GetWorld())
		World->SweepSingleByChannel(Hit, EnterVehicleStartLocation, GetActorLocation(), GetActorRotation().Quaternion(), ECollisionChannel::ECC_Pawn, GetCapsuleComponent()->GetCollisionShape(), QueryParams);

	if (Hit.bBlockingHit)
	{
		SetActorLocation(Hit.Location.IsNearlyZero() ? EnterVehicleStartLocation : Hit.Location);
	}
}

void ASCCounselorCharacter::MULTICAST_ExitVehicle_Implementation(bool bExitBlocked)
{
	if (!CurrentSeat)
		return;

	if (bLeavingSeat)
		return;

	PendingSeat = CurrentSeat;

	CurrentSeat->ParentVehicle->CounselorsInteractingWithVehicle.AddUnique(this);

	// FLY LIKE AN EAGLE!
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	if (bIsDriver && CurrentSeat->ParentVehicle->bIsStarted)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetCharacterController()))
		{
			PC->SetViewTarget(this);
		}
	}

	bLeavingSeat = true;
	SetCurrentSeat(nullptr);

	if (bExitBlocked)
	{
		// Loop through Capsule Components around the vehicle to find a good place to move the counselor to
		FVector NewLocation = PendingSeat->ParentVehicle->GetEmptyCapsuleLocation(this);
		if (NewLocation.IsZero())
		{
			UE_LOG(LogSC, Error, TEXT("ASCCounselorCharacter::MULTICAST_ExitVehicle FAILED : Door is blocked and no space around car to place actor. Counselor placed in a default position (may be bad)"));
			NewLocation = PendingSeat->GetComponentLocation() + PendingSeat->GetComponentRotation().Vector() * 200.f;
		}

		// Kill the exit animation
		GetMesh()->GetAnimInstance()->Montage_Stop(0.f);
		OnExitVehicleComplete();
		
		SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		const float MontageLength = PendingSeat->CounselorInteractAnim->GetPlayLength();
		GetWorldTimerManager().SetTimer(PendingSeat->TimerHandle_ExitSeat, this, &ASCCounselorCharacter::OnExitVehicleComplete, MontageLength, false);
		PendingSeat->ParentVehicle->PlayAnimMontage(PendingSeat->ExitVehicleMontage);

		PendingSeat->EnableExitSpacers(this, true);
	}
}

void ASCCounselorCharacter::OnExitVehicleComplete()
{
	if (PendingSeat)
	{
		PendingSeat->EnableExitSpacers(this, false);

		ASCDriveableVehicle* PendingVehicle = PendingSeat->ParentVehicle;
		if (PendingSeat->IsDriverSeat())
		{
			// Unlock interaction on client only
			if (!PendingVehicle->bIsStarted)
			{
				if (IsLocallyControlled())
				{
					InteractableManagerComponent->UnlockInteraction(PendingVehicle->StartVehicleComponent);
				}
			}
		}

		PendingVehicle->CounselorsInteractingWithVehicle.Remove(this);

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		SetVehicleCollision(true);
		if (Role == ROLE_Authority)
		{
			if (EquippedItem)
			{
				EquippedItem->SetActorEnableCollision(true);
				EquippedItem->SetActorHiddenInGame(false);
			}

			// Repossess the driver late in this case
			if (PendingSeat->IsDriverSeat())
			{
				if (AController* PC = GetCharacterController())
				{
					if (GetOwner() != nullptr)// only repossess if still possessing the car
					{
						PC->Possess(this);
						SetOwner(nullptr);
					}
				}
			}
		}

		bLeavingSeat = false;
		PendingSeat->CounselorInSeat = nullptr;
		PendingSeat = nullptr;
	}

	if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		// Paranoid
		AnimInst->ForceOutOfVehicle();
	}
}

void ASCCounselorCharacter::GrabbedOutOfVehicle(USCContextKillComponent* KillComp)
{
	if (!CurrentSeat)
		return;

	SetVehicleCollision(false);

	PendingSeat = CurrentSeat;

	if (GetWorldTimerManager().GetTimerRemaining(PendingSeat->TimerHandle_ExitSeat) > 0.f)
		GetWorldTimerManager().ClearTimer(PendingSeat->TimerHandle_ExitSeat);

	if (Role == ROLE_Authority)
	{
		if (bIsDriver && CurrentSeat->ParentVehicle->bIsStarted)
		{
			if (AController* PC = GetCharacterController())
			{
				if (GetOwner() != nullptr)
				{
					PC->Possess(this);
					SetOwner(nullptr);
				}
			}
		}
	}

	if (PendingSeat->IsDriverSeat())
	{
		if (IsLocallyControlled())
		{
			if (!PendingSeat->ParentVehicle->bIsStarted)
			{
				InteractableManagerComponent->UnlockInteraction(PendingSeat->ParentVehicle->StartVehicleComponent);
			}
		}
	}

	PendingSeat->ParentVehicle->CounselorsInteractingWithVehicle.AddUnique(this);

	bLeavingSeat = true;
	SetCurrentSeat(nullptr);

	KillComp->KillComplete.AddUniqueDynamic(this, &ASCCounselorCharacter::OnGrabbedOutOfVehicleComplete);
	KillComp->KillAbort.AddUniqueDynamic(this, &ASCCounselorCharacter::OnGrabbedOutOfVehicleComplete);
}

void ASCCounselorCharacter::OnGrabbedOutOfVehicleComplete(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	// parnoid check to ensure the animation is properly ended
	if (PendingSeat)
	{
		PendingSeat->ParentVehicle->StopAnimMontage(PendingSeat->ExitVehicleMontage);
	}
	OnExitVehicleComplete();
}

void ASCCounselorCharacter::MULTICAST_EjectFromVehicle_Implementation(bool bFromCrash)
{
	if (!CurrentSeat)
		return;

	ClearAnyInteractions();

	PendingSeat = CurrentSeat;

	if (Role == ROLE_Authority)
	{
		FVector EjectLocation = bFromCrash ? PendingSeat->ParentVehicle->GetEmptyCapsuleLocation(this) : FVector::ZeroVector;

		// Backup case
		if (EjectLocation.IsZero())
			EjectLocation = GetActorLocation() + CurrentSeat->GetForwardVector() * 200.f;
		
		if (bInWater)
			EjectLocation.Z = GetWaterZ();

		SetActorLocation(EjectLocation);
		SetActorRotation(CurrentSeat->GetForwardVector().ToOrientationRotator().GetInverse());

		SetVehicleCollision(true);
	}

	if (IsLocallyControlled())
	{
		if (!CurrentSeat->ParentVehicle->bIsStarted)
			InteractableManagerComponent->UnlockInteraction(CurrentSeat->ParentVehicle->StartVehicleComponent);
	}

	if (USkeletalMeshComponent* CounselorMesh = GetMesh())
	{
		if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(CounselorMesh->GetAnimInstance()))
		{
			AnimInst->ForceOutOfVehicle();
		}
	}

	SetCurrentSeat(nullptr);
	ReturnCameraToCharacter();
	bLeavingSeat = false;
	PendingSeat->CounselorInSeat = nullptr;
	PendingSeat = nullptr;

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ASCCounselorCharacter::SetCurrentSeat(USCVehicleSeatComponent* Seat)
{
	if (Seat)
	{
		CurrentSeat = Seat;
		bIsDriver = CurrentSeat->IsDriverSeat();
		GetCharacterMovement()->SetMovementMode(MOVE_None);

		// Only set the stance on the server
		if (Role == ROLE_Authority)
			SetStance(ESCCharacterStance::Driving);

		// Kill Swimming
		GetCharacterMovement()->NavAgentProps.bCanSwim = false;
		UpdateSwimmingState();

		// Do some stuff if you're the driver
		if (bIsDriver)
		{
			CurrentSeat->ParentVehicle->Driver = this;
			CLIENT_SetSkeletonToCinematicMode();
			if (!CurrentSeat->ParentVehicle->bIsStarted)
			{
				if (Role == ROLE_Authority)
				{
					CurrentSeat->ParentVehicle->EnableVehicleStarting();
				}
				if (IsLocallyControlled())
				{
					InteractableManagerComponent->LockInteraction(CurrentSeat->ParentVehicle->StartVehicleComponent, EILLInteractMethod::Hold);
				}
			}
			else
			{
				if (Role == ROLE_Authority)
				{
					// Possess the vehicle if it's started
					if (AController* PC = GetCharacterController())
					{
						if (GetOwner() != CurrentSeat->ParentVehicle)
						{
							PC->Possess(CurrentSeat->ParentVehicle);
							SetOwner(CurrentSeat->ParentVehicle);
						}
					}
				}
			}
		}

		// Let's do some attachment!
		AttachToComponent(CurrentSeat->ParentVehicle->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentSeat->AttachSocket);

		// Client side schtuff
		if (IsLocallyControlled())
		{
			// Stop camera from shaking
			StopCameraShakeInstances(UCameraShake::StaticClass());

			if (APlayerController* PC = Cast<APlayerController>(GetCharacterController()))
			{
				// Push appropriate input components
				if (!bIsDriver || !CurrentSeat->ParentVehicle->bIsStarted)
					PC->PushInputComponent(PassengerInputComponent);

				// Face the camera in the vehicle's forward direction
				PC->SetControlRotation(CurrentSeat->ParentVehicle->GetActorForwardVector().ToOrientationRotator());
			}

			if (CurrentSeat->ParentVehicle->VehicleType == EDriveableVehicleType::Car && IsPlayerControlled())
				HideFace(true);

			if (!bIsDriver || CurrentSeat->ParentVehicle->bIsStarted)
			{
				// Don't do a full lock, or else the killer can't pull us out
				InteractableManagerComponent->LockInteractionLocally(CurrentSeat);
				CurrentSeat->SetDrawAtLocation(true, FVector2D(0.08f, 0.85f));
			}
		}
	}
	else
	{
		if (Role == ROLE_Authority)
		{
			if (bIsDriver)
			{
				CurrentSeat->ParentVehicle->ForceNetUpdate();
				CurrentSeat->ParentVehicle->Driver = nullptr;
				CurrentSeat->ParentVehicle->StartVehicleComponent->bIsEnabled = false;
			}
		}

		if (IsLocallyControlled())
		{
			if (AController* CounselorController = GetCharacterController())
			{
				if (bIsDriver)
				{
					CLIENT_SetSkeletonToGameplayMode();
				}

				APlayerController* PC = Cast<APlayerController>(CounselorController);
				if (PC && (!bIsDriver || !CurrentSeat->ParentVehicle->bIsStarted))
				{
					PC->PopInputComponent(PassengerInputComponent);
				}

				LocalVehiclePitch = 0.f;
				LocalVehicleYaw = 0.f;
				CurrentSeat->ParentVehicle->ResetControlRotation();

				if (CurrentSeat->ParentVehicle->VehicleType == EDriveableVehicleType::Car)
					HideFace(false);

				if (PC)
				{
					if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
					{
						FRotator DesiredNewControlRot = FRotator::ZeroRotator;
						DesiredNewControlRot.Yaw = CameraManager->GetCameraRotation().Yaw;
						PC->SetControlRotation(DesiredNewControlRot);
					}
				}
			}

			if (CurrentSeat)
			{
				CurrentSeat->SetDrawAtLocation(false);
				// Forcing the unlock because we have the seat locked locally
				InteractableManagerComponent->UnlockInteraction(CurrentSeat, /*bForce=*/true);
			}
		}

		if (GetAttachParentActor()->IsA(ASCDriveableVehicle::StaticClass()))
		{
			// Only set the stance on the server
			if (Role == ROLE_Authority && !bIsGrabbed)
				SetStance(ESCCharacterStance::Standing);

			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}

		// Turn swimming back on
		GetCharacterMovement()->NavAgentProps.bCanSwim = true;
		UpdateSwimmingState();
		
		CurrentSeat = nullptr;
		bIsDriver = false;
	}
	
	// Occasionally, the mesh will be slightly rotated when getting in/out of the car. This is here as a safety fix to keep it from happening.
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	UpdateInventoryWidget();
}

void ASCCounselorCharacter::NextSmallItem()
{
	if (Role < ROLE_Authority)
	{
		SERVER_NextSmallItem();
	}
	else
	{
		if (!bUsingSmallItem)
		{
			if (++SmallItemIndex >= SmallItemInventory->GetItemList().Num())
			{
				SmallItemIndex = 0;
			}

			// Force call to update listen server host
			OnRep_SmallItemIndex();
		}
	}
}

bool ASCCounselorCharacter::SERVER_NextSmallItem_Validate()
{
	return true;
}

void ASCCounselorCharacter::SERVER_NextSmallItem_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	NextSmallItem();
}

void ASCCounselorCharacter::PrevSmallItem()
{
	if (Role < ROLE_Authority)
	{
		SERVER_PrevSmallItem();
	}
	else
	{
		if (!bUsingSmallItem)
		{
			if (--SmallItemIndex < 0)
			{
				SmallItemIndex = FMath::Max(0, SmallItemInventory->GetItemList().Num() - 1);
			}

			// Force call to update listen server host
			OnRep_SmallItemIndex();
		}
	}
}

bool ASCCounselorCharacter::SERVER_PrevSmallItem_Validate()
{
	return true;
}

void ASCCounselorCharacter::SERVER_PrevSmallItem_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	PrevSmallItem();
}

void ASCCounselorCharacter::UpdateInventoryWidget()
{
	// Update HUD
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			HUD->InventoryChanged();
		}
	}
}

bool ASCCounselorCharacter::CanDodge() const
{
	if (IsDodging())
		return false;

	if (IsAttacking())
		return false;

	if (IsBlocking())
		return false;
	
	if (ActiveSpecialMove)
		return false;

	return true;
}

void ASCCounselorCharacter::DodgeRight()
{
	if (!CanDodge())
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_Dodge(EDodgeDirection::Right);
	}
	else
	{
		if (TryUseStamina(DodgeStaminaCost.Get(this, true)) == true)
			MULTICAST_OnDodge(EDodgeDirection::Right);
	}
}

void ASCCounselorCharacter::DodgeLeft()
{
	if (!CanDodge())
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_Dodge(EDodgeDirection::Left);
	}
	else
	{
		if (TryUseStamina(DodgeStaminaCost.Get(this, true)) == true)
			MULTICAST_OnDodge(EDodgeDirection::Left);
	}
}

void ASCCounselorCharacter::DodgeBack()
{
	if (!CanDodge())
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_Dodge(EDodgeDirection::Back);
	}
	else
	{
		if (TryUseStamina(DodgeStaminaCost.Get(this, true)) == true)
			MULTICAST_OnDodge(EDodgeDirection::Back);
	}
}

bool ASCCounselorCharacter::SERVER_Dodge_Validate(EDodgeDirection DodgeDirection)
{
	return true;
}

void ASCCounselorCharacter::SERVER_Dodge_Implementation(EDodgeDirection DodgeDirection)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	if (TryUseStamina(DodgeStaminaCost.Get(this, true)) == true)
		MULTICAST_OnDodge(DodgeDirection);
}

void ASCCounselorCharacter::MULTICAST_OnDodge_Implementation(EDodgeDirection DodgeDirection)
{
	if (USkeletalMeshComponent* CurrentMesh = GetPawnMesh())
	{
		if (USCCounselorAnimInstance* AnimInstance = Cast<USCCounselorAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			AnimInstance->PlayDodge(DodgeDirection);
		}
	}

	if (CVarDebugCombat.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Cyan, FString::Printf(TEXT("%s dodged %s"), *GetClass()->GetName(), DodgeDirection == EDodgeDirection::Right ? TEXT("right") : (DodgeDirection == EDodgeDirection::Left ? TEXT("left") : TEXT("back"))));
	}
}

bool ASCCounselorCharacter::IsDodging() const
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCCounselorAnimInstance* AnimInstance = Cast<USCCounselorAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			return AnimInstance->IsDodging();
		}
	}

	return false;
}

void ASCCounselorCharacter::DetachWheelchair()
{
	if (bInWheelchair)
	{
		TArray<UActorComponent*> SkeletalMeshComponents = GetComponentsByClass(USkeletalMeshComponent::StaticClass());
		static const FString WheelChair(TEXT("Wheelchair"));
		for (UActorComponent* Component : SkeletalMeshComponents)
		{
			if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
			{
				if (SkeletalMeshComp->GetName() == WheelChair)
				{
					SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					static FName NAME_Ragdoll(TEXT("Ragdoll"));
					SkeletalMeshComp->SetCollisionProfileName(NAME_Ragdoll);
					SkeletalMeshComp->SetCollisionResponseToChannel(ECC_AnimCameraBlocker, ECollisionResponse::ECR_Ignore);
					SkeletalMeshComp->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
					SkeletalMeshComp->SetAllBodiesCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
					SkeletalMeshComp->SetAllBodiesSimulatePhysics(true);
					break;
				}
			}
		}
	}
}

void ASCCounselorCharacter::SetCombatStance(const bool bEnable)
{
	if (bEnable)
	{
		switch (CurrentStance)
		{
		case ESCCharacterStance::Standing:
		case ESCCharacterStance::Crouching:
			break;
		default:
			return;
		}

		if (ActiveSpecialMove || InteractableManagerComponent->GetLockedInteractable())
			return;

		if (ASCGun* Gun = Cast<ASCGun>(EquippedItem))
		{
			if (Gun->HasAmmunition())
				return;
		}

		// We're getting in or out of a car, NO COMBAT
		if (PendingSeat)
			return;

		// No combat while using the sweater!
		if (IsUsingSweaterAbility())
			return;
	}
	else if (IsUsingGamepad()) // !bEnabled implied
	{
		const bool bShouldRun = CurrentInputVector.Size() >= 0.85f;
		SetRunning(bShouldRun);
	}

	Super::SetCombatStance(bEnable);
}

void ASCCounselorCharacter::OnRep_CurrentStance()
{
	Super::OnRep_CurrentStance();

	if (CurrentStance == ESCCharacterStance::Crouching)
	{
		GetCapsuleComponent()->SetCapsuleRadius(CrouchCapsuleRadius, false);

		// Check if our new radius is encroaching on anything
		float Radius, HalfHeight;
		GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);
		const FVector CapsuleExtent(Radius, Radius, HalfHeight);

		FCollisionQueryParams CapsuleParams(FName(TEXT("CrouchTrace")), false, this);
		FCollisionResponseParams ResponseParam;
		GetCapsuleComponent()->InitSweepCollisionParams(CapsuleParams, ResponseParam);

		const bool bEncroached = GetWorld()->OverlapBlockingTestByChannel(GetCapsuleComponent()->GetComponentLocation() - FVector(0.f,0.f,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), FQuat::Identity,
				GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeCapsule(CapsuleExtent), CapsuleParams, ResponseParam);

		// If it is, sweep back a bit and push out as much as we need to
		if (bEncroached)
		{
			const FVector StartLocation = GetActorLocation() - GetActorForwardVector() * (CrouchCapsuleRadius - OriginalCapsuleRadius);
			FHitResult Hit;
			if (GetWorld()->SweepSingleByChannel(Hit, StartLocation, GetActorLocation(), FQuat::Identity, GetCapsuleComponent()->GetCollisionObjectType(), GetCapsuleComponent()->GetCollisionShape(), CapsuleParams))
			{
				SetActorLocation(Hit.Location, true);
			}
		}
	}
	else if (PreviousStance == ESCCharacterStance::Crouching)
	{
		GetCapsuleComponent()->SetCapsuleRadius(OriginalCapsuleRadius, false);
	}

	// make sure the emote wheel goes away when switching to combat stance
	if (IsLocallyControlled())
	{
		if (CurrentStance == ESCCharacterStance::Combat)
			OnHideEmoteSelect();
	}
}

void ASCCounselorCharacter::PutOnPamelasSweater(ASCPamelaSweater* Sweater)
{
	bHasPamelasSweater = true;
	SweaterAbility = Sweater->GetAbility();
	SweaterAbility->SetAbilityOwner(this);
	Sweater->PutOn();

	if (IsLocallyControlled())
	{
		// Put on our sweater outfit
		SERVER_SetWearingSweater(true);

		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			PC->GetSCHUD()->SetSweaterAbilityAvailable(true);
		}
	}
}

void ASCCounselorCharacter::TakeOffPamelasSweater()
{
#if !UE_BUILD_SHIPPING
	// In dev builds, check to see if we have the cheat enabled
	if (!bUnlimitedSweaterStun && !IsInContextKill())
	{
#endif
	bHasPamelasSweater = false;
	if (SweaterAbility)
	{
		SweaterAbility->SetAbilityOwner(nullptr);
		SweaterAbility = nullptr;
	}

	// Revert to our normal clothing
	if (IsLocallyControlled())
		SERVER_SetWearingSweater(false);

#if !UE_BUILD_SHIPPING
	}
#endif

	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		if (USCCounselorAnimInstance* AnimInst = Cast<USCCounselorAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			if (AnimInst->SweaterAbilityMontage && AnimInst->Montage_IsPlaying(AnimInst->SweaterAbilityMontage))
			{
				const static FName NAME_ExitSection(TEXT("Exit"));
				AnimInst->Montage_JumpToSection(NAME_ExitSection, AnimInst->SweaterAbilityMontage);
			}
		}
	}
}

void ASCCounselorCharacter::OnCharacterInView(ASCCharacter* VisibleCharacter)
{
	if (!VisibleCharacter)
		return;

	Super::OnCharacterInView(VisibleCharacter);

	if (VisibleCharacter->IsA(ASCKillerCharacter::StaticClass()))
	{
		bool bIgnoreJason = false;
		if (AAIController* AIC = Cast<AAIController>(Controller))
		{
			bIgnoreJason = AIC->bAIIgnorePlayers && (VisibleCharacter->Controller && VisibleCharacter->Controller->IsPlayerController());
		}

		if (!bIgnoreJason)
		{
			if (ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(VisibleCharacter))
			{
				KillerInView(KillerCharacter);
			}

			if (!bIsKillerOnMap)
			{
				FearManager->PushFearEvent(JasonSpottedFearEvent);
				if (IsLocallyControlled() && !GetWorld()->GetGameState()->IsA(ASCGameState_Paranoia::StaticClass()))
				{
					VisibleCharacter->MinimapIconComponent->SetVisibility(true);
				}

				bIsKillerOnMap = true;
			}
		}
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(VisibleCharacter))
	{
		FriendlyInView(Counselor);

		if (!VisibleCharacter->IsAlive())
		{
			FearManager->PushFearEvent(CorpseSpottedFearEvent);
			NumCorpseSpotted++;
		}
	}
}

void ASCCounselorCharacter::OnCharacterOutOfView(ASCCharacter* VisibleCharacter)
{
	if (!VisibleCharacter)
		return;

	if (VisibleCharacter->IsA(ASCKillerCharacter::StaticClass()))
	{
		KillerOutOfView();

		if (!IsAbilityActive(EAbilityType::Spotting))
		{
			ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(VisibleCharacter);
			OutOfSightTime = Killer->IsVisible() ? 3.0f : 0.1f;
			bIsKillerOnMap = false;
		}
		FearManager->PopFearEvent(JasonSpottedFearEvent);
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(VisibleCharacter))
	{
		FriendlyOutOfView(Counselor);

		if (!VisibleCharacter->IsAlive())
		{
			if (--NumCorpseSpotted == 0)
				FearManager->PopFearEvent(CorpseSpottedFearEvent);
		}
	}
}

float ASCCounselorCharacter::GetNoiseLevel() const
{
	float NoiseLevel = 0.0f;

	if (GetVelocity().SizeSquared() <= 0.0f)
		return 0.0f;

	switch (CurrentStance)
	{
		case ESCCharacterStance::Crouching:
			NoiseLevel = CrouchingNoiseMod;
			break;

		case ESCCharacterStance::Standing:
		case ESCCharacterStance::Combat:
			if (IsSprinting())
				NoiseLevel = SprintingNoiseMod;
			else if (IsRunning())
				NoiseLevel = RunningNoiseMod;
			else
				NoiseLevel = WalkingNoiseMod;
			break;

		case ESCCharacterStance::Swimming:
			NoiseLevel = SwimmingNoiseMod;
			break;
	}

	if (bIsHoldingBreath)
		NoiseLevel -= BreathNoiseMod.Get(this);

	// Apply perk modifiers
	if (ASCPlayerState* PS = GetPlayerState())
	{
		float NoiseModifier = 0.0f;
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			NoiseModifier += Perk->NoiseModifier;

			if (CurrentStance == ESCCharacterStance::Swimming)
				NoiseModifier += Perk->SwimmingNoiseModifier;
			else if (IsSprinting())
				NoiseModifier += Perk->SprintNoiseModifier;
		}

		NoiseLevel += NoiseLevel * NoiseModifier;
	}

	// TODO: setup stuff for charging though a door(10), Opening/Closing a door(6), smashing a window(12), barricading(10), repairing(8), screaming(12)

	return NoiseLevel * NoiseStatMod.Get(this, true);
}

void ASCCounselorCharacter::CLIENT_RemoveAllInputComponents_Implementation()
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->PopInputComponent(PassengerInputComponent);
		PC->PopInputComponent(HidingInputComponent);
		PC->PopInputComponent(WiggleInputComponent);
		PC->PopInputComponent(CombatStanceInputComponent);
		PC->PopInputComponent(AimInputComponent);
		PC->PopInputComponent(InteractMinigameInputComponent);
	}
}

void ASCCounselorCharacter::DetachLimb(USCCharacterBodyPartComponent* Limb, const FVector Impulse /*= FVector::ZeroVector*/, TAssetPtr<UTexture> BloodMaskOverride /*= nullptr*/, const bool bShowLimb /*= true*/)
{
	if (!Limb)
		return;

	// Apply the detach gore to our character and limbs
	ApplyGore(FGoreMaskData(BloodMaskOverride ? BloodMaskOverride : Limb->LimbUVTexture, Limb->TextureParameterName, Limb->ParentMaterialElementIndices));

	// Detach the limb
	Limb->DetachLimb(Impulse, BloodMaskOverride, bShowLimb);
}

void ASCCounselorCharacter::SERVER_DetachLimb_Implementation(USCCharacterBodyPartComponent* Limb, FVector_NetQuantize Impulse)
{
	MULTICAST_DetachLimb(Limb, Impulse);
}

bool ASCCounselorCharacter::SERVER_DetachLimb_Validate(USCCharacterBodyPartComponent* Limb, FVector_NetQuantize Impulse)
{
	return true;
}

void ASCCounselorCharacter::MULTICAST_DetachLimb_Implementation(USCCharacterBodyPartComponent* Limb, FVector_NetQuantize Impulse)
{
	DetachLimb(Limb, Impulse);
}

void ASCCounselorCharacter::ReattachAllLimbs()
{
	const auto AttachLimb = [this](USCCharacterBodyPartComponent*& Limb, FName AttachSocket, USkeletalMeshComponent* ParentMesh)
	{
		ParentMesh->UnHideBoneByName(AttachSocket);
		Limb->AttachToComponent(ParentMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), AttachSocket);
		Limb->SetMasterPoseComponent(ParentMesh);
		Limb->SetVisibility(false, true);
		Limb->HideLimbGoreCaps();
	};

	USkeletalMeshComponent* CharMesh = GetMesh();
	AttachLimb(RightArmLimb, NAME_upperarm_r, CharMesh);
	AttachLimb(LeftArmLimb, NAME_upperarm_l, CharMesh);
	AttachLimb(RightLegLimb, NAME_calf_r, CharMesh);
	AttachLimb(LeftLegLimb, NAME_calf_l, CharMesh);

	// Face just gotta be special
	static FName NAME_NoCollision(TEXT("NoCollision"));
	CharMesh->UnHideBoneByName(FaceLimb->BoneToHide);
	FaceLimb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FaceLimb->SetCollisionProfileName(NAME_NoCollision);
	FaceLimb->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("headSocket"));
	FaceLimb->SetVisibility(true, true);
	FaceLimb->HideLimbGoreCaps();
}

void ASCCounselorCharacter::ShowBloodOnLimb(USCCharacterBodyPartComponent* Limb)
{
	if (!Limb)
		return;

	Limb->ShowBlood();
}

void ASCCounselorCharacter::HideFace(bool bHide)
{
	if (FaceLimb->bHiddenInGame != bHide)
	{
		if (bHide)
			GetMesh()->HideBoneByName(FaceLimb->BoneToHide, EPhysBodyOp::PBO_None);
		else
			GetMesh()->UnHideBoneByName(FaceLimb->BoneToHide);

		FaceLimb->SetHiddenInGame(bHide, true);
	}
}

void ASCCounselorCharacter::SpawnBloodDecal(FName TestSocket, const FVector TestDirection, UMaterial* DecalTexture, const float DecalSize, const float TestLength/* = 5.f*/)
{
	const FVector RayStart = GetMesh()->GetSocketLocation(TestSocket);
	const FVector RayEnd = RayStart + (GetActorRotation().RotateVector(TestDirection) * TestLength);
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, RayStart, RayEnd, ECollisionChannel::ECC_Visibility, QueryParams))
	{
		UGameplayStatics::SpawnDecalAttached(DecalTexture, FVector(1.f, DecalSize, DecalSize), HitResult.Component.Get(), NAME_None, HitResult.ImpactPoint, HitResult.Normal.Rotation(), EAttachLocation::KeepWorldPosition);
	}
}

USCCharacterBodyPartComponent* ASCCounselorCharacter::GetBodyPartWithBone(const FName BoneName) const
{
	if (FaceLimb->BoneIsChildOf(BoneName, NAME_head) || (BoneName == NAME_head))
		return FaceLimb;
	else if (RightArmLimb->BoneIsChildOf(BoneName, NAME_upperarm_r) || BoneName == NAME_upperarm_r)
		return RightArmLimb;
	else if (LeftArmLimb->BoneIsChildOf(BoneName, NAME_upperarm_l) || BoneName == NAME_upperarm_l)
		return LeftArmLimb;
	else if (RightLegLimb->BoneIsChildOf(BoneName, NAME_calf_r) || BoneName == NAME_calf_r)
		return RightLegLimb;
	else if (LeftLegLimb->BoneIsChildOf(BoneName, NAME_calf_l) || BoneName == NAME_calf_l)
		return LeftLegLimb;
	else if (TorsoLimb->BoneIsChildOf(BoneName, NAME_spine_01) || BoneName == NAME_spine_01 || GetMesh()->BoneIsChildOf(BoneName, NAME_spine_01))
		return TorsoLimb;

	return nullptr;
}

void ASCCounselorCharacter::PopLimbs()
{
	if (bDebugLimbsAreDetached)
	{
		ReattachAllLimbs();
	}
	else
	{
		DetachLimb(RightArmLimb, FVector::ZeroVector);
		DetachLimb(LeftArmLimb, FVector::ZeroVector);
		DetachLimb(RightLegLimb, FVector::ZeroVector);
		DetachLimb(LeftLegLimb, FVector::ZeroVector);
		DetachLimb(FaceLimb, FVector::ZeroVector);
	}

	bDebugLimbsAreDetached = !bDebugLimbsAreDetached;
}

void ASCCounselorCharacter::ApplyGore(const FGoreMaskData& GoreData)
{
	if (IsRunningDedicatedServer())
		return;

	PendingGoreDatas.Add(GoreData);

	if (GoreData.BloodMask.IsNull())
	{
		// No BloodMask to load
		DeferredApplyGoreData();
	}
	else
	{
		if (GoreData.BloodMask.Get())
		{
			// Already loaded
			DeferredApplyGoreData();
		}
		else if (!GoreData.BloodMask.IsNull())
		{
			// Load and play when it's ready
			USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
			GameInstance->StreamableManager.RequestAsyncLoad(GoreData.BloodMask.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredApplyGoreData));
		}
	}
}

void ASCCounselorCharacter::MULTICAST_RemoveGore_Implementation()
{
	RemoveGore();
}

void ASCCounselorCharacter::RemoveGore()
{
	if (IsRunningDedicatedServer())
		return;

	// Clears all blood mask parameters (and anything else we might have set!
	for (UMaterialInstanceDynamic* GoreMask : CurrentGoreMasks)
	{
		if (GoreMask)
		{
			GoreMask->ClearParameterValues();
		}
	}

	// Empty our gore lists
	CurrentGoreMasks.Reset();
	PendingGoreDatas.Empty();

	// Take the blood off of all our limbs
	FaceLimb->RemoveBlood();
	TorsoLimb->RemoveBlood();
	RightArmLimb->RemoveBlood();
	LeftArmLimb->RemoveBlood();
	RightLegLimb->RemoveBlood();
	LeftLegLimb->RemoveBlood();

	// NOTE: If you need any persistent parameters turned back on after cleaning up blood, reset them here

	// Reset our dynamic skin texture on our skin material
	if (DynamicSkinTexture)
	{
		UMaterialInstanceDynamic* DynamicSkinMaterial = GetMesh()->CreateDynamicMaterialInstance(SkinMaterialIndex, GetMesh()->GetMaterial(SkinMaterialIndex));
		DynamicSkinMaterial->SetTextureParameterValue(SkinTextureMaterialParameter, DynamicSkinTexture);
	}
}

void ASCCounselorCharacter::DeferredApplyGoreData()
{
	check(!IsRunningDedicatedServer());
	for (int32 DataIndex = PendingGoreDatas.Num()-1; DataIndex >= 0; --DataIndex)
	{
		const FGoreMaskData& PendingGoreData = PendingGoreDatas[DataIndex];
		if (PendingGoreData.BloodMask.Get())
		{
			// Blood on the body
			const int32 NumMaterials = GetMesh()->GetNumMaterials();
			for (int iMat(0); iMat < PendingGoreData.MaterialElementIndices.Num(); ++iMat)
			{
				const int32 MatIndex = PendingGoreData.MaterialElementIndices[iMat];
				if (MatIndex < NumMaterials)
				{
					if (UMaterialInstanceDynamic* Mat = GetMesh()->CreateAndSetMaterialInstanceDynamic(MatIndex))
					{
						Mat->SetTextureParameterValue(PendingGoreData.TextureParameter, PendingGoreData.BloodMask.Get());
						CurrentGoreMasks.AddUnique(Mat);
					}
				}
			}

			// Blood on the clothes
			for (int iMat(0); iMat < CounselorOutfitComponent->GetNumMaterials(); ++iMat)
			{
				if (UMaterialInstanceDynamic* Mat = CounselorOutfitComponent->CreateAndSetMaterialInstanceDynamic(iMat))
				{
					Mat->SetTextureParameterValue(PendingGoreData.TextureParameter, PendingGoreData.BloodMask.Get());
					CurrentGoreMasks.AddUnique(Mat);
				}
			}

			// Apply to limbs as well
			FaceLimb->ShowBlood(PendingGoreData.BloodMask, PendingGoreData.TextureParameter);
			TorsoLimb->ShowBlood(PendingGoreData.BloodMask, PendingGoreData.TextureParameter);
			RightArmLimb->ShowBlood(PendingGoreData.BloodMask, PendingGoreData.TextureParameter);
			LeftArmLimb->ShowBlood(PendingGoreData.BloodMask, PendingGoreData.TextureParameter);
			RightLegLimb->ShowBlood(PendingGoreData.BloodMask, PendingGoreData.TextureParameter);
			LeftLegLimb->ShowBlood(PendingGoreData.BloodMask, PendingGoreData.TextureParameter);

			// Loaded, no longer pending
			PendingGoreDatas.RemoveAtSwap(DataIndex);
		}
	}
}

void ASCCounselorCharacter::ApplyDamageGore(TSubclassOf<UDamageType> DamageType)
{
	if (DamageType == nullptr)
		return;

	for (const FDamageGoreMaskData& GoreData : DamageGoreMasks)
	{
		if (GoreData.DamageType == DamageType)
		{
			if (Health >= GoreData.HealthThreshold)
			{
				ApplyGore(GoreData);
				break;
			}
		}
	}
}

void ASCCounselorCharacter::OnRep_Wounded()
{
	if (IsLocallyControlled())
		SetSprinting(false);
}

void ASCCounselorCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, class AActor* DamageCauser)
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	if (IsLocallyControlled())
	{
		// We dead
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			PC->SetCinematicMode(false, false, false, true, true);
			PC->GetSCHUD()->SetHUDWidgetOpacity(0.0f);
		}

		UGameplayStatics::ClearSoundMixModifiers(GetWorld());
	}
	else
	{
		// They dead
		if (APlayerController* OtherController = UILLGameBlueprintLibrary::GetFirstLocalPlayerController(this))
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherController->GetPawn()))
			{
				// In-view or in-distance? Skip the dead body stinger
				if (Counselor->FriendliesInView.Contains(this)
				&& GetSquaredDistanceTo(Counselor) < FMath::Square(MaxCorpseSpottedJumpDistance))
					Counselor->AlreadySeenDeadCounselors.AddUnique(this);
			}
		}
	}

	if (!IsInContextKill())
	{
		if (bIsGrabbed)
		{
			if (GrabbedBy)
				GrabbedBy->DropCounselor();
		}
		else if (!bPlayedDeathAnimation)
		{
			// Play dem death animations
			if (USCAnimInstance* AI = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
			{
				const FVector KillDirection = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() : GetActorForwardVector();
				bIgnoreDeathRagdoll = AI->PlayDeathAnimation(KillDirection);
				if (const USCBaseDamageType* const DamageType = Cast<USCBaseDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject()))
					bIgnoreDeathRagdoll &= DamageType->bPlayDeathAnim;
				bPauseAnimsOnDeath = !bIgnoreDeathRagdoll;
				bPlayedDeathAnimation = true;
			}
		}
	}

	// Detach the wheelchair from our counselor
	if (bInWheelchair)
		DetachWheelchair();

	// we dont want screen effects anymore once the player is dead, nuke that shit
	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		PPV->Destroy();
		PPV = nullptr;
	}

	const bool HasImposterItem = HasSpecialItem(ASCImposterOutfit::StaticClass()) != nullptr;

	if (Role == ROLE_Authority)
	{
		// Drop items on shore if we get drowneded
		if (CurrentWaterVolume)
		{
			DropItemsOnShore();
		}
		else
		{
			// Drop large item
			if (EquippedItem)
			{
				if (CurrentHidingSpot)
				{
					// We have to move the item outside the hiding spot...
					EquippedItem->OnDropped(this, CurrentHidingSpot->GetItemDropLocation());
				}
				else
				{
					EquippedItem->OnDropped(this);
				}

				EquippedItem = nullptr;
				OnRep_EquippedItem();
			}

			// Drop small items
			const int32 NumItems = SmallItemInventory->GetItemList().Num();
			for (int32 iSmallItem(NumItems - 1); iSmallItem >= 0; --iSmallItem)
			{
				if (ASCItem* SmallItem = Cast<ASCItem>(SmallItemInventory->GetItemList()[iSmallItem]))
				{
					SmallItemInventory->RemoveItem(SmallItem);
					if (CurrentHidingSpot)
					{
						// We have to move the item outside the hiding spot...
						SmallItem->OnDropped(this, CurrentHidingSpot->GetItemDropLocation(iSmallItem, SmallItem->GetMesh()->Bounds.BoxExtent.Z * 0.5f));
					}
					else
					{
						SmallItem->OnDropped(this);
					}
				}
			}

			// drop special paranoia item
			if (ASCSpecialItem* ParanoiaOutfit = Cast<ASCSpecialItem>(HasSpecialItem(ASCImposterOutfit::StaticClass())))
			{
				RemoveSpecialItem(ParanoiaOutfit, false);
				ParanoiaOutfit->OnDropped(this);
			}
		}
	}

	// the person that had the imposter outfit died, play a global noise
	if (HasImposterItem)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ImposterDeathNoise);
	}

	// Kill sense effects
	SetSenseEffects(false);

	// Kill flashlight
	ShutOffFlashlight();

	Super::OnDeath(KillingDamage, DamageEvent, KillerPlayerState, DamageCauser);

	// Head collision must be enabled so people can shoot you in the face or throw a knife at your head
	// When we die, it must be turned off (if it's still connected to your body) or it collides with the ragdoll and you get a Weekend at Bernie's effect
	if (FaceLimb->GetAttachParent())
		FaceLimb->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Bodies hitting the car fucked things up with the car so we're removing it.
	SetVehicleCollision(false);
}

void ASCCounselorCharacter::EnterHidingSpot(ASCHidingSpot* HidingSpot)
{
	bInHiding = true;
	FearManager->PushFearEvent(HiddenFearEvent);

	if (bIsFlashlightOn && Role == ROLE_Authority)
		ToggleFlashlight();

	HidingSpot->SetOwner(this);
	CurrentHidingSpot = HidingSpot;

	// Broadcast the event if a delegate is bound.
	if (OnIsHidingChanged.IsBound())
		OnIsHidingChanged.Broadcast(bInHiding);

	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		PlayerController->PushInputComponent(HidingInputComponent);

		if (IsLocallyControlled())
		{
			// Modify user sensitivity while hiding
			ModifyInputSensitivity(EKeys::MouseX, HidingSensitivityMod);
			ModifyInputSensitivity(EKeys::MouseY, HidingSensitivityMod);
			ModifyInputSensitivity(EKeys::Gamepad_RightX, HidingSensitivityMod);
			ModifyInputSensitivity(EKeys::Gamepad_RightY, HidingSensitivityMod);

			if (PlayerController->GetSCHUD())
			{
				if (ASCInGameHUD* HUD = PlayerController->GetSCHUD())
				{
					HUD->SetBreathVisible(true);
					HUD->CloseCounselorMap();
				}

			}

			OnHideEmoteSelect();
		}
	}
}

void ASCCounselorCharacter::OnExitHidingPressed()
{
	if (CurrentHidingSpot)
	{
		OnInteract(CurrentHidingSpot->GetInteractComponent(), EILLInteractMethod::Press);
	}
}

void ASCCounselorCharacter::ExitHidingSpot(ASCHidingSpot* HidingSpot)
{
	FearManager->PopFearEvent(HiddenFearEvent);

	// Broadcast the event if a delegate is bound.
	if (OnIsHidingChanged.IsBound())
		OnIsHidingChanged.Broadcast(bInHiding);

	// reset input sensitivity from hiding
	if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
	{
		PlayerController->PopInputComponent(HidingInputComponent);

		if (IsLocallyControlled())
		{
			RestoreInputSensitivity(EKeys::MouseX);
			RestoreInputSensitivity(EKeys::MouseY);
			RestoreInputSensitivity(EKeys::Gamepad_RightX);
			RestoreInputSensitivity(EKeys::Gamepad_RightY);

			if (PlayerController->GetSCHUD())
				PlayerController->GetSCHUD()->SetBreathVisible(false);

			if (bIsHoldingBreath)
				OnReleaseBreath();

			OnHideEmoteSelect();
		}
	}
}

void ASCCounselorCharacter::FinishedExitingHiding()
{
	if (CurrentHidingSpot)
	{
		CurrentHidingSpot->SetOwner(nullptr);
		CurrentHidingSpot = nullptr;
	}

	bInHiding = false;
}

bool ASCCounselorCharacter::CanAttack() const
{
	if (IsDodging())
		return false;

	if (bIsGrabbed)
		return false;

	if (bInHiding)
		return false;

	if (CurrentSeat)
		return false;

	if (PendingSeat)
		return false;

	if (CurrentEmote && bEmoteBlocksMovement)
		return false;

	if (IsUsingSweaterAbility())
		return false;

	return Super::CanAttack();
}

bool ASCCounselorCharacter::TestLights()
{
	if (!GetCapsuleComponent())
		return false;

	if (bIsFlashlightOn)
		return true;

	for (TObjectIterator<ULightComponent> LightIter; LightIter; ++LightIter)
	{
		ULightComponent* Light = *LightIter;

		if (!Light || Light->GetWorld() != GetWorld())
			continue;

		if (Light->IsA(UPointLightComponent::StaticClass()) && Light->bVisible && Light->AffectsPrimitive(GetCapsuleComponent()))
		{
			// Calculate light intensity
			const UPointLightComponent* LightComp = Cast<UPointLightComponent>(Light);
			if (LightComp)
			{
				const float Dist = FVector::Dist(GetCapsuleComponent()->GetComponentLocation(), Light->GetComponentLocation());

				if (FMath::Pow(FMath::Max(0.f, 1.f - (Dist / LightComp->AttenuationRadius)), (LightComp->LightFalloffExponent + 1)) * (LightComp->AttenuationRadius * 1.25f) > MinLightStrength)
				{
					return true;
				}
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
// Stumbling

void ASCCounselorCharacter::OnRep_SetStumbling(bool bWasStumbling)
{
	if (bIsStumbling)
	{
		UAnimMontage* StumbleAnim = IsSprinting() ? SprintStumbleAnim : IsRunning() ? RunStumbleAnim : WalkStumbleAnim;
		PlayAnimMontage(StumbleAnim);

		OnStartStumble();
	}
	else
	{
		OnStopStumble();
	}
}

float ASCCounselorCharacter::EvaluateStumbleFrequency() const
{
	uint32 CurveCount = 0;
	float TotalCurveResult = 0.f;

	if (StumbleFearFrequecy != nullptr)
	{
		++CurveCount;
		TotalCurveResult += StumbleFearFrequecy->GetFloatValue(GetNormalizedFear());
	}

	if (StumbleStaminaFrequency != nullptr)
	{
		++CurveCount;
		TotalCurveResult += StumbleStaminaFrequency->GetFloatValue(1.f - (GetStamina() / GetMaxStamina()));
	}

	if (CurveCount == 0)
		return 1.f;

	if (CVarDebugStumbling.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Fear Frequency: %.1f (Stamina Value: %.1f, Fear value: %.1f)"), StumbleFearFrequecy ? StumbleFearFrequecy->GetFloatValue(GetNormalizedFear()) : 0.f, GetStamina(), GetFear()));
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Stamina Frequency: %.1f"), StumbleStaminaFrequency ? StumbleStaminaFrequency->GetFloatValue(1.f - (GetStamina() / GetMaxStamina())) : 0.f));
	}

	return TotalCurveResult / (float)CurveCount;
}

float ASCCounselorCharacter::EvaluateStumbleChance() const
{
	float TotalCurveResult = 0.f;

	float AvoidFearStumbleChance = 0.f;
	if (ASCPlayerState* PS = GetPlayerState())
	{
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			AvoidFearStumbleChance += Perk->AvoidFearStumble;
		}
	}

	// Check perks to avoid stumbling based on fear
	if (FMath::FRand() > AvoidFearStumbleChance)
	{
		if (StumbleFearChance != nullptr)
			TotalCurveResult += StumbleFearChance->GetFloatValue(GetNormalizedFear());
	}
		
	if (StumbleStaminaChance != nullptr)
		TotalCurveResult += StumbleStaminaChance->GetFloatValue(1.f - (GetStamina() / GetMaxStamina()));

	if (IsWounded())
		TotalCurveResult += StumbleWoundedChance;

	if (CVarDebugStumbling.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Fear Chance: %.1f"), StumbleFearChance ? StumbleFearChance->GetFloatValue(GetNormalizedFear()) : 0.f));
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Stamina Chance: %.1f"), StumbleStaminaChance ? StumbleStaminaChance->GetFloatValue(1.f - (GetStamina() / GetMaxStamina())) : 0.f));
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Wounded Chance: %.1f"), IsWounded() ? StumbleWoundedChance : 0.f));
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan, FString::Printf(TEXT("Stat Chance Mod: %.2f"), StumbleStatMod.Get(this, true)));
	}

	return TotalCurveResult * StumbleStatMod.Get(this, true);
}

bool ASCCounselorCharacter::CanStumble() const
{
	// Must be moving
	if (GetRequestedMoveInput().SizeSquared() < KINDA_SMALL_NUMBER)
		return false;

	// Can't be walking
	if (!IsRunning() && !IsSprinting())
		return false;

	// Can't be busy doing something else
	if (IsInSpecialMove())
		return false;

	// Can't be swimming, crouching, combatting, or whatever else. Gotta be standing
	if (CurrentStance != ESCCharacterStance::Standing)
		return false;

	// Can't be swinging
	if (IsAttacking())
		return false;

	// Can't be held
	if (bIsGrabbed)
		return false;

	// Getting out of a vehicle (entering is a special move)
	if (bLeavingSeat)
		return false;

	return true;
}

void ASCCounselorCharacter::UpdateStumbling_SERVER(float DeltaSeconds)
{
	// If we can't stumble, move our timer forward
	if (!CanStumble())
	{
		LastStumbleRollTimestamp += DeltaSeconds;
		return;
	}

	const float CurrentFrequency = EvaluateStumbleFrequency();
	const float CurrentFrameTime = GetWorld()->GetTimeSeconds();

	if ((CurrentFrameTime - LastStumbleRollTimestamp) <= CurrentFrequency)
		return;

	LastStumbleRollTimestamp += CurrentFrequency;

	// If we're stumbling (or in cooldown), we don't need to re-roll
	if (bIsStumbling || (CurrentFrameTime - LastStumbleTimestamp) <= StumbleCooldown)
		return;

	// Roll from (0->100] and invert chance so it makes sense as a roll (e.g.: 40 or higher)
	const float Roll = FMath::FRandRange(SMALL_NUMBER, 100.f);
	const float Chance = 100.f - EvaluateStumbleChance();
	if (Roll > Chance)
	{
		SetStumbling(true, false);
	}

	if (CVarDebugStumbling.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, CurrentFrequency * (bIsStumbling ? 5.f : 1.5f), bIsStumbling ? FColor::Green : FColor::Red, FString::Printf(TEXT("Stumble attempt: rolled a %.2f against a %.2f"), Roll, Chance));
	}
}

void ASCCounselorCharacter::SetStumbling(bool bStumbling, bool bFromStaminaDepletion /*= false*/)
{
	// Do nothing if we're not changing anything
	if (bIsStumbling == bStumbling)
		return;

	// Can't stumble in a wheelchair
	if (bInWheelchair)
		return;

	if (Role < ROLE_Authority)
	{
		ServerSetStumbling(bStumbling);
	}

	if (Role == ROLE_Authority || (Role == ROLE_AutonomousProxy && GetDefault<AGameNetworkManager>()->ClientAuthorativePosition))
	{
		bIsStumbling = bStumbling;

		if (bStumbling)
		{
			LastStumbleTimestamp = GetWorld()->GetTimeSeconds();

			UAnimMontage* StumbleAnim = IsSprinting() ? SprintStumbleAnim : IsRunning() ? RunStumbleAnim : WalkStumbleAnim;

			const float StumbleAnimDuration = PlayAnimMontage(bFromStaminaDepletion ? OutOfStaminaStumbleAnim : StumbleAnim);
			if (StumbleAnimDuration > 0.f)
			{
				FTimerDelegate Delegate;
				Delegate.BindLambda([this](){ SetStumbling(false); });
				GetWorldTimerManager().SetTimer(TimerHandle_Stumble, Delegate, FMath::Max(0.1f, StumbleAnimDuration), false);

				OnStartStumble();
			}
			else
			{
				SetStumbling(false);
			}
		}
		else
		{
			OnStopStumble();
		}
	}
}

bool ASCCounselorCharacter::ServerSetStumbling_Validate(bool bStumbling, bool bFromStaminaDepletion /*= false*/)
{
	return true;
}

void ASCCounselorCharacter::ServerSetStumbling_Implementation(bool bStumbling, bool bFromStaminaDepletion /*= false*/)
{
	SetStumbling(bStumbling, bFromStaminaDepletion);
}

// ~Stumbling
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Console Commands

void ASCCounselorCharacter::LockStaminaTo(float InStamina, bool Lock /* = true */)
{
	if (Role < ROLE_Authority)
	{
		SERVER_LockStaminaTo(InStamina, Lock);
	}

	CurrentStamina = FMath::Clamp(InStamina, 0.f, 100.f);
	bLockStamina = Lock;
}

bool ASCCounselorCharacter::SERVER_LockStaminaTo_Validate(float NewStamina, bool Lock)
{
	return true;
}

void ASCCounselorCharacter::SERVER_LockStaminaTo_Implementation(float NewStamina, bool Lock)
{
	LockStaminaTo(NewStamina, Lock);
}

// ~Console Commands
////////////////////////////////////////////////////////////////////////////////

void ASCCounselorCharacter::Destroyed()
{
	// Some stuff should only happen on the server
	if (Role == ROLE_Authority)
	{
		if (EquippedItem)
		{
			DropEquippedItem();
		}
	}

	if (MusicComponent)
	{
		MusicComponent->ShutdownMusicComponent();
		MusicComponent->DestroyComponent();
		MusicComponent = nullptr;
	}

	if (VoiceOverComponent)
	{
		VoiceOverComponent->CleanupVOComponent();
	}

	Super::Destroyed();
}

bool ASCCounselorCharacter::IsLocallyControlled() const
{
	if (Controller)
		return Super::IsLocallyControlled();

	if (CurrentSeat && CurrentSeat->IsDriverSeat())
	{
		if (CurrentSeat->ParentVehicle && CurrentSeat->ParentVehicle->Controller)
		{
			return CurrentSeat->ParentVehicle->IsLocallyControlled();
		}
	}
	else if (PendingSeat && PendingSeat->IsDriverSeat())
	{
		if (PendingSeat->ParentVehicle && PendingSeat->ParentVehicle->Controller)
		{
			return PendingSeat->ParentVehicle->IsLocallyControlled();
		}
	}

	return false;
}

bool ASCCounselorCharacter::IsPlayerControlled() const
{
	if (Controller)
		return Super::IsPlayerControlled();

	if (CurrentSeat && CurrentSeat->IsDriverSeat())
	{
		if (CurrentSeat->ParentVehicle && CurrentSeat->ParentVehicle->Controller)
		{
			return CurrentSeat->ParentVehicle->IsPlayerControlled();
		}
	}
	else if (PendingSeat && PendingSeat->IsDriverSeat())
	{
		if (PendingSeat->ParentVehicle && PendingSeat->ParentVehicle->Controller)
		{
			return PendingSeat->ParentVehicle->IsPlayerControlled();
		}
	}

	return false;
}

void ASCCounselorCharacter::AddVehicleYawInput(float Val)
{
	if (Val)
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			float YawRotation = Val * PC->InputYawScale * CameraRotateScale.Y;
			if (IsUsingGamepad())
			{
				const float DeltaSeconds = [this]()
				{
					if (UWorld* World = GetWorld())
						return World->GetDeltaSeconds();
					return 0.f;
				}();

				// Not sure why, but rate seems to be doubled, so we half it and scale it by time
				YawRotation *= (CameraRotateRate.Y * 0.5f * DeltaSeconds);
			}

			LocalVehicleYaw += YawRotation;
			if (CurrentSeat && CurrentSeat->ParentVehicle && CurrentSeat->ParentVehicle->VehicleType == EDriveableVehicleType::Car)
				LocalVehicleYaw = FMath::Clamp(LocalVehicleYaw, -MaxVehicleCameraYawAngle, MaxVehicleCameraYawAngle);
		}
	}
}

void ASCCounselorCharacter::AddVehiclePitchInput(float Val)
{
	if (Val)
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			float PitchRotation = Val * PC->InputPitchScale * CameraRotateScale.X;
			if (IsUsingGamepad())
			{
				const float DeltaSeconds = [this]()
				{
					if (UWorld* World = GetWorld())
						return World->GetDeltaSeconds();
					return 0.f;
				}();

				// Not sure why, but rate seems to be doubled, so we half it and scale it by time
				PitchRotation *= (CameraRotateRate.X * 0.5f * DeltaSeconds);
			}

			LocalVehiclePitch += PitchRotation;
			if (CurrentSeat && CurrentSeat->ParentVehicle)
			{
				if (CurrentSeat->ParentVehicle->VehicleType == EDriveableVehicleType::Car)
					LocalVehiclePitch = FMath::Clamp(LocalVehiclePitch, -MaxVehicleCameraPitchAngle, MaxVehicleCameraPitchAngle);
				else if (CurrentSeat->ParentVehicle->VehicleType == EDriveableVehicleType::Boat)
					LocalVehiclePitch = FMath::Clamp(LocalVehiclePitch, -90.f, -5.f);
			}
		}
	}
}

bool ASCCounselorCharacter::ShouldShowCharacter() const
{
	// No naked people!
	if (!bAppliedOutfit)
		return false;

	return Super::ShouldShowCharacter();
}

void ASCCounselorCharacter::UpdateCharacterMeshVisibility()
{
	const bool bPrevHidden = bHidden;
	Super::UpdateCharacterMeshVisibility();

	// Extra debug information to hopefully track down when/why counselors are not being shown
	if (bPrevHidden != bHidden)
	{
		UE_LOG(LogCounselorClothing, Log, TEXT("%s::UpdateCharacterMeshVisibility: Visibility updated (%s prev: %s) bAppliedOutfit = %s"),
			*GetNameSafe(this),
			bHidden ? TEXT("hidden") : TEXT("visible"),
			bPrevHidden ? TEXT("hidden") : TEXT("visible"),
			bAppliedOutfit ? TEXT("true") : TEXT("false"));
	}
}

void ASCCounselorCharacter::OnReceivedPlayerState()
{
	Super::OnReceivedPlayerState();

	UWorld* World = GetWorld();
	if (ASCPlayerState* PS = World ? GetPlayerState() : nullptr)
	{
		PS->OnPlayerSettingsUpdated.AddUniqueDynamic(this, &ThisClass::PlayerStateSettingsUpdated);

		// Apply any relevant perks
		ApplyPerks();

		// Outfit customization
		if (!bAppliedOutfit)
		{
			// Our fallback in case no outfit is selected (SetCurrentOutfit will clear the timer)
			if (const USCClothingData* const ClothingData = CurrentOutfit.ClothingClass ? CurrentOutfit.ClothingClass.GetDefaultObject() : nullptr)
			{
				// These aren't my clothes!
				if (!ClothingData->Counselor.Get() || !IsA(ClothingData->Counselor.Get()))
				{
					if (!GetWorldTimerManager().IsTimerActive(TimerHandle_RevertToDefaultOutfit))
						GetWorldTimerManager().SetTimer(TimerHandle_RevertToDefaultOutfit, this, &ThisClass::RevertToDefaultOutfit, DEFAULT_CLOTHING_DELAY);
				}
			}

			// Try and load our outfit from the player state
			const FSCCounselorOutfit& PendingOutfit = PS->GetPickedCounselorOutfit();
			if (CurrentOutfit != PendingOutfit)
			{
				const USCClothingData* const ClothingData = PendingOutfit.ClothingClass ? PendingOutfit.ClothingClass.GetDefaultObject() : nullptr;

				// Apply this counselor's selected outfit
				if (ClothingData && ClothingData->Counselor.Get() && IsA(ClothingData->Counselor.Get()))
				{
					SetCurrentOutfit(PendingOutfit);
				}
			}
		}

		// Update this player's in match presence
		if (IsLocallyControlled())
		{
			// HACK: Don't throw exceptions just because we're cycling characters in sandbox
			const ASCGameState_Sandbox* GS = World->GetGameState<ASCGameState_Sandbox>();
			if (!GS)
			{
				FString Presence;
				const FString MapName = World->GetMapName();
				PS->SetRichPresence(MapName, true);
			}
		}
	}
}

bool ASCCounselorCharacter::CanInteractAtAll() const
{
	if (CurrentEmote)
		return false;

	if (bIsGrabbed)
		return false;

	if (IsEscaping() || HasEscaped())
		return false;

	// make sure that we can't interact while picking up or dropping items
	// if we interact in the middle we might cancel the animation which means OnDropped/OnPickedUp never gets called in the anim instance
	if (EquippedItem && EquippedItem->bIsPicking)
		return false;

	return Super::CanInteractAtAll();
}

bool ASCCounselorCharacter::GetInfiniteAmmo()
{
	return (CVarDebugInfAmmo.GetValueOnGameThread() > 0);
}

bool ASCCounselorCharacter::CanDropItem() const
{
	if (IsInVehicle())
		return false;

	if (PendingSeat)
		return false;

	if (CurrentWaterVolume)
		return false;

	if (ActiveSpecialMove)
		return false;

	if (GetInteractableManagerComponent()->GetLockedInteractable())
		return false;

	if (IsUsingSweaterAbility())
		return false;

	if (bDroppingLargeItem || bDroppingSmallItem)
		return false;

	return true;
}

void ASCCounselorCharacter::DropItemsOnShore()
{
	check(Role == ROLE_Authority);

	UWorld* World = GetWorld();
	if (!World)
		return;

	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();
	if (!GameMode)
		return;

	const FVector PlayerLocation = GetActorLocation();
	FTransform ItemTransform(FTransform::Identity);
	
	// Drop Large Item
	if (ASCItem* CurrentEquippedItem = GetEquippedItem())
	{
		if (GameMode->GetShoreLocation(PlayerLocation, CurrentEquippedItem, ItemTransform))
		{
			DropEquippedItem(ItemTransform.GetLocation(), true);
		}
		else
		{
			DropEquippedItem(FVector::ZeroVector, true);
		}
	}

	// Drop small items
	while (SmallItemInventory->GetItemList().Num())
	{
		if (ASCItem* SmallItem = Cast<ASCItem>(SmallItemInventory->GetItemList()[0]))
		{
			SmallItemInventory->RemoveItem(SmallItem);

			if (GameMode->GetShoreLocation(PlayerLocation, SmallItem, ItemTransform))
			{
				SmallItem->OnDropped(nullptr, ItemTransform.GetLocation());
			}
			else
			{
				SmallItem->OnDropped(nullptr);
			}
		}
		else
		{
			SmallItemInventory->RemoveAtIndex(0);
		}
	}

	// drop special paranoia item
	if (ASCSpecialItem* ParanoiaOutfit = Cast<ASCSpecialItem>(HasSpecialItem(ASCImposterOutfit::StaticClass())))
	{
		RemoveSpecialItem(ParanoiaOutfit, false);
		ParanoiaOutfit->OnDropped(nullptr, ItemTransform.GetLocation());
	}
}

void ASCCounselorCharacter::OnDelayedDropItem()
{
	// call to game mode escape func
	if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
	{
		DropItemsOnShore();
		GameMode->CharacterEscaped(Controller, this);
	}
}

void ASCCounselorCharacter::FriendlyInView(ASCCounselorCharacter* VisibleCharacter)
{
	FriendliesInView.AddUnique(VisibleCharacter);
}

void ASCCounselorCharacter::FriendlyOutOfView(ASCCounselorCharacter* VisibleCharacter)
{
	FriendliesInView.Remove(VisibleCharacter);
}

void ASCCounselorCharacter::CheckForDeadFriendliesInRange()
{
	// Grab the character controller in case we're possessing another actor (car/boat/etc.)
	UWorld* World = GetWorld();
	if (!World || !Controller)
		return;

	// T.J. Aint scared of no dead peeps
	if (bIsHunter)
		return;

	const bool bSwimming = (CurrentStance == ESCCharacterStance::Swimming || CurrentWaterVolume); // Not while swimming
	const bool bInteracting = (bIsGrabbed || InteractableManagerComponent->GetLockedInteractable() || IsInSpecialMove()); // Not while interacting or being interacted with
	const bool bAwareOfKiller = (KillerInViewPtr || bIsBeingChased); // Not if the killer is in view or chasing us
	const bool bInRangeDeathStingersBlocked = (bSwimming || bInteracting || bAwareOfKiller);

	for (ASCCounselorCharacter* Friendly : FriendliesInView)
	{
		if (!Friendly)
			continue;

		// Ignore them if we've seen them
		if (AlreadySeenDeadCounselors.Contains(Friendly))
			continue;

		// Check to see if the friendly is dead
		bool bIsDead = !Friendly->IsAlive();
		if (!bIsDead && Friendly->IsInContextKill())
		{
			if (USCSpecialMove_SoloMontage* SM = Cast<USCSpecialMove_SoloMontage>(Friendly->ActiveSpecialMove))
			{
				bIsDead = SM->DieOnFinish();
			}
		}

		// Only dead bodies are scary
		if (!bIsDead)
			continue;

		// Check distance
		const FVector FriendlyLocation = Friendly->GetMesh()->GetComponentLocation();
		if (FVector::DistSquared(GetActorLocation(), FriendlyLocation) >= FMath::Square(MaxCorpseSpottedJumpDistance))
			continue;

		// Are we on cooldown or in some mutually exclusive state? Ignore this dead body forever
		if (bSeeDeadFriendliesCooldown || bInRangeDeathStingersBlocked)
		{
			AlreadySeenDeadCounselors.Add(Friendly);
			continue;
		}

		// Play the 'I've seen a dead body' music stinger
		if (USCMusicComponent_Counselor* MusicComponent_Counselor = Cast<USCMusicComponent_Counselor>(MusicComponent))
		{
			MusicComponent_Counselor->PlayDeadFriendlyStinger();
		}

		// Face the dead body
		const FVector CounselorHead = (GetActorLocation() + FVector::UpVector * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f));
		const FVector DirectionTo = FriendlyLocation - CounselorHead;
		const FRotator DirectionRotation(DirectionTo.Rotation());
		SetActorRotation(FRotator(0.f, DirectionRotation.Yaw, 0.f));
		Controller->SetControlRotation(FRotator(DirectionRotation.Pitch, DirectionRotation.Yaw, 0.f));

		// Play a camera shake
		PlayCameraShake(DeadBodyShake, 1.f, ECameraAnimPlaySpace::CameraLocal, FRotator::ZeroRotator);

		// Fake sight on Jason so we flee
		if (ASCCounselorAIController* AIController = Cast<ASCCounselorAIController>(Controller))
		{
			ASCGameState* GameState = GetWorld()->GetGameState<ASCGameState>();
			AIController->ReceivedKillerStimulus(GameState->CurrentKiller, FriendlyLocation, true, 1.f);
		}

		// Play animations
		DeadFriendlySpotted();

		// Tell the server
		SERVER_DeadFriendlySpotted();

		// Add them to the list of seen dead bodies
		AlreadySeenDeadCounselors.Add(Friendly);

		// Don't get annoying, suppress subsequent loops
		bSeeDeadFriendliesCooldown = true;
		GetWorldTimerManager().SetTimer(TimerHandle_SeeDeadFriendliesCooldown, this, &ASCCounselorCharacter::DeadFriendliesCooldownDone, DeadFriendliesCooldownTime);
	}
}

bool ASCCounselorCharacter::SERVER_DeadFriendlySpotted_Validate()
{
	return true;
}

void ASCCounselorCharacter::SERVER_DeadFriendlySpotted_Implementation()
{
	MULTICAST_DeadFriendlySpotted();
}

void ASCCounselorCharacter::MULTICAST_DeadFriendlySpotted_Implementation()
{
	// Skip it locally, that should happen in CheckForDeadFriendliesInRange
	if (!IsLocallyControlled())
	{
		DeadFriendlySpotted();
	}
}

void ASCCounselorCharacter::DeadFriendlySpotted()
{
	// Play an animation
	const float AnimationTime = PlayAnimMontage(CorpseSpottedJumpMontage);

	// Play the VO
	if (KillerInViewPtr)
	{
		static FName NAME_SeeJasonKillCounselor = TEXT("SeeJasonKillCounselor");
		VoiceOverComponent->PlayVoiceOver(NAME_SeeJasonKillCounselor);
	}
	else
	{
		static FName NAME_SeeDeadBody = TEXT("SeeDeadBody");
		VoiceOverComponent->PlayVoiceOver(NAME_SeeDeadBody);
	}
}

void ASCCounselorCharacter::DeadFriendliesCooldownDone()
{
	bSeeDeadFriendliesCooldown = false;
	TimerHandle_SeeDeadFriendliesCooldown.Invalidate();
}

void ASCCounselorCharacter::KillerInView(ASCKillerCharacter* VisibleKiller)
{
	if (!KillerInViewPtr)
	{
		KillerInViewPtr = VisibleKiller;

		// Fake sensor sight to keep that consistent with the pawn sensing system, we only hit this if they also are in sensor range
		if (ASCCounselorAIController* AIController = Cast<ASCCounselorAIController>(Controller))
		{
			AIController->OnSensorSeePawn(KillerInViewPtr);
		}
	}
}

void ASCCounselorCharacter::KillerOutOfView()
{
	if (KillerInViewPtr)
	{
		KillerInViewPtr = nullptr;
	}
}

void ASCCounselorCharacter::SetSenseEffects(bool Enable)
{
	GetMesh()->SetRenderCustomDepth(Enable);
	if (FaceLimb)
		FaceLimb->SetRenderCustomDepth(Enable);
	if (Hair)
		Hair->SetRenderCustomDepth(Enable);
	if (CounselorOutfitComponent)
		CounselorOutfitComponent->SetRenderCustomDepth(Enable);

	if (Enable)
	{
		if (SenseEffectComponent == nullptr)
			SenseEffectComponent = UGameplayStatics::SpawnEmitterAttached(SenseEffectParticle, RootComponent, TEXT("SenseEffect"), FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	}
	else
	{
		if (SenseEffectComponent)
		{
			SenseEffectComponent->DestroyComponent();
			SenseEffectComponent = nullptr;
		}
	}
}

bool ASCCounselorCharacter::CanHoldBreath() const
{
	if (IsInContextKill())
		return false;

	return bCanHoldBreath;
}

void ASCCounselorCharacter::OnHoldBreath()
{
	if (CanHoldBreath())
	{
		SERVER_SetHoldBreath(true);
		bPressedBreath = true;
	}
}


void ASCCounselorCharacter::OnReleaseBreath()
{
	if (bPressedBreath)
	{
		SERVER_SetHoldBreath(false);
		bPressedBreath = false;
	}
}

bool ASCCounselorCharacter::SERVER_SetHoldBreath_Validate(bool IsHoldingBreath)
{
	return true;
}

void ASCCounselorCharacter::SERVER_SetHoldBreath_Implementation(bool IsHoldingBreath)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	const bool DeltaBreath = bIsHoldingBreath != IsHoldingBreath;
	bIsHoldingBreath = IsHoldingBreath;

	if (DeltaBreath)
		OnRep_IsHoldingBreath();
}

void ASCCounselorCharacter::OnRep_IsHoldingBreath()
{
	if (IsInHiding())
	{
		if (bIsHoldingBreath)
		{
			// Play the Hold Breath VO for everyone, this will stop the "I hope he didn't see me" audio
			Cast<USCVoiceOverComponent_Counselor>(VoiceOverComponent)->HoldBreath(true);
		}
		else
		{
			// either let go or ran out of breath, if you want to see if they ran out of breath check CurrentBreath
			Cast<USCVoiceOverComponent_Counselor>(VoiceOverComponent)->HoldBreath(false);

			if (Role == ROLE_Authority)
			{
				LastBreathTimer = MinHoldBreathTimer;
				bCanHoldBreath = false;
			}
		}
	}
}

bool ASCCounselorCharacter::IsUsingPerk(TSubclassOf<USCPerkData> PerkInQuestion) const
{
	if (ASCPlayerState* PS = GetPlayerState())
	{
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			if (Perk->GetClass() == PerkInQuestion)
				return true;
		}
	}

	return false;
}

void ASCCounselorCharacter::ApplyPerks()
{
	UWorld* World = GetWorld();
	if (ASCPlayerState* PS = World ? GetPlayerState() : nullptr)
	{
		// Add Starting Items
		if (!PS->bPerksApplied && PS->GetActivePerks().Num() > 0)
		{
			// Look through perks for stat modifications and items
			float VoIPDistanceModifier = 0.f;

			for (const USCPerkData* Perk : PS->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				StrengthAttackMod.StatModifiers[(uint8)ECounselorStats::Strength] += Perk->StatModifiers[(uint8)ECounselorStats::Strength];
				StrengthAttackMod.SetDirty();

				VoIPDistanceModifier += Perk->VoIPDistanceModifier;

				// Spawn Perk items
				if (Role == ROLE_Authority)
				{
					for (const auto& ItemClass : Perk->StartingItems)
					{
						if (ItemClass)
						{
							// We already have a weapon, skip this one
							if (ItemClass->IsChildOf(ASCWeapon::StaticClass()) && GetCurrentWeapon())
							{
								continue;
							}

							// Check to see if we already have this item and if we can have more
							if (HasItemInInventory(ItemClass))
							{
								if (const ASCItem* const Item = ItemClass->GetDefaultObject<ASCItem>())
								{
									const int32 ItemMax = Item->GetInventoryMax();
									if (ItemMax > 0)
									{
										if (NumberOfItemTypeInInventory(ItemClass) >= ItemMax)
										{
											continue;
										}
									}
								}
							}

							GiveItem(ItemClass);
						}
					}
				}
			}

			if (IsValid(PS->VoiceAudioComponent))
				PS->VoiceAudioComponent->AttenuationOverrides.FalloffDistance += PS->VoiceAudioComponent->AttenuationOverrides.FalloffDistance * VoIPDistanceModifier;

			PS->bPerksApplied = true;
		}
	}
}

int32 ASCCounselorCharacter::GetNumCounselorsAroundMe(const float Radius/* = 2000.0f*/) const
{
	int32 NumCounselors = 0;
	const FVector MyLocation = GetActorLocation();
	if (const ASCGameState* GS = GetWorld()->GetGameState<ASCGameState>())
	{
		for (const ASCCharacter* Character : GS->PlayerCharacterList)
		{
			if (Character == this || !Character->IsA<ASCCounselorCharacter>())
				continue;

			if (!Character->IsAlive())
				continue;

			const float Distance = (MyLocation - Character->GetActorLocation()).Size();
			if (Distance <= Radius)
			{
				NumCounselors++;
			}
		}
	}

	return NumCounselors;
}

bool ASCCounselorCharacter::IsInRangeOfRadioPlaying()
{
	for (int32 iRadio(0); iRadio < PerkModifyingRadios.Num(); ++iRadio)
	{
		// Sometimes we get a bad pointer in this list (falls out of relevancy?), just clean them up
		if (!IsValid(PerkModifyingRadios[iRadio]))
		{
			PerkModifyingRadios.RemoveAt(iRadio--);
		}
		else if (PerkModifyingRadios[iRadio]->IsPlaying())
		{
			return true;
		}
	}

	return false;
}

#if !UE_BUILD_SHIPPING
void ASCCounselorCharacter::RegisterPerkCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Load all perk data
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName(TEXT("AssetRegistry")));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (!ConsoleManager.IsNameRegistered(TEXT("sc.AddActivePerk")))
	{
		ConsoleManager.RegisterConsoleCommand(TEXT("sc.AddActivePerk"),
											  TEXT("Adds a perk to this player's active perks"),
											  FConsoleCommandWithArgsDelegate::CreateUObject(this, &ASCCounselorCharacter::AddActivePerk),
											  ECVF_Default
		);
	}

	// Need to do this if running in the editor with -game to make sure that the assets in the following path are available
	AssetRegistry.ScanPathsSynchronous({ TEXT("/Game/Blueprints/Perks") });

	// Get all the assets in the folder
	TArray<FAssetData> PerkAssetList;
	AssetRegistry.GetAssetsByPath(FName("/Game/Blueprints/Perks"), PerkAssetList);

	// Loop through all the assets, get their default object, register the command, and add them to the perk class map
	for (const FAssetData& PerkAsset : PerkAssetList)
	{
		const FString PerkClassPath = FString::Printf(TEXT("%s_C"), *PerkAsset.ObjectPath.ToString());
		if (UClass* PerkClass = StaticLoadClass(USCPerkData::StaticClass(), nullptr, *PerkClassPath))
		{
			if (USCPerkData* PerkDefault = PerkClass->GetDefaultObject<USCPerkData>())
			{
				const FString PerkName = PerkDefault->PerkName.ToString().Replace(TEXT(" "), TEXT(""));
				PerkClassMapping.Add(*PerkName, PerkDefault);

				const FString PerkCommand = FString::Printf(TEXT("sc.AddActivePerk %s"), *PerkName);
				if (!ConsoleManager.IsNameRegistered(*PerkCommand))
				{
					ConsoleManager.RegisterConsoleCommand(*PerkCommand, *FString::Printf(TEXT("Adds %s perk to this player's active perks"), *PerkName));
				}
			}
		}
	}
}

void ASCCounselorCharacter::UnregisterPerkCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	// Unregister perk commands
	for (const auto& PerkInfo : PerkClassMapping)
	{
		if (IConsoleObject* CVar = ConsoleManager.FindConsoleObject(*FString::Printf(TEXT("sc.AddActivePerk %s"), *PerkInfo.Key)))
		{
			ConsoleManager.UnregisterConsoleObject(CVar);
		}
	}

	PerkClassMapping.Empty();
}
#endif // !UE_BUILD_SHIPPING

void ASCCounselorCharacter::AddActivePerk(const TArray<FString>& Args)
{
#if !UE_BUILD_SHIPPING
	for (const FString& PerkName : Args)
	{
		if (PerkName.IsEmpty())
			continue;

		if (ASCPlayerState* PS = GetPlayerState())
		{
			TArray<USCPerkData*> ActivePerks = PS->GetActivePerks();

			if (PerkClassMapping.Contains(PerkName))
			{
				USCPerkData* Perk = PerkClassMapping[*PerkName];
				if (ActivePerks.Contains(Perk))
				{
					ActivePerks.Remove(Perk);
				}
				else
				{
					ActivePerks.Add(Perk);

					// Give the player any items this perk has
					for (const TSubclassOf<ASCItem>& ItemClass : Perk->StartingItems)
					{
						GiveItem(ItemClass);
					}
				}
			}

			PS->SetActivePerks(ActivePerks);
		}
	}
#endif // !UE_BUILD_SHIPPING
}

bool ASCCounselorCharacter::SERVER_SetWearingSweater_Validate(const bool bIsWearingSweater)
{
	return true;
}

void ASCCounselorCharacter::SERVER_SetWearingSweater_Implementation(const bool bIsWearingSweater)
{
	bWearingSweater = bIsWearingSweater;
	OnRep_WearingSweater();
}

void ASCCounselorCharacter::OnRep_WearingSweater()
{
	if (bWearingSweater && SweaterOutfit)
		SetCounselorOutfitMesh(SweaterOutfit);
	else
		SetCounselorOutfitMesh(StandardOutfit);
}

void ASCCounselorCharacter::DeferredLoadOutfit()
{
	if (bLoadingOutfit)
		return;
	check(!IsRunningDedicatedServer());

	if (!CurrentOutfit.ClothingClass)
	{
		UE_LOG(LogCounselorClothing, Warning, TEXT("%s::DeferredLoadOutfit: No outfit set, cannot apply a null outfit."), *GetNameSafe(this));
		return;
	}

	// Are we the right class for this outfit?
	const USCClothingData* const OutfitData = CurrentOutfit.ClothingClass.GetDefaultObject();
	if (!OutfitData->Counselor.Get() || !IsA(OutfitData->Counselor.Get()))
	{
		UE_LOG(LogCounselorClothing, Warning, TEXT("%s::DeferredLoadOutfit: Outfit %s doesn't go on %s. Reverting to default."), *GetNameSafe(this), *GetNameSafe(CurrentOutfit.ClothingClass), *GetClass()->GetName());
		RevertToDefaultOutfit();
		return;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_RevertToDefaultOutfit);

	// Turn on our guard
	bLoadingOutfit = true;
	UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: Applying outfit %s to %s"), *GetNameSafe(this), *GetNameSafe(CurrentOutfit.ClothingClass), *GetClass()->GetName());

	// Combine our skin material with our alpha mask
	if (!AlphaMaskApplicationMaterial.IsValid())
	{
		UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: AlphaMaskApplicationMaterial not pre-loaded, loading synchronously."), *GetNameSafe(this));
		AlphaMaskApplicationMaterial.LoadSynchronous();
	}

	UMaterialInstance* AlphaMaterial = AlphaMaskApplicationMaterial.Get();
	UTexture2D* AlphaMask = OutfitData->SkinAlphaMask.Get();
	if (AlphaMaterial && AlphaMask)
	{
		// Make sure we have a dnyamic material instance
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(AlphaMaterial);
		if (!DynamicMaterial)
			DynamicMaterial = UMaterialInstanceDynamic::Create(AlphaMaterial, this);

		// Pass in our alpha mask
		DynamicMaterial->SetTextureParameterValue(AlphaMaskMaterialParameter, AlphaMask);

#if PLATFORM_DESKTOP
		const int32 MipBias = FMath::Clamp<int32>(3 - Scalability::GetQualityLevels().TextureQuality, 0, 3);
#else
		const int32 MipBias = 2;
#endif

		const int32 Width  = AlphaMask->GetSizeX() >> MipBias;
		const int32 Height = AlphaMask->GetSizeY() >> MipBias;

		UE_LOG(LogCounselorClothing, Verbose, TEXT("%s::DeferredLoadOutfit: Creating dynamic skin material at %dx%d"), *GetNameSafe(this), Width, Height);

		// Render our alpha material to a texture
		if (!DynamicSkinTexture)
		{
			DynamicSkinTexture = UKismetRenderingLibrary::CreateRenderTarget2D(this, Width, Height);
			DynamicSkinTexture->LODGroup = TextureGroup::TEXTUREGROUP_CharacterSpecular;
		}

		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, DynamicSkinTexture, DynamicMaterial);

		// Make sure our skin material is also dynamic
		UMaterialInstanceDynamic* DynamicSkinMaterial = GetMesh()->CreateDynamicMaterialInstance(SkinMaterialIndex, GetMesh()->GetMaterial(SkinMaterialIndex));

		// Set our dynamic skin texture on our skin material
		if (DynamicSkinMaterial)
			DynamicSkinMaterial->SetTextureParameterValue(SkinTextureMaterialParameter, DynamicSkinTexture);

		UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: Applying skin mask \"%s\""), *GetNameSafe(this), *SkinTextureMaterialParameter.ToString());
	}
	else
	{
		if (!AlphaMaterial)
		{
#if WITH_EDITOR
			FMessageLog("PIE").Warning()
				->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Couldn't load AlphaMaterial \"%s\" for counselor "), *AlphaMaskApplicationMaterial.ToSoftObjectPath().ToString()))))
				->AddToken(FUObjectToken::Create(GetClass()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(", some clipping may occur."))));
#endif
			UE_LOG(LogCounselorClothing, Error, TEXT("%s::DeferredLoadOutfit: Couldn't load AlphaMaterial \"%s\" for counselor %s, some clipping may occur."), *GetNameSafe(this), *AlphaMaskApplicationMaterial.ToSoftObjectPath().ToString(), *GetClass()->GetName());
		}

		if (!AlphaMask)
		{
#if WITH_EDITOR
			FMessageLog("PIE").Warning()
				->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Outfit ")))))
				->AddToken(FUObjectToken::Create(OutfitData->GetClass()))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" has no AlphaMask, some clipping may occur."))));
#endif
			UE_LOG(LogCounselorClothing, Error, TEXT("%s::DeferredLoadOutfit: Outfit %s has no AlphaMask, some clipping may occur."), *GetNameSafe(this), *OutfitData->GetClass()->GetName());
		}
	}

	// Set our skeletal mesh
	UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: Setting skeletal meshes"), *GetNameSafe(this));
	StandardOutfit = OutfitData->ClothingMesh.Get();
	SweaterOutfit = OutfitData->SweaterOutfitMesh.Get();
	SetCounselorOutfitMesh(StandardOutfit);

	// Apply outfit to limbs
	UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: Setting dismemberment meshes"), *GetNameSafe(this));
	FaceLimb->ApplyOutfit(&CurrentOutfit);
	TorsoLimb->ApplyOutfit(&CurrentOutfit);
	RightArmLimb->ApplyOutfit(&CurrentOutfit);
	LeftArmLimb->ApplyOutfit(&CurrentOutfit);
	RightLegLimb->ApplyOutfit(&CurrentOutfit);
	LeftLegLimb->ApplyOutfit(&CurrentOutfit);

	if (!OutfitData->FlashlightTransform.GetScale3D().IsNearlyZero())
		LightMesh->SetRelativeTransform(OutfitData->FlashlightTransform);

	if (!OutfitData->WalkieTalkieTransform.GetScale3D().IsNearlyZero())
		WalkieTalkie->SetRelativeTransform(OutfitData->WalkieTalkieTransform);

	// Outfit applied, show our character
	bAppliedOutfit = true;
	UpdateCharacterMeshVisibility();

	UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: Outfit applied for handle %d"), *GetNameSafe(this), CurrentOutfitLoadingHandle);

	CurrentOutfitLoadingHandle = INDEX_NONE;

	if (bUseMenuPose)
	{
		UE_LOG(LogCounselorClothing, Log, TEXT("%s::DeferredLoadOutfit: Playing menu pose %s for loaded outfit"), *GetNameSafe(this), *GetNameSafe(OutfitData->MenuAnimation));
		PlayAnimMontage(OutfitData->MenuAnimation);
	}

	// Turn off our guard
	bLoadingOutfit = false;
}

void ASCCounselorCharacter::CleanupOutfit()
{
	if (IsRunningDedicatedServer())
		return;

	// Hide our character so we don't show up naked
	bAppliedOutfit = false;
	UpdateCharacterMeshVisibility();

	// Remove all meshes
	SetCounselorOutfitMesh(nullptr);
	FaceLimb->ApplyOutfit(nullptr);
	TorsoLimb->ApplyOutfit(nullptr);
	RightArmLimb->ApplyOutfit(nullptr);
	LeftArmLimb->ApplyOutfit(nullptr);
	RightLegLimb->ApplyOutfit(nullptr);
	LeftLegLimb->ApplyOutfit(nullptr);

	// Clear references to outfits
	StandardOutfit = nullptr;
	SweaterOutfit = nullptr;

	// Clear our skin texture
	if (UMaterialInstanceDynamic* DynamicSkinMaterial = Cast<UMaterialInstanceDynamic>(GetMesh()->GetMaterial(SkinMaterialIndex)))
		DynamicSkinMaterial->ClearParameterValues();

	if (DynamicSkinTexture)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, DynamicSkinTexture, FLinearColor::Black);
	}

	// Reset our accessories to their default positions
	if (const ASCCounselorCharacter* const DefaultCharacterClass = GetClass()->GetDefaultObject<const ASCCounselorCharacter>())
	{
		LightMesh->SetRelativeTransform(DefaultCharacterClass->LightMesh->GetRelativeTransform());
		WalkieTalkie->SetRelativeTransform(DefaultCharacterClass->WalkieTalkie->GetRelativeTransform());
	}

	// No outfit data to unload
	if (!CurrentOutfit.ClothingClass)
		return;

	// Finally, unload data
	UWorld* World = GetWorld();
	USCGameInstance* GameInstance = World ? Cast<USCGameInstance>(World->GetGameInstance()) : nullptr;
	if (!GameInstance)
		return;

	GameInstance->GetCounselorWardrobe()->UnloadOutfit(CurrentOutfit);

	CurrentOutfit.ClothingClass = nullptr;
	CurrentOutfit.MaterialPairs.Empty();
}

void ASCCounselorCharacter::RandomOutfit()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	USCGameInstance* GameInstance = World ? Cast<USCGameInstance>(World->GetGameInstance()) : nullptr;
	USCCounselorWardrobe* Wardrobe = GameInstance ? GameInstance->GetCounselorWardrobe() : nullptr;
	if (Wardrobe)
	{
		UE_LOG(LogCounselorClothing, Verbose, TEXT("%s::RandomOutfit: New outfit!"), *GetNameSafe(this));
		SetCurrentOutfit(Wardrobe->GetRandomOutfit(GetClass(), Cast<AILLPlayerController>(GetController())));
	}
#endif
}

void ASCCounselorCharacter::SetCounselorOutfitMesh(USkeletalMesh* NewSkeletalMesh)
{
	// NOTE: Don't check if we're the same mesh because maybe the materials have changed

	// Clear our current override materials
	for (int32 iMat(0); iMat < CounselorOutfitComponent->GetNumMaterials(); ++iMat)
	{
		CounselorOutfitComponent->SetMaterial(iMat, nullptr);
	}

	// Set our new mesh
	CounselorOutfitComponent->SetSkeletalMesh(NewSkeletalMesh);

	// No clothing class, don't need to update materials
	if (!CurrentOutfit.ClothingClass)
		return;

	// Apply our materials
	const USCClothingData* const OutfitData = CurrentOutfit.ClothingClass.GetDefaultObject();
	for (const FSCClothingMaterialPair& Pair : CurrentOutfit.MaterialPairs)
	{
		const int32 MaterialIndex = OutfitData->GetMaterialIndexFromSlotIndex(NewSkeletalMesh, Pair.SlotIndex);
		if (MaterialIndex != INDEX_NONE)
		{
			if (const USCClothingSlotMaterialOption* const SlotOption = Pair.SelectedSwatch.GetDefaultObject())
			{
				CounselorOutfitComponent->SetMaterial(MaterialIndex, SlotOption->MaterialInstance.Get());
			}
		}
	}
}

void ASCCounselorCharacter::RevertToDefaultOutfit()
{
	if (ensureMsgf(DefaultCounselorOutfit, TEXT("%s has no default outfit! Fix this!"), *GetClass()->GetName()))
	{
		FSCCounselorOutfit DefaultOutfit;
		DefaultOutfit.ClothingClass = DefaultCounselorOutfit;
		SetCurrentOutfit(DefaultOutfit);
	}
}

void ASCCounselorCharacter::SetCurrentOutfit(const FSCCounselorOutfit& NewOutfit, bool bFromMenu/*=false*/)
{
	// Don't reset!
	if (bAppliedOutfit && CurrentOutfit == NewOutfit)
	{

		UE_LOG(LogCounselorClothing, Warning, TEXT("%s::SetCurrentOutfit: Attempting to reset outfit bAppliedOutfit: %s CurrentOutfit == NerOutfit: %s "), *GetNameSafe(this), bAppliedOutfit ? "True" : "False", CurrentOutfit == NewOutfit ? "True" : "False");
		return;
	}

	// Don't load clothing on a dedicated server. They like to crash.
	if (IsRunningDedicatedServer())
		return;

	// Gotta put something on!
	if (!IsValid(*NewOutfit.ClothingClass))
	{
		UE_LOG(LogCounselorClothing, Warning, TEXT("%s::SetCurrentOutfit: Called with no outfit class! Ignoring."), *GetNameSafe(this));
		return;
	}

	const USCClothingData* const ClothingData = NewOutfit.ClothingClass->GetDefaultObject<USCClothingData>();
	if (!ClothingData || !ClothingData->Counselor.Get() || !IsA(ClothingData->Counselor.Get()))
	{
		UE_LOG(LogCounselorClothing, Warning, TEXT("%s::SetCurrentOutfit: Outfit %s doesn't go on %s! Ignoring."), *GetNameSafe(this), *GetNameSafe(NewOutfit.ClothingClass), *GetClass()->GetName());
		return;
	}

	UE_LOG(LogCounselorClothing, Log, TEXT("%s::SetCurrentOutfit: Updating current outfit to %s (Previous: %s Handle: %d)"), *GetNameSafe(this), *GetNameSafe(NewOutfit.ClothingClass), *GetNameSafe(CurrentOutfit.ClothingClass), CurrentOutfitLoadingHandle);

	CleanupOutfit();

	// Set our new outfit and never go back to default
	GetWorldTimerManager().ClearTimer(TimerHandle_RevertToDefaultOutfit);
	CurrentOutfit = NewOutfit;

	bUseMenuPose = bFromMenu;

	// Make sure we don't load any old pending outfit
	USCGameInstance* GameInstance = Cast<USCGameInstance>(GetWorld()->GetGameInstance());
	USCCounselorWardrobe* Wardrobe = GameInstance->GetCounselorWardrobe();
	if (CurrentOutfitLoadingHandle != INDEX_NONE)
		Wardrobe->CancelLoadingOutfit(CurrentOutfitLoadingHandle);

	// Hide our mesh
	bAppliedOutfit = false;
	UpdateCharacterMeshVisibility();

	// Finally, start our async load of our new outfit
	FSCClothingLoaded Callback;
	Callback.BindDynamic(this, &ThisClass::DeferredLoadOutfit);
	TArray<TAssetPtr<UObject>> AdditionalDependancies;
	AdditionalDependancies.Add(AlphaMaskApplicationMaterial);
	CurrentOutfitLoadingHandle = Wardrobe->StartLoadingOutfit(CurrentOutfit, Callback, AdditionalDependancies);

	// If we get back -1, no handle was needed the outfit was already loaded and applied
	if (CurrentOutfitLoadingHandle != INDEX_NONE)
		UE_LOG(LogCounselorClothing, Log, TEXT("%s::SetCurrentOutfit: New handle %d"), *GetNameSafe(this), CurrentOutfitLoadingHandle);
}

void ASCCounselorCharacter::PlayerStateSettingsUpdated(AILLPlayerState* UpdatedPlayerState)
{
	if (ASCPlayerState* PS = GetPlayerState())
	{
		// Update clothing
		const FSCCounselorOutfit& PendingOutfit = PS->GetPickedCounselorOutfit();
		if (PendingOutfit != CurrentOutfit)
		{
			SetCurrentOutfit(PendingOutfit);
		}

		// Apply Perks
		ApplyPerks();
	}
}

void ASCCounselorCharacter::UpdateBreath(float DeltaSeconds)
{
	if (bIsHoldingBreath)
	{
		if (CurrentBreath > 0.0f)
		{
			CurrentBreath -= BreathHoldModifier.Get(this, true) * DeltaSeconds;
			CurrentBreath = FMath::Max(CurrentBreath, 0.0f);
		}
		else if (CurrentBreath <= 0.0f)
		{
			SERVER_SetHoldBreath(false);
			bPressedBreath = false;

			bCanHoldBreath = false;
		}
	}
	else if (CurrentBreath < MaxBreath || !bCanHoldBreath)
	{
		CurrentBreath += BreathRechargeModifier.Get(this) * DeltaSeconds;
		CurrentBreath = FMath::Min(CurrentBreath, MaxBreath);

		if (LastBreathTimer > 0.0f)
			LastBreathTimer -= DeltaSeconds;

		if (!bCanHoldBreath)
			bCanHoldBreath = CurrentBreath > MinHoldBreathThreshold && LastBreathTimer <= 0.0f;
	}
}

void ASCCounselorCharacter::PowerCutSFXVO()
{
	if (IsLocallyControlled() && IsIndoors())
	{
		UGameplayStatics::PlaySound2D(GetWorld(), PowerCutSFX);
	}

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_DelayVO, this, &ASCCounselorCharacter::PlayPowerCutVO, 0.8f);
}

void ASCCounselorCharacter::PlayPowerCutVO()
{
	static FName NAME_PowerCutVO(TEXT("SeeJasonKillCounselor"));
	VoiceOverComponent->PlayVoiceOver(NAME_PowerCutVO);
}

void ASCCounselorCharacter::OnRequestShowEmoteSelect()
{
	if (!CanPlayEmote())
		return;

	float ShowDelay = 0.f;
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
	{
		ShowDelay = IsUsingGamepad() ? EmoteHoldDelayController : EmoteHoldDelayKeyboard;
	}

	if (ShowDelay <= KINDA_SMALL_NUMBER)
		OnShowEmoteSelect();
	else if (GetWorld())
		GetWorldTimerManager().SetTimer(TimerHandle_ShowEmoteSelect, this, &ThisClass::OnShowEmoteSelect, ShowDelay);
}

void ASCCounselorCharacter::OnShowEmoteSelect()
{
	// Don't even bring up the hud widget if you can't play an emote
	if (!CanPlayEmote())
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
	{
		PC->OnShowEmoteSelect();
	}
}

void ASCCounselorCharacter::OnHideEmoteSelect()
{
	if (GetWorld())
		GetWorldTimerManager().ClearTimer(TimerHandle_ShowEmoteSelect);

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetCharacterController()))
	{
		PC->OnHideEmoteSelect();
	}
}

bool ASCCounselorCharacter::SERVER_PlayEmote_Validate(TSubclassOf<USCEmoteData> EmoteClass)
{
	return true;
}

void ASCCounselorCharacter::SERVER_PlayEmote_Implementation(TSubclassOf<USCEmoteData> EmoteClass)
{
	if (CanPlayEmote())
	{
		if (const USCEmoteData* const Emote = Cast<USCEmoteData>(EmoteClass->GetDefaultObject()))
		{
			UAnimMontage* EmoteMontage = GetIsFemale() ? Emote->FemaleMontage : Emote->MaleMontage;
			if (EmoteMontage)
			{
				MULTICAST_PlayEmote(EmoteMontage, Emote->bLooping);
			}
		}
	}
}

void ASCCounselorCharacter::MULTICAST_PlayEmote_Implementation(UAnimMontage* EmoteMontage, bool bLooping)
{
	CurrentEmote = EmoteMontage;
	const float MontageLength = PlayAnimMontage(CurrentEmote);
	if (MontageLength <= KINDA_SMALL_NUMBER)
	{
		CurrentEmote = nullptr;
		return;
	}

	// Non-looping animations don't allow movement. Looping animations don't allow movement until you release the stick (keyboard don't care)
	bEmoteBlocksMovement = !bLooping || IsUsingGamepad();
	bLoopingEmote = bLooping;

	// Input blocking only needs to happen on the client
	if (IsLocallyControlled() && GetWorld())
	{
		if (!bLooping)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_Emote, this, &ASCCounselorCharacter::SERVER_CancelEmote, MontageLength);
		}
		else if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(Controller))
		{
			PlayerController->SetIgnoreMoveInput(true);
			GetWorldTimerManager().SetTimer(TimerHandle_Emote, PlayerController, &ASCPlayerController::ResetIgnoreMoveInput, 0.35f);
		}
	}
}

bool ASCCounselorCharacter::SERVER_CancelEmote_Validate()
{
	return true;
}

void ASCCounselorCharacter::SERVER_CancelEmote_Implementation()
{
	MULTICAST_CancelEmote();
}

void ASCCounselorCharacter::MULTICAST_CancelEmote_Implementation()
{
	bEmoteBlocksMovement = false;
	bLoopingEmote = false;

	if (CurrentEmote)
	{
		if (GetMesh() && GetMesh()->GetAnimInstance())
		{
			GetMesh()->GetAnimInstance()->Montage_Stop(0.25f, CurrentEmote);
		}

		CurrentEmote = nullptr;
	}
}

bool ASCCounselorCharacter::CanPlayEmote() const
{
	if (IsInSpecialMove())
		return false;

	if (IsAttacking())
		return false;

	// Check move state
	if (GetCharacterMovement()->MovementMode != MOVE_Walking)
		return false;

	if (bIsAiming)
		return false;

	// Already playing an emote
	if (CurrentEmote)
		return false;

	if (CurrentHidingSpot)
		return false;

	if (GetInteractableManagerComponent()->GetLockedInteractable())
		return false;

	// No emotes while using the sweater.
	if (IsUsingSweaterAbility())
		return false;

	return true;
}

bool ASCCounselorCharacter::IsUsingOfflineLogic() const
{
	if (ASCCounselorAIController* BotController = Cast<ASCCounselorAIController>(Controller))
		return BotController->IsUsingOfflineBotsLogic();

	return false;
}

bool ASCCounselorCharacter::IsPlayingConversationVO() const
{
	if (USCVoiceOverComponent_Counselor* CounselorVO = Cast<USCVoiceOverComponent_Counselor>(VoiceOverComponent))
		return CounselorVO->IsCounselorTalking();

	return false;
}

#undef LOCTEXT_NAMESPACE