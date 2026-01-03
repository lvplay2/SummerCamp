// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerCharacter.h"

// UE4
#include "Animation/AnimMontage.h"
#include "Perception/PawnSensingComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Landscape.h"
#include "MessageLog.h"
#include "OnlineKeyValuePair.h"
#include "UObjectToken.h"

// IGF
#include "IllExclusionVolume.h"
#include "ILLGameBlueprintLibrary.h"
#include "ILLInventoryComponent.h"
#include "ILLPlayerInput.h"

// SC
#include "SCBackendInventory.h"
#include "SCCabin.h"
#include "SCContextKillActor.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAIController.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCDestructableWall.h"
#include "SCDoor.h"
#include "SCDriveableVehicle.h"
#include "SCDynamicContextKill.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameMode_SPChallenges.h"
#include "SCGameState_Hunt.h"
#include "SCGameState_Sandbox.h"
#include "SCGameState_SPChallenges.h"
#include "SCGun.h"
#include "SCHidingSpot.h"
#include "SCIndoorComponent.h"
#include "SCInGameHUD.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerAnimInstance.h"
#include "SCKillerMorphPoint.h"
#include "SCKillerMask.h"
#include "SCLocalPlayer.h"
#include "SCMaterialOverride.h"
#include "SCMinimapIconComponent.h"
#include "SCMusicComponent_Killer.h"
#include "SCNoiseMaker_Projectile.h"
#include "SCPerkData.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCPocketKnife.h"
#include "SCRadio.h"
#include "SCRepairComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSpecialMoveComponent.h"
#include "SCThrowingKnifeProjectile.h"
#include "SCTrap.h"
#include "SCVehicleSeatComponent.h"
#include "SCVoiceOverComponent_Killer.h"
#include "SCWeapon.h"
#include "SCWindow.h"
#include "SCWorldSettings.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCKillerCharacter"

TAutoConsoleVariable<int32> CVarDebugGrab(TEXT("sc.DebugGrab"), 0,
										  TEXT("Displays debug information for grabbing.\n")
										  TEXT(" 0: None\n")
										  TEXT(" 1: Grab attempt info\n")
										  TEXT(" 2: Grab interp info\n"));

TAutoConsoleVariable<int32> CVarDebugContextKill(TEXT("sc.DebugContextKill"), 0,
												 TEXT("Displays debug information for context kills.\n")
												 TEXT(" 0: None\n")
												 TEXT(" 1: Context kill info\n"));

TAutoConsoleVariable<int32> CVarDebugStunArea(TEXT("sc.DebugFallStunArea"), 0,
											  TEXT("Draws the stun area to be checked for fall back stuns")
											  TEXT(" 0: Disabled\n")
											  TEXT(" 1: Enabled\n"));

TAutoConsoleVariable<int32> CVarDebugKillerStealth(TEXT("sc.DebugKillerStealth"), 0,
												   TEXT("Draws debug information for stealth.")
												   TEXT(" 0: Disabled\n")
												   TEXT(" 1: Enabled\n"));

TAutoConsoleVariable<int32> CVarDebugPoliceStun(TEXT("sc.DebugPoliceStun"), 0,
												TEXT("Draws debug information for police stun and jason placement. iterations start as red and turn blue along the trace path.")
												TEXT(" 0: Disabled\n")
												TEXT(" 1: Enabled\n"));

extern TAutoConsoleVariable<int32> CVarDebugStuck;

static const FName NAME_ShowWeaponBlood(TEXT("ShowWeaponBlood"));
static const FName NAME_TrapFloorTag(TEXT("Trappable"));

ASCKillerCharacter::ASCKillerCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)

// Grab Kill
, GrabbedCapsuleSize(50.f)
, GrabbedMeshOffset(-30.f)

, NearCounselorMusicMaxVolumeDistance(300.0f)

// Combat
, StunDamageModifier(0.2f)
, DamageCharacterModifier(1.f)
, DamageWorldModifier(1.f)
, AttackDelay(1.f)


, MaskPopOffThreshold(30.0f)
, MurderHealthBonus(10.0f)
, bMaskOn(true)
, AOELightRadius(500.f)
, MaxGrabDistance(200.f)
, MinGrabDistance(100.f)
, MinGrabAngle(45.f)
, CombatFOV(90.f)
, UnderwaterSpeed(400.f)
, UnderwaterFullSpeedDepth(275.f)
, UnderwaterKillDepth(290.f)
, NoiseDetectionMinTime(3.f)
, NoiseDetectionMaxTime(8.f)
, NoiseLevelThreshold(2.f)
, NoiseDistanceThreshold(50000.f)
, SoundBlipMaxOffsetRadius(2500.f)
, SoundLockonTime(10.f)
, MinBlipFrequencyTime(0.5f)
, MaxBlipFrequencyTime(2.f)
, KnifeAttachSocket(TEXT("handGrabSocket"))
, MaxAttackInteractCounselorDistance(200.f)
, MaxAttackInteractAngle(45.f)
{
	bReplicates = true;
	bReplicateMovement = true;

	PoliceStunIterationAngle = 4.f;

	// Keep in sync with ASCKillerPlayerStart's capsule size!
	GetCapsuleComponent()->InitCapsuleSize(34.f, 100.f);

	static FName NAME_JasonSkeletalMask(TEXT("JasonSkeletalMask"));
	JasonSkeletalMask = CreateDefaultSubobject<USkeletalMeshComponent>(NAME_JasonSkeletalMask);
	JasonSkeletalMask->SetupAttachment(GetMesh());

	static FName NAME_UnderwaterVisualRoot(TEXT("UnderwaterVisualRoot"));
	UnderwaterVisualRoot = CreateDefaultSubobject<USceneComponent>(NAME_UnderwaterVisualRoot);
	UnderwaterVisualRoot->SetupAttachment(RootComponent);

	static FName NAME_VoiceOverComponent(TEXT("VoiceOverComponent_Killer"));
	VoiceOverComponent = CreateDefaultSubobject<USCVoiceOverComponent_Killer>(NAME_VoiceOverComponent);

	static FName NAME_UnderwaterInteractComponent(TEXT("UnderwaterInteractComponent"));
	UnderwaterInteractComponent = CreateDefaultSubobject<USCInteractComponent>(NAME_UnderwaterInteractComponent);
	UnderwaterInteractComponent->SetupAttachment(UnderwaterVisualRoot);

	static FName NAME_MusicComponent(TEXT("MusicComponent_Killer"));
	MusicComponent = CreateDefaultSubobject<USCMusicComponent_Killer>(NAME_MusicComponent);

	TeleportEffectAmount = 1.0f;
	TeleportScreenTime = 1.0f;
	MaxTeleportViewDistance = 2000.0f;
	FringeIntensity = 0.0f;
	StunnedDistortionDelta = 0.0f;
	StunnedAlphaDelta = 0.0f;

	TrapPlacementDistance = 75.0f;
	TrapRadius = 45.0f;
	StalkActiveTime = 20.0f;
	RageUnlockTime = 900.0f;

	IdleStalkModifier = 0.1f;

	InitialTrapCount = 5;

	ThrowingKnifeCooldown = 3.5f;
	ThrowingKnifeCooldownTimer = 0.0f;

	MinBlipRadius = 1000.0f;

	bSoundBlipsVisible = true;

	NearCounselorMusicDistance = 4000.0f;

	GrabModifier = 1.0f;

	TeleportScreenTimer = 0.0f;
	bHasTeleportEffectActive = false;

	MinimumHealth = 1.0f;

	// set max values for quantized floats using reflection class default values
	const ASCKillerCharacter* const DefaultClass = GetClass()->GetDefaultObject<ASCKillerCharacter>();
	float MaxUnlockTime = 0.0f;
	for (uint8 i(0); i < (uint8) EKillerAbilities::MAX; ++i)
	{
		if (MaxUnlockTime < DefaultClass->AbilityUnlockTime[i])
			MaxUnlockTime = DefaultClass->AbilityUnlockTime[i];
	}

	AbilityUnlockTimer.SetMaxValue(MaxUnlockTime);

	AbilityRechargeTimer[(uint8) EKillerAbilities::Morph].SetMaxValue(DefaultClass->AbilityRechargeTime[(uint8) EKillerAbilities::Morph]);
	AbilityRechargeTimer[(uint8) EKillerAbilities::Sense].SetMaxValue(DefaultClass->AbilityRechargeTime[(uint8) EKillerAbilities::Sense]);
	AbilityRechargeTimer[(uint8) EKillerAbilities::Stalk].SetMaxValue(DefaultClass->AbilityRechargeTime[(uint8) EKillerAbilities::Stalk]);
	AbilityRechargeTimer[(uint8) EKillerAbilities::Shift].SetMaxValue(DefaultClass->AbilityRechargeTime[(uint8) EKillerAbilities::Shift]);

	ActiveSenseTimer.SetMaxValue(DefaultClass->SenseDuration);
	RageUnlockTimer.SetMaxValue(DefaultClass->RageUnlockTime);
	StalkActiveTimer.SetMaxValue(DefaultClass->StalkActiveTime);

	StealthNoiseScale = .2f;
	StealthRadiusScale = .4f;
	AntiStealthNoiseScale = 1.5f;
	AntiStealthRadiusScale = 1.1f;

	AntiStealthHeardInfluence = .5f;
	StealthHeardDecayRate = .7f;
	StealthHeardOccurrenceLimit = 32.f;

	AntiStealthIndirectlySensedInfluence = .4f;
	StealtIndirectlySensedDecayRate = .3f;
	StealthIndirectlySensedOccurrenceLimit = 8.f;

	AntiStealthSeenInfluence = .7f;
	StealtSeenDecayRate = .2f;
	StealthSeenOccurrenceLimit = 8.f;

	AntiStealthMadeNoiseInfluence = .3f;
	StealthMadeNoiseDecayRate = .7f;
	StealthMadeNoiseOccurrenceLimit = 12.f;

	GrabKillClasses[0] = nullptr;
	GrabKillClasses[1] = nullptr;
	GrabKillClasses[2] = nullptr;
	GrabKillClasses[3] = nullptr;
}

void ASCKillerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCKillerCharacter, AbilityUnlocked);
	DOREPLIFETIME(ASCKillerCharacter, AbilityUnlockTimer);
	DOREPLIFETIME(ASCKillerCharacter, AbilityRechargeTimer);

	DOREPLIFETIME(ASCKillerCharacter, bIsSenseActive);
	DOREPLIFETIME(ASCKillerCharacter, ActiveSenseTimer);

	DOREPLIFETIME(ASCKillerCharacter, bIsRageActive);
	DOREPLIFETIME(ASCKillerCharacter, RageUnlockTimer);

	DOREPLIFETIME(ASCKillerCharacter, bUnderWater);
	DOREPLIFETIME(ASCKillerCharacter, NumKnives);
	DOREPLIFETIME(ASCKillerCharacter, GrabKills);
	DOREPLIFETIME(ASCKillerCharacter, bMaskOn);

	DOREPLIFETIME(ASCKillerCharacter, bIsShiftActive);

	DOREPLIFETIME(ASCKillerCharacter, EnableTeleportEffect);
	DOREPLIFETIME(ASCKillerCharacter, bGrabKillsActive);

	DOREPLIFETIME_CONDITION(ASCKillerCharacter, GrabbedCounselor, COND_InitialOnly);

	DOREPLIFETIME(ASCKillerCharacter, bIsStalkActive);
	DOREPLIFETIME(ASCKillerCharacter, StalkActiveTimer);

	DOREPLIFETIME(ASCKillerCharacter, TrapCount);

	DOREPLIFETIME_CONDITION(ASCKillerCharacter, bWantsToBreakDown, COND_SkipOwner);

	DOREPLIFETIME(ASCKillerCharacter, MeshSkinOverride);
	DOREPLIFETIME(ASCKillerCharacter, MaskSkinOverride);
	DOREPLIFETIME(ASCKillerCharacter, NearCounselorOverrideMusic);
}

void ASCKillerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (VoiceOverObjectClass)
	{
		VoiceOverComponent->InitVoiceOverComponent(VoiceOverObjectClass);
	}

	const auto CreateDynamicMaterials = [](USkeletalMeshComponent* MeshComponent)
	{
		if (IsValid(MeshComponent))
		{
			TArray<UMaterialInterface*> Materials = MeshComponent->GetMaterials();
			for (int32 i(0); i < Materials.Num(); ++i)
			{
				MeshComponent->CreateDynamicMaterialInstance(i, Materials[i]);
			}
		}
	};

	if (!IsRunningDedicatedServer())
	{
		CreateDynamicMaterials(GetMesh());
		CreateDynamicMaterials(JasonSkeletalMask);
	}

	// Used to reset capsule size when dropping a counselor
	DefaultCapsuleSize = GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	// Set our traps to whatever is assigned in BP
	if (Role == ROLE_Authority)
		TrapCount = InitialTrapCount;

	// Populate the custom ability unlock array for later (OnReceivedPlayerState() will set these values to the user's order).
	LoadDefaultAbilityUnlockOrder();
}

void ASCKillerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Create an effect for the surface of the water so we can see where we are
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Instigator;
	UnderwaterVisuals = GetWorld()->SpawnActor<AActor>(UnderwaterVisualClass, SpawnParams);

	if (UnderwaterVisuals != nullptr)
	{
		UnderwaterVisuals->SetActorHiddenInGame(true);
		UnderwaterVisuals->SetActorEnableCollision(ECollisionEnabled::NoCollision);
		UnderwaterVisuals->AttachToComponent(UnderwaterVisualRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);
		if (UStaticMeshComponent* WaterMesh = Cast<UStaticMeshComponent>(UnderwaterVisuals->GetComponentByClass(UStaticMeshComponent::StaticClass())))
		{
			if (!IsRunningDedicatedServer())
			{
				UnderwaterVisualsMaterial = WaterMesh->CreateDynamicMaterialInstance(0, WaterMesh->GetMaterial(0));
				UnderwaterVisualsMaterial->SetScalarParameterValue(TEXT("Intensity"), IsLocallyControlled() ? 2.f : 0.5f);
			}
		}
	}

	if (UnderwaterInteractComponent)
	{
		UnderwaterInteractComponent->OnCanInteractWith.BindDynamic(this, &ASCKillerCharacter::CanInteractUnderwater);
		UnderwaterInteractComponent->OnInteract.AddDynamic(this, &ASCKillerCharacter::OnInteractUnderwater);
		UnderwaterInteractComponent->bIsEnabled = false;
	}

	// Make sure the begin play for the component doesn't override us
	const bool bLocallyControlled = IsLocallyControlled() && IsPlayerControlled();
	MinimapIconComponent->SetVisibility(bLocallyControlled);
	MinimapIconComponent->bStartVisible = bLocallyControlled;

	if (!TeleportScreenEffectInst)
		TeleportScreenEffectInst = UMaterialInstanceDynamic::Create(TeleportScreenEffect, this);

#if WITH_EDITOR
	if (GetWorld()->IsPlayInEditor())
	{
		if (Role == ROLE_Authority)
		{
			NumKnives = 4;
			TrapCount = InitialTrapCount;
		}
	}
#endif

	if (TrapCount > 0)
	{
		if (!GetWorldTimerManager().IsTimerActive(TimerHandle_TerrainTest))
			GetWorldTimerManager().SetTimer(TimerHandle_TerrainTest, this, &ASCKillerCharacter::UpdateTrapPlacementTrace, 0.2f, true);
	}
}

void ASCKillerCharacter::OnReceivedController()
{
	Super::OnReceivedController();

	if (IsLocallyControlled())
	{
		// Make sure the begin play for the component doesn't override us
		MinimapIconComponent->SetVisibility(true);
		MinimapIconComponent->bStartVisible = true;

		if (UnderwaterVisualClass && !UnderwaterVisuals)
		{
			if (UnderwaterVisualsMaterial)
				UnderwaterVisualsMaterial->SetScalarParameterValue(TEXT("Intensity"), 2.f);
		}

		if (APostProcessVolume* PPV = GetPostProcessVolume())
		{
			if (!AOELightScreenEffectInst)
			{
				AOELightScreenEffectInst = UMaterialInstanceDynamic::Create(AOELightScreenEffect, this);

				if (AOELightScreenEffectInst)
				{
					PPV->Settings.AddBlendable(AOELightScreenEffectInst, 1.0f);
					AOELightScreenEffectInst->SetScalarParameterValue("Radius", 0.0f);
				}
			}

			if (!ShiftScreenEffectInst)
				ShiftScreenEffectInst = UMaterialInstanceDynamic::Create(ShiftScreenEffect, this);

			if (!SenseCounselorMaterialInstance)
				SenseCounselorMaterialInstance = UMaterialInstanceDynamic::Create(SenseCounselorMaterial, this);

			if (!SenseCabinMaterialInstance)
				SenseCabinMaterialInstance = UMaterialInstanceDynamic::Create(SenseCabinMaterial, this);

			if (!StunScreenEffectMaterialInst)
			{
				StunScreenEffectMaterialInst = UMaterialInstanceDynamic::Create(StunScreenEffectMaterial, this);

				if (StunScreenEffectMaterialInst)
					PPV->Settings.AddBlendable(StunScreenEffectMaterialInst, 1.0f);

				PPV->Settings.bOverride_VignetteIntensity = true;
				PPV->Settings.VignetteIntensity = 0.0f;
				ExposureValue = 1.2f;
			}
		}
	}

	if (Role == ROLE_Authority)
	{
		// If we're in single player we want all the abilities unlocked from the start.
		if (ASCGameState_SPChallenges* GS = Cast<ASCGameState_SPChallenges>(GetWorld()->GetGameState()))
		{
			// the hud has to already be initialized before you activate all of the abilites or the hud
			// ability widget wont update correctly.
			FTimerHandle tempHandle;
			GetWorldTimerManager().SetTimer(tempHandle, this, &ASCKillerCharacter::FillAbilities, 3.0f, false);
		}
	}
}

void ASCKillerCharacter::DrawDebug()
{
#if !UE_BUILD_SHIPPING
	Super::DrawDebug();

	// Context kills
	if (CVarDebugContextKill.GetValueOnGameThread() >= 1 && IsLocallyControlled() && IsInContextKill())
	{
		const USceneComponent* CounselorRoot = GrabbedCounselor ? GrabbedCounselor->GetRootComponent() : nullptr;
		if (CounselorRoot)
		{
			const USceneComponent* Parent = CounselorRoot->GetAttachParent();
			const FString AttachInfo = Parent ? FString::Printf(TEXT("%s:%s:%s"), *Parent->GetOwner()->GetName(), *Parent->GetName(), *CounselorRoot->GetAttachSocketName().ToString()) : TEXT("Unattached");
			DrawDebugString(GetWorld(), FVector(-50.f, 0.f, 50.f), FString::Printf(TEXT("%s attached to %s"), *GrabbedCounselor->GetName(), *AttachInfo), GrabbedCounselor, FColor::Cyan, 0.f, false);
			DrawDebugSphere(GetWorld(), GetMesh()->GetSocketLocation(CharacterAttachmentSocket), 16.f, 8, FColor::Cyan, false, 0.f);

			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			FAnimMontageInstance* MontInst = AnimInstance->GetActiveMontageInstance();
			if (MontInst)
			{
				UAnimMontage* Mont = MontInst->Montage;
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Cyan,
					FString::Printf(TEXT("Playing %s PR:%.3f"),
						*Mont->GetName(),
						MontInst->GetPlayRate()
					), false);
			}
		}
	}

	// Output weapon stats
	if (CVarDebugCombat.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("DamageCharacterModifier: %.2f"), DamageCharacterModifier), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("DamageWorldModifier: %.2f"), DamageWorldModifier), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("AttackDelay: %.2f"), AttackDelay), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("StunDamageModifier: %.2f"), StunDamageModifier), false);

		if (ASCWeapon* Weapon = GetCurrentWeapon())
		{
			const FWeaponStats& Stats = Weapon->WeaponStats;
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("%s Stats"), *Weapon->GetName()), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Base Damage: %.2f"), Stats.Damage), false);
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Stun: %d%%  Stuck: %d%%"), Stats.StunChance, Stats.StuckChance), false);
		}

		if (BreakDownActor)
		{
			if (ASCDoor* BreakDownDoor = Cast<ASCDoor>(BreakDownActor))
			{
				DrawDebugString(GetWorld(), FVector::UpVector * 50.f, FString::Printf(TEXT("Door Health: %d / %d"), BreakDownDoor->Health, BreakDownDoor->BaseDoorHealth), BreakDownDoor, FColor::Orange, 0.f);
			}
			else if (ASCDestructableWall* BreakDownWall = Cast<ASCDestructableWall>(BreakDownActor))
			{
				DrawDebugString(GetWorld(), FVector::UpVector * 50.f, FString::Printf(TEXT("Wall Health: %.2f"), BreakDownWall->GetHealth()), BreakDownDoor, FColor::Orange, 0.f);
			}
		}
	}

	// Draw stealth debug
	if (CVarDebugKillerStealth.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("StealthRatio: %3.2f"), StealthRatio), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("StealthHeardAccumulator: %3.2f (decay: %3.2f, limit: %3.2f)"), StealthHeardAccumulator, StealthHeardDecayRate, StealthHeardOccurrenceLimit), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("StealthIndirectlySensedAccumulator: %3.2f (decay: %3.2f, limit: %3.2f)"), StealthIndirectlySensedAccumulator, StealtIndirectlySensedDecayRate, StealthIndirectlySensedOccurrenceLimit), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("StealthSeenAccumulator: %3.2f (decay: %3.2f, limit: %3.2f)"), StealthSeenAccumulator, StealtSeenDecayRate, StealthSeenOccurrenceLimit), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("StealthMadeNoiseAccumulator: %3.2f (decay: %3.2f, limit: %3.2f)"), StealthMadeNoiseAccumulator, StealthMadeNoiseDecayRate, StealthMadeNoiseOccurrenceLimit), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Stealth Difficulty Multiplier: %3.2f"), CalcDifficultyStealth()), false);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Orange, FString::Printf(TEXT("Stealth Noise Multiplier: %3.2f"), CalcStealthNoise()), false);
	}

	// Draw stun area
	if (CVarDebugStunArea.GetValueOnGameThread() > 0 && IsLocallyControlled())
	{
		const FCollisionShape StunCheckArea = FCollisionShape::MakeBox(StunCheckAreaSize);
		DrawDebugBox(GetWorld(), GetActorLocation() + (GetActorForwardVector() * StunCheckArea.Box.HalfExtentX * -1), StunCheckAreaSize, FQuat(GetActorRotation()), FColor::Red, false, 10.0f, 1, 1.f);
	}
#endif //!UE_BUILD_SHIPPING
}

void ASCKillerCharacter::DrawDebugStuck(float& ZPos)
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

	if (BreakDownActor)
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Trying to %sbreak down %s"), bWantsToBreakDown ? TEXT("") : TEXT("abort "), *BreakDownActor->GetName()));
	}

	if (bThrowingKnife)
	{
		DisplayMessageFunc(FColor::Red, FString::Printf(TEXT("Trying to throw %s"), *GetNameSafe(CurrentThrowable)));
	}
#endif //!UE_BUILD_SHIPPING
}

void ASCKillerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_GrabKill);
		GetWorldTimerManager().ClearTimer(TimerHandle_Teleport);
		GetWorldTimerManager().ClearTimer(TimerHandle_ShiftMusicPitchDelayTime);
		GetWorldTimerManager().ClearTimer(TimerHandle_TerrainTest);
		GetWorldTimerManager().ClearTimer(TimerHandle_PlaceTrap);
	}
}

UTexture2D* ASCKillerCharacter::GetGrabKillIcon(int32 RequestedKill, bool HUDIcon /* = true*/)
{
	if (RequestedKill >= 0 && RequestedKill < GRAB_KILLS_MAX)
	{
		if (GrabKills[RequestedKill] == nullptr)
			return nullptr;

		if (HUDIcon)
			return GrabKills[RequestedKill]->GetContextKillHUDIcon();

		return GrabKills[RequestedKill]->GetContextKillMenuIcon();
	}

	return nullptr;
}

void ASCKillerCharacter::SetupPlayerInputComponent(class UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	InInputComponent->BindAction(TEXT("KLR_FastWalk_PC"), IE_Pressed, this, &ThisClass::OnStartFastWalk);
	InInputComponent->BindAction(TEXT("KLR_FastWalk_PC"), IE_Released, this, &ThisClass::OnStopFastWalk);
	InInputComponent->BindAction(TEXT("KLR_FastWalk_CTRL"), IE_Pressed, this, &ThisClass::OnToggleFastWalk);

	InInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Pressed, this, &ThisClass::OnAttack);
	InInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Released, this, &ThisClass::OnStopAttack);
	InInputComponent->BindAction(TEXT("KLR_KnifeThrow"), IE_Pressed, this, &ThisClass::PressKnifeThrow);

	InInputComponent->BindAction(TEXT("KLR_Grab"), IE_Pressed, this, &ThisClass::BeginGrab);

	InInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ThisClass::OnMapToggle);

	InInputComponent->BindAction(TEXT("KLR_Sense_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust01);
	InInputComponent->BindAction(TEXT("KLR_Morph_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust02);
	InInputComponent->BindAction(TEXT("KLR_Shift_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust03);
	InInputComponent->BindAction(TEXT("KLR_Stalk_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust04);
	InInputComponent->BindAction(TEXT("KLR_EnablePowers"), IE_Pressed, this, &ThisClass::OnEnablePowersPressed);

	InInputComponent->BindAction(TEXT("KLR_KillMusic"), IE_Pressed, this, &ThisClass::KillJasonMusic);
	InInputComponent->BindAction(TEXT("KLR_PlaceTrap"), IE_Pressed, this, &ThisClass::AttemptPlaceTrap);

	InInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::OnShowScoreboard);
	InInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::OnHideScoreboard);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Add bindings to parent class input components
	if (CombatStanceInputComponent)
	{
		CombatStanceInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Pressed, this, &ThisClass::OnAttack);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Released, this, &ThisClass::OnStopAttack);

		CombatStanceInputComponent->BindAction(TEXT("KLR_Grab"), IE_Pressed, this, &ThisClass::BeginGrab);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_KLR_Block"), IE_Pressed, this, &ThisClass::OnStartBlock);
		CombatStanceInputComponent->BindAction(TEXT("CMBT_KLR_Block"), IE_Released, this, &ThisClass::OnEndBlock);

		CombatStanceInputComponent->BindAction(TEXT("KLR_FastWalk_PC"), IE_Pressed, this, &ThisClass::OnStartFastWalk);
		CombatStanceInputComponent->BindAction(TEXT("KLR_FastWalk_PC"), IE_Released, this, &ThisClass::OnStopFastWalk);
		CombatStanceInputComponent->BindAction(TEXT("KLR_FastWalk_CTRL"), IE_Pressed, this, &ThisClass::OnToggleFastWalk);

		CombatStanceInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ThisClass::OnMapToggle);
	}

	if (WiggleInputComponent)
	{
		WiggleInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ThisClass::OnMapToggle);
		WiggleInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::OnShowScoreboard);
		WiggleInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::OnHideScoreboard);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (!AbilitiesInputComponent)
	{
		AbilitiesInputComponent = NewObject<UInputComponent>(Controller, TEXT("AbilitiesInput"));
		AbilitiesInputComponent->bBlockInput = true;
	}

	AbilitiesInputComponent->ClearBindingValues();
	AbilitiesInputComponent->ClearActionBindings();
	AbilitiesInputComponent->BindAxis(TEXT("GLB_Forward_PC"), this, &ThisClass::OnForwardPC);
	AbilitiesInputComponent->BindAxis(TEXT("GLB_Forward_CTRL"), this, &ThisClass::OnForwardController);
	AbilitiesInputComponent->BindAxis(TEXT("GLB_Right_PC"), this, &ThisClass::OnRightPC);
	AbilitiesInputComponent->BindAxis(TEXT("GLB_Right_CTRL"), this, &ThisClass::OnRightController);
	AbilitiesInputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::AddControllerPitchInput);
	AbilitiesInputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::AddControllerYawInput);

	AbilitiesInputComponent->BindAction(TEXT("KLR_Sense_CTRL"), IE_Pressed, this, &ThisClass::ActivateBloodlust01);
	AbilitiesInputComponent->BindAction(TEXT("KLR_Morph_CTRL"), IE_Pressed, this, &ThisClass::ActivateBloodlust02);
	AbilitiesInputComponent->BindAction(TEXT("KLR_Shift_CTRL"), IE_Pressed, this, &ThisClass::ActivateBloodlust03);
	AbilitiesInputComponent->BindAction(TEXT("KLR_Stalk_CTRL"), IE_Pressed, this, &ThisClass::ActivateBloodlust04);
	AbilitiesInputComponent->BindAction(TEXT("KLR_EnablePowers"), IE_Released, this, &ThisClass::OnEnablePowersReleased);

	if (!MorphInputComponent)
	{
		MorphInputComponent = NewObject<UInputComponent>(Controller, TEXT("MorphInput"));
		MorphInputComponent->bBlockInput = true;
	}

	MorphInputComponent->ClearBindingValues();
	MorphInputComponent->ClearActionBindings();
	MorphInputComponent->BindAction(TEXT("KLR_Morph_Activate"), IE_Pressed, this, &ThisClass::OnAttemptMorph);
	MorphInputComponent->BindAction(TEXT("KLR_Morph_Cancel"), IE_Pressed, this, &ThisClass::CancelMorph);
	MorphInputComponent->BindAction(TEXT("KLR_EnablePowers"), IE_Pressed, this, &ThisClass::OnEnablePowersPressed);

	MorphInputComponent->BindAxis(TEXT("KLR_Morph_CursorUp"), this, &ThisClass::MorphCursorUp);
	MorphInputComponent->BindAxis(TEXT("KLR_Morph_CursorRight"), this, &ThisClass::MorphCursorRight);

	if (!ShiftInputComponent)
	{
		ShiftInputComponent = NewObject<UInputComponent>(Controller, TEXT("ShiftInput"));
		ShiftInputComponent->bBlockInput = true;
	}

	ShiftInputComponent->ClearBindingValues();
	ShiftInputComponent->ClearActionBindings();
	ShiftInputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::AddControllerPitchInput);
	ShiftInputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::AddControllerYawInput);

	ShiftInputComponent->BindAction(TEXT("KLR_Sense_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust01);
	ShiftInputComponent->BindAction(TEXT("KLR_Morph_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust02);
	ShiftInputComponent->BindAction(TEXT("KLR_Shift_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust03);
	ShiftInputComponent->BindAction(TEXT("KLR_Stalk_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust04);

	ShiftInputComponent->BindAction(TEXT("KLR_EnablePowers"), IE_Pressed, this, &ThisClass::OnEnablePowersPressed);
	ShiftInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Pressed, this, &ThisClass::CancelShift);
	ShiftInputComponent->BindAction(TEXT("KLR_Grab"), IE_Pressed, this, &ThisClass::CancelShift);

	if (!GrabInputComponent)
	{
		GrabInputComponent = NewObject<UInputComponent>(Controller, TEXT("GrabInput"));
		GrabInputComponent->bBlockInput = true;
	}

	GrabInputComponent->ClearBindingValues();
	GrabInputComponent->ClearActionBindings();
	GrabInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Pressed, this, &ThisClass::OnAttack);
	GrabInputComponent->BindAction(TEXT("CMBT_KLR_Attack"), IE_Released, this, &ThisClass::OnStopAttack);
	GrabInputComponent->BindAction(TEXT("KLR_Grab"), IE_Pressed, this, &ThisClass::BeginGrab);
	GrabInputComponent->BindAction(TEXT("CMBT_KLR_GrabKillTop"), IE_Released, this, &ThisClass::UseTopGrabKill);
	GrabInputComponent->BindAction(TEXT("CMBT_KLR_GrabKillRight"), IE_Released, this, &ThisClass::UseRightGrabKill);
	GrabInputComponent->BindAction(TEXT("CMBT_KLR_GrabKillBottom"), IE_Released, this, &ThisClass::UseBottomGrabKill);
	GrabInputComponent->BindAction(TEXT("CMBT_KLR_GrabKillLeft"), IE_Released, this, &ThisClass::UseLeftGrabKill);
	GrabInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ThisClass::OnMapToggle);

	GrabInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Pressed, this, &ThisClass::OnInteract1Pressed);
	GrabInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Released, this, &ThisClass::OnInteract1Released);

	GrabInputComponent->BindAxis(TEXT("GLB_Forward_PC"), this, &ThisClass::OnForwardPC);
	GrabInputComponent->BindAxis(TEXT("GLB_Forward_CTRL"), this, &ThisClass::OnForwardController);
	GrabInputComponent->BindAxis(TEXT("GLB_Right_PC"), this, &ThisClass::OnRightPC);
	GrabInputComponent->BindAxis(TEXT("GLB_Right_CTRL"), this, &ThisClass::OnRightController);
	GrabInputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::AddControllerPitchInput);
	GrabInputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::AddControllerYawInput);
	GrabInputComponent->BindAction(TEXT("KLR_FastWalk_PC"), IE_Pressed, this, &ThisClass::OnStartFastWalk);
	GrabInputComponent->BindAction(TEXT("KLR_FastWalk_PC"), IE_Released, this, &ThisClass::OnStopFastWalk);
	GrabInputComponent->BindAction(TEXT("KLR_FastWalk_CTRL"), IE_Pressed, this, &ThisClass::OnToggleFastWalk);

	GrabInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::OnShowScoreboard);
	GrabInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::OnHideScoreboard);

	if (!UnderwaterInputComponent)
	{
		UnderwaterInputComponent = NewObject<UInputComponent>(Controller, TEXT("UnderwaterInput"));
		UnderwaterInputComponent->bBlockInput = true;
	}

	UnderwaterInputComponent->ClearBindingValues();
	UnderwaterInputComponent->ClearActionBindings();
	UnderwaterInputComponent->BindAxis(TEXT("GLB_Forward_PC"), this, &ThisClass::OnForwardPC);
	UnderwaterInputComponent->BindAxis(TEXT("GLB_Forward_CTRL"), this, &ThisClass::OnForwardController);
	UnderwaterInputComponent->BindAxis(TEXT("GLB_Right_PC"), this, &ThisClass::OnRightPC);
	UnderwaterInputComponent->BindAxis(TEXT("GLB_Right_CTRL"), this, &ThisClass::OnRightController);
	UnderwaterInputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::AddControllerYawInput);
	UnderwaterInputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::AddControllerPitchInput);
	UnderwaterInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ThisClass::OnMapToggle);

	UnderwaterInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Pressed, this, &ThisClass::OnInteract1Pressed);
	UnderwaterInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Released, this, &ThisClass::OnInteract1Released);

	UnderwaterInputComponent->BindAction(TEXT("KLR_Sense_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust01);
	UnderwaterInputComponent->BindAction(TEXT("KLR_Morph_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust02);
	UnderwaterInputComponent->BindAction(TEXT("KLR_Stalk_PC"), IE_Pressed, this, &ThisClass::ActivateBloodlust04);
	UnderwaterInputComponent->BindAction(TEXT("KLR_EnablePowers"), IE_Pressed, this, &ThisClass::OnEnablePowersPressed);
	UnderwaterInputComponent->BindAction(TEXT("KLR_EnablePowers"), IE_Released, this, &ThisClass::OnEnablePowersReleased);

	UnderwaterInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::OnShowScoreboard);
	UnderwaterInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::OnHideScoreboard);

	if (!ThrowInputComponent)
	{
		ThrowInputComponent = NewObject<UInputComponent>(Controller, TEXT("ThrowInput"));
		ThrowInputComponent->bBlockInput = true;
	}

	ThrowInputComponent->ClearBindingValues();
	ThrowInputComponent->ClearActionBindings();
	ThrowInputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::AddControllerPitchInput);
	ThrowInputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::AddControllerYawInput);
	ThrowInputComponent->BindAction(TEXT("KLR_KnifeThrow"), IE_Released, this, &ThisClass::PerformThrow);
	ThrowInputComponent->BindAction(TEXT("KLR_KnifeCancel"), IE_Pressed, this, &ThisClass::EndThrow);

	ThrowInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Pressed, this, &ThisClass::OnShowScoreboard);
	ThrowInputComponent->BindAction(TEXT("GLB_ShowScoreboard"), IE_Released, this, &ThisClass::OnHideScoreboard);
}

void ASCKillerCharacter::ForceKillCharacter(ASCCharacter* Killer, FDamageEvent DamageEvent/* = FDamageEvent()*/)
{
	AController* KillerController = Killer ? Killer->Controller : nullptr;
	Die(Health, DamageEvent, KillerController, Killer);
	GetWorldTimerManager().ClearTimer(TimerHandle_ForceKill);
}

void ASCKillerCharacter::OnInteract1Pressed()
{
	if (IsGrabKilling() || IsAttemptingGrab())
		return;

	if (IsInSpecialMove() || bIsPlacingTrap)
		return;

	Super::OnInteract1Pressed();
}

void ASCKillerCharacter::OnClientLockedInteractable(UILLInteractableComponent* LockingInteractable, const EILLInteractMethod InteractMethod, const bool bSuccess)
{
	Super::OnClientLockedInteractable(LockingInteractable, InteractMethod, bSuccess);

	// Make sure we're not attempting to lock to our current break down actor so that it doesn't take multiple hits in one swing.
	if (bSuccess && LockingInteractable->GetOwner() != BreakDownActor)
	{
		ClearBreakDownActor();
	}
}

void ASCKillerCharacter::ClearAnyInteractions()
{
	// No getting out of a context kill!
	if (IsInContextKill())
		return;

	Super::ClearAnyInteractions();

	// Cancel knife throw
	if (IsPerformingThrow())
	{
		EndThrow();
	}
}

void ASCKillerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentAttackDelay -= DeltaSeconds;

	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		if (!GS->IsMatchInProgress())
		{
			if (bIsSenseActive)
				CancelSense();

			if (bIsShiftActive)
				ForceCancelShift();

			if (bIsMorphActive)
				CancelMorph();

			if (bIsStalkActive)
				CancelStalk();

			if (IsStunned())
				EndStun();
		}
	}

	if (Role == ROLE_Authority)
	{
		UpdateAbilities(DeltaSeconds);

		UpdateStealth(DeltaSeconds);

		if (USCKillerAnimInstance* KillerAnimInst = Cast<USCKillerAnimInstance>(GetMesh() == nullptr ? nullptr : GetMesh()->GetAnimInstance()))
		{
			if (!IsInSpecialMove())
			{
				const bool WasUnderWater = bUnderWater;
				if (CurrentWaterVolume)
				{
					const float MyZ = GetActorLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					bUnderWater = MyZ <= GetWaterZ();
				}
				else
				{
					bUnderWater = false;
				}

				if (WasUnderWater != bUnderWater)
				{
					OnRep_UnderWater();
					//SetVehicleCollision(!bUnderWater);
				}
			}
		}

		if (bIsStalkActive)
		{
			StalkActiveTimer -= DeltaSeconds * (GetVelocity().Size() > 0.0f ? 1.0f : IdleStalkModifier);

			if (StalkActiveTimer <= 0.0f)
			{
				bIsStalkActive = false;
				OnRep_Stalk();
			}

			if (!bIsStalkActive)
			{
				AbilityRechargeTimer[(uint8) EKillerAbilities::Stalk] = 0.0f;
			}
		}

		// in the rare event that somehow the player manages to grab a counselor and activate shift at the same time
		// cancel shift
		if (IsShiftActive())
		{
			if (GrabbedCounselor)
				CancelShift();
		}
	}

	NativeOnUnderwaterUpdate();

	// Update grabbed counselors
	if (bIsPickingUpCounselor)
	{
		if (GrabbedCounselor == nullptr || GrabbedCounselor->IsAlive() == false)
		{
			bIsPickingUpCounselor = false;

			if (Role == ROLE_Authority)
			{
				DropCounselor();
			}
		}
		else
		{
			// Grab the weight from our asset and make sure it's clamped between 0 and 1 to negate popping issues.
			float GrabBlendWeight = 1.f;
			if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(GetMesh()->GetAnimInstance()))
			{
				static const FName NAME_GrabWeight(TEXT("GrabWeight"));
				GrabBlendWeight = FMath::Clamp(AnimInstance->GetCurveValue(NAME_GrabWeight), 0.f, 1.f);
			}

			// If we're at 1, stop interping and hard attach to the hand
			if (GrabBlendWeight >= 1.f && !IsInContextKill())
			{
				bIsPickingUpCounselor = false;
				if (!GrabbedCounselor->IsInContextKill())
					GrabbedCounselor->ReturnCameraToCharacter(2.5f, VTBlend_Cubic, 2.f);

				GrabbedCounselor->BeginGrabMinigame();

				//See if we're overlapping a trap and trigger it if we are.
				TArray<AActor*> OverlappingActors;
				GetOverlappingActors(OverlappingActors, ASCTrap::StaticClass());

				for(AActor* TrapActor : OverlappingActors)
				{
					if(ASCTrap* Trap = Cast<ASCTrap>(TrapActor))
					{
						Trap->OnBeginOverlap(nullptr, this, GetCapsuleComponent(), 0, false, FHitResult());
					}
				}
			}
			else if (GrabBlendWeight >= 0.8f && IsInContextKill())
			{
				GrabbedCounselor->AttachToKiller(true);
			}
		}
	}

	if (bIsSenseActive)
		UpdateSense(DeltaSeconds);

	if (bIsShiftActive)
	{
		if (ShiftWarmUpTime > 0.0f)
		{
			ShiftWarmUpTime -= DeltaSeconds;
		}

		if (ShiftWarmUpTime < 0.0f)
		{
			CurrentShiftTime += DeltaSeconds;
			if (CurrentShiftTime > ShiftTime)
			{
				if (IsLocallyControlled())
					SERVER_SetShift(false);
			}
		}
	}

	if (IsLocallyControlled())
	{
		if (bIsShiftActive && ShiftWarmUpTime < 0.0f)
		{
			AddMovementInput(GetActorForwardVector());
			bUseControllerRotationYaw = true;
		}

		if (ThrowingKnifeCooldownTimer > 0.0f)
			ThrowingKnifeCooldownTimer -= DeltaSeconds;

		if (!IsInContextKill() && bSoundBlipsVisible)
			UpdateSoundBlips(DeltaSeconds);

		if (AOELightScreenEffectInst)
		{
			static const FName NAME_Position(TEXT("PlayerPosition"));
			static const FName NAME_Radius(TEXT("Radius"));
			AOELightScreenEffectInst->SetVectorParameterValue(NAME_Position, GetActorLocation());
			AOELightScreenEffectInst->SetScalarParameterValue(NAME_Radius, AOELightRadius);
		}

		if (CurrentThrowable)
		{
			FRotator Rotation = FRotator::ZeroRotator;
			Rotation.Yaw = GetControlRotation().Yaw;
			// Set the rotation locally and on the server.
			SetActorRotation(Rotation);
			SERVER_SetActorRotation(Rotation.Vector());
		}

		// effects
		if (APostProcessVolume* PPV = GetPostProcessVolume())
		{
			const bool NotInKill = !IsInContextKill();
			PPV->Settings.bOverride_ColorGradingLUT = NotInKill;
			PPV->Settings.bOverride_ColorGradingIntensity = NotInKill;
			PPV->Settings.bOverride_AutoExposureMinBrightness = NotInKill;
			PPV->Settings.bOverride_AutoExposureMaxBrightness = NotInKill;
			PPV->Settings.bOverride_SceneFringeIntensity = NotInKill;
			PPV->Settings.ColorGradingIntensity = FMath::FInterpTo(PPV->Settings.ColorGradingIntensity, bColorGradeEnabled ? 1.0f : 0.0f, DeltaSeconds, 1.0f);

			PPV->Settings.AutoExposureMinBrightness = FMath::FInterpTo(PPV->Settings.AutoExposureMinBrightness, ExposureValue, DeltaSeconds, 1.0f);
			PPV->Settings.AutoExposureMaxBrightness = FMath::FInterpTo(PPV->Settings.AutoExposureMaxBrightness, ExposureValue, DeltaSeconds, 1.0f);

			PPV->Settings.SceneFringeIntensity = FMath::FInterpTo(PPV->Settings.SceneFringeIntensity, FringeIntensity, DeltaSeconds, bIsStunned ? 8.0f : 1.0f);

			StunnedDistortionDelta = FMath::FInterpTo(StunnedDistortionDelta, bIsStunned ? 4.0f : 0.0f, DeltaSeconds, bIsStunned ? 8.0f : 1.0f);
			StunnedAlphaDelta = FMath::FInterpTo(StunnedAlphaDelta, bIsStunned ? 5.0f : 0.0f, DeltaSeconds, bIsStunned ? 8.0f : 1.0f);

			StunScreenEffectMaterialInst->SetScalarParameterValue(TEXT("DistortionDelta"), StunnedDistortionDelta);
			StunScreenEffectMaterialInst->SetScalarParameterValue(TEXT("AlphaDelta"), StunnedAlphaDelta);
		}

		// Make sure cursor stays on if we have the morph map up
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (bShowMorphMap != PC->bShowMouseCursor)
			{
				PC->bShowMouseCursor = bShowMorphMap;
			}
		}
	}

	if (EnableTeleportEffect)
	{
		TeleportEffectAmount = FMath::Max(TeleportEffectAmount - DeltaSeconds, 0.0f);
	}
	else
		TeleportEffectAmount = FMath::Min(TeleportEffectAmount + DeltaSeconds, 1.0f);

	SetKillerOpacity(TeleportEffectAmount);

	if (TeleportScreenTimer > 0.0f)
	{
		TeleportScreenTimer -= DeltaSeconds;

		if (TeleportScreenEffectInst)
		{
			static const FName NAME_OffsetAmount(TEXT("OffsetAmount"));
			TeleportScreenEffectInst->SetScalarParameterValue(NAME_OffsetAmount, TeleportJitterCurve->GetFloatValue(1.0f - TeleportScreenTimer));
		}
	}

	if (TeleportScreenTimer <= 0.0f && bHasTeleportEffectActive)
	{
		ASCCounselorCharacter* Counselor = GetLocalCounselor();

		if (Counselor != nullptr)
		{
			if (APostProcessVolume* PPV = Counselor->GetPostProcessVolume())
			{
				if (TeleportScreenEffectInst)
				{
					PPV->Settings.RemoveBlendable(TeleportScreenEffectInst);
					bHasTeleportEffectActive = false;

					UGameplayStatics::ClearSoundMixModifiers(GetWorld());

					TeleportScreenTimer = 0.0f;
				}
			}
		}
	}
}

ASCCounselorCharacter* ASCKillerCharacter::GetLocalCounselor() const
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
	{
		if (PC->GetPawn())
		{
			const bool IsCar = PC->GetPawn()->IsA(ASCDriveableVehicle::StaticClass());

			return Cast<ASCCounselorCharacter>(IsCar ? Cast<ASCDriveableVehicle>(PC->GetPawn())->Driver : PC->GetPawn());
		}
	}

	return nullptr;
}

void ASCKillerCharacter::Destroyed()
{
	Super::Destroyed();

	if (EquippedItem)
	{
		EquippedItem->Destroy();
		EquippedItem = nullptr;
	}

	if (VoiceOverComponent)
	{
		VoiceOverComponent->CleanupVOComponent();
	}
}

void ASCKillerCharacter::ContextKillStarted()
{
	Super::ContextKillStarted();

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			HUD->FadeHUDWidgetOut();
		}
	}

	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		FringeIntensity = 0.0f;

		PPV->Settings.bOverride_DepthOfFieldMethod = false;
		PPV->Settings.bOverride_DepthOfFieldFocalDistance = false;
		PPV->Settings.bOverride_DepthOfFieldNearTransitionRegion = false;
		PPV->Settings.bOverride_DepthOfFieldFarTransitionRegion = false;
		PPV->Settings.bOverride_DepthOfFieldNearBlurSize = false;
		PPV->Settings.bOverride_DepthOfFieldFarBlurSize = false;
	}
}

void ASCKillerCharacter::ContextKillEnded()
{
	Super::ContextKillEnded();

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (ASCInGameHUD* HUD = PC->GetSCHUD())
		{
			HUD->FadeHUDWidgetIn();
		}
	}

	if (bDelayPoliceStun)
	{
		// un-hide our weapon
		if (ASCWeapon* CurrentWeapon = GetCurrentWeapon())
			CurrentWeapon->SetActorHiddenInGame(false);

		// Get stunned
		bDelayPoliceStun = false;
		StunnedByPolice(PendingStunInfo.DamageCauser, PendingStunInfo.PoliceEscapeForwardDir, PendingStunInfo.PoliceEscapeRadius);

		// Clear our pending stun info
		PendingStunInfo.DamageCauser = nullptr;
		PendingStunInfo.PoliceEscapeForwardDir = FVector::ZeroVector;
		PendingStunInfo.PoliceEscapeRadius = 0.f;
	}
}

bool ASCKillerCharacter::IsVisible() const
{
	return TeleportEffectAmount > 0.0f;
}

void ASCKillerCharacter::BreakGrab(bool StunWithPocketKnife/* = false*/)
{
	bPocketKnifed = StunWithPocketKnife;

	if (StunWithPocketKnife)
		BeginStun(BreakGrabPocketKnifeJasonStunClass, FSCStunData());
	else
		BeginStun(BreakGrabJasonStunClass, FSCStunData());

	BP_BreakGrab(StunWithPocketKnife);
}

void ASCKillerCharacter::OnRep_CurrentThrowable()
{
	if (CurrentThrowable)
	{
		BP_BeginThrow();
		if (USCKillerAnimInstance* KillerAnimInst = Cast<USCKillerAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			KillerAnimInst->PlayThrow();
		}

		if (IsLocallyControlled() && !bThrowingKnife)
		{
			PerformThrow();
		}
	}
	else
	{
		BP_EndThrow();
		bThrowingKnife = false;
		if (USCKillerAnimInstance* KillerAnimInst = Cast<USCKillerAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			KillerAnimInst->EndThrow();
		}

		if (IsLocallyControlled())
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			{
				if (PC->GetSCHUD())
				{
					PC->GetSCHUD()->OnHideKnifeReticle();
				}

				PC->PopInputComponent(ThrowInputComponent);

				RestoreInputSensitivity(EKeys::Gamepad_RightX);
				RestoreInputSensitivity(EKeys::Gamepad_RightY);
			}
		}
	}
}

bool ASCKillerCharacter::CanThrowKnife() const
{
	// So we have a lot of cases in which you cannot throw a knife. . .

	// How were we not checking this? We're already throwing a knife, no do it again!
	if (bThrowingKnife)
		return false;

	// YOU DON'T HAVE ANY KNIVES TO THROW, DUMMY!
	if (NumKnives <= 0)
		return false;

	// I think this means we're already throwing a knife?
	if (CurrentThrowable)
		return false;

	// We're moving to do a thing
	if (IsInSpecialMove())
		return false;

	// Shifting and throwing knives don't mix
	if (IsShiftActive())
		return false;

	// we arent even looking at the world, you cant throw knives
	if (bIsMorphActive || bIsMapEnabled)
		return false;

	// Hands are full. . .
	if (IsAttemptingGrab() || GrabbedCounselor)
		return false;

	// I feel dizzy
	if (IsStunned())
		return false;

	// Have you ever tried throwing things under water?
	if (bUnderWater)
		return false;

	// I'm busy breaking shit
	if (BreakDownActor)
		return false;

	// Swinging on a fool
	if (IsAttacking())
		return false;

	// NO ONE LOOK AT THESE INCONSPICUOUS LEAVES
	if (bIsPlacingTrap)
		return false;

	if (ThrowingKnifeCooldownTimer > 0.0f)
		return false;

	if (IsAttemptingGrab())
		return false;

	// cant throw a knife if powers are active
	if (bEnabledPowers)
		return false;

	// cant throw a knife if we're in voyeur mode.
	if (bInVoyeurMode)
		return false;

	return true;
}

TSubclassOf<ASCThrowable> ASCKillerCharacter::GetThrowingKnifeClass() const
{
	if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings()))
	{
		if (WorldSettings->JasonCustomizeData.ThrowingKinfeClass != nullptr)
			return WorldSettings->JasonCustomizeData.ThrowingKinfeClass;
	}

	return ThrowingKnifeClass;
}

void ASCKillerCharacter::PressKnifeThrow()
{
	if (!CanThrowKnife())
		return;

	bThrowingKnife = true;
	SERVER_SpawnKnife();

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->GetSCHUD())
		{
			PC->GetSCHUD()->OnShowKnifeReticle();
		}

		PC->PushInputComponent(ThrowInputComponent);

		ModifyInputSensitivity(EKeys::Gamepad_RightX, 0.35f);
		ModifyInputSensitivity(EKeys::Gamepad_RightY, 0.35f);
	}
}

void ASCKillerCharacter::AddKnives(int32 NumToAdd)
{
	if (Role == ROLE_Authority)
	{
		NumKnives += NumToAdd;
	}
}

bool ASCKillerCharacter::SERVER_SpawnKnife_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_SpawnKnife_Implementation()
{
	if (CurrentThrowable)
		return;

	FActorSpawnParameters SpawnParams = FActorSpawnParameters();
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Instigator;

	if (ASCThrowingKnifeProjectile* NewKnife = GetWorld()->SpawnActor<ASCThrowingKnifeProjectile>(GetThrowingKnifeClass(), SpawnParams))
	{
		MoveIgnoreActorAdd(NewKnife);
		NewKnife->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, KnifeAttachSocket);
		CurrentThrowable = NewKnife;
		OnRep_CurrentThrowable();
	}
}

void ASCKillerCharacter::PerformThrow()
{
	if (NumKnives <= 0)
		return;

	bThrowingKnife = false;
	if (CurrentThrowable != nullptr)
	{
		if (Role < ROLE_Authority)
		{
			SERVER_PerformThrow();
		}
		else
		{
			MULTICAST_PerformThrow();
		}
	}
}

bool ASCKillerCharacter::SERVER_PerformThrow_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_PerformThrow_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	// Make noise
	MakeStealthNoise(.5f, this);

	PerformThrow();
}

void ASCKillerCharacter::MULTICAST_PerformThrow_Implementation()
{
	BP_PerformThrow();

	if (USCKillerAnimInstance* KillerAnimInst = Cast<USCKillerAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		KillerAnimInst->PlayThrow();
	}

	GetWorldTimerManager().SetTimer(TimerHandle_ThrowKnife, this, &ASCKillerCharacter::ThrowKnife, 0.2f);
}

void ASCKillerCharacter::EndThrow()
{
	if (IsLocallyControlled())
	{
		bThrowingKnife = false;
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			if (PC->GetSCHUD())
			{
				PC->GetSCHUD()->OnHideKnifeReticle();
			}

			PC->PopInputComponent(ThrowInputComponent);

			RestoreInputSensitivity(EKeys::Gamepad_RightX);
			RestoreInputSensitivity(EKeys::Gamepad_RightY);
		}
	}

	SERVER_EndThrow();

	GetWorldTimerManager().ClearTimer(TimerHandle_EndThrow);
}

bool ASCKillerCharacter::SERVER_EndThrow_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_EndThrow_Implementation()
{
	// paranoia
	if (Role == ROLE_Authority)
	{
		if (CurrentThrowable)
		{
			if (CurrentThrowable->ThrownBy == nullptr)
			{
				CurrentThrowable->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				CurrentThrowable->Destroy();
				CurrentThrowable->MarkPendingKill();
				CurrentThrowable->SetActorHiddenInGame(true);
			}

			CurrentThrowable = nullptr;
			OnRep_CurrentThrowable();
		}
	}
}

void ASCKillerCharacter::ThrowKnife()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_ThrowKnife);

	if (CurrentThrowable)
	{
		// Register that this throwable has been thrown so that it's not auto deleted on EndThrow()
		CurrentThrowable->ThrownBy = this;
		if (IsLocallyControlled())
		{
			CurrentThrowable->Use(this);

			// Only set our cooldown if we actually throw the knife
			ThrowingKnifeCooldownTimer = ThrowingKnifeCooldown;
			GetWorldTimerManager().SetTimer(TimerHandle_EndThrow, this, &ASCKillerCharacter::EndThrow, 0.2f);
		}

		if (Role == ROLE_Authority)
		{
			MoveIgnoreActorRemove(CurrentThrowable);
			--NumKnives;
			GetWorldTimerManager().SetTimer(TimerHandle_EndThrow, this, &ASCKillerCharacter::EndThrow, 0.2f);
		}
	}
}

void ASCKillerCharacter::MULTICAST_DamageTaken_Implementation(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsLocallyControlled())
	{
		if (bIsMorphActive)
			CancelMorph();
		else if (bIsMapEnabled)
			OnMapToggle();
	}

	if (Role == ROLE_Authority && !bPocketKnifed && !IsGrabKilling() && GrabbedCounselor)
		DropCounselor();

	Super::MULTICAST_DamageTaken_Implementation(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void ASCKillerCharacter::OnAttack()
{
	// Murder
	if (GrabbedCounselor)
	{
		PerformGrabKill(1);
		return;
	}

	// Lots of ways for attack to fail. CHECK THEM ALL IN THE SAME PLACE
	if (!CanAttack())
		return;

	// DO NOT ADD MORE EARLY OUTS HERE, PUT THEM IN CanAttack() WHERE THEY BELONG

	// Break
	USCInteractComponent* BestInteractable = Cast<USCInteractComponent>(InteractableManagerComponent->GetBestInteractableComponent());
	if (BestInteractable && BestInteractable->CanAttackToInteract())
	{
		if (ASCDoor* Door = Cast<ASCDoor>(BestInteractable->GetOwner()))
		{
			if (TryStartBreakingDoor(Door))
			{
				// Lock interaction here to avoid edge-cases of the counselor locking the component before Jason has snapped 
				// to the appropriate distance from the door. 
				InteractableManagerComponent->LockInteraction(BestInteractable, EILLInteractMethod::Press);

				BreakDownActor = Door;
				SERVER_SetBreakDownActor(BreakDownActor);
				return;
			}
		}
		else if (ASCDestructableWall* Wall = Cast<ASCDestructableWall>(BestInteractable->GetOwner()))
		{
			if (TryStartBreakingWall(Wall))
			{
				InteractableManagerComponent->LockInteraction(BestInteractable, EILLInteractMethod::Press);

				BreakDownActor = Wall;
				SERVER_SetBreakDownActor(BreakDownActor);
				return;
			}
		}

		FVector KillerToComponent = (BestInteractable->GetComponentLocation() - GetActorLocation());
		KillerToComponent.Z = 0.f;
		KillerToComponent.Normalize();

		const auto DefaultFunc = [](const ASCCounselorCharacter* Counselor, USCInteractComponent* Interact, const FVector InKillerToComponent) -> bool
		{
			FVector CounselorToComponent = (Interact->GetComponentLocation() - Counselor->GetActorLocation());
			CounselorToComponent.Z = 0.f;
			CounselorToComponent.Normalize();

			if ((InKillerToComponent | CounselorToComponent) > 0.f)
				return false;

			return true;
		};

		if (ShouldInteractionTakePrecedenceOverNormalAttack(BestInteractable, DefaultFunc, BestInteractable, KillerToComponent))
		{
			OnInteract(BestInteractable, EILLInteractMethod::Press);
			return;
		}
	}

	// Don't attack if our delay isn't up
	if (CurrentAttackDelay > 0.f)
		return;

	CurrentAttackDelay = AttackDelay;

	// As a last resort I guess we can attack.
	Super::OnAttack();
}

void ASCKillerCharacter::OnStopAttack()
{
	// SAVE THE DOOR
	if (bWantsToBreakDown)
	{
		bWantsToBreakDown = false;

		if (Role != ROLE_Authority)
		{
			SERVER_AbortBreakingDown();
		}

		return;
	}

	// Done attacking
	Super::OnStopAttack();
}

void ASCKillerCharacter::UseTopGrabKill()
{
	PerformGrabKill(3);
}

void ASCKillerCharacter::UseRightGrabKill()
{
	PerformGrabKill(0);
}

void ASCKillerCharacter::UseBottomGrabKill()
{
	// This check will probably fuck something up once we can re-bind keys
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				if (USCInteractComponent* Interactable = GetInteractable())
					return;
			}
		}
	}

	PerformGrabKill(1);
}

void ASCKillerCharacter::UseLeftGrabKill()
{
	PerformGrabKill(2);
}

bool ASCKillerCharacter::CanGrabKill() const
{
	if (!bGrabKillsActive)
		return false;

	if (GrabbedCounselor)
	{
		if (GrabbedCounselor->HasItemInInventory(ASCPocketKnife::StaticClass()))
			return false;

		// no knife? murder that fool
		return true;
	}

	// no grabbed counselor, you cant kill if you dont have a victim
	return false;
}

bool ASCKillerCharacter::IsGrabKillAvailable(int32 RequestedKill) const
{
	if (!CanGrabKill())
		return false;

	if (RequestedKill < 0 || RequestedKill >= GRAB_KILLS_MAX)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Requested kill was not within available kill array."));
		return false;
	}

	// This check will probably fuck something up once we can re-bind keys
	if (RequestedKill == 1)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
			{
				if (Input->IsUsingGamepad())
				{
					if (USCInteractComponent* Interactable = GetInteractable())
						return false;
				}
			}
		}
	}

	if (GrabKills[RequestedKill] == nullptr)
		return false;

	return GrabKills[RequestedKill]->DoesKillVolumeFit(this);
}

void ASCKillerCharacter::PerformGrabKill(uint32 Kill)
{
	if (!IsGrabKillAvailable(Kill))
		return;

	if (!GrabbedCounselor)
		return;

	if (IsInSpecialMove())
		return;

	if (GetInteractableManagerComponent() && GetInteractableManagerComponent()->GetLockedInteractable() != nullptr)
		return;

	// Perform selected context kill...
	ASCDynamicContextKill* GrabKill = GrabKills[Kill];
	if (GrabKill == nullptr)
		return;

	GrabKill->ActivateKill(this, EILLInteractMethod::Press);
}

USoundBase* ASCKillerCharacter::GetNearJasonMusic() const
{
	return NearCounselorOverrideMusic != nullptr ? NearCounselorOverrideMusic : NearCounselorMusic;
}

void ASCKillerCharacter::SetKillerOpacity(float NewOpacity)
{
	const float Opacity = 1.0f - NewOpacity;
	SetOpacity(GetMesh(), Opacity);
	SetOpacity(JasonSkeletalMask, Opacity);
}

void ASCKillerCharacter::SetOpacity(USkeletalMeshComponent* MeshComponent, float NewOpacity)
{
	if (IsRunningDedicatedServer())
		return;

	static const FName AlphaDelta = TEXT("0_AlphaDelta");

	TArray<UMaterialInterface*> Materials = MeshComponent->GetMaterials();
	for (UMaterialInterface* Mat : Materials)
	{
		if (UMaterialInstanceDynamic* DynamicMat = Mat ? Cast<UMaterialInstanceDynamic>(Mat) : nullptr)
		{
			DynamicMat->SetScalarParameterValue(AlphaDelta, NewOpacity);
		}
	}
}

void ASCKillerCharacter::OnRep_UnderWater()
{
	OnUnderWater(bUnderWater);

	if (bUnderWater)
	{
		// Make the water not slow us down so much when stopping
		GetCharacterMovement()->GroundFriction = 0.f;

		// If we're aiming a knife, get rid of it.
		if (CurrentThrowable)
		{
			EndThrow();
		}
	}
	else
	{
		// PUT IT BACK
		GetCharacterMovement()->GroundFriction = 8.f;
		GetMesh()->SetHiddenInGame(false);
		GetKillerMaskMesh()->SetHiddenInGame(false);
		if (EquippedItem)
			EquippedItem->GetMesh()->SetHiddenInGame(false);
	}

	if (IsLocallyControlled())
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			if (bUnderWater)
			{
				PC->PushInputComponent(UnderwaterInputComponent);

				UGameplayStatics::PushSoundMixModifier(GetWorld(), UnderwaterSoundMix);
				UnderWaterAudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), UnderWaterCue);
				UnderWaterAudioComponent->FadeIn(1.0f, 1.0f);
			}
			else
			{
				PC->PopInputComponent(UnderwaterInputComponent);

				UGameplayStatics::PopSoundMixModifier(GetWorld(), UnderwaterSoundMix);
				if (UnderWaterAudioComponent)
				{
					UnderWaterAudioComponent->FadeOut(1.0f, 0.0f);
				}
			}
		}

		UnderwaterInteractComponent->bIsEnabled = bUnderWater;

		if (bIsShiftActive)
			SERVER_SetShift(false);

		if (!bInContextKill)
			DropCounselor();
	}

	if (UnderwaterVisuals)
		UnderwaterVisuals->SetActorHiddenInGame(!bUnderWater);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, bUnderWater ? ECollisionResponse::ECR_Ignore : ECollisionResponse::ECR_Block);

	// Remove blood from weapon
	if (bUnderWater)
	{
		ApplyBloodToWeapon(0, NAME_ShowWeaponBlood, 0.0f);
	}

	if (Role == ROLE_Authority)
	{
		if (bUnderWater && UnderwaterContextKill == nullptr)
		{
			FActorSpawnParameters SpawnParams = FActorSpawnParameters();
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Instigator;

			UnderwaterContextKill = GetWorld()->SpawnActor<ASCDynamicContextKill>(UnderwaterContextKillClass, SpawnParams);
			if (UnderwaterContextKill != nullptr)
			{
				UnderwaterContextKill->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
		}
		else if (!bUnderWater && UnderwaterContextKill != nullptr)
		{
			UnderwaterContextKill->SetLifeSpan(0.01f);
			UnderwaterContextKill = nullptr;
		}
	}
}


void ASCKillerCharacter::UpdateAbilities(float DeltaSeconds)
{
	const EKillerAbilities MaxAbilityID = GetAbilityAtPosition((uint8)EKillerAbilities::MAX - 1);

	// If the user's final ability is unlocked, there's no need to update unlock timers.
	if (!AbilityUnlocked[(uint8)MaxAbilityID])
	{
		for (uint8 i(0); i < (uint8)EKillerAbilities::MAX; ++i)
		{
			EKillerAbilities AbilityID = GetAbilityAtPosition(i);
			if (!AbilityUnlocked[(uint8)AbilityID])
			{
				AbilityUnlockTimer = FMath::Min(AbilityUnlockTimer + DeltaSeconds, AbilityUnlockTime[i]);
				if (AbilityUnlockTimer >= AbilityUnlockTime[i])
				{
					AbilityUnlocked[(uint8)AbilityID] = true;
					AbilityRechargeTimer[(uint8)AbilityID] = AbilityRechargeTime[(uint8)AbilityID];
					AbilityUnlockTimer = 0.0f;

					OnRep_AbilityUnlocked();
				}

				break;
			}
		}
	}

	// Update recharge timers for abilities that are unlocked.
	for (uint8 i(0); i < (uint8) EKillerAbilities::MAX; ++i)
	{
		EKillerAbilities AbilityID = GetAbilityAtPosition(i);

		if (!AbilityUnlocked[(uint8)AbilityID])
			continue;

		if (!IsAbilityAvailable((EKillerAbilities)AbilityID))
			AbilityRechargeTimer[(uint8)AbilityID] = FMath::Min(AbilityRechargeTimer[(uint8)AbilityID] + DeltaSeconds * (bIsRageActive ? RagedRechargeTimeModifier : 1.0f), AbilityRechargeTime[(uint8)AbilityID]);
	}

	if (RageUnlockTimer < RageUnlockTime)
	{
		RageUnlockTimer += DeltaSeconds;

		if (RageUnlockTimer >= RageUnlockTime)
		{
			ActivateRage();
		}
	}
}

void ASCKillerCharacter::LoadDefaultAbilityUnlockOrder() 
{
	UserDefinedAbilityOrder.Empty((int32)EKillerAbilities::MAX);
	UserDefinedAbilityOrder.Append({ EKillerAbilities::Morph, EKillerAbilities::Sense, EKillerAbilities::Shift, EKillerAbilities::Stalk });
}

TSubclassOf<ASCContextKillActor> ASCKillerCharacter::GetJasonKillClass() const
{
	if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings()))
	{
		if (WorldSettings->JasonCustomizeData.ContextDeathClass != nullptr)
			return WorldSettings->JasonCustomizeData.ContextDeathClass;
	}

	return JasonKillClass;
}

void ASCKillerCharacter::OnRep_AbilityUnlocked()
{
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		if (GS->HasMatchEnded())
			return;
	}

	// some game modes dont want noises
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		if (GS->bPlayJasonAbilityUnlockSounds)
		{
			if (IsLocallyControlled())
			{
				if (MusicComponent)
				{
					MusicComponent->PlayAbilitiesWarning();
				}
			}
			else
			{
				for (ASCCharacter* Character : GS->PlayerCharacterList)
				{
					if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
					{
						if (Counselor->IsLocallyControlled() && Counselor->MusicComponent)
						{
							Counselor->MusicComponent->PlayAbilitiesWarning();
							break;
						}
					}
				}
			}
		}
	}

	for (uint8 i(0); i < (uint8)EKillerAbilities::MAX; ++i)
	{
		EKillerAbilities CustomOrderAbility = GetAbilityAtPosition(i);

		if (AbilityUnlocked[(uint8)CustomOrderAbility] != OldUnlocked[(uint8)CustomOrderAbility])
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			{
				if (PC->GetSCHUD())
				{
					PC->GetSCHUD()->OnJasonAbilityUnlocked((EKillerAbilities)CustomOrderAbility);
				}
			}

			OldUnlocked[(uint8)CustomOrderAbility] = AbilityUnlocked[(uint8)CustomOrderAbility];
		}
	}
}

void ASCKillerCharacter::StunnedBySweater(ASCCounselorCharacter* SweaterInstigator)
{
	if (PamelasSweaterStun)
	{
		FSCStunData StunData;
		StunData.StunInstigator = SweaterInstigator;
		BeginStun(PamelasSweaterStun, StunData);
		bStunnedBySweater = true;

		// Keep track of sweater stuns for the player that stunned us
		if (Role == ROLE_Authority && SweaterInstigator)
		{
			if (ASCPlayerState* PS = SweaterInstigator->GetPlayerState())
			{
				PS->SweaterStunnedJason();
				PS->EarnedBadge(SweaterInstigator->SweaterStunnedJasonBadge);
			}
		}
	}
}

void ASCKillerCharacter::EnterKillStance()
{
	if (Role < ROLE_Authority)
	{
		SERVER_EnterKillStance();
	}
	else
	{
		if (PamelasSweaterStun)
		{
			BeginStun(PamelasSweaterStun, FSCStunData());

			Health = 1.f;
			// We got stunned while affected by the sweater, spawn the Jason kill
			FActorSpawnParameters spawnParams;
			spawnParams.Owner = this;
			spawnParams.Instigator = Instigator;

			JasonKillObject = GetWorld()->SpawnActor<ASCContextKillActor>(GetJasonKillClass(), spawnParams);
			if (JasonKillObject != nullptr)
			{
				JasonKillObject->SetActorLocation(GetActorLocation() - FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				JasonKillObject->SetActorRotation(GetActorRotation());
				// force the kill stance animation
				MULTICAST_BeginStun(KillStanceMontage, MaskKnockedOffStun, FSCStunData(), true);
			}
		}
	}
}

bool ASCKillerCharacter::SERVER_EnterKillStance_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_EnterKillStance_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	EnterKillStance();
}

uint8 ASCKillerCharacter::GetPositionInUnlockOrder(EKillerAbilities AbilityType) const
{
	for (uint8 i = 0; i < (uint8)EKillerAbilities::MAX; i++)
	{
		if (UserDefinedAbilityOrder[i] == AbilityType)
		{
			return i;
		}
	}

	return (uint8)EKillerAbilities::MAX;
}

float ASCKillerCharacter::GetCurrentRechargeTimePercent(EKillerAbilities Ability) const
{
	const uint8 Ab = (uint8) Ability;

	switch (Ability)
	{
		case EKillerAbilities::Stalk:
		{
			if (bIsStalkActive)
			{
				return FMath::Clamp(StalkActiveTimer / StalkActiveTime, 0.0f, 1.0f);
			}
			break;
		}
		case EKillerAbilities::Shift:
		{
			if (bIsShiftActive)
			{
				return 1.0f - FMath::Clamp(CurrentShiftTime / ShiftTime, 0.0f, 1.0f);
			}
			break;
		}

		case EKillerAbilities::Sense:
		{
			if (bIsSenseActive)
			{
				//const int CurrentLevel = SenseLevel - 1;
				const float ActiveMaxTime = SenseDuration;

				return 1.0f - FMath::Clamp(ActiveSenseTimer / ActiveMaxTime, 0.0f, 1.0f);
			}
			break;
		}
	}

	return FMath::Clamp(AbilityRechargeTimer[Ab] / AbilityRechargeTime[Ab], 0.0f, 1.0f);
}

bool ASCKillerCharacter::IsAbilityAvailable(EKillerAbilities Ability) const
{
	const uint8 AbilityIndex = static_cast<uint8>(Ability);
	if (AbilityUnlocked[AbilityIndex] && AbilityRechargeTimer[AbilityIndex] >= AbilityRechargeTime[AbilityIndex])
	{
		// Can't use abilities while we're in a special move
		if (IsInSpecialMove())
			return false;

		// Can't use abilities while wailing on a door/wall
		if (BreakDownActor)
			return false;

		// No using abilities while stunned
		if (IsStunned())
			return false;

		// No shifting underwater
		if (Ability == EKillerAbilities::Shift && bUnderWater)
			return false;

		// cant use an ability if you are throwing a knife
		if (CurrentThrowable)
			return false;

		return true;
	}

	return false;
}

float ASCKillerCharacter::GetAbilityUnlockPercent(EKillerAbilities Ability) const
{
	if (IsAbilityUnlocked(Ability))
		return 1.f;

	const uint8 CustomOrderIndex = GetPositionInUnlockOrder(Ability);

	if (CustomOrderIndex != 0)
	{
		EKillerAbilities PreceedingAbility = GetAbilityAtPosition(CustomOrderIndex - 1);
		if (!IsAbilityUnlocked(PreceedingAbility))
		{
			return 0.f;
		}
	}

	return AbilityUnlockTimer / AbilityUnlockTime[CustomOrderIndex];
}

bool ASCKillerCharacter::IsAbilityActive(EKillerAbilities Ability) const
{
	switch (Ability)
	{
	case EKillerAbilities::Stalk: return bIsStalkActive;
	case EKillerAbilities::Sense: return bIsSenseActive;
	case EKillerAbilities::Morph: return bIsMorphActive;
	case EKillerAbilities::Shift: return bIsShiftActive;
	}

	check(0);
	return false;
}

void ASCKillerCharacter::OnToggleFastWalk()
{
	if (!IsSprinting())
		SetCombatStance(false);
	SetSprinting(!IsSprinting());
}

void ASCKillerCharacter::OnStartFastWalk()
{
	SetCombatStance(false);
	SetSprinting(true);
}

void ASCKillerCharacter::OnStopFastWalk()
{
	SetSprinting(false);
}

void ASCKillerCharacter::OnMapToggle()
{
	if (bIsMorphActive)
		return;

	OnHideScoreboard();
	bIsMapEnabled = !bIsMapEnabled;
	ToggleMap_Native(bIsMapEnabled, false);
}

void ASCKillerCharacter::OnSenseActive()
{
	if (bIsSenseActive)
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_OnSenseActive();
	}
	else
	{
		ActiveSenseTimer = 0.0f;
		bIsSenseActive = true;

		OnRep_SenseActive();

		UWorld* World = GetWorld();
		if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
			GameState->KillerAbilityUsed(EKillerAbilities::Sense);
	}
}

bool ASCKillerCharacter::SERVER_OnSenseActive_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_OnSenseActive_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	OnSenseActive();
}

void ASCKillerCharacter::CancelSense()
{
	if (bIsSenseActive)
		SERVER_CancelSense();
}

bool ASCKillerCharacter::SERVER_CancelSense_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_CancelSense_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	const float PercentageLeft = (SenseDuration - ActiveSenseTimer) / SenseDuration;

	AbilityRechargeTimer[(uint8) EKillerAbilities::Sense] = AbilityRechargeTime[(uint8) EKillerAbilities::Sense] * FMath::Clamp(PercentageLeft, 0.0f, 0.66f);
	ActiveSenseTimer = 0.0f;

	bIsSenseActive = false;

	OnRep_SenseActive();
}

void ASCKillerCharacter::UpdateSense(float DeltaSeconds)
{
	if (IsLocallyControlled())
	{
		ASCCabin* OurCabin = nullptr;
		for (USCIndoorComponent* TestIndoorVolume : OverlappingIndoorComponents)
		{
			if (ASCCabin* TestCabin = Cast<ASCCabin>(TestIndoorVolume->GetOwner()))
			{
				OurCabin = TestCabin;
				break;
			}
		}

		const FVector JasonLoc = GetActorLocation();
		for (FSenseReference& Reference : SenseReferences)
		{
			ASCCounselorCharacter* Counselor = Reference.Counselor;
			if (Counselor && Counselor->IsAlive() && !Counselor->HasEscaped())
			{
				const float CurrentFear = Counselor->GetFear();
				const float MaxRadius = SenseFearRadiusCurve->GetFloatValue(CurrentFear) * RageSenseDistanceCurve->GetFloatValue(GetRageUnlockPercentage()) * (bIsRageActive ? RagedSenseDistanceModifier : 1.0f);
				const float MaxHideRadius = bIsRageActive ? (SenseFearHidingRadiusCurve->GetFloatValue(CurrentFear) * RageSenseDistanceCurve->GetFloatValue(GetRageUnlockPercentage()) * RagedSenseDistanceModifier) : 0.f; // Jason can only find people in hiding spots if he is raged

				const bool bCounselorHiding = Counselor->IsInHiding();
				if (bCounselorHiding && MaxHideRadius <= 0.f)
					Counselor->SetSenseEffects(false);

				const float Distance = (JasonLoc - Counselor->GetActorLocation()).Size2D();
				if (Distance <= (bCounselorHiding ? MaxHideRadius : MaxRadius))
				{
					// set up counselor glow
					if (Counselor->IsIndoors())
					{
						Counselor->SetSenseEffects(false);

						for (USCIndoorComponent* IndoorVolume : Counselor->OverlappingIndoorComponents)
						{
							if (ASCCabin* Cabin = Cast<ASCCabin>(IndoorVolume->GetOwner()))
							{
								Reference.Cabin = Cabin;

								if (Cabin != OurCabin)
									Cabin->SetSenseEffects(true);
								break;
							}
						}
					}
					else if (bCounselorHiding)
					{
						Counselor->SetSenseEffects(false);

						if (ASCHidingSpot* HidingSpot = Counselor->GetCurrentHidingSpot())
						{
							Reference.HidingSpot = HidingSpot;
							HidingSpot->SetSenseEffects(!IsInContextKill());
						}
					}
					else
					{
						Counselor->SetSenseEffects(!IsInContextKill());

						if (Reference.Cabin)
						{
							Reference.Cabin->SetSenseEffects(false);
							Reference.Cabin = nullptr;
						}

						if (Reference.HidingSpot)
						{
							Reference.HidingSpot->SetSenseEffects(false);
							Reference.HidingSpot = nullptr;
						}
					}
				}
			}
			else
			{
				// make sure sense effects are off
				Counselor->SetSenseEffects(false);
			}
		}

		// Never have sense effects on for the cabin we're in
		if (OurCabin)
		{
			OurCabin->SetSenseEffects(false);
		}
	}

	if (Role == ROLE_Authority)
	{
		ActiveSenseTimer += DeltaSeconds;
		if (ActiveSenseTimer >= SenseDuration)
		{
			bIsSenseActive = false;
			ActiveSenseTimer = 0.0f;
			AbilityRechargeTimer[(uint8) EKillerAbilities::Sense] = 0.0f;

			OnRep_SenseActive();
		}
	}
}

void ASCKillerCharacter::StartSense()
{
	SenseReferences.Empty();
	if (ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		for (ASCCharacter* Character : State->PlayerCharacterList)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				// Look through counselor perks to see if the counselor can avoid the sense ability
				if (ASCPlayerState* PS = Counselor->GetPlayerState())
				{
					float AvoidSense = 0.f;
					for (const USCPerkData* Perk : PS->GetActivePerks())
					{
						check(Perk);

						if (!Perk)
							continue;

						AvoidSense += Perk->AvoidSenseChance;

						if (Counselor->IsIndoors())
							AvoidSense += Perk->AvoidSenseIndoorsChance;

						if (Counselor->CurrentStance == ESCCharacterStance::Swimming)
							AvoidSense += Perk->SwimmingAvoidSenseChance;

						const ASCHidingSpot* HidingSpot = Counselor->GetCurrentHidingSpot();
						if (HidingSpot && HidingSpot->GetHidingSpotType() == ESCHidingSpotType::Tent)
						{
							AvoidSense += Perk->AvoidSenseInSleepingBagChance;
						}
					}

					// Check if this counselor avoids the ability
					if (FMath::FRand() < AvoidSense)
					{
						continue;
					}
				}

				if (Counselor->IsAlive())
				{
					FSenseReference Reference;
					Reference.Counselor = Counselor;
					Reference.Cabin = nullptr;
					Reference.HidingSpot = nullptr;
					SenseReferences.Push(Reference);
				}
			}
		}
	}

	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		PPV->Settings.ColorGradingLUT = SenseColorGrade;
		bColorGradeEnabled = true;
		ExposureValue = 0.8f;

		PPV->Settings.bOverride_AutoExposureMinBrightness = true;
		PPV->Settings.bOverride_AutoExposureMaxBrightness = true;

		PPV->Settings.bOverride_DepthOfFieldMethod = true;
		PPV->Settings.DepthOfFieldMethod = EDepthOfFieldMethod::DOFM_Gaussian;

		PPV->Settings.bOverride_DepthOfFieldFocalDistance = true;
		PPV->Settings.DepthOfFieldFocalDistance = 1.0f;

		PPV->Settings.bOverride_DepthOfFieldFocalRegion = true;
		PPV->Settings.DepthOfFieldFocalRegion = 7000.0f;

		PPV->Settings.bOverride_DepthOfFieldNearTransitionRegion = true;
		PPV->Settings.DepthOfFieldNearTransitionRegion = 0.0f;

		PPV->Settings.bOverride_DepthOfFieldFarTransitionRegion = true;
		PPV->Settings.DepthOfFieldFarTransitionRegion = 10000.0f;

		PPV->Settings.bOverride_DepthOfFieldNearBlurSize = true;
		PPV->Settings.DepthOfFieldNearBlurSize = 0.0f;

		PPV->Settings.bOverride_DepthOfFieldFarBlurSize = true;
		PPV->Settings.DepthOfFieldFarBlurSize = 2.5f;

		PPV->Settings.AddBlendable(SenseCounselorMaterialInstance, 1.0f);
		PPV->Settings.AddBlendable(SenseCabinMaterialInstance, 1.0f);

		if (AOELightScreenEffectInst)
			PPV->Settings.RemoveBlendable(AOELightScreenEffectInst);
	}

	OnSenseChanged(true);
}

void ASCKillerCharacter::EndSense()
{
	for (FSenseReference& Reference : SenseReferences)
	{
		if (Reference.Counselor)
			Reference.Counselor->SetSenseEffects(false);
		if (Reference.Cabin)
			Reference.Cabin->SetSenseEffects(false);
		if (Reference.HidingSpot)
			Reference.HidingSpot->SetSenseEffects(false);
	}
	SenseReferences.Empty();

	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		if (bIsStalkActive)
		{
			PPV->Settings.ColorGradingLUT = StalkColorGrade;
		}
		else
		{
			bColorGradeEnabled = false;
		}

		ExposureValue = 1.2f;

		PPV->Settings.bOverride_AutoExposureMinBrightness = false;
		PPV->Settings.bOverride_AutoExposureMaxBrightness = false;
		PPV->Settings.bOverride_DepthOfFieldMethod = false;
		PPV->Settings.bOverride_DepthOfFieldFocalDistance = false;
		PPV->Settings.bOverride_DepthOfFieldFocalRegion = false;
		PPV->Settings.bOverride_DepthOfFieldNearTransitionRegion = false;
		PPV->Settings.bOverride_DepthOfFieldFarTransitionRegion = false;
		PPV->Settings.bOverride_DepthOfFieldNearBlurSize = false;
		PPV->Settings.bOverride_DepthOfFieldFarBlurSize = false;

		PPV->Settings.RemoveBlendable(SenseCounselorMaterialInstance);
		PPV->Settings.RemoveBlendable(SenseCabinMaterialInstance);

		if (AOELightScreenEffectInst)
			PPV->Settings.AddBlendable(AOELightScreenEffectInst, 1.0f);
	}

	OnSenseChanged(false);
}

void ASCKillerCharacter::OnRep_SenseActive()
{
	if (IsLocallyControlled())
	{
		if (!bIsSenseActive)
			EndSense();
		else
			StartSense();
	}
}

bool ASCKillerCharacter::IsRageActive() const
{
	return bIsRageActive;
}

void ASCKillerCharacter::OnRep_Rage()
{
	if (bIsRageActive)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			if (PC->GetSCHUD())
			{
				PC->GetSCHUD()->OnRageActive();
			}
		}
	}
}

void ASCKillerCharacter::Clients_KilledCounselor_Implementation()
{
}

bool ASCKillerCharacter::CanGrab() const
{
	if (GrabbedCounselor)
		return false;

	if (InCombatStance())
		return false;

	if (IsAttacking())
		return false;

	if (IsStunned() || bIsStunned)
		return false;

	if (IsRecoveringFromStun())
		return false;

	if (ActiveSpecialMove)
		return false;

	if (bUnderWater)
		return false;

	if (!IsVisible())
		return false;

	if (bIsPlacingTrap)
		return false;

	if (CurrentThrowable)
		return false;

	if (IsShiftActive())
		return false;

	return true;
}

bool ASCKillerCharacter::IsCharacterInGrabRange() const
{
	if (!CanGrab())
		return false;

	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		// Using the Killer's orientation as the view direction, change GetActorRotation to GetControlRotation
		const FVector ViewDirection = FRotator(0.f, GetActorRotation().Yaw, 0.f).RotateVector(FVector::ForwardVector);
		// See if there is a Counselor within range and in front of us.
		const float MinGrabAngleAsDot = FMath::Cos(FMath::DegreesToRadians(MinGrabAngle));
		const FVector KillerLocation = GetActorLocation();
		const float ShortestDistSquared = MaxGrabDistance * MaxGrabDistance;

		for (ASCCharacter* Character : GS->PlayerCharacterList)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				// Can't grab counselors who are hiding or dead or in a special move
				if (!Counselor->IsAlive() || Counselor->IsInHiding() || Counselor->CurrentStance == ESCCharacterStance::Crouching || Counselor->PendingSeat)
					continue;

				const FVector CounselorLocation = Counselor->GetActorLocation();
				const FVector KillerToCounselor = CounselorLocation - KillerLocation;
				const float Dot = FVector::DotProduct(KillerToCounselor.GetSafeNormal(), ViewDirection);

				// Is this our best option?
				if (Dot > MinGrabAngleAsDot && KillerToCounselor.SizeSquared() < ShortestDistSquared)
					return true;
			}
		}
	}

	return false;
}

void ASCKillerCharacter::ToggleCombatStance()
{
	if (IsAttemptingGrab())
		return;

	// no going into combat when we're a peeper.
	if (bInVoyeurMode)
		return;

	Super::ToggleCombatStance();
}

void ASCKillerCharacter::BeginGrab()
{
	if (!CanGrab() || IsAttemptingGrab())
		return;

	// Can't grab while fapping.
	if (bInVoyeurMode)
		return;

	// Can we interact with this thing by grabbing?
	USCInteractComponent* InteractComp = GetInteractable();
	if (InteractComp != nullptr)
	{
		if (InteractComp->CanGrabToInteract())
		{
			OnInteract(InteractComp, EILLInteractMethod::Press);
			return;
		}
	}

	if (Role < ROLE_Authority)
	{
		SERVER_BeginGrab();
	}
	else
	{
		MULTICAST_BeginGrab();
	}
}

bool ASCKillerCharacter::SERVER_BeginGrab_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_BeginGrab_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	// Make AI noise
	MakeStealthNoise(.5f, this);

	BeginGrab();
}

void ASCKillerCharacter::MULTICAST_BeginGrab_Implementation()
{
	if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->PlayGrabAnim();
	}
}

void ASCKillerCharacter::GrabCounselor(ASCCounselorCharacter* CounselorOverride /*= nullptr*/)
{
	if (Role != ROLE_Authority)
		return;

	if (!CounselorOverride && (ActiveSpecialMove || IsAttacking()))
		return;

	// Can't grab another counselor
	if (GrabbedCounselor)
		return;

	ASCCounselorCharacter* CounselorToGrab = CounselorOverride;
	if (!CounselorToGrab)
	{
		const ASCGameState* GameState = Cast<ASCGameState>(GetWorld()->GetGameState());
		if (!GameState)
			return;

		// Using the Killer's orientation as the view direction, change GetActorRotation to GetControlRotation
		const FVector ViewDirection = FRotator(0.f, GetActorRotation().Yaw, 0.f).RotateVector(FVector::ForwardVector);

		// See if there is a Counselor within range and in front of us.
		const float MinGrabAngleAsDot = FMath::Cos(FMath::DegreesToRadians(MinGrabAngle));

		if (CVarDebugGrab.GetValueOnGameThread() > 0)
		{
			DrawDebugCone(GetWorld(), GetActorLocation(), ViewDirection, MaxGrabDistance, FMath::DegreesToRadians(MinGrabAngle), 0.f, 8, FColor::Red, true, 5.f);
			if (MinGrabDistance > 0.f)
				DrawDebugCone(GetWorld(), GetActorLocation(), ViewDirection, MinGrabDistance, PI / 2.f, 0.f, 8, FColor::Red, true, 5.f);
		}

		const FVector KillerLocation = GetActorLocation();
		float ShortestDistSquared = MaxGrabDistance * MaxGrabDistance;
		const float MinDistSq = MinGrabDistance * MinGrabDistance;
		for (ASCCharacter* Character : GameState->PlayerCharacterList)
		{
			ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character);
			if (!Counselor)
				continue;

			const bool bIsDead = !Counselor->IsAlive();

#if 0 // Disabled while we're not doing body dragging
			const FVector CounselorLocation = Counselor->GetActorLocation();
			FVector KillerToCounselor = CounselorLocation - GetActorLocation();
			if (bIsDead)
			{
				// We don't want to pick up dead counselors.
				KillerToCounselor = Counselor->GetMesh()->GetComponentLocation() - GetActorLocation();
			}

			// If they're dead... double our range? This seems arbitrary, maybe remove the limit if they're dead?
			float Dot = FVector::DotProduct(KillerToCounselor.GetSafeNormal(), ViewDirection);
			if (bIsDead)
			{
				Dot *= 2.f;
			}
#else
			// Can't grab counselors who are hiding or dead or in a special move
			if (bIsDead || Counselor->IsInHiding() || Counselor->PendingSeat || Counselor->IsInVehicle() || Counselor->IsCharacterDisabled())
				continue;

			const FVector CounselorLocation = Counselor->GetActorLocation();
			const FVector KillerToCounselor = CounselorLocation - KillerLocation;
			const float Dot = FVector::DotProduct(KillerToCounselor.GetSafeNormal(), ViewDirection);
#endif

			// If we're within the min distance, only do a half space check
			const bool bDotValid = MinDistSq >= KillerToCounselor.SizeSquared() ? Dot > 0.f : Dot > MinGrabAngleAsDot;

			// Is this our best option?
			if (bDotValid && KillerToCounselor.SizeSquared() < ShortestDistSquared)
			{
				// Make sure we have line of sight
				static FName GrabOcclusionTag(TEXT("GrabOcclusionTrace"));
				FCollisionQueryParams TraceParams(GrabOcclusionTag);
				TraceParams.bTraceAsyncScene = true;
				TraceParams.bReturnPhysicalMaterial = false;

				// Ignore us
				TArray<AActor*> AttachedActors;
				GetAttachedActors(AttachedActors);
				AttachedActors.Add(this);
				TraceParams.AddIgnoredActors(AttachedActors);

				// Ignore them
				Counselor->GetAttachedActors(AttachedActors);
				AttachedActors.Add(Counselor);
				TraceParams.AddIgnoredActors(AttachedActors);

				// Ray trace
				FHitResult HitResult;
				const bool bIsOccluded = GetWorld()->LineTraceSingleByChannel(HitResult, KillerLocation, CounselorLocation, ECollisionChannel::ECC_WorldDynamic, TraceParams);

				// Not blocked, 
				if (bIsOccluded == false)
				{
					CounselorToGrab = Counselor;
					ShortestDistSquared = KillerToCounselor.SizeSquared();

					if (CVarDebugGrab.GetValueOnGameThread() > 0)
						DrawDebugSphere(GetWorld(), CounselorLocation, 15.f, 8, FColor::Green, true, 5.f, 0); // Can grab!
				}
				else if (CVarDebugGrab.GetValueOnGameThread() > 0)
				{
					DrawDebugSphere(GetWorld(), CounselorLocation, 15.f, 8, FColor::Orange, true, 5.f, 0); // Occluded
				}
			}
			else if (CVarDebugGrab.GetValueOnGameThread() > 0)
			{
				DrawDebugSphere(GetWorld(), Counselor->GetActorLocation(), 15.f, 8, FColor::Red, true, 5.f, 0); // Can't reach
			}
		}
	}

	if (CounselorToGrab)
	{
		MULTICAST_GrabSuccess(CounselorToGrab);
	}
	else
	{
		MULTICAST_GrabFailed();
	}
}

void ASCKillerCharacter::FinishGrabCounselor(ASCCounselorCharacter* Counselor)
{
	check(Counselor);

	Counselor->SetVehicleCollision(false);
	if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		if (AnimInstance->IsGrabEnterPlaying())
		{
			const float GrabTimeRemaining = AnimInstance->UpdateGrabState(true);
			if (Role == ROLE_Authority)
			{
				if (GrabTimeRemaining > 0.f)
				{
					FTimerDelegate GrabKillLambda;
					GrabKillLambda.BindLambda([this]()
					{
						if (IsValid(this))
						{
							bGrabKillsActive = true;
						}
					});

					if (GetWorld())
						GetWorldTimerManager().SetTimer(TimerHandle_GrabKill, GrabKillLambda, GrabTimeRemaining, false);
				}
				else
				{
					bGrabKillsActive = true;
				}
			}
		}
	}

	BP_BeginGrab();

	if (IsLocallyControlled())
	{
		SetCombatStance(false);
		SetSprinting(false);
	}

	if (IsLocallyControlled() || Counselor->IsLocallyControlled())
	{
		// Play grab sound here.
		if (GrabbedSound)
		{
			GrabbedAudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), GrabbedSound);
			GrabbedAudioComponent->Play();
		}
	}

	auto IgnoreEverything = [](UCapsuleComponent* Capsule, TArray<AActor*>& IgnoreActors)
	{
		for (auto Actor : IgnoreActors)
		{
			Capsule->IgnoreActorWhenMoving(Actor, true);
		}
	};

	TArray<AActor*> AttachedActors;
	// Ignore Counselor collision
	Counselor->GetAttachedActors(AttachedActors);
	AttachedActors.Add(Counselor);
	IgnoreEverything(GetCapsuleComponent(), AttachedActors);

	// Ignore Jason collision
	GetAttachedActors(AttachedActors);
	AttachedActors.Add(this);
	IgnoreEverything(Counselor->GetCapsuleComponent(), AttachedActors);

	if (Counselor->IsAlive())
	{
		bIsPickingUpCounselor = true;

		GrabValue = 1.f;
		Counselor->OnGrabbed(this);
		if (!Counselor->IsInContextKill())
			Counselor->TakeCameraControl(this);
		UpdateGrabCapsule(true);

		Counselor->AttachToKiller(true);
	}

	GrabbedCounselor = Counselor;
	OnGrabbedCounselor.Broadcast(GrabbedCounselor);

	if (IsLocallyControlled())
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			PC->PushInputComponent(GrabInputComponent);
		}
	}
}

void ASCKillerCharacter::MULTICAST_GrabSuccess_Implementation(ASCCounselorCharacter* CounselorToGrab)
{
	FinishGrabCounselor(CounselorToGrab);
}

void ASCKillerCharacter::MULTICAST_GrabFailed_Implementation()
{
	MULTICAST_DropCounselor();
}

void ASCKillerCharacter::DropCounselor(bool bMoveCapsule /* = true*/)
{
	if (Role < ROLE_Authority)
	{
		SERVER_DropCounselor(bMoveCapsule);
	}
	else
	{
		MULTICAST_DropCounselor(bMoveCapsule);
	}
}

bool ASCKillerCharacter::SERVER_DropCounselor_Validate(bool bMoveCapsule /* = true*/)
{
	return true;
}

void ASCKillerCharacter::SERVER_DropCounselor_Implementation(bool bMoveCapsule /* = true*/)
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	DropCounselor(bMoveCapsule);

	// Make AI noise
	MakeStealthNoise(.25f, this);
}

void ASCKillerCharacter::MULTICAST_DropCounselor_Implementation(bool bMoveCapsule /* = true*/)
{
	if (GrabbedCounselor)
	{
		// NOTE: Making a copy as some of these functions may clear the member variable
		ASCCounselorCharacter* AffectedCounselor = GrabbedCounselor;

		if (Role == ROLE_Authority && AffectedCounselor->IsInWheelchair())
		{
			// Kill them since they can't walk and they've been detached from their wheelchair
			AffectedCounselor->ForceKillCharacter(this);
		}

		AffectedCounselor->OnDropped(false, bInWater);

		if (!AffectedCounselor->IsAlive())
		{
			AffectedCounselor->GetMesh()->SetSimulatePhysics(false);
			AffectedCounselor->GetMesh()->SetPhysicsBlendWeight(1.f);
		}

		auto StopIgnoringEverything = [](UCapsuleComponent* Capsule)
		{
			if (IsValid(Capsule))
				Capsule->ClearMoveIgnoreActors();
		};

		// Ignore Counselor collision
		StopIgnoringEverything(GetCapsuleComponent());

		// Ignore Jason collision
		StopIgnoringEverything(AffectedCounselor->GetCapsuleComponent());

		if (bMoveCapsule)
			UpdateGrabCapsule(false);
	}

	GrabValue = 0.f;
	GrabbedCounselor = nullptr;
	OnGrabbedCounselor.Broadcast(GrabbedCounselor);

	if (IsLocallyControlled())
		SERVER_SetLockOnTarget(nullptr);

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->SetCinematicMode(false, true, false);
		if (!bIsStunned)
			PC->SetIgnoreMoveInput(false);

		if (IsLocallyControlled())
		{
			PC->PopInputComponent(GrabInputComponent);
		}
	}

	if (USCAnimInstance* AI = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AI->StopGrabAnim();
	}
}

void ASCKillerCharacter::UpdateGrabCapsule(bool BeginGrab)
{
	if (bIsUpdatingGrabCapsule)
		return;

	// the mesh offset will only happen for the server and the killer local player
	// the capusle resizing will happen for everyone but because the mesh offset wont change
	// it breaks for everyone else, so for now we are going to ignore it for everyone else until that issue can be fixed.
	if (!IsLocallyControlled() && Role != ROLE_Authority)
		return;

	bIsUpdatingGrabCapsule = true; // Turn on overflow protection

	FVector StartingLocation = GetActorLocation();
	FVector MeshLocation = GetMesh()->GetRelativeTransform().GetLocation();
	const FVector GrabMeshOffsetVector = FVector(GrabbedMeshOffset, 0.f, 0.f);
	if (BeginGrab)
	{
		// Adjust our capsule/position to prevent clipping
		GetCapsuleComponent()->SetCapsuleRadius(GrabbedCapsuleSize, false);
		StartingLocation -= GetActorRotation().RotateVector(GrabMeshOffsetVector);

		// Why Add? Just set it.
		MeshLocation.X = GrabbedMeshOffset;// FVector(GrabbedMeshOffset, 0.f, 0.f);
	}
	else
	{
		// Reset our capsule/position
		GetCapsuleComponent()->SetCapsuleRadius(DefaultCapsuleSize, false);
		StartingLocation += GetActorRotation().RotateVector(GrabMeshOffsetVector);

		// Why Subtract? Just set it.
		MeshLocation.X = 0.f;// FVector(0.f, 0.f, 0.f);
	}

	SetActorLocation(StartingLocation);
	GetMesh()->SetRelativeLocation(MeshLocation);
	bGrabKillsActive = false;

	bIsUpdatingGrabCapsule = false; // Turn off overflow protection
}

bool ASCKillerCharacter::IsGrabKilling() const
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			return AnimInstance->IsGrabKilling();
		}
	}

	return false;
}

bool ASCKillerCharacter::IsAttemptingGrab() const
{
	if (USCAnimInstance* AI = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		return AI->IsGrabAnimPlaying();
	}
	return false;
}

void ASCKillerCharacter::GrabCounselorOutOfVehicle(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp)
{
	check(Role == ROLE_Authority);
	check(Counselor);
	check(KillComp);

	// Need to have counselors drop any large items here before the grab code tries to handle it
	// Grab code will place the item INSIDE the vehicle
	if (Counselor)
	{
		if (Counselor->GetCurrentSeat())
		{
			if (ASCItem* CounselorEquippedItem = Counselor->GetEquippedItem())
			{
				CounselorEquippedItem->SetActorHiddenInGame(false);
				CounselorEquippedItem->SetActorEnableCollision(true);
				Counselor->DropEquippedItem(Counselor->GetCurrentSeat()->GetComponentLocation() + Counselor->GetCurrentSeat()->GetComponentRotation().Vector() * 300.f, true);
				CounselorEquippedItem = nullptr;
			}
		}

		// Grabbed a bot out of the vehicle
		if (!Counselor->IsPlayerControlled())
		{
			ASCCounselorAIController* BotController = [&Counselor]() -> ASCCounselorAIController*
			{
				if (Counselor->IsInVehicle() && Counselor->IsDriver())
					return Cast<ASCCounselorAIController>(Counselor->GetVehicle()->GetController());

				return Cast<ASCCounselorAIController>(Counselor->GetController());
			}();

			// Clear our desired interactables since we've been grabbed
			if (BotController)
				BotController->ClearInteractables();
		}
	}

	CurrentVehicleGrabKillComp = KillComp;
	CurrentVehicleGrabKillComp->KillDestinationReached.AddUniqueDynamic(this, &ASCKillerCharacter::OnGrabCounselorOutOfVehicleStarted);
	CurrentVehicleGrabKillComp->SERVER_ActivateContextKill(this, Counselor);
}

void ASCKillerCharacter::MULTICAST_GrabCounselorOutOfVehicle_Implementation(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp)
{
	SetVehicleCollision(false);
	Counselor->GrabbedOutOfVehicle(KillComp);
	KillComp->KillComplete.AddUniqueDynamic(this, &ASCKillerCharacter::OnGrabCounselorOutOfVehicleComplete);
}

void ASCKillerCharacter::OnGrabCounselorOutOfVehicleStarted(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (ASCGameState_Hunt* GS = GetWorld()->GetGameState<ASCGameState_Hunt>())
	{
		GS->CounselorsPulledFromCarCount++;
	}

	MULTICAST_GrabCounselorOutOfVehicle(Cast<ASCCounselorCharacter>(Victim), CurrentVehicleGrabKillComp);
}

void ASCKillerCharacter::OnGrabCounselorOutOfVehicleComplete(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (Role == ROLE_Authority)
	{
		CurrentVehicleGrabKillComp = nullptr;
	}

	SetVehicleCollision(true);
	// Make sure we got 'em!
	if (GrabbedCounselor)
	{
		bGrabKillsActive = true;
	}
}

void ASCKillerCharacter::OnGrabCounselorOutOfVehicleAborted(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	if (Role == ROLE_Authority)
	{
		CurrentVehicleGrabKillComp = nullptr;
	}

	SetVehicleCollision(true);
	DropCounselor();
}


void ASCKillerCharacter::KillCounselorInBoat(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp)
{
	check(Role == ROLE_Authority);
	check(Counselor);
	check(KillComp);

	MULTICAST_KillCounselorInBoat(Counselor, KillComp);
	KillComp->SERVER_ActivateContextKill(this, Counselor);
}

void ASCKillerCharacter::MULTICAST_KillCounselorInBoat_Implementation(ASCCounselorCharacter* Counselor, USCContextKillComponent* KillComp)
{
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	Counselor->GrabbedOutOfVehicle(KillComp);
	KillComp->KillComplete.AddUniqueDynamic(this, &ASCKillerCharacter::OnKillCounselorInBoatComplete);
}

void ASCKillerCharacter::OnKillCounselorInBoatComplete(ASCCharacter* Interactor, ASCCharacter* Victim)
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

bool ASCKillerCharacter::CanAttack() const
{
	// Killing someone, one sec
	if (IsGrabKilling())
		return false;

	// No animation cancelling in this here game!
	if (IsAttemptingGrab())
		return false;

	// Got a bitch grabbed (OnAttack will perform a grab kill if this is the case)
	if (GrabbedCounselor)
		return false;

	// Placing a trap
	if (bIsPlacingTrap)
		return false;

	// Shifting, morphing, etc.
	if (!IsVisible())
		return false;

	// We're throwing a knife
	if (CurrentThrowable)
		return false;

	// LET'S BREAK IT DOWN!
	if (BreakDownActor)
		return false;

	// Stop trying to do things when peeping.
	if (bInVoyeurMode)
		return false;

	// Check base class stuff
	return Super::CanAttack();
}

ASCCounselorCharacter* ASCKillerCharacter::GetGrabbedCounselor() const
{
	return GrabbedCounselor;
}

void ASCKillerCharacter::PerformGrabKill()
{
	if (Role < ROLE_Authority)
	{
		SERVER_PerformGrabKill();
	}
	else
	{
		if (USkeletalMeshComponent* CurrentMesh = GetMesh())
		{
			if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(CurrentMesh->GetAnimInstance()))
			{
				int32 SelectedGrabKill = AnimInstance->ChooseGrabKillAnim();
				MULTICAST_PerformGrabKill(SelectedGrabKill);
			}
		}
	}
}

bool ASCKillerCharacter::SERVER_PerformGrabKill_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_PerformGrabKill_Implementation()
{
	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	// Make AI noise
	MakeStealthNoise(.5f, this);

	PerformGrabKill();
}

void ASCKillerCharacter::MULTICAST_PerformGrabKill_Implementation(int32 SelectedAttack)
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			AnimInstance->PlayGrabKillAnim(SelectedAttack);
		}
	}
}

void ASCKillerCharacter::OnRep_GrabbedCounselor()
{
	if (GrabbedCounselor)
	{
		FinishGrabCounselor(GrabbedCounselor);
	}
}

bool ASCKillerCharacter::Morph(FVector NewLocation)
{
	bActivateMorph = false;
	if (!IsAbilityAvailable(EKillerAbilities::Morph))
	{
		return false;
	}

	const float Distance = 3000.0f;
	TArray<FOverlapResult> MorphLocations;
	GetWorld()->OverlapMultiByChannel(MorphLocations, NewLocation, FQuat::Identity, ECC_MorphPoint, FCollisionShape::MakeSphere(Distance));

	if (MorphLocations.Num() <= 0)
	{
#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("No morph locations near area."));
#endif
		return false;
	}

	FVector Car4Loc = FVector::ZeroVector;
	FVector Car2Loc = FVector::ZeroVector;
	float Car4Radius = 0.0f;
	float Car2Radius = 0.0f;
	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		if (ASCDriveableVehicle* Vehicle = GS->GetCarVehicle(true))
		{
			Car4Loc = Vehicle->GetActorLocation();
			Car4Radius = Vehicle->GetSimpleCollisionRadius() * 1.5f;
		}

		if (ASCDriveableVehicle* Vehicle = GS->GetCarVehicle(false))
		{
			Car2Loc = Vehicle->GetActorLocation();
			Car2Radius = Vehicle->GetSimpleCollisionRadius() * 1.5f;
		}
	}

	ClosestPoint = MorphLocations[0].Actor.Get()->GetActorTransform();
	float DistSQ = Distance * Distance;
	bool ValidPoint = false;
	for (FOverlapResult& Result : MorphLocations)
	{
		AActor* MorphActor = Result.Actor.Get();
		if (ASCKillerMorphPoint* MorphPoint = Cast<ASCKillerMorphPoint>(MorphActor))
		{
			if (!MorphPoint->IsEncroached(this))
			{
				const FVector PointLoc = MorphPoint->GetActorLocation();
				const float NewDist = (PointLoc - NewLocation).SizeSquared2D();
				if (NewDist < DistSQ)
				{
					if (!Car4Loc.IsNearlyZero())
					{
						const FVector Car4Dist = PointLoc - Car4Loc;
						if (Car4Dist.Size() < Car4Radius)
							continue;
					}

					if (!Car2Loc.IsNearlyZero())
					{
						const FVector Car2Dist = PointLoc - Car2Loc;
						if (Car2Dist.Size() < Car2Radius)
							continue;
					}

					DistSQ = NewDist;
					ClosestPoint = MorphPoint->GetActorTransform();
				}
				ValidPoint = true;
			}
		}
	}

	if (!ValidPoint)
	{
#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("No morph locations near area."));
#endif
		return false;
	}

	MorphLocation = ClosestPoint.GetTranslation();
	MorphRotation = ClosestPoint.Rotator();
	SERVER_OnMorph(MorphLocation, MorphRotation.Vector());

	return true;
}

bool ASCKillerCharacter::SERVER_OnMorph_Validate(FVector_NetQuantize NewLocation, FVector_NetQuantizeNormal NewRotation)
{
	return true;
}

void ASCKillerCharacter::SERVER_OnMorph_Implementation(FVector_NetQuantize NewLocation, FVector_NetQuantizeNormal NewRotation)
{
	if (IsStunned())
		return;

	SERVER_OnTeleportEffect(true);
	MorphLocation = NewLocation;
	MorphRotation = NewRotation.Rotation();
	bIsMorphActive = true;
	GetWorldTimerManager().SetTimer(TimerHandle_Teleport, this, &ASCKillerCharacter::OnMorphEnded, TeleportScreenTime, false);
}

void ASCKillerCharacter::OnMorphEnded()
{
	if (Role == ROLE_Authority)
	{
		if (!IsLocallyControlled())
			CLIENT_OnMorphEnded();

		SERVER_OnTeleportEffect(false);

		// Make AI noise
		MakeStealthNoise(.25f, this);
	}

	bShowMorphMap = false;
	bIsMorphActive = false;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		PC->PopInputComponent(MorphInputComponent);
		PC->CurrentMouseCursor = PC->DefaultMouseCursor;
		PC->SetControlRotation(FRotator::ZeroRotator);
		PC->BumpIdleTime();
	}

	ToggleMap_Native(false, false);

	ResetCameras();

	// Teleport
	SetActorLocationAndRotation(MorphLocation, MorphRotation);
	MorphLocation = FVector::ZeroVector;
	AbilityRechargeTimer[(uint8) EKillerAbilities::Morph] = 0.0f;

	UWorld* World = GetWorld();
	if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
		GameState->KillerAbilityUsed(EKillerAbilities::Morph);
}

void ASCKillerCharacter::CLIENT_OnMorphEnded_Implementation()
{
	OnMorphEnded();
}

void ASCKillerCharacter::OnAttemptMorph()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->GetSCHUD())
		{
			PC->GetSCHUD()->OnKillerAttemptMorph();
		}
	}
}

void ASCKillerCharacter::MorphCursorUp(float Val)
{
	if (Val != 0.0f)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			FIntPoint MousePos;
			ULocalPlayer* Local = PC->GetLocalPlayer();
			if (Local)
			{
			Local->ViewportClient->Viewport->GetMousePos(MousePos);
			const float DeltaSeconds = GetWorld()->GetDeltaSeconds();

			MousePos.Y -= Val * 100.0f * DeltaSeconds;

			PC->SetMouseLocation(MousePos.X, MousePos.Y);
			}
		}
	}
}

void ASCKillerCharacter::MorphCursorRight(float Val)
{
	if (Val != 0.0f)
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			FIntPoint MousePos;
			ULocalPlayer* Local = PC->GetLocalPlayer();
			if (Local)
			{
			Local->ViewportClient->Viewport->GetMousePos(MousePos);
			const float DeltaSeconds = GetWorld()->GetDeltaSeconds();

			MousePos.X += Val * 100.0f * DeltaSeconds;

			PC->SetMouseLocation(MousePos.X, MousePos.Y);
			}
		}
	}
}

bool ASCKillerCharacter::SERVER_SetShift_Validate(bool bShift)
{
	return true;
}

void ASCKillerCharacter::SERVER_SetShift_Implementation(bool bShift)
{
	if (ShiftWarmUpTime > 0.0f && bShift)
		return;

	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	bIsShiftActive = EnableTeleportEffect = bShift;
	OnRep_ShiftActive();
	OnRep_Teleport();

	if (!bIsShiftActive)
	{
		AbilityRechargeTimer[(uint8) EKillerAbilities::Shift] = 0.0f;
	}

	// Make noise for AI to hear
	MakeStealthNoise(.25f, this);
}

void ASCKillerCharacter::OnRep_ShiftActive()
{
	if (bIsShiftActive)
	{
		OnEnablePowersReleased();
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			PC->PushInputComponent(ShiftInputComponent);

		// For shifting we are going to push a sound mix then pitch up the music.
		if (IsLocallyControlled())
		{
			if (APostProcessVolume* PPV = GetPostProcessVolume())
			{
				if (ShiftScreenEffectInst)
				{
					PPV->Settings.AddBlendable(ShiftScreenEffectInst, 1.0f);
				}
			}

			UGameplayStatics::PushSoundMixModifier(GetWorld(), ShiftAudioSoundMix);

			GetWorld()->GetTimerManager().SetTimer(TimerHandle_ShiftMusicPitchDelayTime, this, &ASCKillerCharacter::ShiftMusicPitchDelayTime, 0.8f);
		}

		ShiftWarmUpTime = 0.7f;

		// ignore any currently active projectiles
		if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			for (ASCProjectile* Projectile : GS->ProjectileList)
			{
				if (Projectile)
				{
					Projectile->SetIgnoredActor(this);
					GetCapsuleComponent()->IgnoreActorWhenMoving(Projectile, true);
				}
			}
		}

		UWorld* World = GetWorld();
		if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
			GameState->KillerAbilityUsed(EKillerAbilities::Shift);
	}
	else
	{
		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
			PC->PopInputComponent(ShiftInputComponent);

		if (APostProcessVolume* PPV = GetPostProcessVolume())
		{
			if (ShiftScreenEffectInst)
			{
				PPV->Settings.RemoveBlendable(ShiftScreenEffectInst);
			}
		}

		// Done with shifting we are going to reset all audio changes back to normal.
		if (IsLocallyControlled())
		{
			// Make sure if shift started we don't fire all the sfx and everything.
			if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_ShiftMusicPitchDelayTime))
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ShiftMusicPitchDelayTime);
			}

			if (ShiftAudioComponent)
			{
				ShiftAudioComponent->FadeOut(0.5f, 0.f);
			}

			if (MusicComponent)
			{
				MusicComponent->SetAdjustPitchMultiplier(1.0f);
			}

			UGameplayStatics::PopSoundMixModifier(GetWorld(), ShiftAudioSoundMix);
		}

		// make sure Jason can still take damage from projectiles
		if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			for (ASCProjectile* Projectile : GS->ProjectileList)
			{
				if (Projectile)
				{
					Projectile->RemoveIgnoredActor(this);
					GetCapsuleComponent()->IgnoreActorWhenMoving(Projectile, false);
				}
			}
		}
	}

	CurrentShiftTime = 0.0f;
	OnBlink(bIsShiftActive);
}

void ASCKillerCharacter::ShiftMusicPitchDelayTime()
{
	if (MusicComponent)
	{
		MusicComponent->SetAdjustPitchMultiplier(1.5);
	}

	if (ShiftAudioCue)
	{
		ShiftAudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), ShiftAudioCue);
		ShiftAudioComponent->FadeIn(0.5f, 1.0f);
	}
}

void ASCKillerCharacter::StunnedByPolice(USceneComponent* DamageCauser, FVector EscapeForward, float Radius)
{
	if (bInContextKill)
	{
		// Store the stun info for after the kill has been completed
		if (Role == ROLE_Authority)
		{
			bDelayPoliceStun = true;
			PendingStunInfo.DamageCauser = DamageCauser;
			PendingStunInfo.PoliceEscapeForwardDir = EscapeForward;
			PendingStunInfo.PoliceEscapeRadius = Radius;
		}

		return;
	}

	SetStance(ESCCharacterStance::Standing);

	FVector ToActor = FVector(FVector2D(GetActorLocation() - DamageCauser->GetComponentLocation()), 0.f).GetSafeNormal();
	SetActorRotation((-ToActor).ToOrientationQuat());

	if (Role == ROLE_Authority)
	{
		UWorld* World = GetWorld();
		// What happened?!?!?
		if (!World)
			return;

		const FVector PoliceRight = FVector::CrossProduct(FVector::UpVector, EscapeForward);
		const FVector PoliceLoc = DamageCauser->GetComponentLocation();
		const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FVector NewLoc = GetActorLocation();

		// Add our player capsule to our test radius.
		Radius += GetCapsuleComponent()->GetScaledCapsuleRadius() + 5.f; // We're adding 5cm to the radius so that we're sure the player is placed outside the volume.

		FHitResult HitResult;
		static const FName NAME_PoliceStunTrace(TEXT("PoliceStunTrace"));
		FCollisionQueryParams Params(NAME_PoliceStunTrace, false, this);

		static const FName NAME_SweepTag(TEXT("PoliceStunTraceTag"));
		FCollisionQueryParams QueryParams(NAME_SweepTag, true);
		QueryParams.AddIgnoredActor(this);

		FCollisionResponseParams ResponseParams;
		ResponseParams.CollisionResponse.SetResponse(ECC_Camera, ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_Boat, ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_DynamicContextBlocker, ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_Foliage, ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_Killer, ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

		// We should never be rotating more than 180 degrees.
		int MaxIterations = FMath::CeilToInt(180.f / PoliceStunIterationAngle);
		const int TotalIterations = MaxIterations;

		// Check which direction we should be sweeping our tests.
		float IterationAngleDelta = PoliceStunIterationAngle;
		if (FVector::DotProduct(ToActor, PoliceRight) > 0.f)
			IterationAngleDelta *= -1.f;

		if (CVarDebugPoliceStun.GetValueOnGameThread() > 0)
		{
			// DEBUG DRAW cus math is hard.
			DrawDebugLine(World, PoliceLoc, PoliceLoc + PoliceRight * 300, FColor::Green, false, 10.0f, 0, 2);
			DrawDebugLine(World, PoliceLoc, PoliceLoc + EscapeForward * 300, FColor::Green, false, 10.0f, 0, 2);
		}

		// Test each iterated location to see if Jason's capsule fits, if it does just place him there and activate the stun.
		do
		{
			NewLoc = PoliceLoc + (ToActor * Radius);
			if (CVarDebugPoliceStun.GetValueOnGameThread() > 0)
			{
				DrawDebugLine(World, PoliceLoc, NewLoc,FColor(255 * (MaxIterations / TotalIterations), 0, 255 - (255 * (MaxIterations / TotalIterations)), 0), false, 10.0f, 0, 2);
			}

			// Find the terrain
			World->LineTraceSingleByChannel(HitResult, NewLoc + FVector::UpVector * HalfHeight, NewLoc - FVector::UpVector * HalfHeight, ECC_Visibility, Params);
			if (HitResult.bBlockingHit)
			{
				NewLoc = HitResult.ImpactPoint + FVector::UpVector * HalfHeight;
				QueryParams.AddIgnoredActor(HitResult.GetActor());

				// Does the capsule fit?
				World->SweepSingleByChannel(HitResult, NewLoc + FVector::UpVector * 25.0f, NewLoc + FVector::UpVector,
					GetCapsuleComponent()->GetComponentQuat(), ECC_WorldStatic, GetCapsuleComponent()->GetCollisionShape(), QueryParams, ResponseParams);

				if (!HitResult.bBlockingHit)
				{
					SetActorLocation(NewLoc);
					SetActorRotation((-ToActor).ToOrientationQuat());
					break;
				}
			}

			if (FVector::DotProduct(ToActor, EscapeForward) > 0.95f && MaxIterations > 2) // 0.05 in a normalized dot product is 4.5 degrees
			{
				ToActor = EscapeForward;
				// We want our next test to be our last
				MaxIterations = 2;
			}
			else
			{
				ToActor = ToActor.RotateAngleAxis(IterationAngleDelta, FVector::UpVector);
			}

		} while (--MaxIterations > 0);

		TakeDamage(0.0f, FDamageEvent(PoliceShootJasonStunClass), Controller, DamageCauser->GetOwner());
	}
	BP_StunnedByPolice();
}

void ASCKillerCharacter::MULTICAST_SpottedByPolice_Implementation(USoundCue* PlayAudio, const FVector_NetQuantize Location)
{
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), PlayAudio, Location);
}

bool ASCKillerCharacter::SERVER_OnTeleportEffect_Validate(bool Enable)
{
	return true;
}

void ASCKillerCharacter::SERVER_OnTeleportEffect_Implementation(bool Enable)
{
	if (EnableTeleportEffect != Enable)
	{
		EnableTeleportEffect = Enable;
		OnRep_Teleport();
	}
}

void ASCKillerCharacter::SetVoyeurMode(bool InVoyeur)
{
	bInVoyeurMode = InVoyeur;
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (PC->IsMoveInputIgnored() != bInVoyeurMode)
			PC->SetIgnoreMoveInput(bInVoyeurMode);
	}

	// Force combat stance off since the input component can block leaving voyeur mode
	if (bInVoyeurMode)
		SetCombatStance(false);
}

void ASCKillerCharacter::OnRep_Teleport()
{
	if (USkeletalMeshComponent* CurrentMesh = GetMesh())
	{
		if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(CurrentMesh->GetAnimInstance()))
		{
			AnimInstance->OnTeleport();
		}
	}

	if (!IsCharacterDisabled())
	{
		if (ASCWeapon* Weapon = GetCurrentWeapon())
		{
			Weapon->SetActorHiddenInGame(EnableTeleportEffect);
		}
	}

	TeleportEffectAmount = EnableTeleportEffect ? 1.0f : 0.0f;

	PlayDistortionEffect();
}

void ASCKillerCharacter::SERVER_Attack_Implementation(bool HeavyAttack /*= false*/)
{
	if (IsInSpecialMove())
		return;

	// If we have a counselor grabbed, simply kill them rather than do an attack.
	if (GrabbedCounselor)
	{
		PerformGrabKill();
		return;
	}

	Super::SERVER_Attack_Implementation(HeavyAttack);
}

float ASCKillerCharacter::GetSpeedModifier() const
{
	return 1.0f;
}

void ASCKillerCharacter::SetSprinting(bool bSprinting)
{
	if (bSprinting && GrabbedCounselor != nullptr)
		return;

	Super::SetSprinting(bSprinting);
}

void ASCKillerCharacter::SetCombatStance(const bool bEnable)
{
	if (bEnable && (GrabbedCounselor || BreakDownActor || bThrowingKnife || IsAttemptingGrab()))
		return;

	if (Role == ROLE_Authority)
	{
		Super::SetCombatStance(bEnable);
	}
	else
	{
		SERVER_SetCombatStance(bEnable);
	}
}

bool ASCKillerCharacter::SERVER_SetCombatStance_Validate(const bool bEnable)
{
	return true;
}

void ASCKillerCharacter::SERVER_SetCombatStance_Implementation(const bool bEnable)
{
	if (bEnable && (GrabbedCounselor || BreakDownActor || bThrowingKnife || IsAttemptingGrab()))
		return;

	Super::SetCombatStance(bEnable);
}

void ASCKillerCharacter::ProcessWeaponHit(const FHitResult& Hit)
{
	// Paranoid
	ASCWeapon* Weapon = GetCurrentWeapon();
	if (!Weapon)
		return;

	// If we have a valid BreakDownActor, do damage to it directly
	FHitResult HitOverride = Hit; // Use the passed in hit as a base
	if (BreakDownActor)
	{
		HitActors.Add(BreakDownActor);
		//bWeaponHitProcessed = true; // removed so that the player can attempt to hit counselors through the breakable actor

		// Fill out an override hit to pass along to the Super. This will ensure we spawn the correct hit fx
		HitOverride.Actor = BreakDownActor;
		if (ASCDoor* Door = Cast<ASCDoor>(BreakDownActor))
		{
			HitOverride.Component = Door->Mesh;
			HitOverride.PhysMaterial = Door->Mesh->GetBodyInstance()->GetSimplePhysicalMaterial();
		}
		else if (ASCDestructableWall* Wall = Cast<ASCDestructableWall>(BreakDownActor))
		{
			HitOverride.Component = Wall->ContextMesh;
			HitOverride.PhysMaterial = Wall->ContextMesh->GetBodyInstance()->GetSimplePhysicalMaterial();
		}
		Weapon->SpawnHitEffect(HitOverride); // If we're not marking bWeaponHitProcessed we need to trigger hit effects for each Actor hit.
	}

	if (Hit.GetActor())
	{
		// Calculate damage to Counselors
		if (ASCCounselorCharacter* HitCounselor = Cast<ASCCounselorCharacter>(Hit.GetActor()))
		{
			if (HitCounselor->IsInHiding())
				return;

			if (Hit.GetComponent()->IsA<USkeletalMeshComponent>())
			{
				// Each Killer has a base damage modifier to apply to Counselors
				const float Damage = Weapon->WeaponStats.Damage * DamageCharacterModifier;

				// If we actually did damage, do some other stuff
				if (HitCounselor->TakeDamage(Damage, FPointDamageEvent(Damage, Hit, FVector::ZeroVector, Weapon->WeaponStats.WeaponDamageClass), GetCharacterController(), this) > 0.f)
				{
					// Killed the counselor! Some Killers get health back after murdering
					if (!HitCounselor->IsAlive())
					{
						//Health += MurderHealthBonus; Temporarily removing this per request. Revisiting will see it added for ALL kills.
						Health = FMath::Clamp((float)Health, 0.f, 100.f);
					}

					// Apply weapon blood
					ApplyBloodToWeapon(0, NAME_ShowWeaponBlood, 1.f);
					//bWeaponHitProcessed = true; // DGarcia: Removed so that the player can hit multiple counselors in one swing
					Weapon->SpawnHitEffect(Hit); // If we're not marking bWeaponHitProcessed we need to trigger hit effects for each counselor hit.
				}
			}
		}
		// We hit a door or window we want to apply damage to
		else if (Hit.GetActor()->IsA<ASCDoor>() || Hit.GetActor()->IsA<ASCWindow>()) // Window damage moved to interaction/special move handling
		{
			// Only deal damage if we're not locked onto this object. If we are locked on we'll automatically apply damage anyhow
			if (Hit.GetActor() != BreakDownActor)
				Hit.GetActor()->TakeDamage(Weapon->WeaponStats.Damage * DamageWorldModifier, FDamageEvent(), GetCharacterController(), this);

			//bWeaponHitProcessed = true; // DGarcia: Removed so that the player can hit a counselor after hitting a door or window
			Weapon->SpawnHitEffect(Hit); // If we're not marking bWeaponHitProcessed we need to trigger hit effects for each Actor hit.
		}
	}

	// Don't recoil if we're in a special move, it can cause interaction locking when trying to abort.
	if (!IsInSpecialMove())
	{
		// If we hit a character, check if they're blocking (also always recoil against Killers
		if (ASCCharacter* HitCharacter = Cast<ASCCharacter>(Hit.GetActor()))
		{
			if (Hit.GetComponent() && Hit.GetComponent()->IsA<USkeletalMeshComponent>())
			{
				if (HitCharacter->IsBlocking())
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
					bWeaponHitProcessed = true; // CPederson: Only stop processing if we get stuck or play the recoil
				}
				else if (ImpactAngle <= Weapon->GetMaximumRecoilAngle() || FMath::Abs(Hit.Distance) >= Weapon->GetMinimumStuckPenetration())
				{
					PlayRecoil(Hit);
					bWeaponHitProcessed = true; // CPederson: Only stop processing if we get stuck or play the recoil
				}
				else
				{
					HitActors.Remove(Hit.GetActor());
				}
			}
		}
	}

	if (bWeaponHitProcessed)
		Weapon->SpawnHitEffect(Hit);
}

void ASCKillerCharacter::Suicide()
{
	if (GrabbedCounselor)
		DropCounselor();

	Super::Suicide();
}

bool ASCKillerCharacter::CanTakeDamage() const
{
	if (!IsVisible())
		return false;

	if (IsStunned())
		return IsStunnedBySweater();

	return Super::CanTakeDamage();
}

float ASCKillerCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	check(Role == ROLE_Authority);

	// if player already is dead, get the f out of here
	if (!IsAlive() || bInContextKill)
		return 0.0f;

	if (bIsShiftActive)
		return 0.0f;

	// cant take damage, set the damage to 0
	if (!CanTakeDamage())
		Damage = 0.0f;

	GetController()->ResetIgnoreMoveInput();

	ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
	float DamageModifier = 1.0f;
	const bool bShotgunDamage = DamageEvent.DamageTypeClass == LightShotgunStun || DamageEvent.DamageTypeClass == MediumShotgunStun || DamageEvent.DamageTypeClass == HeavyShotgunStun;

	bool bCanStunBeBlocked = true;
	const USCStunDamageType* const StunDamage = DamageEvent.DamageTypeClass ? Cast<const USCStunDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;

	if (StunDamage)
		bCanStunBeBlocked = StunDamage->bCanBeBlocked;

	// attempt a block
	bool bBlockedHit = false;
	if (IsBlocking() && !IsStunned() && !bShotgunDamage && bCanStunBeBlocked)
	{
		bBlockedHit = true;
		DamageModifier = BlockedHitDamageModifier;

		if (CVarDebugCombat.GetValueOnGameThread() > 0)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s blocked attack from %s"), *GetClass()->GetName(), *DamageCauser->GetClass()->GetName()));
		}
	}

	// if the attack should stun and it wasn't blocked, apply the stun
	bool bStunned = false;
	float ActualStunDamage = 0.0f;
	if (!bBlockedHit)
	{
		if (StunDamage)
		{
			bStunned = true;
			const FVector VectorToDamageCauser = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() : FVector::ZeroVector;
			FSCStunData StunData;
			StunData.StunInstigator = DamageCauser;
			StunData.StunTimeModifier = 1.0f;

			// Check to see if this counselor had any stun modification perks
			ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(DamageCauser);
			if (Counselor)
			{
				if (ASCWeapon* CounselorWeapon = Counselor->GetCurrentWeapon())
				{
					if (ASCPlayerState* PS = Counselor->GetPlayerState())
					{
						for (const USCPerkData* Perk : PS->GetActivePerks())
						{
							check(Perk);

							if (!Perk)
								continue;

							// Check if they are using the correct weapon to apply this perk modifier
							if (Perk->RequiredWeapon && CounselorWeapon && Perk->RequiredWeapon != CounselorWeapon->GetClass())
								continue;

							// Check if this perk modifier affects us
							if (Perk->RequiredCharacterClassToHit.Get() && (!GetClass()->IsChildOf(Perk->RequiredCharacterClassToHit.Get()) && Perk->RequiredCharacterClassToHit.Get() != GetClass()))
								continue;

							StunData.StunTimeModifier += Perk->WeaponStunDurationModifier;
						}
					}
				}
			}

			BeginStun(StunDamage->GetClass(), StunData, GetHitDirection(VectorToDamageCauser));

			// Make AI noise so nearby counselors can be made aware of the stun
			MakeStealthNoise(1.f, this);

			//	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(DamageCauser))
			//		Counselor->RequestFearEvent(EFearEventType::StunnedJason);

			ActualStunDamage = AILLCharacter::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser) * StunDamageModifier;
			if (CVarDebugCombat.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s took %.2f damage from %s"), *GetName(), ActualStunDamage, DamageCauser ? *DamageCauser->GetName() : TEXT("Suicide")));
			}

			if (Counselor)
			{
				if (GameMode)
				{
					GameMode->HandleScoreEvent(Counselor->PlayerState, Counselor->StunJasonScoreEvent);
				}

				if (!bMaskOn && bStunnedBySweater && bCanStunBeBlocked && GetJasonKillClass() && Counselor->GetCurrentWeapon() != nullptr && !Counselor->GetCurrentWeapon()->IsA(ASCGun::StaticClass()))
				{
					// We got stunned while affected by the sweater, spawn the Jason kill
					FActorSpawnParameters spawnParams;
					spawnParams.Owner = this;
					spawnParams.Instigator = Instigator;

					JasonKillObject = GetWorld()->SpawnActor<ASCContextKillActor>(GetJasonKillClass(), spawnParams);
					if (JasonKillObject != nullptr)
					{
						JasonKillObject->SetActorLocation(GetActorLocation() - FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
						JasonKillObject->SetActorRotation(GetActorRotation());
						// force the kill stance animation
						MULTICAST_BeginStun(KillStanceMontage, StunDamage->GetClass(), FSCStunData(), true);
					}
				}
			}

			// Track the number of times Jason has been stunned
			if (ASCGameState_Hunt* GS = GameMode ? GameMode->GetGameState<ASCGameState_Hunt>() : nullptr)
			{
				GS->JasonStunnedCount++;
			}
		}
		// We took normal damage, pop us out of the sweater stun
		else if (IsStunnedBySweater())
		{
			EndStun();
		}
	}

	// If we are going to get our mask knocked off, don't call super
	const bool bGoingToKnockMaskOff = bMaskOn && (((Health - Damage) * DamageModifier) <= MaskPopOffThreshold);

	// Attempt to apply damage
	const float ActualDamage = bStunned ? ActualStunDamage : bGoingToKnockMaskOff ? (Damage * DamageModifier) : (Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser) * DamageModifier);
	if (ActualDamage > 0.0f)
	{
		if (ASCPlayerState* PS = GetPlayerState())
		{
			PS->TookDamage();
		}
		Health = FMath::Clamp(Health - ActualDamage, MinimumHealth, (float) GetMaxHealth());

		ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(DamageCauser);
		if (GameMode && Counselor)
		{
			GameMode->HandleScoreEvent(Counselor->PlayerState, Counselor->HitJasonScoreEvent);

			// Keep track of who damaged us
			DamageCausers.AddUnique(Counselor->PlayerState);
		}

		// Track the number of times Jason has been hit
		if (ASCGameState_Hunt* GS = GameMode ? GameMode->GetGameState<ASCGameState_Hunt>() : nullptr)
		{
			GS->JasonDamagedCount++;
		}

		if (RageUnlockTimer < RageUnlockTime)
		{
			// if Jason takes damage increase his rage meter by 5%
			RageUnlockTimer += RageUnlockTime * 0.05f;
			RageUnlockTimer = FMath::Min((float) RageUnlockTimer, RageUnlockTime);

			if (RageUnlockTimer >= RageUnlockTime)
				ActivateRage();
		}

		// if this attack killed the player
		if (!IsAlive())
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

			if (CVarDebugCombat.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("%s Killed"), *GetClass()->GetName()));
			}
		}// else if this attack wounded the player
		else if (bMaskOn && Health <= MaskPopOffThreshold)
		{
			if (MaskKnockedOffStun)
			{
				const FVector VectorToDamageCauser = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() : FVector::ZeroVector;
				FSCStunData StunData;
				StunData.StunInstigator = DamageCauser;
				BeginStun(MaskKnockedOffStun, StunData, GetHitDirection(VectorToDamageCauser));
			}

			SetMaskOn(false);

			// Spawn the mask Pickup
			if (KillerMaskPickupClass)
			{
				FActorSpawnParameters spawnParams;
				spawnParams.Owner = this;
				spawnParams.Instigator = Instigator;
				if (AActor* MaskPickup = GetWorld()->SpawnActor<AActor>(KillerMaskPickupClass, spawnParams))
				{
					if (ASCItem* MaskItem = Cast<ASCItem>(MaskPickup))
					{
						const FName MaskSocket = "maskSocket";
						MaskItem->SetActorRotation(GetActorRotation());
						MaskItem->OnDropped(this);
					}
				}
			}

			//	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(DamageCauser))
			//		Counselor->PushFearEvent(EFearEventType::KnockMaskOff);

			if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(DamageCauser))
			{
				// Play the knock the mask off VO.
				Character->MULTICAST_PlayCounselorKnockedJasonsMaskOffVO();

				if (ASCPlayerState* PS = Character->GetPlayerState())
					PS->KnockedJasonsMaskOff();
			}

			// Give everyone who has damaged us the score event for knocking the mask off.
			if (GameMode)
			{
				for (auto PS : DamageCausers)
				{
					GameMode->HandleScoreEvent(PS, KnockMaskOffScoreEvent);
				}
			}

			if (CVarDebugCombat.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Blue, FString::Printf(TEXT("%s had mask knocked off"), *GetClass()->GetName()));
			}
		}
	}

	MULTICAST_DamageTaken(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	return ActualDamage;
}

void ASCKillerCharacter::SetActorHiddenInGame(bool bNewHidden)
{
	Super::SetActorHiddenInGame(bNewHidden);

	if (GetCurrentWeapon())
		GetCurrentWeapon()->SetActorHiddenInGame(bNewHidden);
}

void ASCKillerCharacter::OnReceivedPlayerState()
{
	Super::OnReceivedPlayerState();

	ASCPlayerState* PS = GetPlayerState();
	if (Role == ROLE_Authority && PS)
	{
		// give selected weapon class to killer
		TSoftClassPtr<ASCWeapon> SelectedWeaponClass = WeaponClass;
		if (!PS->GetKillerWeapon().IsNull())
		{
			SelectedWeaponClass = PS->GetKillerWeapon();
		}

		SelectedWeaponClass.LoadSynchronous();
		if (SelectedWeaponClass.Get() && !GetCurrentWeapon())
		{
			GiveItem(SelectedWeaponClass);
		}

		// Spawn grab kills
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;

		TSubclassOf<ASCContextKillActor> DefaultKillList[GRAB_KILLS_MAX] = {nullptr};
		// copy off default list and add default weapon kill to list
		for (uint8 i(0); i<GRAB_KILLS_MAX; ++i)
		{
			DefaultKillList[i] = GrabKillClasses[i];

			// there is nothing else in the list, add the weapon kill and end the loop
			if (GrabKillClasses[i] == nullptr)
			{
				if (const ASCWeapon* const Weapon = Cast<ASCWeapon>(SelectedWeaponClass->GetDefaultObject()))
					DefaultKillList[1] = Weapon->DefaultGrabKill;
				break;
			}
		}

		const TArray<TSubclassOf<ASCContextKillActor>>& PickedGrabKills = PS->GetPickedGrabKills();
		for (int32 KillIndex = 0; KillIndex < GRAB_KILLS_MAX; ++KillIndex)
		{
			UClass* KillClass = (PickedGrabKills.IsValidIndex(KillIndex) && PickedGrabKills[KillIndex]) ? *PickedGrabKills[KillIndex] : *DefaultKillList[KillIndex];
			if (!KillClass)
				continue;

			GrabKills[KillIndex] = GetWorld()->SpawnActor<ASCDynamicContextKill>(KillClass, SpawnParams);

			// Could be null at this point, just don't.
			if (GrabKills[KillIndex])
			{
				GrabKills[KillIndex]->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				// Move the volume to the ground.
				GrabKills[KillIndex]->SetActorRelativeLocation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			}
		}

		// load killer skin
		if (PS->GetKillerSkin())
		{
			if (const USCJasonSkin* const Skin = Cast<USCJasonSkin>(PS->GetKillerSkin()->GetDefaultObject()))
			{
				Skin->MeshMaterial.LoadSynchronous(); // TODO: Make Async
				Skin->MaskMaterial.LoadSynchronous(); // TODO: Make Async
				MeshSkinOverride = Skin->MeshMaterial.Get();
				MaskSkinOverride = Skin->MaskMaterial.Get();

				NearCounselorOverrideMusic = Skin->OverrideNearCounselorMusic;

				OnRep_MeshSkin();
				OnRep_MaskSkin();
			}
		}
	}

	// Update this player's in match presence
	// HACK: Don't throw exceptions just because we're cycling characters in sandbox
	const ASCGameState_Sandbox* GS = GetWorld()->GetGameState<ASCGameState_Sandbox>();
	if (IsLocallyControlled() && !GS)
	{
		FString Presence;
		const FString MapName = GetWorld()->GetMapName();
		PS->SetRichPresence(MapName, false);
	}

	if (PS)
	{
		UserDefinedAbilityOrder.Empty((int32)EKillerAbilities::MAX);
		if (PS->HasReceivedProfile() && PS->GetPlayerLevel() >= ABILITY_UNLOCKING_LEVEL)
		{
			UserDefinedAbilityOrder = PS->GetAbilityUnlockOrder();
		}

		// Fallback/error case.
		if (UserDefinedAbilityOrder.Num() == 0)
		{
			LoadDefaultAbilityUnlockOrder();
		}
	}
}

void ASCKillerCharacter::OnInteract(UILLInteractableComponent* Interactable, EILLInteractMethod InteractMethod)
{
	if (IsGrabKilling() || IsAttemptingGrab())
		return;

	if (IsInSpecialMove() || bIsPlacingTrap)
		return;

	// Can't use things while we're busting down a wall/door
	if (BreakDownActor)
		return;

	Super::OnInteract(Interactable, InteractMethod);
}

void ASCKillerCharacter::OnRep_MaskOn()
{
	JasonSkeletalMask->SetVisibility(bMaskOn);
	OnMaskOnChanged(bMaskOn);
}

void ASCKillerCharacter::ActivateBloodlust01()
{
	if (IsAttemptingGrab())
		return;

	if (bIsMapEnabled)
		return;

	// No using abilities if we're in voyeur mode.
	if (bInVoyeurMode)
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return;
	}

	if (bIsSenseActive)
		CancelSense();

	if (IsAbilityAvailable(EKillerAbilities::Sense))
		OnSenseActive();
}

void ASCKillerCharacter::ActivateBloodlust02()
{
	if (IsAttemptingGrab() || GrabbedCounselor || IsShiftActive() || IsStunned())
		return;

	if (bIsMapEnabled && !bIsMorphActive)
		return;

	// No using abilities if we're in voyeur mode.
	if (bInVoyeurMode)
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return;
	}

	if (bIsMorphActive)
		CancelMorph();

	if (IsAbilityAvailable(EKillerAbilities::Morph))
	{
		// is map open and is it the morph map?
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			OnEnablePowersReleased();
			PC->PushInputComponent(MorphInputComponent);
			bIsMorphActive = true;
			if (bShowMorphMap && PC->bShowMouseCursor)
			{
				//begin morph.
				bActivateMorph = true;
				return;
			}

			int32 SizeX =0, SizeY = 0;
			PC->GetViewportSize(SizeX, SizeY);
			if (SizeX > KINDA_SMALL_NUMBER && SizeY > KINDA_SMALL_NUMBER)
				PC->SetMouseLocation(SizeX /2, SizeY /2);
		}
		ToggleMap_Native(true, true);
	}
}

void ASCKillerCharacter::ActivateBloodlust03()
{
	if (IsAttemptingGrab() || bIsPlacingTrap)
		return;

	if (bIsMapEnabled || bIsMorphActive)
		return;

	if (ShiftWarmUpTime > 0.0f)
		return;

	// No using abilities if we're in voyeur mode.
	if (bInVoyeurMode)
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return;
	}

	if (bIsShiftActive)
		CancelShift();

	if (IsAbilityAvailable(EKillerAbilities::Shift))
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			SERVER_SetShift(!bIsShiftActive);
		}
	}
}

void ASCKillerCharacter::ActivateBloodlust04()
{
	if (IsAttemptingGrab())
		return;

	if (bIsMapEnabled)
		return;

	// No using abilities if we're in voyeur mode.
	if (bInVoyeurMode)
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return;
	}

	if (bIsStalkActive)
		CancelStalk();

	if (IsAbilityAvailable(EKillerAbilities::Stalk))
		KillJasonMusic();
}

void ASCKillerCharacter::OnEnablePowersPressed()
{
	// make sure we cant attempt to enable powers if we are throwing a knife
	if (CurrentThrowable)
		return;

	// No using abilities if we're in voyeur mode.
	if (bInVoyeurMode)
		return;

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		if (!PC->IsGameInputAllowed())
			return;
	}

	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		PC->PushInputComponent(AbilitiesInputComponent);
		bEnabledPowers = true;
	}
}

void ASCKillerCharacter::OnEnablePowersReleased()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
	{
		PC->PopInputComponent(AbilitiesInputComponent);
		bEnabledPowers = false;
	}
}

void ASCKillerCharacter::ApplyBloodToWeapon(int32 MaterialIndex, FName MaterialParameterName, float MaterialParameterValue)
{
	if (Role == ROLE_Authority)
	{
		const bool ShouldApplyBlood = MaterialParameterValue > 0;
		if (ShouldApplyBlood != bIsWeaponBloody)
		{
			bIsWeaponBloody = ShouldApplyBlood;
			MULTICAST_ApplyBloodToWeapon(MaterialIndex, MaterialParameterName, MaterialParameterValue);
		}
	}
}

void ASCKillerCharacter::MULTICAST_ApplyBloodToWeapon_Implementation(int32 MaterialIndex, FName MaterialParameterName, float MaterialParameterValue)
{
	if (IsRunningDedicatedServer())
		return;

	auto UpdateWeaponBlood = [&MaterialIndex, &MaterialParameterName, &MaterialParameterValue](UMeshComponent* WeaponMesh)
	{
		UMaterialInstanceDynamic* WeaponMat = WeaponMesh ? WeaponMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex) : nullptr;
		if (WeaponMat)
		{
			WeaponMat->SetScalarParameterValue(MaterialParameterName, MaterialParameterValue);
		}
	};

	ASCWeapon* CurrentWeapon = GetCurrentWeapon();
	UMeshComponent* WeaponMesh = CurrentWeapon ? CurrentWeapon->GetMesh() : nullptr;
	// Update game weapon
	UpdateWeaponBlood(WeaponMesh);
	// Update anim weapon
	UpdateWeaponBlood(TempWeapon);
}

void ASCKillerCharacter::CancelShift()
{
	if (bIsShiftActive && ShiftWarmUpTime <= 0.0f)
		SERVER_SetShift(false);
}

void ASCKillerCharacter::ForceCancelShift()
{
	if (bIsShiftActive)
		SERVER_SetShift(false);
}

void ASCKillerCharacter::CancelMorph()
{
	if (bShowMorphMap)
	{
		ToggleMap_Native(false, false);
		bIsMorphActive = false;
		SERVER_OnTeleportEffect(false);

		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			PC->PopInputComponent(MorphInputComponent);
			PC->CurrentMouseCursor = PC->DefaultMouseCursor;
		}
	}
}

void ASCKillerCharacter::CancelScreenEffectAbilities()
{
	if (bIsSenseActive)
		CancelSense();

	if (bIsStalkActive)
		CancelStalk();
}

void ASCKillerCharacter::ToggleMap_Implementation(bool bShowMap, bool bIsMorphMap)
{
	// Handled in BP
}

void ASCKillerCharacter::ToggleMap_Native(bool bShowMap, bool bIsMorphMap)
{
	// Call BP
	ToggleMap(bShowMap, bIsMorphMap);

	// Toggle look control
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (bShowMap)
		{
			PC->SetIgnoreLookInput(true);
		}
		else
		{
			// Setting to false will only decrement, we want to make sure we get look control fully back
			PC->ResetIgnoreLookInput();
		}
	}
}

void ASCKillerCharacter::SetSoundBlipVisibility(bool Visible)
{
	// Not locally controlled then there shouldn't be any sound blips.
	if (!IsLocallyControlled())
		return;

	bSoundBlipsVisible = Visible;

	if (!Visible)
	{
		UWorld* World = GetWorld();

		// Destroy existing sound blips.
		for (TActorIterator<AActor> It(World, SoundBlipClass); It; ++It) // OPTIMIZE: pjackson
		{
			It->Destroy();
		}
	}
}

void ASCKillerCharacter::UpdateSoundBlips(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	const float TimeSeconds = World->GetTimeSeconds();
	ASCGameState* State = World->GetGameState<ASCGameState>();
	ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>();

	FActorSpawnParameters BlipSpawnParams;
	BlipSpawnParams.Owner = this;

	for (ASCCharacter* Character : State->PlayerCharacterList)
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
		{
			if (TimeSeconds >= Counselor->NextSoundBlipTime)
			{
				const bool bIsCounselorTalking = GameMode && GameMode->IsA<ASCGameMode_SPChallenges>() && Counselor->IsPlayingConversationVO();
				if (Counselor->IsAlive() && !Counselor->HasEscaped() && (Counselor->GetNoiseLevel() > 0.0f || bIsCounselorTalking))
				{
					const FVector Location = GetActorLocation();
					FVector CounselorLocation = Counselor->GetActorLocation();
					const float AbsoluteDistance = (CounselorLocation - Location).Size();

					if (AbsoluteDistance < MinBlipRadius)
						continue;

					const float Distance = 1.0f - FMath::Min(AbsoluteDistance / NoiseDistanceThreshold, 1.0f);
					const float NoiseLevel = (FMath::Min(((bIsCounselorTalking ? 3.f : Counselor->GetNoiseLevel()) * (bIsRageActive ? 0.5f : 1.0f)), 17.0f)) * Distance;
					if (NoiseLevel > NoiseLevelThreshold) // if noise still over threshold
					{
						// (MaxRadius * NoiseLevelMod * DistanceMod)^3 * CurveScaler The magic numbers at the end are to scale the curve (x^3 * 0.0009)
						const float Range = FMath::Min(FMath::Pow(SoundBlipMaxOffsetRadius * (1.0f - FMath::Min((NoiseLevel / 17.0f), 1.0f)) * (1.0f - Distance), 3) * 0.00009f, SoundBlipMaxOffsetRadius);
						CounselorLocation.X += FMath::FRandRange(Range * -0.5f, Range * 0.5f);
						CounselorLocation.Y += FMath::FRandRange(Range * -0.5f, Range * 0.5f);

						World->SpawnActor<AActor>(SoundBlipClass, Counselor->GetTransform(), BlipSpawnParams);

						Counselor->NextSoundBlipTime = TimeSeconds + FMath::FRandRange(MinBlipFrequencyTime, MaxBlipFrequencyTime);
					}
				}
			}
		}
	}

	for (ASCProjectile* Projectile : State->ProjectileList)
	{
		if (ASCNoiseMaker_Projectile* NoiseMaker = Cast<ASCNoiseMaker_Projectile>(Projectile))
		{
			NoiseMaker->SoundBlipFrequencyTimer -= DeltaSeconds;
			if (NoiseMaker->SoundBlipFrequencyTimer <= 0.f)
			{
				const FVector Location = GetActorLocation();
				FVector NoiseLocation = NoiseMaker->GetActorLocation();
				const float Distance = 1.0f - FMath::Min((NoiseLocation - Location).Size() / NoiseDistanceThreshold, 1.f);
				const float NoiseLevel = (FMath::Min((10.f * (bIsRageActive ? 0.5f : 1.f)), 17.f)) * Distance;
				if (NoiseLevel > NoiseLevelThreshold)
				{
					const float Range = FMath::Min(FMath::Pow(SoundBlipMaxOffsetRadius * (1.f - FMath::Min((NoiseLevel / 17.f), 1.f)) * (1.f - Distance), 3) * 0.00009f, SoundBlipMaxOffsetRadius);
					NoiseLocation.X += FMath::FRandRange(Range * -0.5f, Range * 0.5f);
					NoiseLocation.Y += FMath::FRandRange(Range * -0.5f, Range * 0.5f);

					World->SpawnActor<AActor>(SoundBlipClass, NoiseMaker->GetTransform(), BlipSpawnParams);

					NoiseMaker->SoundBlipFrequencyTimer = FMath::FRandRange(MinBlipFrequencyTime, MaxBlipFrequencyTime);
				}
			}
		}
	}

	for (TActorIterator<ASCRadio> It(World, ASCRadio::StaticClass()); It; ++It) // OPTIMIZE: pjackson
	{
		if (It)
		{
			ASCRadio* Radio = *It;

			if (Radio->IsPlaying())
			{
				Radio->SoundBlipFrequencyTimer -= DeltaSeconds;
				if (Radio->SoundBlipFrequencyTimer <= 0.f)
				{
					const FVector Location = GetActorLocation();
					FVector NoiseLocation = Radio->GetActorLocation();
					const float Distance = 1.f - FMath::Min((NoiseLocation - Location).Size() / NoiseDistanceThreshold, 1.f);
					const float NoiseLevel = (FMath::Min((10.f * (bIsRageActive ? 0.5f : 1.f)), 17.f)) * Distance;
					if (NoiseLevel > NoiseLevelThreshold)
					{
						const float Range = FMath::Min(FMath::Pow(SoundBlipMaxOffsetRadius * (1.f - FMath::Min((NoiseLevel / 17.f), 1.f)) * (1.f - Distance), 3) * 0.00009f, SoundBlipMaxOffsetRadius);
						NoiseLocation.X += FMath::FRandRange(Range * -0.5f, Range * 0.5f);
						NoiseLocation.Y += FMath::FRandRange(Range * -0.5f, Range * 0.5f);

						World->SpawnActor<AActor>(SoundBlipClass, Radio->GetTransform(), BlipSpawnParams);

						Radio->SoundBlipFrequencyTimer = FMath::FRandRange(MinBlipFrequencyTime, MaxBlipFrequencyTime);
					}
				}
			}
		}
	}
}

void ASCKillerCharacter::FillAbilities()
{
	if (Role < ROLE_Authority)
	{
		SERVER_FillAbilities();
	}
	else
	{
		for (uint8 i(0); i < (uint8) EKillerAbilities::MAX; ++i)
		{
			AbilityUnlocked[i] = true;
			AbilityRechargeTimer[i] = AbilityRechargeTime[i];
		}

		OnRep_AbilityUnlocked();
	}
}

bool ASCKillerCharacter::SERVER_FillAbilities_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_FillAbilities_Implementation()
{
	FillAbilities();
}

void ASCKillerCharacter::ActivateRage()
{
	if (Role < ROLE_Authority)
	{
		SERVER_ActivateRage();
	}
	else
	{
		bIsRageActive = true;
		RageUnlockTimer = RageUnlockTime;
		OnRep_Rage();
	}
}

bool ASCKillerCharacter::SERVER_ActivateRage_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_ActivateRage_Implementation()
{
	ActivateRage();
}

void ASCKillerCharacter::RefillKnives()
{
	if (Role < ROLE_Authority)
	{
		SERVER_RefillKnives();
	}
	else
	{
		NumKnives = 99;
	}
}

bool ASCKillerCharacter::SERVER_RefillKnives_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_RefillKnives_Implementation()
{
	RefillKnives();
}

void ASCKillerCharacter::RefillTraps()
{
	if (Role < ROLE_Authority)
	{
		SERVER_RefillTraps();
	}
	else
	{
		TrapCount = InitialTrapCount;
	}
}

bool ASCKillerCharacter::SERVER_RefillTraps_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_RefillTraps_Implementation()
{
	RefillTraps();
}

void ASCKillerCharacter::UpdateCounselorRagdolls()
{
	if (!IsLocallyControlled())
	{
		//UE_LOG(LogTemp, Error, TEXT("Update counselor Ragdolls should be Jason authoritative!"));
		return;
	}

	if (++RagdollUpdateCounter < 2)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (ASCGameState* State = World->GetGameState<ASCGameState>())
	{
		for (ASCCharacter* Character : State->PlayerCharacterList)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				if (Counselor->IsAlive())
					continue;

				SERVER_SetCounselorRagdollPosition(Counselor, Counselor->GetMesh()->GetComponentLocation());
			}
		}
	}
	RagdollUpdateCounter = 0;
}

bool ASCKillerCharacter::SERVER_SetCounselorRagdollPosition_Validate(class ASCCounselorCharacter* Character, FVector_NetQuantize NewLocation)
{
	return true;
}

void ASCKillerCharacter::SERVER_SetCounselorRagdollPosition_Implementation(class ASCCounselorCharacter* Character, FVector_NetQuantize NewLocation)
{
	MULTICAST_SetCounselorRagdollPosition(Character, NewLocation);
}

void ASCKillerCharacter::MULTICAST_SetCounselorRagdollPosition_Implementation(class ASCCounselorCharacter* Character, FVector_NetQuantize NewLocation)
{
	if (Character != nullptr && Character->GetMesh() != nullptr)
	{
		if (FVector::Dist(Character->GetMesh()->GetComponentLocation(), NewLocation) > 20.f && GrabbedCounselor != Character)
		{
			Character->GetMesh()->AddForce(((NewLocation - Character->GetMesh()->GetComponentLocation()) - Character->GetMesh()->GetPhysicsLinearVelocity()) * Character->GetMesh()->GetMass() * 20.f);
		}
	}
}

bool ASCKillerCharacter::IsValidStunArea(AActor* Attacker /*= nullptr*/)
{
	// Ignore all actors involved
	TArray<AActor*> IgnoredActors;
	GetAttachedActors(IgnoredActors);
	IgnoredActors.Add(this);

	if (Attacker)
	{
		TArray<AActor*> AttackerIgnored;
		Attacker->GetAttachedActors(AttackerIgnored);
		AttackerIgnored.Add(Attacker);

		for (AActor* Ignore : AttackerIgnored)
		{
			IgnoredActors.Add(Ignore);
		}
	}

	// Test the behind us to see if we can fall back
	const FCollisionShape StunCheckArea = FCollisionShape::MakeBox(StunCheckAreaSize);
	const FVector Start = GetActorLocation();
	const FVector End = Start + (GetActorForwardVector() * StunCheckArea.Box.HalfExtentX * -1);
	FHitResult HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActors(IgnoredActors);
	Params.bTraceComplex = true;
	if (GetWorld()->SweepSingleByChannel(HitResults, Start, End, FQuat(GetActorRotation()), ECC_Visibility, StunCheckArea, Params))
	{
		return false;
	}

	return true;
}

void ASCKillerCharacter::MULTICAST_BeginStun_Implementation(UAnimMontage* StunMontage, TSubclassOf<USCStunDamageType> StunClass, const FSCStunData& StunData, bool bIsValidStunArea)
{
	const USCStunDamageType* const StunDefaults = StunClass ? Cast<const USCStunDamageType>(StunClass->GetDefaultObject()) : nullptr;

	if (GrabbedCounselor)
	{
		if (USCAnimInstance* AnimInstance = Cast<USCAnimInstance>(GetMesh()->GetAnimInstance()))
			AnimInstance->StopGrabAnim();

		// Don't drop the counselor if it was a pocket knife stun
		if (Role == ROLE_Authority && !bPocketKnifed)
			DropCounselor();
	}

	if (IsLocallyControlled())
	{
		CancelMorph();
		ForceCancelShift();
		CancelStalk();
		CancelSense();
		// make sure the powers button is reset if Jason is stunned
		OnEnablePowersReleased();
		SetCombatStance(false);
	}

	// Play the Pamela VO that Jason got stunned or the sweater is being used against him.
	if (StunDefaults && StunDefaults->IsPamelasSweater)
	{
		static FName NAME_PamelaSweaterActive(TEXT("PamelaSweaterActive"));
		VoiceOverComponent->PlayVoiceOver(NAME_PamelaSweaterActive);
	}
	else if (StunDefaults)
	{
		static FName NAME_PamelaJasonStunned(TEXT("PamelaJasonStunned"));
		VoiceOverComponent->PlayVoiceOver(NAME_PamelaJasonStunned);
	}

	// make sure to turn vehicle collision back on just in case we were grabbing a counselor out of a vehicle when stunned
	SetVehicleCollision(true);

	bIsStunned = true;
	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		FringeIntensity = 5.0f;

		PPV->Settings.bOverride_DepthOfFieldMethod = true;
		PPV->Settings.DepthOfFieldMethod = EDepthOfFieldMethod::DOFM_Gaussian;

		PPV->Settings.bOverride_DepthOfFieldFocalDistance = true;
		PPV->Settings.DepthOfFieldFocalDistance = 2000.0f;

		PPV->Settings.bOverride_DepthOfFieldNearTransitionRegion = true;
		PPV->Settings.DepthOfFieldNearTransitionRegion = 0.0f;

		PPV->Settings.bOverride_DepthOfFieldFarTransitionRegion = true;
		PPV->Settings.DepthOfFieldFarTransitionRegion = 1000.0f;

		PPV->Settings.bOverride_DepthOfFieldNearBlurSize = true;
		PPV->Settings.DepthOfFieldNearBlurSize = 0.0f;

		PPV->Settings.bOverride_DepthOfFieldFarBlurSize = true;
		PPV->Settings.DepthOfFieldFarBlurSize = 1.0f;
	}

	Super::MULTICAST_BeginStun_Implementation(StunMontage, StunClass, StunData, bIsValidStunArea);
}


void ASCKillerCharacter::MULTICAST_EndStun_Implementation()
{
	if (JasonKillObject && !bInContextKill)
	{
		if (Role == ROLE_Authority)
		{
			JasonKillObject->InteractComponent->bIsEnabled = false;
			JasonKillObject = nullptr;
		}
	}

	bStunnedBySweater = false;
	bIsStunned = false;
	bPocketKnifed = false;
	if (APostProcessVolume* PPV = GetPostProcessVolume())
	{
		FringeIntensity = 0.0f;

		PPV->Settings.bOverride_DepthOfFieldMethod = false;
		PPV->Settings.bOverride_DepthOfFieldFocalDistance = false;
		PPV->Settings.bOverride_DepthOfFieldNearTransitionRegion = false;
		PPV->Settings.bOverride_DepthOfFieldFarTransitionRegion = false;
		PPV->Settings.bOverride_DepthOfFieldNearBlurSize = false;
		PPV->Settings.bOverride_DepthOfFieldFarBlurSize = false;
	}

	Super::MULTICAST_EndStun_Implementation();
}

bool ASCKillerCharacter::HasItemInInventory(TSubclassOf<ASCItem> ItemClass) const
{
	if (ItemClass->IsChildOf(ASCThrowingKnifeProjectile::StaticClass()))
		return NumKnives > 0;

	// Super handles the large item
	return Super::HasItemInInventory(ItemClass);
}

int32 ASCKillerCharacter::NumberOfItemTypeInInventory(TSubclassOf<ASCItem> ItemClass) const
{
	int32 Count = 0;

	if (ItemClass->IsChildOf(ASCThrowingKnifeProjectile::StaticClass()))
		Count += NumKnives;

	// Super handles the large item
	Count += Super::NumberOfItemTypeInInventory(ItemClass);

	return Count;
}

float ASCKillerCharacter::GetUnderwaterDepthModifier()
{
	const float CharacterZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Dist = FMath::Clamp(GetWaterZ() - CharacterZ, 0.f, UnderwaterFullSpeedDepth);

	return FMath::Clamp(Dist / UnderwaterFullSpeedDepth, 0.f, 1.f);
}

void ASCKillerCharacter::NativeOnUnderwaterUpdate()
{
	if (!bUnderWater)
		return;

	// Turn off combat stance when entering the water so that we can perform interactions underwater (drown-counselor)
	SetCombatStance(false);

	OnUnderWaterUpdate();

	UnderwaterInteractComponent->DistanceLimit = FVector::Dist(UnderwaterInteractComponent->GetInteractionLocation(this), GetActorLocation()) + 50.f;
	UnderwaterInteractComponent->HighlightDistance = UnderwaterInteractComponent->DistanceLimit * 2.f;

	if (GrabbedCounselor && !bInContextKill)
		DropCounselor();

	if (IsLocallyControlled())
	{
		FRotator Rotation = GetControlRotation().GetNormalized();
		Rotation.Pitch = FMath::Clamp(Rotation.Pitch, -90.f, -5.f);
		Controller->SetControlRotation(Rotation);
	}

	// Don't show our boy when he's wet and not murdering
	GetMesh()->SetHiddenInGame(!IsInSpecialMove());
	GetKillerMaskMesh()->SetHiddenInGame(!IsInSpecialMove());
	if (EquippedItem)
		EquippedItem->GetMesh()->SetHiddenInGame(!IsInSpecialMove());
}

int32 ASCKillerCharacter::CanInteractUnderwater(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// Don't re-trigger the drown while it's already happening
	if (IsInSpecialMove())
		return 0;

	// Don't allow interaction if we're not on the water's bottom
	if (GetWaterDepth() < UnderwaterFullSpeedDepth)
		return 0;

	TArray<ASCCounselorCharacter*> DrownableCounselors;
	CheckForDrownableCounselors(DrownableCounselors);

	return DrownableCounselors.Num() > 0 ? (int32) EILLInteractMethod::Press : 0;
}

void ASCKillerCharacter::OnInteractUnderwater(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	InteractableManagerComponent->UnlockInteraction(UnderwaterInteractComponent);

	// Paranoid check
	if (GetWaterDepth() < UnderwaterFullSpeedDepth)
		return;

	TArray<ASCCounselorCharacter*> DrownableCounselors;
	CheckForDrownableCounselors(DrownableCounselors);

	if (DrownableCounselors.Num() <= 0)
		return;

	float ClosestDistSq = FLT_MAX;
	ASCCounselorCharacter* ClosestCounselor = nullptr;
	for (ASCCounselorCharacter* Counselor : DrownableCounselors)
	{
		// Check that distance, son!
		float DistSq = (GetActorLocation() - Counselor->GetActorLocation()).SizeSquared();
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			ClosestCounselor = Counselor;
		}
	}

	if (ClosestCounselor)
	{
		if (UnderwaterContextKill)
		{
			UnderwaterContextKill->MULTICAST_DetachFromParent();
			//UnderwaterContextKill->SetActorLocation(FVector(ClosestCounselor->GetActorLocation().X, ClosestCounselor->GetActorLocation().Y, UnderwaterContextKill->GetActorLocation().Z));
			UnderwaterContextKill->SetActorRotation(ClosestCounselor->GetActorRotation());
			UnderwaterContextKill->ContextKillComponent->SERVER_ActivateContextKill(this, ClosestCounselor);
		}
	}
}

void ASCKillerCharacter::CheckForDrownableCounselors(TArray<ASCCounselorCharacter*>& OutDrownableCounselors)
{
	OutDrownableCounselors.Empty();
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = World->GetGameState<ASCGameState>())
		{
			for (ASCCharacter* PlayerCharacter : GS->PlayerCharacterList)
			{
				if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(PlayerCharacter))
				{
					// Don't drown dead counselors
					if (!Counselor->IsAlive())
						continue;

					// Not actually in the water could be on a dock or on a rock near water's edge
					if (Counselor->CurrentStance != ESCCharacterStance::Swimming)
						continue;

					// He's in the boat
					if (Counselor->GetCurrentSeat())
						continue;

					// He's getting in/out of the boat
					if (Counselor->PendingSeat)
						continue;

					// Do a line trace down to the ground and make sure that area is deep enough for Jason to perform a drown kill
					FHitResult HitResult;
					static const FName NAME_DrowningDepthTrace(TEXT("DrowningDepthTrace"));
					FCollisionQueryParams Params(NAME_DrowningDepthTrace, false, this);
					Params.AddIgnoredActor(Counselor);
					World->LineTraceSingleByChannel(HitResult, Counselor->GetActorLocation(), Counselor->GetActorLocation() - FVector::UpVector * 350.f, ECC_WorldDynamic, Params);
					if (!HitResult.bBlockingHit)
						continue;

					if ((GetWaterZ() - GetActorLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) < UnderwaterKillDepth)
						continue;

					// Range check
					if ((GetActorLocation() - Counselor->GetActorLocation()).SizeSquared() <= FMath::Square(UnderwaterInteractComponent->DistanceLimit))
					{
						OutDrownableCounselors.Add(Counselor);
					}
				}
			}
		}
	}
}

bool ASCKillerCharacter::ShouldAbortBreakingDown() const
{
	// Man. It'd be really nice,
	if (ASCDoor* Door = Cast<ASCDoor>(BreakDownActor))
		if (Door->IsDestroyed())
			return true;

	// if these had a common base.
	if (ASCDestructableWall* Wall = Cast<ASCDestructableWall>(BreakDownActor))
		if (Wall->IsDestroyed())
			return true;

	if (bWantsToBreakDown)
		return false;

	return true;
}

bool ASCKillerCharacter::SERVER_AbortBreakingDown_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_AbortBreakingDown_Implementation()
{
	bWantsToBreakDown = false;
}

void ASCKillerCharacter::ClearBreakDownActor()
{
	if (!BreakDownActor)
		return;

	if (UILLInteractableComponent* LockedComponent = InteractableManagerComponent->GetLockedInteractable())
	{
		if (LockedComponent->GetOwner() == BreakDownActor)
			InteractableManagerComponent->UnlockInteraction(LockedComponent, true);
	}

	BreakDownActor = nullptr;
	SERVER_SetBreakDownActor(BreakDownActor);
}

void ASCKillerCharacter::CLIENT_ClearBreakDownActor_Implementation()
{
	ClearBreakDownActor();
}

bool ASCKillerCharacter::SERVER_SetBreakDownActor_Validate(AActor* BreakActor)
{
	return true;
}

void ASCKillerCharacter::SERVER_SetBreakDownActor_Implementation(AActor* BreakActor)
{
	if (BreakDownActor == BreakActor)
		return;

	if (!BreakActor)
		CLIENT_ClearBreakDownActor();

	BreakDownActor = BreakActor;
}

template<typename TCounselorCheckFunc, typename... TVarTypes>
bool ASCKillerCharacter::ShouldInteractionTakePrecedenceOverNormalAttack(const USCInteractComponent* TestComponent, TCounselorCheckFunc CounselorCheckFunc, TVarTypes... Vars) const
{
	// Is the component in front of us?
	const FVector KillerLocation = GetActorLocation();
	FVector KillerToComponent = (TestComponent->GetComponentLocation() - KillerLocation);
	KillerToComponent.Z = 0.f;
	KillerToComponent.Normalize();
	const FVector ViewDirection = FRotator(0.f, GetViewRotation().Yaw, 0.f).Vector();
	if ((KillerToComponent | ViewDirection) < FMath::Acos(FMath::DegreesToRadians(MaxAttackInteractAngle)))
		return false;

	// Would we rather attack a near by counselor?
	const float MinDistSquared = FMath::Square(MaxAttackInteractCounselorDistance);
	ASCGameState* State = Cast<ASCGameState>(GetWorld()->GetGameState());
	for (const ASCCharacter* Character : State->PlayerCharacterList)
	{
		const ASCCounselorCharacter* Counselor = Cast<const ASCCounselorCharacter>(Character);
		if (IsValid(Counselor) == false || Counselor->IsAlive() == false)
			continue;

		if (FVector::DistSquared(KillerLocation, Counselor->GetActorLocation()) > MinDistSquared)
			continue;

		// If you are getting a linker error here, make sure you follow this template for the lambda:
		//		[](const ASCCounselorCharacter*, ...) -> bool { return true; }
		// The calling function causing the linker error will be about two down from this error
		if (CounselorCheckFunc(Counselor, Vars...) == false)
			return false;
	}

	return true;
}

bool ASCKillerCharacter::TryStartBreakingDoor(ASCDoor* Door)
{
	// Does it make sense to attack this door?
	if (Door->IsOpen() || Door->IsDestroyed() || Door->IsBeingBrokenDown()) // Last one supports more than one killer, whynot!
		return false;

	const bool bIsKillerInside = Door->IsInteractorInside(this);
	const auto DoorCheckFunc = [bIsKillerInside, Door](const ASCCounselorCharacter* Counselor) -> bool
	{
		if (bIsKillerInside == Door->IsInteractorInside(Counselor))
			return false;
		return true;
	};

	const float OldAttackAngle = MaxAttackInteractAngle;
	MaxAttackInteractAngle = 90.f;
	if (ShouldInteractionTakePrecedenceOverNormalAttack(Door->InteractComponent, DoorCheckFunc) == false)
	{
		MaxAttackInteractAngle = OldAttackAngle;
		return false;
	}

	MaxAttackInteractAngle = OldAttackAngle;
	bWantsToBreakDown = true;

	// Looks like we're good, let's go to town on it!
	SERVER_StartBreakingDownDoor(Door);

	return true;
}

bool ASCKillerCharacter::SERVER_StartBreakingDownDoor_Validate(ASCDoor* Door)
{
	return true;
}

void ASCKillerCharacter::SERVER_StartBreakingDownDoor_Implementation(ASCDoor* Door)
{
	// Might have gotten broken while we were waiting for this RPC
	if (InteractableManagerComponent->GetLockedInteractable() || CurrentThrowable || !IsValid(Door) || Door->IsDestroyed())
	{
		CLIENT_ClearBreakDownActor();
		return;
	}

	USCSpecialMoveComponent* SpecialMove = Door->IsInteractorInside(this) ? Door->BreakDownFromInside_SpecialMoveHandler : Door->BreakDownFromOutside_SpecialMoveHandler;
	SpecialMove->ActivateSpecial(this);

	Door->KillerBreakingDown = this;
	bWantsToBreakDown = true;
}

bool ASCKillerCharacter::TryStartBreakingWall(ASCDestructableWall* Wall)
{
	// Does it make sense to attack this wall?
	if (Wall->IsDestroyed() || Wall->IsBeingBrokenDown()) // Last one supports more than one killer, whynot!
		return false;

	const bool bIsKillerInside = Wall->IsInteractorInside(this);
	const auto WallCheckFunc = [bIsKillerInside, Wall](const ASCCounselorCharacter* Counselor) -> bool
	{
		if (bIsKillerInside == Wall->IsInteractorInside(Counselor))
			return false;
		return true;
	};

	if (ShouldInteractionTakePrecedenceOverNormalAttack(Wall->InteractComponent, WallCheckFunc) == false)
		return false;

	bWantsToBreakDown = true;

	// Looks like we're good, let's go to town on it!
	SERVER_StartBreakingDownWall(Wall);

	return true;
}

bool ASCKillerCharacter::SERVER_StartBreakingDownWall_Validate(ASCDestructableWall* Wall)
{
	return true;
}

void ASCKillerCharacter::SERVER_StartBreakingDownWall_Implementation(ASCDestructableWall* Wall)
{
	// Might have gotten broken while we were waiting for this RPC
	if (InteractableManagerComponent->GetLockedInteractable() || CurrentThrowable || Wall->IsDestroyed())
	{
		CLIENT_ClearBreakDownActor();
		return;
	}

	USCSpecialMoveComponent* SpecialMove = Wall->IsInteractorInside(this) ? Wall->BreakDownFromInside_SpecialMoveHandler : Wall->BreakDownFromOutside_SpecialMoveHandler;
	SpecialMove->ActivateSpecial(this);

	Wall->KillerBreakingDown = this;
	bWantsToBreakDown = true;
}

void ASCKillerCharacter::OnCharacterInView(ASCCharacter* VisibleCharacter)
{
	if (!VisibleCharacter)
		return;

	Super::OnCharacterInView(VisibleCharacter);

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(VisibleCharacter))
	{
		if (USCMusicComponent_Killer* KillerMusic = Cast<USCMusicComponent_Killer>(MusicComponent))
		{
			KillerMusic->CounselorInView(Counselor);
		}
	}
}

void ASCKillerCharacter::OnCharacterOutOfView(ASCCharacter* VisibleCharacter)
{
	if (!VisibleCharacter)
		return;

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(VisibleCharacter))
	{
		if (USCMusicComponent_Killer* KillerMusic = Cast<USCMusicComponent_Killer>(MusicComponent))
		{
			KillerMusic->CounselorOutOfView(Counselor);
		}
	}
}

void ASCKillerCharacter::MULTICAST_PlayPamelaVOFromCounselorDeath_Implementation()
{
	static const FName NAME_PamelaDeath(TEXT("PamelaCounselorDeath"));
	VoiceOverComponent->PlayVoiceOver(NAME_PamelaDeath);
	VoiceOverComponent->StartCooldown(NAME_PamelaDeath, 10.f); // Prevent double playing VO
}

void ASCKillerCharacter::KillJasonMusic()
{
	if (!bIsStalkActive)
		SERVER_KillJasonMusic();
}

bool ASCKillerCharacter::SERVER_KillJasonMusic_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_KillJasonMusic_Implementation()
{
	if (!bIsStalkActive)
	{
		bIsStalkActive = true;
		StalkActiveTimer = StalkActiveTime;
		OnRep_Stalk();
	}
}

void ASCKillerCharacter::OnRep_Stalk()
{
	if (!bIsSenseActive)
	{
		if (APostProcessVolume* PPV = GetPostProcessVolume())
		{
			PPV->Settings.ColorGradingLUT = StalkColorGrade;
			bColorGradeEnabled = bIsStalkActive;
		}
	}

	// When stalk is active we push a sound mix.
	if (IsLocallyControlled())
	{
		if (bIsStalkActive)
		{
			UGameplayStatics::PushSoundMixModifier(GetWorld(), StalkAudioSoundMix);
		}
		else
		{
			UGameplayStatics::PopSoundMixModifier(GetWorld(), StalkAudioSoundMix);
		}
	}


	if (bIsStalkActive)
	{
		PlayDistortionEffect();

		UWorld* World = GetWorld();
		if (ASCGameState* GameState = Cast<ASCGameState>(World ? World->GetGameState() : nullptr))
			GameState->KillerAbilityUsed(EKillerAbilities::Stalk);
	}
}

void ASCKillerCharacter::PlayDistortionEffect()
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld())))
	{
		ASCCounselorCharacter* Counselor = nullptr;

		if (PC->GetPawn() == nullptr)
			return;

		const bool IsCar = PC->GetPawn()->IsA(ASCDriveableVehicle::StaticClass());
		Counselor = Cast<ASCCounselorCharacter>(IsCar ? Cast<ASCDriveableVehicle>(PC->GetPawn())->Driver : PC->GetPawn());

		if (Counselor == nullptr)
			return;

		const float Distance = (Counselor->GetActorLocation() - GetActorLocation()).Size2D();
		const float NewLocDistance = (Counselor->GetActorLocation() - MorphLocation).Size2D();

		if (Distance > MaxTeleportViewDistance  && (MorphLocation == FVector::ZeroVector || NewLocDistance > MaxTeleportViewDistance))
		{
			if (Counselor->GetKillerInView() != this)
				return;
		}

		if (APostProcessVolume* PPV = Counselor->GetPostProcessVolume())
		{
			if (TeleportScreenEffectInst && TeleportScreenTimer == 0.0f)
			{
				Counselor->SetTeleportScreenEffect(TeleportScreenEffectInst);
				PPV->Settings.AddBlendable(TeleportScreenEffectInst, 1.0f);
				bHasTeleportEffectActive = true;
			}
		}

		UGameplayStatics::PlaySound2D(GetWorld(), TeleportCueCounselorHears);
		UGameplayStatics::PushSoundMixModifier(GetWorld(), TeleportCounselorHearsMix);

		/*if (Counselor->MusicComponent)
		{
		Counselor->MusicComponent->SetAdjustPitchMultiplier(0.f, 3.0f);
		}*/

		TeleportScreenTimer = TeleportScreenTime;
	}
}

void ASCKillerCharacter::CancelStalk()
{
	SERVER_CancelStalk();
}

bool ASCKillerCharacter::SERVER_CancelStalk_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_CancelStalk_Implementation()
{
	if (bIsStalkActive)
	{
		bIsStalkActive = false;
		AbilityRechargeTimer[(uint8) EKillerAbilities::Stalk] = 0.0f;
		OnRep_Stalk();
	}
}

bool ASCKillerCharacter::CanPlaceTrap(FVector& OutImpactPoint, FRotator& OutTrapRotation) const
{
	if (TrapCount <= 0)
		return false;
	
	if (IsPlacingTrap())
		return false;
	
	if (IsStunned())
		return false;
	
	if (IsAttemptingGrab())
		return false;

	if (GrabbedCounselor)
		return false;
	
	if (IsGrabKilling())
		return false;

	if (IsInSpecialMove())
		return false;
	
	if (IsPerformingThrow())
		return false;
	
	if (IsShiftActive())
		return false;
	
	if (bUnderWater)
		return false;
	
	if (!bStandingOnTerrain)
		return false;

	if (GetMesh()->GetAnimInstance()->Montage_IsPlaying(PlaceTrapMontage))
		return false;

	// Check if we would be attempting to place the trap through a wall. Only trace after everything else has succeeded.
	FCollisionQueryParams TraceParams(FName(TEXT("TrapTrace")));
	TraceParams.AddIgnoredActor(this);

	FHitResult Hit;
	const FVector TrapGroundLocation = GetActorLocation() + (GetActorForwardVector() * TrapPlacementDistance) - (FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), TrapGroundLocation, ECollisionChannel::ECC_WorldStatic, TraceParams))
	{
		if (!Hit.Actor.IsValid() || (!Hit.Actor->IsA(ALandscape::StaticClass()) && !Hit.Actor->Tags.Contains(NAME_TrapFloorTag)))
		{
			return false;
		}
	}

	// Find the ground where the trap will sit and see if it fits.
	const FVector TraceStart = GetActorLocation() + GetActorForwardVector() * TrapPlacementDistance;
	const FVector TraceEnd = TraceStart - FVector::UpVector * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 25.0f);

	for (ASCTrap* Trap : PlacedTraps)
	{
		if (Trap->GetIsArmed() && FVector::DistSquaredXY(Trap->GetActorLocation(), TraceStart) < FMath::Square(TrapPlacementDistance))
		{
			return false;
		}
	}

	if (GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeSphere(TrapRadius), TraceParams))
	{
		if (!Hit.Actor.IsValid() || (!Hit.Actor->IsA(ALandscape::StaticClass()) && !Hit.Actor->Tags.Contains(NAME_TrapFloorTag)))
		{
			return false;
		}

		OutTrapRotation = FRotationMatrix::MakeFromZX(Hit.ImpactNormal, GetActorForwardVector()).Rotator();
		OutImpactPoint = Hit.ImpactPoint;
	}

	return true;
}

bool ASCKillerCharacter::CanPlaceTrap() const
{
	FVector DummyVec;
	FRotator DummyRot;

	return CanPlaceTrap(DummyVec, DummyRot);
}

void ASCKillerCharacter::UpdateTrapPlacementTrace()
{
	bStandingOnTerrain = false;

	if (TrapCount <= 0)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_TerrainTest);
		return;
	}

	FCollisionQueryParams TraceParams(FName(TEXT("TrapTrace")));
	TraceParams.AddIgnoredActor(this);
	TraceParams.bReturnPhysicalMaterial = true;

	// Trace in front of the character to see if we'll be trying to place the trap in a wall.
	FHitResult Hit;
	const FVector TraceStart = GetActorLocation() + GetActorForwardVector() * TrapPlacementDistance;
	const FVector TraceEnd = TraceStart - FVector::UpVector * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 25.0f);
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldStatic, TraceParams))
	{
		if (Hit.Actor != nullptr)
			bStandingOnTerrain = Hit.Actor->IsA(ALandscape::StaticClass()) || Hit.Actor->Tags.Contains(NAME_TrapFloorTag);
	}
}

TSubclassOf<ASCTrap> ASCKillerCharacter::GetKillerTrapClass() const
{
	if (ASCWorldSettings* WorldSettings = Cast<ASCWorldSettings>(GetWorldSettings()))
	{
		if (WorldSettings->JasonCustomizeData.TrapClass != nullptr)
			return WorldSettings->JasonCustomizeData.TrapClass;
	}

	return KillerTrapClass;
}

void ASCKillerCharacter::AttemptPlaceTrap()
{
	if (!CanPlaceTrap())
	{
		CLIENT_FailPlaceTrap();
		return;
	}

	SERVER_AttemptPlaceTrap();
}

bool ASCKillerCharacter::SERVER_AttemptPlaceTrap_Validate()
{
	return true;
}

void ASCKillerCharacter::SERVER_AttemptPlaceTrap_Implementation()
{
	if (TrapCount <= 0)
	{
		CLIENT_FailPlaceTrap();
		return;
	}

	// Bump idle timer
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		PC->BumpIdleTime();

	FVector TrapLocation;
	FRotator TrapRotation;

	if (CanPlaceTrap(TrapLocation, TrapRotation))
	{
		TrapCount -= 1;

		FActorSpawnParameters SpawnParams = FActorSpawnParameters();
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;

		if (ASCTrap* NewTrap = GetWorld()->SpawnActor<ASCTrap>(GetKillerTrapClass(), SpawnParams))
		{
			PlacedTraps.Add(NewTrap);
			NewTrap->SetActorLocationAndRotation(TrapLocation, TrapRotation);
			NewTrap->SetTrapArmer(this);
			MULTICAST_PlaceTrap();

			// Track trap placement
			if (ASCPlayerState* PS = GetPlayerState())
			{
				PS->TrackItemUse(NewTrap->GetClass(), (uint8) ESCItemStatFlags::Used);
			}

			// Make AI noise
			MakeStealthNoise(.25f, this);

			return;
		}
	}

	CLIENT_FailPlaceTrap();
}

void ASCKillerCharacter::MULTICAST_PlaceTrap_Implementation()
{
	if (USCKillerAnimInstance* AnimInstance = Cast<USCKillerAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->Montage_Play(PlaceTrapMontage);

		bIsPlacingTrap = true;

		if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
		{
			// IgnoreMoveInput is an incremental counter, not a bool. so we want to make sure we're not setting it to true multiple times or it can break things.
			if (!PC->IsMoveInputIgnored())
				PC->SetIgnoreMoveInput(true);
		}

		FTimerDelegate EndTrapLambda;
		EndTrapLambda.BindLambda([this]()
		{
			if (IsValid(this))
			{
				if (ASCPlayerController* PC = Cast<ASCPlayerController>(Controller))
				{
					PC->SetIgnoreMoveInput(false);
				}
				bIsPlacingTrap = false;
			}
		});

		GetWorldTimerManager().SetTimer(TimerHandle_PlaceTrap, EndTrapLambda, PlaceTrapMontage->CalculateSequenceLength(), false);

	}
}

void ASCKillerCharacter::CLIENT_FailPlaceTrap_Implementation()
{
	if (IsLocallyControlled())
	{
		// play fail place noise or whatever
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Failed to place Killer Trap!"));
		bIsPlacingTrap = false;
	}
}

void ASCKillerCharacter::SetSkin(TSubclassOf<USCJasonSkin> NewSkin)
{
	// if the NewSkin is valid, extract the materials to apply
	TSubclassOf<USCMaterialOverride> MeshSkin = nullptr;
	TSubclassOf<USCMaterialOverride> MaskSkin = nullptr;
	if (NewSkin != nullptr)
	{
		if (const USCJasonSkin* const Skin = Cast<USCJasonSkin>(NewSkin->GetDefaultObject()))
		{
			Skin->MeshMaterial.LoadSynchronous(); // TODO: Make Async
			Skin->MaskMaterial.LoadSynchronous(); // TODO: Make Async
			MeshSkin = Skin->MeshMaterial.Get();
			MaskSkin = Skin->MaskMaterial.Get();
		}
	}

	// if the material has changed, set it and call the OnRep to apply it
	if (MeshSkinOverride != MeshSkin)
	{
		MeshSkinOverride = MeshSkin;
		OnRep_MeshSkin();
	}

	if (MaskSkinOverride != MaskSkin)
	{
		MaskSkinOverride = MaskSkin;
		OnRep_MaskSkin();
	}
}

void ASCKillerCharacter::OnRep_MeshSkin()
{
	// if the Material set is valid, grab the default object and apply the material to our mask
	if (MeshSkinOverride != nullptr)
	{
		USCMaterialOverride* MaterialOverride = Cast<USCMaterialOverride>(MeshSkinOverride->GetDefaultObject());
		USCMaterialOverride::ApplyMaterialOverride(GetWorld(), MaterialOverride, GetMesh());
	}
	else if (!IsRunningDedicatedServer())
	{
		// if no material set was provided, revert back to the default objects materials
		if (const ASCKillerCharacter* const Default = Cast<ASCKillerCharacter>(GetClass()->GetDefaultObject()))
		{
			for (int32 i = 0; i < Default->GetMesh()->GetNumMaterials(); ++i)
			{
				GetMesh()->CreateDynamicMaterialInstance(i, Default->GetMesh()->GetMaterial(i));
			}
		}
	}
}

void ASCKillerCharacter::OnRep_MaskSkin()
{
	// if the Material set is valid, grab the default object and apply the material to our mask
	if (MaskSkinOverride != nullptr)
	{
		USCMaterialOverride* MaterialOverride = Cast<USCMaterialOverride>(MaskSkinOverride->GetDefaultObject());
		USCMaterialOverride::ApplyMaterialOverride(GetWorld(), MaterialOverride, JasonSkeletalMask);
	}
	else if (!IsRunningDedicatedServer())
	{
		// if no material set was provided, revert back to the default objects materials
		if (const ASCKillerCharacter* const Default = Cast<ASCKillerCharacter>(GetClass()->GetDefaultObject()))
		{
			for (int32 i = 0; i < Default->JasonSkeletalMask->GetNumMaterials(); ++i)
			{
				JasonSkeletalMask->CreateDynamicMaterialInstance(i, Default->JasonSkeletalMask->GetMaterial(i));
			}
		}
	}
}

void ASCKillerCharacter::MakeStealthNoise(float Loudness/* = 1.f*/, APawn* NoiseInstigator/* = nullptr*/, FVector NoiseLocation/* = FVector::ZeroVector*/, float MaxRange/* = 0.f*/, FName Tag/* = NAME_None*/)
{
	Loudness = CalcStealthNoise(Loudness);
	StealthMadeNoiseAccumulator = FMath::Min(StealthMadeNoiseAccumulator + Loudness, StealthMadeNoiseOccurrenceLimit);
	Super::MakeStealthNoise(Loudness, NoiseInstigator, NoiseLocation, MaxRange, Tag);
}

float ASCKillerCharacter::CalcDifficultyStealth() const
{
	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;
	if (!GameMode)
		return StealthRatio;

	// Re-normalize Stealth by difficulty
	const float MinStealth = GameMode->GetCurrentDifficultyDownwardAlpha(.15f, .15f); // Minimum stealth: Hard = 0.0, Normal = 0.15, Easy = 0.3
	return FMath::Lerp(MinStealth, 1.f, StealthRatio);
}

float ASCKillerCharacter::CalcStealthNoise(const float Noise/* = 1.f*/) const
{
	const float ScaledNoise = Noise * FMath::Lerp(AntiStealthNoiseScale, StealthNoiseScale, CalcDifficultyStealth());
	return FMath::Clamp(ScaledNoise, 0.f, 1.f);
}

void ASCKillerCharacter::CounselorReceivedStimulus(const bool bVisual, const float Loudness, const bool bShared)
{
	if (bShared)
	{
		StealthIndirectlySensedAccumulator = FMath::Min(StealthIndirectlySensedAccumulator + Loudness, StealthIndirectlySensedOccurrenceLimit);
	}
	else if (bVisual)
	{
		StealthSeenAccumulator = FMath::Min(StealthSeenAccumulator + Loudness, StealthSeenOccurrenceLimit);
	}
	else
	{
		StealthHeardAccumulator = FMath::Min(StealthHeardAccumulator + Loudness, StealthHeardOccurrenceLimit);
	}
}

bool ASCKillerCharacter::IsStealthHearbleBy(ASCCounselorAIController* HeardBy, const FVector& Location, float Volume) const
{
	// Only hear killers that are not shifting
	// They already do not play footstep noises while stalking, but we do not want to ignore weapon swing noises while stalking
	return !IsShiftActive();
}

bool ASCKillerCharacter::IsStealthVisibleFor(ASCCounselorAIController* SeenBy) const
{
	// Only see killers that are not shifting
	if (IsShiftActive())
		return false;

	const float SightRadius = SeenBy->PawnSensingComponent->SightRadius;
	const float DistanceLimit = SightRadius * FMath::Lerp(AntiStealthRadiusScale, StealthRadiusScale, CalcDifficultyStealth());
	return GetSquaredDistanceTo(SeenBy->GetPawn()) <= FMath::Square(DistanceLimit);
}

void ASCKillerCharacter::UpdateStealth(const float DeltaSeconds)
{
	UWorld* World = GetWorld();
	ASCGameMode* GameMode = World ? World->GetAuthGameMode<ASCGameMode>() : nullptr;
	if (!GameMode)
		return;

	const float DifficultyMultiplier = GameMode->GetCurrentDifficultyDownwardAlpha(.25f);
	auto ProcessStealthStimulus = [&](float& SenseCounter, const float Rate) -> void
	{
		if (SenseCounter > 0.f)
		{
			SenseCounter -= DeltaSeconds * Rate * DifficultyMultiplier;
			if (SenseCounter < 0.f)
				SenseCounter = 0.f;
		}
	};

	ProcessStealthStimulus(StealthHeardAccumulator, StealthHeardDecayRate);
	ProcessStealthStimulus(StealthIndirectlySensedAccumulator, StealtIndirectlySensedDecayRate);
	ProcessStealthStimulus(StealthSeenAccumulator, StealtSeenDecayRate);
	ProcessStealthStimulus(StealthMadeNoiseAccumulator, StealthMadeNoiseDecayRate);

	const float HeardAntiStealth = (StealthHeardAccumulator / StealthHeardOccurrenceLimit) * AntiStealthHeardInfluence;
	const float IndirectlySensedAntiStealth = (StealthIndirectlySensedAccumulator / StealthIndirectlySensedOccurrenceLimit) * AntiStealthIndirectlySensedInfluence;
	const float SeenAntiStealth = (StealthSeenAccumulator / StealthSeenOccurrenceLimit) * AntiStealthSeenInfluence;
	const float MadeNoiseAntiStealth = (StealthMadeNoiseAccumulator / StealthMadeNoiseOccurrenceLimit) * AntiStealthMadeNoiseInfluence;
	StealthRatio = 1.f - FMath::Clamp(HeardAntiStealth + IndirectlySensedAntiStealth + SeenAntiStealth + MadeNoiseAntiStealth, 0.f, 1.f);
	StealthRatio = FMath::Min(StealthRatio*GameMode->JasonStealthRange, 1.f);
}

#undef LOCTEXT_NAMESPACE
