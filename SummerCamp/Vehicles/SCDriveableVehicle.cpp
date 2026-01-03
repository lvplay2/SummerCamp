// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCDriveableVehicle.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTree.h"
#include "DisplayDebugHelpers.h"

#include "ILLGameBlueprintLibrary.h"
#include "ILLPlayerInput.h"

#include "SCAIController.h"
#include "SCCounselorAIController.h"
#include "SCAudioSettingsSaveGame.h"
#include "SCCameraSplineComponent.h"
#include "SCContextKillComponent.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCDestructibleActor.h"
#include "SCEscapeVolume.h"
#include "SCGameInstance.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCLocalPlayer.h"
#include "SCNavModifierComponent_Vehicle.h"
#include "SCPerkData.h"
#include "SCPlayerController.h"
#include "SCPlayerState.h"
#include "SCProjectile.h"
#include "SCRepairPart.h"
#include "SCSpecialMoveComponent.h"
#include "SCSplineCamera.h"
#include "SCVehicleSeatComponent.h"
#include "SCVehicleStarterComponent.h"
#include "SCVoiceOverComponent.h"
#include "SCWheeledVehicleMovementComponent.h"

#define BEGINPLAY_SIMULATE_TIME 1.5f
#define MOVE_BOAT_ITERATIONS 5 // The number of times to attempt a sweep to find a good distance to move the boat for jason's boat flip.

DEFINE_LOG_CATEGORY_STATIC(LogDriveableVehicle, Warning, All);

TAutoConsoleVariable<int32> CVarDebugVehicles(TEXT("sc.DebugVehicles"), 0,
	TEXT("Displays debug info for cars and boats.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On\n"));

ASCDriveableVehicle::ASCDriveableVehicle(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<USCWheeledVehicleMovementComponent>(AWheeledVehicle::VehicleMovementComponentName))
, VehicleType(EDriveableVehicleType::Car)
, MinRestartTime(5.f)
, MaxRestartTime(5.f)
, MaxImpactAngle(28.f)
, MinLightImpactSpeed(250.f)
, MinHeavyImpactSpeed(600.f)
, MinKillCounselorSpeed(300.f)
, MinWaterDepthForSinking(150.f)
, MinUseCarDoorSpeed(50.f)
, MaxCameraYawAngle(75.f)
, MaxCameraPitchAngle(40.f)
, FlipAngleThreshold(80.f)
, CameraRotateRate(90.f, 90.f, 90.f)
, CameraRotateScale(0.5f, 0.5f)
, MinValidThrottleTime(1.5f)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateMovement = true;
	bAlwaysRelevant = true;
	MinNetUpdateFrequency = 10.f;
	NetUpdateFrequency = 50.f; // pjackson: Had to halve this from default due to being bAlwaysRelevant, saturating the network
	
	HornAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HornAudioComponent"));
	HornAudioComponent->SetupAttachment(RootComponent);

	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudioComponent"));
	EngineAudioComponent->SetupAttachment(RootComponent);

	RadioAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("RadioAudioComponent"));
	RadioAudioComponent->bAutoActivate = false;
	RadioAudioComponent->SetupAttachment(RootComponent);

	ImpactAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ImpactAudioComponent"));
	ImpactAudioComponent->bAutoActivate = false;
	ImpactAudioComponent->SetupAttachment(RootComponent);

	StartVehicleComponent = CreateDefaultSubobject<USCVehicleStarterComponent>(TEXT("StartVehicleComponent"));
	StartVehicleComponent->InteractMethods = (int32)EILLInteractMethod::Press | (int32)EILLInteractMethod::Hold;
	StartVehicleComponent->HoldTimeLimit = 5.f;
	StartVehicleComponent->bIsEnabled = false;
	StartVehicleComponent->SetupAttachment(RootComponent);

	JasonInteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("JasonInteractComponent"));
	JasonInteractComponent->InteractMethods = (int32)EILLInteractMethod::Press;
	JasonInteractComponent->bIsEnabled = false;
	JasonInteractComponent->SetupAttachment(RootComponent);

	JasonCarInteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("JasonCarInteractComponent"));
	JasonCarInteractComponent->InteractMethods = (int32)EILLInteractMethod::Press;
	JasonCarInteractComponent->bIsEnabled = false;
	JasonCarInteractComponent->SetupAttachment(RootComponent);

	CheaterKillVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CheaterKillVolume"));
	CheaterKillVolume->RelativeLocation.Set(0.f, 0.f, 250.f);
	CheaterKillVolume->SetBoxExtent(FVector(216.f, 76.f, 16.f));
	CheaterKillVolume->SetupAttachment(RootComponent);

	KillerFrontCollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("KillerFrontCollisionVolume"));
	KillerFrontCollisionVolume->RelativeLocation.Set(250.f, 0.f, 100.f);
	KillerFrontCollisionVolume->SetBoxExtent(FVector(40.f, 90.f, 100.f));
	KillerFrontCollisionVolume->SetupAttachment(RootComponent);

	KillerRearCollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("KillerRearCollisionVolume"));
	KillerRearCollisionVolume->RelativeLocation.Set(-280.f, 0.f, 100.f);
	KillerRearCollisionVolume->SetBoxExtent(FVector(40.f, 90.f, 100.f));
	KillerRearCollisionVolume->SetupAttachment(RootComponent);

	SlamSpline = CreateDefaultSubobject<USCCameraSplineComponent>(TEXT("SlamSpline"));
	SlamSpline->SetupAttachment(RootComponent);

	SlamSplineCamera = CreateDefaultSubobject<USCSplineCamera>(TEXT("SlamSplineCamera"));
	SlamSplineCamera->SetupAttachment(SlamSpline);

	NavigationModifier = CreateDefaultSubobject<USCNavModifierComponent_Vehicle>(TEXT("NavigationModifier"));

	FrontRayTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("FrontRayTraceStart"));
	FrontRayTraceStart->SetupAttachment(RootComponent);

	RearRayTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("RearRayTraceStart"));
	RearRayTraceStart->SetupAttachment(RootComponent);

	RestartTimeMod.BaseValue = 1.f;

	bDisablePhysicsOnBegin = false;
	DisablePhysicsOnBeginTimer = BEGINPLAY_SIMULATE_TIME;

	MaxAngleToKillCheaters = 30.0f;
}

void ASCDriveableVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCDriveableVehicle, bIsRepaired);
	DOREPLIFETIME(ASCDriveableVehicle, bIsStarted);
	DOREPLIFETIME(ASCDriveableVehicle, bFirstStartComplete);
	DOREPLIFETIME(ASCDriveableVehicle, Driver);
	DOREPLIFETIME(ASCDriveableVehicle, NumParts);
	DOREPLIFETIME(ASCDriveableVehicle, ThrottleVal);
	DOREPLIFETIME(ASCDriveableVehicle, SteeringVal);
	DOREPLIFETIME(ASCDriveableVehicle, bHandbrake);
	DOREPLIFETIME(ASCDriveableVehicle, bIsFrontBlockedByKiller);
	DOREPLIFETIME(ASCDriveableVehicle, bIsRearBlockedByKiller);
	DOREPLIFETIME(ASCDriveableVehicle, RestartTime);
	DOREPLIFETIME(ASCDriveableVehicle, bIsDisabled);
	DOREPLIFETIME(ASCDriveableVehicle, bJasonFlippingBoat);
}

void ASCDriveableVehicle::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetMesh()->OnComponentBeginOverlap.AddDynamic(this, &ASCDriveableVehicle::OnMeshOverlapBegin);
	GetMesh()->OnComponentEndOverlap.AddDynamic(this, &ASCDriveableVehicle::OnMeshOverlapEnd);
	GetMesh()->OnComponentHit.AddDynamic(this, &ASCDriveableVehicle::OnVehicleHit);

	StartVehicleComponent->OnInteract.AddDynamic(this, &ASCDriveableVehicle::OnInteractStartVehicle);
	StartVehicleComponent->OnHoldStateChanged.AddDynamic(this, &ASCDriveableVehicle::OnStartVehicleHoldChanged);
	StartVehicleComponent->OnCanInteractWith.BindDynamic(this, &ASCDriveableVehicle::CanInteractWithStartVehicle);

	JasonInteractComponent->OnInteract.AddDynamic(this, &ASCDriveableVehicle::OnJasonInteract);
	JasonInteractComponent->OnCanInteractWith.BindDynamic(this, &ASCDriveableVehicle::CanJasonInteract);

	JasonCarInteractComponent->OnInteract.AddDynamic(this, &ASCDriveableVehicle::OnJasonDestroyCar);
	JasonCarInteractComponent->OnCanInteractWith.BindDynamic(this, &ASCDriveableVehicle::CanJasonDestroyCar);

	// Loop through repair components and subscribe to its OnInteract
	// and keep a tally of the required parts
	NumRequiredParts = 0;
	TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
	for (UActorComponent* Comp : RepairPoints)
	{
		// Skip the vehicle start interactable
		if (Comp == StartVehicleComponent)
		{
			continue;
		}

		if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
		{
			RepairPoint->OnInteract.AddDynamic(this, &ASCDriveableVehicle::Repair);
			NumRequiredParts += RepairPoint->GetNumRequiredParts();
		}
	}

	Seats.Empty();
	TArray<UActorComponent*> SeatComps = GetComponentsByClass(USCVehicleSeatComponent::StaticClass());
	for (UActorComponent* Comp : SeatComps)
	{
		if (USCVehicleSeatComponent* SeatComp = Cast<USCVehicleSeatComponent>(Comp))
		{
			Seats.Add(SeatComp);
		}
	}

	TArray<UActorComponent*> CapsuleComps = GetComponentsByClass(UCapsuleComponent::StaticClass());
	for (UActorComponent* Comp : CapsuleComps)
	{
		if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Comp))
		{
			ExitCapsules.Add(Capsule);
		}
	}

	CheaterKillVolume->OnComponentBeginOverlap.AddDynamic(this, &ASCDriveableVehicle::OnCheaterVolumeOverlapBegin);

	KillerFrontCollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ASCDriveableVehicle::OnKillerFrontCollisionVolumeOverlapBegin);
	KillerFrontCollisionVolume->OnComponentEndOverlap.AddDynamic(this, &ASCDriveableVehicle::OnKillerFrontCollisionVolumeOverlapEnd);

	KillerRearCollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ASCDriveableVehicle::OnKillerRearCollisionVolumeOverlapBegin);
	KillerRearCollisionVolume->OnComponentEndOverlap.AddDynamic(this, &ASCDriveableVehicle::OnKillerRearCollisionVolumeOverlapEnd);

	CheaterKillVolume->SetCollisionEnabled(bIsStarted ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void ASCDriveableVehicle::BeginPlay()
{
	Super::BeginPlay();

	OpenHood();

	DefaultMaxEngineRPM = GetVehicleMovement()->GetEngineMaxRotationSpeed();
	GetVehicleMovement()->StopMovementImmediately();

	bDisablePhysicsOnBegin = Controller == nullptr;

	// Shuffle the songs like a pleb
	if (Role == ROLE_Authority)
	{
		if (Songs.Num() > 0)
		{
			CurrentSong = FMath::RandHelper(Songs.Num());
		}
	}
}

void ASCDriveableVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up timers
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_SlamStall);
		GetWorldTimerManager().ClearTimer(TimerHandle_LocalSlamFinish);
		GetWorldTimerManager().ClearTimer(TimerHandle_SlamFinish);
		GetWorldTimerManager().ClearTimer(TimerHandle_OnEscaped);

		GetWorldTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCDriveableVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USCWheeledVehicleMovementComponent* VM = Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement());
	if (FMath::Abs(VM->GetThrottleInput()) > 0.f)
	{
		if (VM->GetForwardSpeed() < GetMinUseCarDoorSpeed())
		{
			ValidThrottleTime += DeltaSeconds;
		}
		else
		{
			ValidThrottleTime = 0.f;
		}
	}

	if (bDisablePhysicsOnBegin)
	{
		DisablePhysicsOnBeginTimer -= DeltaSeconds;
		if (DisablePhysicsOnBeginTimer < 0.0f)
		{
			MULTI_SetPhysicsState(0.0f);
			GetVehicleMovement()->StopMovementImmediately();
			bDisablePhysicsOnBegin = false;
		}
	}

	CachedSpeed = GetVehicleMovement()->GetForwardSpeed();

	if (Role == ROLE_Authority)
	{
		// See if we need to kill anyone in front of the car
		if (!IsBeingSlammed())
		{
			if (CachedSpeed >= MinKillCounselorSpeed)
			{
				for (int32 iCounselor(0); iCounselor < CounselorsInFrontOfCar.Num(); ++iCounselor)
				{
					ASCCounselorCharacter* Counselor = CounselorsInFrontOfCar[iCounselor];
					if (!Counselor->IsAlive())
					{
						CounselorsInFrontOfCar.RemoveAt(iCounselor--);
					}
					else
					{
						Counselor->TakeDamage(100.f, FDamageEvent(KillCounselorDamageType), Controller, this);
						PlayImpactSound(HeavyImpactCue, Counselor->GetActorLocation());
					}
				}
			}
			// See if we need to kill anyone behind the car
			else if (CachedSpeed <= -MinKillCounselorSpeed)
			{
				for (int32 iCounselor(0); iCounselor < CounselorsBehindCar.Num(); ++iCounselor)
				{
					ASCCounselorCharacter* Counselor = CounselorsBehindCar[iCounselor];
					if (!Counselor->IsAlive())
					{
						CounselorsBehindCar.RemoveAt(iCounselor--);
					}
					else
					{
						Counselor->TakeDamage(100.f, FDamageEvent(KillCounselorDamageType), Controller, this);
						PlayImpactSound(HeavyImpactCue, Counselor->GetActorLocation());
					}
				}
			}
		}

		if (ExitSpline)
		{
			UpdateAIMovement(DeltaSeconds);
		}

		if (CurrentWaterVolume)
		{
			const FVector VehicleLocation = GetActorLocation();

			// Grab the Z of the top of the water volume
			const float WaterZ = CurrentWaterVolume->GetActorLocation().Z + CurrentWaterVolume->GetSimpleCollisionHalfHeight();
				
			// See if we're below the top of the water volume +/- an offset
			bool bIsInDeepWater = (VehicleLocation.Z + MinWaterDepthForSinking) < WaterZ;

			if (!bIsInDeepWater)
			{
				if (VehicleType == EDriveableVehicleType::Boat)
				{
					// Let's skip over false positives (the boat could collide with a rock in the water and pop out above our Min Water Depth
					TArray<FHitResult> Hits;
					GetWorld()->LineTraceMultiByChannel(Hits, GetActorLocation(), GetActorLocation() - FVector::UpVector * 150.f, ECC_Boat, FCollisionQueryParams(TEXT("BoatTrace"), false, this), FCollisionResponseParams());
					for (const FHitResult& Hit : Hits)
					{
						if (Hit.Actor.Get() == CurrentWaterVolume)
						{
							bIsInDeepWater = true;
							break;
						}
					}
				}
			}

			// If we're a boat and not in deep water, shut off. If we're a car and in deep water, shut off.
			if ((VehicleType == EDriveableVehicleType::Boat && !bIsInDeepWater) || (VehicleType == EDriveableVehicleType::Car && bIsInDeepWater))
			{
				PlayImpactSound(WaterImpactCue, GetActorLocation());

				for (USCVehicleSeatComponent* Seat : Seats)
				{
					if (Seat->CounselorInSeat)
					{
						EjectCounselor(Seat->CounselorInSeat, true);
					}

					Seat->bIsEnabled = false;
				}

				SetStarted(false);
				StartVehicleComponent->bIsEnabled = false; // Hard shut this off (SetStarted turns it back on)
				SetActorTickEnabled(false);
				bIsDisabled = true;

				// If a car, hide it and disable collision
				if (VehicleType == EDriveableVehicleType::Car)
				{
					SetActorHiddenInGame(true);
					SetActorEnableCollision(ECollisionEnabled::NoCollision);
				}
			}
		}

		const float CurrentMaxRPM = GetVehicleMovement()->GetEngineMaxRotationSpeed();
		if (Driver)
		{
			// Apply any speed perks the driver might have
			ASCCounselorCharacter* CounselorDriver = Cast<ASCCounselorCharacter>(Driver);
			if (ASCPlayerState* PS = CounselorDriver ? Cast<ASCPlayerState>(PlayerState) : nullptr)
			{
				float RPMModifier = 0.0f;
				for (const USCPerkData* Perk : PS->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					if (Perk->GetClass() == CounselorDriver->SpeedDemonPerk && GetNumberOfPeopleInVehicle() == 1)
					{
						RPMModifier += Perk->SoloEscapeVehicleSpeedModifier;
					}

					if (VehicleType == EDriveableVehicleType::Boat)
					{
						RPMModifier += Perk->BoatSpeedModifier;
					}
					else if (VehicleType == EDriveableVehicleType::Car)
					{
						RPMModifier += Perk->CarSpeedModifier;
					}
				}

				float DesiredMaxEngineRPM = DefaultMaxEngineRPM + DefaultMaxEngineRPM * RPMModifier;

				if (DesiredMaxEngineRPM != CurrentMaxRPM)
				{
					Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement())->UpdateMaxRPM(DesiredMaxEngineRPM);
				}

				if (CVarDebugVehicles.GetValueOnGameThread() > 0)
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.f, FColor::Red, FString::Printf(TEXT("Max RPM: %.1f --- Default RPM: %.1f --- Modifier: %.1f --- Throttle: %.2f"),
						DesiredMaxEngineRPM, DefaultMaxEngineRPM, RPMModifier, Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement())->GetThrottleInput()));
				}
			}
		}
		// Reset Max RPM
		else if (CurrentMaxRPM != DefaultMaxEngineRPM)
		{
			Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement())->UpdateMaxRPM(DefaultMaxEngineRPM);
		}

		float Dot = GetActorUpVector() | FVector::UpVector;

		if (Dot <= FMath::Cos(FMath::DegreesToRadians(FlipAngleThreshold)))
		{
			for (USCVehicleSeatComponent* Seat : Seats)
			{
				if (Seat->CounselorInSeat)
				{
					EjectCounselor(Seat->CounselorInSeat, true);
				}

				Seat->bIsEnabled = false;
			}

			SetStarted(false);
			MULTI_SetPhysicsState(1.0f);
			StartVehicleComponent->bIsEnabled = false; // Hard shut this off (SetStarted turns it back on)
			SetActorTickEnabled(false);
			bIsDisabled = true;
		}
	}

	if (IsLocallyControlled())
	{
		if (Controller)
		{
			FRotator NewRotation = GetActorRotation();
			NewRotation.Yaw += LocalYaw;
			NewRotation.Pitch += LocalPitch;
			NewRotation.Roll = 0.f;
			Controller->SetControlRotation(NewRotation);
		}
	}

	// Update some sounds!
	if (UWheeledVehicleMovementComponent* Movement = GetVehicleMovement())
	{
		if (bIsStarted)
		{
			static const FName NAME_RPM = TEXT("RPM");
			EngineAudioComponent->SetFloatParameter(NAME_RPM, Movement->GetEngineRotationSpeed());
		}
	}

	if (CVarDebugVehicles.GetValueOnGameThread() > 0)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector MassLocation = GetMesh()->GetCenterOfMass();
			DrawDebugSphere(World, MassLocation, 20.f, 8, FColor::Cyan);
			GEngine->AddOnScreenDebugMessage(uint64(-1), 0.f, FColor::Cyan, FString::Printf(TEXT("Center of Mass - X: %.2f, Y: %.2f, Z: %.2f"), MassLocation.X, MassLocation.Y, MassLocation.Z));

			const FVector ActorLocation = GetActorLocation();
			DrawDebugSphere(World, GetActorLocation(), 20, 8, FColor::Green);
			GEngine->AddOnScreenDebugMessage(uint64(-1), 0.f, FColor::Green, FString::Printf(TEXT("Car Location - X: %.2f, Y: %.2f, Z: %.2f"), ActorLocation.X, ActorLocation.Y, ActorLocation.Z));
		}
	}
}

void ASCDriveableVehicle::GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const
{
	if (USkeletalMeshComponent* VehicleMesh = GetMesh())
	{
		CollisionRadius = VehicleMesh->Bounds.BoxExtent.Y;
		CollisionHalfHeight = 500.f;
	}
}

void ASCDriveableVehicle::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	if (IsLocallyControlled())
	{
		static const FName NAME_DebugVehicleTransmission = TEXT("VehicleTransmission");
		if (DebugDisplay.IsDisplayOn(NAME_DebugVehicleTransmission))
		{
			if (UWheeledVehicleMovementComponent* VM = GetVehicleMovement())
			{
				const UFont* SmallFont = GEngine->GetSmallFont();
				Canvas->SetDrawColor(FColor::Cyan);

				YPos += YL;
				Canvas->DrawText(SmallFont, FString::Printf(TEXT("Current Gear: %d"), VM->GetCurrentGear()), 4.f, YPos);

				YPos += YL;
				Canvas->DrawText(SmallFont, FString::Printf(TEXT("Current RPM: %.2f"), VM->GetEngineRotationSpeed()), 4.f, YPos);

				YPos += YL;
				Canvas->DrawText(SmallFont, FString::Printf(TEXT("Max RPM: %.2f"), VM->GetEngineMaxRotationSpeed()), 4.f, YPos);

				YPos += YL;
				Canvas->DrawText(SmallFont, FString::Printf(TEXT("Current Speed: %.2f"), VM->GetForwardSpeed()), 4.f, YPos);
			}
		}
	}
}

void ASCDriveableVehicle::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	InInputComponent->BindAxis(TEXT("GLB_VHCL_Forward"), this, &ASCDriveableVehicle::OnForward);
	InInputComponent->BindAxis(TEXT("GLB_VHCL_Reverse"), this, &ASCDriveableVehicle::OnReverse);
	InInputComponent->BindAxis(TEXT("GLB_VHCL_Steer"), this, &ASCDriveableVehicle::SetSteering);

	InInputComponent->BindAxis(TEXT("LookUp"), this, &ASCDriveableVehicle::AddControllerPitchInput);
	InInputComponent->BindAxis(TEXT("Turn"), this, &ASCDriveableVehicle::AddControllerYawInput);

	InInputComponent->BindAction(TEXT("VHCL_Brake"), IE_Pressed, this, &ASCDriveableVehicle::OnHandbrakeDown);
	InInputComponent->BindAction(TEXT("VHCL_Brake"), IE_Released, this, &ASCDriveableVehicle::OnHandbrakeUp);

	InInputComponent->BindAction(TEXT("VHCL_RearView"), IE_Pressed, this, &ASCDriveableVehicle::OnLookBehindPressed);
	InInputComponent->BindAction(TEXT("VHCL_RearView"), IE_Released, this, &ASCDriveableVehicle::OnLookBehindReleased);

	InInputComponent->BindAction(TEXT("VHCL_Horn"), IE_Pressed, this, &ASCDriveableVehicle::StartHorn);
	InInputComponent->BindAction(TEXT("VHCL_Horn"), IE_Released, this, &ASCDriveableVehicle::StopHorn);

	InInputComponent->BindAction(TEXT("GLB_Interact1"), IE_Released, this, &ASCDriveableVehicle::OnDriverExitVehicle);
	InInputComponent->BindAction(TEXT("GLB_Map"), IE_Pressed, this, &ASCDriveableVehicle::ToggleDriverMap);

}

void ASCDriveableVehicle::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	MULTI_SetPhysicsState(1.0f);
}

void ASCDriveableVehicle::UnPossessed()
{
	Super::UnPossessed();

	Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement())->FullStop();

	MULTI_SetPhysicsState(0.0f);

	StopHorn();
}

void ASCDriveableVehicle::AddControllerYawInput(float Val)
{
	if (Val)
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			UILLPlayerInput* PlayerInput = Cast<UILLPlayerInput>(PC->PlayerInput);
			float YawRotation = Val * PC->InputYawScale * CameraRotateScale.Y;
			if (PlayerInput && PlayerInput->IsUsingGamepad())
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

			LocalYaw += YawRotation;
			if (VehicleType == EDriveableVehicleType::Car)
				LocalYaw = FMath::Clamp(LocalYaw , -MaxCameraYawAngle, MaxCameraYawAngle);
		}
	}
}

void ASCDriveableVehicle::AddControllerPitchInput(float Val)
{
	if (Val)
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			UILLPlayerInput* PlayerInput = Cast<UILLPlayerInput>(PC->PlayerInput);
			float PitchRotation = Val * PC->InputPitchScale * CameraRotateScale.X;
			if (PlayerInput && PlayerInput->IsUsingGamepad())
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

			LocalPitch += PitchRotation;
			if (VehicleType == EDriveableVehicleType::Car)
				LocalPitch = FMath::Clamp(LocalPitch, -MaxCameraPitchAngle, MaxCameraPitchAngle);
			else if (VehicleType == EDriveableVehicleType::Boat)
				LocalPitch = FMath::Clamp(LocalPitch, -90.f, -5.f);
		}
	}
}

void ASCDriveableVehicle::UpdateNavigationRelevance()
{
	if (GetMesh())
	{
		GetMesh()->SetCanEverAffectNavigation(bCanAffectNavigationGeneration);
	}
}

void ASCDriveableVehicle::MULTI_SetPhysicsState_Implementation(float PhysicsBlend)
{
	GetMesh()->SetPhysicsBlendWeight(PhysicsBlend);
}

void ASCDriveableVehicle::Repair(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	if (InteractMethod != EILLInteractMethod::Hold)
		return;

	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
	if (!Counselor)
		return;

	// Unlock all interactions
	TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
	for (UActorComponent* Comp : RepairPoints)
	{
		if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
		{
			Counselor->GetInteractableManagerComponent()->UnlockInteraction(RepairPoint);
		}
	}

	if (!bIsRepaired)
	{
		NumParts++;
		PlayVO(TEXT("AppliedPart"), Counselor);

		ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
		if (GameMode)
		{
			GameMode->HandleScoreEvent(Counselor->PlayerState, PartRepairedScoreEvent);
		}

		if (ASCPlayerState* PS = Counselor->GetPlayerState())
		{
			if (VehicleType == EDriveableVehicleType::Boat)
				PS->BoatPartRepaired();
			else if (VehicleType == EDriveableVehicleType::Car)
				PS->CarPartRepaired();
		}

		// Keep track of everyone who has repaired this vehicle
		VehicleRepairers.AddUnique(Counselor->PlayerState);

		if (Counselor->IsAbilityActive(EAbilityType::Repair))
		{
			Counselor->RemoveAbility(EAbilityType::Repair);
		}

		if (NumParts >= NumRequiredParts)
		{
			SetRepaired(true);

			if (GameMode)
			{
				// Award fully repaired points to everyone that helped repair this vehicle
				for (APlayerState* PS : VehicleRepairers)
				{
					GameMode->HandleScoreEvent(PS, VehicleRepairedScoreEvent);
				}
			}
		}
	}
}

void ASCDriveableVehicle::OnInteractStartVehicle(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
	if (!Counselor)
		return;

	Counselor->GetInteractableManagerComponent()->UnlockInteraction(StartVehicleComponent);

	if (Counselor->IsInSpecialMove())
		return;

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		// Exit the vehicle
		OnDriverExitVehicle();
		break;

	case EILLInteractMethod::Hold:
		// Start the car, item use is handled in the component itself
		StartVehicle(Counselor);
		break;
	}
}

void ASCDriveableVehicle::OnStartVehicleHoldChanged(AActor* Interactor, EILLHoldInteractionState NewState)
{
	// Intentionally left empty
	switch (NewState)
	{
		case EILLHoldInteractionState::Interacting:
		{
			if (StartVehicleMontage)
				MULTICAST_PlayAnimMontage(StartVehicleMontage);
		}
		break;

		case EILLHoldInteractionState::Canceled:
		case EILLHoldInteractionState::Success:
		{
			if (StartVehicleMontage)
				MULTICAST_StopAnimMontage(StartVehicleMontage);
		}
		break;
	}
}

int32 ASCDriveableVehicle::CanInteractWithStartVehicle(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// Just in case someone is stuck in the car, make sure they can't start it unless they're actually driving it
	if (Driver != Character)
		return 0;

	// Also, can't start the boat if it's currently being flipped.
	if (bJasonFlippingBoat)
		return 0;

	return StartVehicleComponent->InteractMethods;
}

void ASCDriveableVehicle::OnJasonInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor);
	if (!Killer)
		return;

	// Block the interaction here in cases of high lag (CanInteractWith may have incomplete data on the client, this hits on the Server)
	if (CounselorsInteractingWithVehicle.Num() > 0)
	{
		Killer->GetInteractableManagerComponent()->UnlockInteraction(JasonInteractComponent);
		return;
	}

	switch (InteractMethod)
	{
		case EILLInteractMethod::Press:
		{
			USCSpecialMoveComponent* SpecialMove = nullptr;
			TArray<USceneComponent*> ChildComponents;
			JasonInteractComponent->GetChildrenComponents(false, ChildComponents);
			for (USceneComponent* Component : ChildComponents)
			{
				if (Component->IsA<USCSpecialMoveComponent>())
				{
					SpecialMove = Cast<USCSpecialMoveComponent>(Component);
					break;
				}
			}

			if (SpecialMove)
			{
				// Make sure the special move is on the side Jason pressed interact on
				const FVector BoatRight = GetActorRightVector();
				const FVector DirToKiller = (GetActorLocation() - Killer->GetActorLocation()).GetSafeNormal2D();
				const FVector DirToInteract = (GetActorLocation() - SpecialMove->GetComponentLocation()).GetSafeNormal2D();
				FVector DesiredInteractLocation = SpecialMove->RelativeLocation;
				FRotator DesiredInteractRotation = SpecialMove->RelativeRotation;
				const float RightDot = FVector::DotProduct(BoatRight, DirToKiller);
				const float InteractDot = FVector::DotProduct(BoatRight, DirToInteract);
				if (RightDot < 0.f)
				{
					// Right side
					if (InteractDot > 0.f)
					{
						// Move the special move to the right side of the boat
						DesiredInteractLocation.Y *= -1.f;
						DesiredInteractRotation.Yaw = 180.f;
						SpecialMove->SetRelativeLocationAndRotation(DesiredInteractLocation, DesiredInteractRotation.Quaternion());
					}
				}
				else
				{
					// Left Side
					if (InteractDot < 0.f)
					{
						// Move the special move to the left side of the boat
						DesiredInteractLocation.Y *= -1.f;
						DesiredInteractRotation.Yaw = 0.f;
						SpecialMove->SetRelativeLocationAndRotation(DesiredInteractLocation, DesiredInteractRotation.Quaternion());
					}
				}

				// Check if the boat is too close to a wall, if so we need to move it.
				static const FName NAME_BoatFlipSpace(TEXT("BoatFlipSpaceTrace"));
				FCollisionQueryParams QueryParams(NAME_BoatFlipSpace, false, this);
				QueryParams.AddIgnoredActor(Killer);

				FVector BoatLocation = JasonInteractComponent->GetComponentLocation();
				const FVector SpecialMoveLocation = SpecialMove->GetComponentLocation() + (FVector::UpVector * 5.0f); // We're pulling the trace up a smidgeon so that we're not tracing right in the terrain.
				BoatLocation.Z = SpecialMoveLocation.Z;

				const FVector TraceDirection = SpecialMoveLocation - BoatLocation;

				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByChannel(Hit, BoatLocation, BoatLocation + TraceDirection, ECollisionChannel::ECC_WorldStatic, QueryParams))
				{
					const FVector TraceNormal = TraceDirection.GetSafeNormal();

					// If our component is already in a safe area no need to move the boat.
					if (FVector::DotProduct(Hit.ImpactNormal, TraceNormal) < 0.f)
					{
						const float DistanceToMove = (Hit.ImpactPoint - SpecialMoveLocation).Size() + Killer->GetCapsuleComponent()->GetScaledCapsuleRadius();

						bool foundValidPosition = false;

						// Attempt to incrementally move the boat outwards from the obstruction. 
						for (uint8 i(0); i < MOVE_BOAT_ITERATIONS; ++i)
						{
							if (SetActorLocation(GetActorLocation() - (TraceNormal * DistanceToMove + (i * 400.f)), true))
							{
								foundValidPosition = true;
								break;
							}
						}

						// Still bad positioning? Then don't allow the interaction, as Jason can fall through the world's floor in some cases
						if (!foundValidPosition)
						{
							Killer->GetInteractableManagerComponent()->UnlockInteraction(JasonInteractComponent);
							return;
						}
					}
				}
				
				// Moved this under the test since there's a potential return up there.
				if (Driver && bIsStarted)
				{
					if (Driver->GetOwner() != nullptr)
					{
						if (Controller)
							Controller->Possess(Cast<ASCCounselorCharacter>(Driver));

						Driver->SetOwner(nullptr);
					}
				}

				Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement())->FullStop();
				MULTI_SetPhysicsState(0.0f);
				SpecialMove->SpecialComplete.AddUniqueDynamic(this, &ASCDriveableVehicle::OnJasonInteractSpecialMoveComplete);

				SpecialMove->ActivateSpecial(Killer);
				bJasonFlippingBoat = true;
			}
		}
		break;
	}
}

int32 ASCDriveableVehicle::CanJasonDestroyCar(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character);
	if (!Killer)
		return 0;

	if (Killer->GetGrabbedCounselor())
		return 0;

	if (Killer->IsInSpecialMove())
		return 0;

	if (bIsDisabled)
		return 0;

	if (FMath::Abs(GetVehicleMovement()->GetForwardSpeed()) > KillerSlamSpeed)
		return 0;

	if (IsBeingSlammed())
		return false;

	// Check if Jason will fit at the location of the interact
	USCSpecialMoveComponent* SpecialMove = nullptr;
	TArray<USceneComponent*> ChildComponents;
	JasonInteractComponent->GetChildrenComponents(false, ChildComponents);
	for (USceneComponent* Component : ChildComponents)
	{
		if (Component->IsA<USCSpecialMoveComponent>())
		{
			SpecialMove = Cast<USCSpecialMoveComponent>(Component);
			break;
		}
	}

	if (SpecialMove)
	{

	}

	return JasonCarInteractComponent->InteractMethods;
}

void ASCDriveableVehicle::OnJasonDestroyCar(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
		{
			StartSlamJam(Killer);
			Killer->GetInteractableManagerComponent()->UnlockInteraction(JasonCarInteractComponent);
		}
		break;
	}
}

void ASCDriveableVehicle::OnJasonInteractSpecialMoveComplete(ASCCharacter* Interactor)
{
	SetStarted(false);
	if (Interactor)
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(JasonInteractComponent);

	// Reset the roll and pitch
	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = 0.f;
	CurrentRotation.Roll = 0.f;
	SetActorRotation(CurrentRotation);

	// Snap to the boat floor
	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() - FVector::UpVector * 200.f, ECC_Boat))
		{
			SetActorLocation(Hit.Location);
		}
	}

	// grant badge for flipping boat
	if (Interactor)
	{
		if (ASCPlayerState* PS = Interactor->GetPlayerState())
		{
			PS->EarnedBadge(BoatFlipBadge);
		}
	}

	bJasonFlippingBoat = false;
}

int32 ASCDriveableVehicle::CanJasonInteract(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character);
	if (!Killer)
		return 0;

	if (!Killer->bUnderWater)
		return 0;

	if (CounselorsInteractingWithVehicle.Num() > 0)
		return 0;

	if (GetNumberOfPeopleInVehicle() == 0)
		return 0;

	if (bEscaping)
		return 0;

	USCSpecialMoveComponent* SpecialMove = nullptr;
	TArray<USceneComponent*> ChildComponents;
	JasonInteractComponent->GetChildrenComponents(false, ChildComponents);
	for (USceneComponent* Component : ChildComponents)
	{
		if (Component->IsA<USCSpecialMoveComponent>())
		{
			SpecialMove = Cast<USCSpecialMoveComponent>(Component);
			break;
		}
	}

	if (SpecialMove)
	{
		// Check if the boat is too close to a wall, if so we need to move it.
		static const FName NAME_BoatFlipSpace(TEXT("BoatFlipSpaceTrace"));
		FCollisionQueryParams QueryParams(NAME_BoatFlipSpace, false, this);
		QueryParams.AddIgnoredActor(Killer);

		FVector BoatLocation = GetActorLocation();
		BoatLocation.Z = SpecialMove->GetComponentLocation().Z;

		const FVector TraceDirection = SpecialMove->GetComponentLocation() - BoatLocation;
		const float TraceSize = TraceDirection.Size();

		FHitResult Hit;

		// Trace from the actual boat to each end point of the final trace, if both hit terrain we're too close to shore for the flip.
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), BoatLocation + TraceDirection, ECollisionChannel::ECC_WorldStatic, QueryParams))
		{
			if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), BoatLocation - TraceDirection, ECollisionChannel::ECC_WorldStatic, QueryParams))
			{
				// We hit something both ways, cancel.
				return 0;
			}
		}
	}

	return JasonInteractComponent->InteractMethods;
}

void ASCDriveableVehicle::StartVehicle(ASCCounselorCharacter* Counselor)
{
	check(Counselor);
	check(Role == ROLE_Authority);

	if (!bIsStarted)
	{
		bJustStarted = true;
		SetStarted(true);
		PlayVO(TEXT("AppliedLastPart"), Counselor);
	}
}

int32 ASCDriveableVehicle::GetNumberOfPeopleInVehicle() const
{
	int32 NumOccupants = 0;

	for (const USCVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->CounselorInSeat)
			NumOccupants++;
	}

	return NumOccupants;
}

void ASCDriveableVehicle::SetRepaired(const bool bRepaired, const bool bForced /*= false*/)
{
	check(Role == ROLE_Authority);

	bIsRepaired = bRepaired;
	bWasForceReapired = bForced;

	// Enable the driver seat for interaction
	for (USCVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->IsDriverSeat())
		{
			Seat->bIsEnabled = true;
			break;
		}
	}

	// Explicit call to OnRep for listen server
	OnRep_IsRepaired();
}

void ASCDriveableVehicle::OnRep_IsRepaired()
{
	if (bIsRepaired)
	{
		// Notify Blueprint
		OnRepaired();
	}
}

void ASCDriveableVehicle::ForceRepair()
{
	check(Role == ROLE_Authority);
	
	if (bIsRepaired)
		return;

	// Disable all of the repair components
	TArray<UActorComponent*> RepairComponents = GetComponentsByClass(USCRepairComponent::StaticClass());
	for (UActorComponent* Comp : RepairComponents)
	{
		if (Comp == StartVehicleComponent)
			continue;

		if (USCRepairComponent* RepairComp = Cast<USCRepairComponent>(Comp))
		{
			RepairComp->ForceRepair();
		}
	}

	StartVehicleComponent->ForceRepair();

	SetRepaired(true, true);
}

void ASCDriveableVehicle::SetStarted(const bool bStarted)
{
	check(Role == ROLE_Authority);

	bIsStarted = bStarted;
	StartVehicleComponent->bIsEnabled = !bStarted;

	CheaterKillVolume->SetCollisionEnabled(bIsStarted ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	if (VehicleType == EDriveableVehicleType::Car)
		JasonCarInteractComponent->bIsEnabled = bStarted;

	if (bStarted)
	{
		bFirstStartComplete = true;

		UnlockSeats();
		for (USCVehicleSeatComponent* Seat : Seats)
		{
			if (Seat->IsDriverSeat())
			{
				Seat->RequiredPartClass = nullptr; // Disable the required part since it was used to start the car
				if (Seat->CounselorInSeat)
				{
					Seat->CounselorInSeat->GetInteractableManagerComponent()->UnlockInteraction(Seat);
				}

				break;
			}
		}
	}

	// Explicit call to OnRep for Listen server #LANThings
	OnRep_IsStarted();
}

void ASCDriveableVehicle::OnRep_IsStarted()
{
	if (bIsStarted)
	{
		// Notify Blueprint
		OnStarted();

		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Driver))
		{
			if (Counselor->IsLocallyControlled())
			{
				if (USCVehicleSeatComponent* DriversSeat = Counselor->GetCurrentSeat())
				{
					Counselor->GetInteractableManagerComponent()->LockInteractionLocally(DriversSeat);
					DriversSeat->SetDrawAtLocation(true, FVector2D(0.08f, 0.85f));
				}

				if (APlayerController* PC = Cast<APlayerController>(Counselor->GetCharacterController()))
				{
					PC->PopInputComponent(Counselor->PassengerInputComponent);
				}
			}

			if (Role == ROLE_Authority)
			{
				BaseRestartTime = 0.f;
				RestartTime = 0.f;
				OnRep_RestartTime();

				if (AController* PC = Counselor->GetCharacterController())
				{
					if (Counselor->GetOwner() != this)
					{
						PC->Possess(this);
						Counselor->SetOwner(this);
					}
				}
			}
		}

		StartVehicleComponent->GetRequiredPartClasses().Empty();

		bool bPlayJasonEngineCue = false;
		if (JasonEngineCue)
		{
			if (AILLPlayerController* LocalPlayerController = UILLGameBlueprintLibrary::GetFirstLocalPlayerController(GetWorld()))
			{
				if (APawn* LocalPawn = LocalPlayerController->GetPawn())
				{
					bPlayJasonEngineCue = LocalPawn->IsA<ASCKillerCharacter>();
				}
			}
		}

		if (EngineAudioComponent)
		{
			EngineAudioComponent->SetSound(bPlayJasonEngineCue ? JasonEngineCue : EngineCue);
			EngineAudioComponent->Play();
		}

		if (VehicleType == EDriveableVehicleType::Car)
			PlaySong();
	}
	else
	{
		OnStalled();
		if (ASCCounselorCharacter* DrivingCounselor = Cast<ASCCounselorCharacter>(Driver))
		{
			if (Role == ROLE_Authority)
			{
				if (Controller)
				{
					if (DrivingCounselor->GetOwner() != nullptr)
					{
						Controller->Possess(DrivingCounselor);
						DrivingCounselor->SetOwner(nullptr);
					}
				}

				CalculateStartTime();
			}

			if (DrivingCounselor->IsLocallyControlled())
			{
				if (VehicleType == EDriveableVehicleType::Car)
				{
					if (USCVehicleSeatComponent* DriverSeat = DrivingCounselor->GetCurrentSeat())
					{
						DrivingCounselor->GetCurrentSeat()->SetDrawAtLocation(false);
					}

					if (APlayerController* PC = Cast<APlayerController>(DrivingCounselor->Controller))
					{
						PC->PushInputComponent(DrivingCounselor->PassengerInputComponent);
					}

					DrivingCounselor->GetInteractableManagerComponent()->LockInteraction(StartVehicleComponent, EILLInteractMethod::Hold, true);
				}
			}

			DrivingCounselor->OnStanceChanged(ESCCharacterStance::Driving);
		}

		// Cut audio
		if (VehicleType == EDriveableVehicleType::Car)
			StopMusic();

		if (HornAudioComponent)
		{
			HornAudioComponent->Stop();
		}

		if (EngineAudioComponent)
		{
			EngineAudioComponent->SetSound(EngineDieCue);
			EngineAudioComponent->Play();
		}
	}
}

void ASCDriveableVehicle::SetEscaped(ASCEscapeVolume* EscapeVolume, USplineComponent* ExitPath, float EscapeDelay)
{
	bEscaping = true;
	OnHandbrakeUp();
	// Don't run into thinnymore, we need to get out.
	if (VehicleType != EDriveableVehicleType::Boat)
	{
		SetActorEnableCollision(false);

		// Snap our position to the escape volume's position to mitgate AI driving out of the world
		SetActorLocation(EscapeVolume->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);

		const FVector Rot = ExitPath->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World) - ExitPath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		SetActorRotation(Rot.Rotation(), ETeleportType::TeleportPhysics);
	}

	for (USCVehicleSeatComponent* Seat : Seats)
	{
		if (ASCCounselorCharacter* Counselor = Seat->CounselorInSeat)
		{
			if (Counselor->Role == ROLE_Authority)
			{
				AController* PlayerController = Controller ? Controller : Counselor->GetCharacterController();
				if (Seat->IsDriverSeat() && PlayerController)
				{
					if (Counselor->GetOwner() != nullptr)
					{
						PlayerController->Possess(Counselor);
						Counselor->SetOwner(nullptr);
					}
				}

				Counselor->GetInteractableManagerComponent()->UnlockInteraction(Seat, true);
			}
			
			if (Counselor->IsLocallyControlled())
			{
				Counselor->HideFace(false);
			}

			Counselor->SetEscaped(nullptr, EscapeDelay);
		}
	}

	if (Role == ROLE_Authority)
	{
		if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (VehicleType == EDriveableVehicleType::Car)
				GS->SetCarEscaped(Seats.Num() > 2, true);
			else if (VehicleType == EDriveableVehicleType::Boat)
				GS->SetBoatEscaped(true);
		}

		if (ASCAIController* AI = Cast<ASCAIController>(GetController()))
		{
			if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AI->BrainComponent))
			{
				BTComp->StopTree();
			}
		}

		if (ASCAIController* AI = GetWorld()->SpawnActor<ASCAIController>())
		{
			AI->Possess(this);
			CurrentSplineIndex = 1;
		}

		ExitSpline = ExitPath;
	}

	if (VehicleType != EDriveableVehicleType::Boat)
		GetVehicleMovement()->StopMovementImmediately();
}

void ASCDriveableVehicle::EscapeFinished()
{
	check(Role == ROLE_Authority);

	SetActorHiddenInGame(true);

	for (const USCVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->CounselorInSeat)
		{
			Seat->CounselorInSeat->SetActorHiddenInGame(true);
		}
	}

	MULTICAST_EscapeFinished();
}

void ASCDriveableVehicle::MULTICAST_EscapeFinished_Implementation()
{
	bEscaping = false;
	bEscaped = true;
	OnEscaped();

	FTimerDelegate Delegate;
	Delegate.BindLambda([this]()
	{
		SetActorHiddenInGame(true);

		if (Role == ROLE_Authority)
			Destroy();
	});
	GetWorldTimerManager().SetTimer(TimerHandle_OnEscaped, Delegate, 10.0f, false);
}

void ASCDriveableVehicle::ForceStart()
{
	check(Role == ROLE_Authority);

	if (bIsStarted)
		return;

	// Force repair as well
	ForceRepair();
	SetStarted(true);
}

void ASCDriveableVehicle::CalculateStartTime()
{
	if (ASCCounselorCharacter* DrivingCounselor = Cast<ASCCounselorCharacter>(Driver))
	{
		BaseRestartTime = FMath::RandRange(MinRestartTime, MaxRestartTime);
		RestartTime = BaseRestartTime * RestartTimeMod.Get(DrivingCounselor, true);
		if (ASCPlayerState* PS = DrivingCounselor->GetPlayerState())
		{
			float RestartModifier = 0.0f;
			for (const USCPerkData* Perk : PS->GetActivePerks())
			{
				check(Perk);

				if (!Perk)
					continue;

				if (VehicleType == EDriveableVehicleType::Boat)
				{
					RestartModifier += Perk->BoatStartTimeModifier;
				}
				else if (VehicleType == EDriveableVehicleType::Car)
				{
					RestartModifier += Perk->CarStartTimeModifier;
				}
			}

			RestartTime -= RestartTime * RestartModifier;
		}
		OnRep_RestartTime();
	}
}

void ASCDriveableVehicle::UnlockSeats()
{
	check(Role == ROLE_Authority);

	if (!bIsRepaired)
		return;

	for (USCVehicleSeatComponent* Seat : Seats)
	{
		Seat->bIsEnabled = true;
	}
}

void ASCDriveableVehicle::OnRep_RestartTime()
{
	if (RestartTime > 0.f)
		StartVehicleComponent->HoldTimeLimit = RestartTime;
}

void ASCDriveableVehicle::UpdateAIMovement(float DeltaSeconds)
{
	if (CurrentSplineIndex >= ExitSpline->GetNumberOfSplinePoints())
	{
		EscapeFinished();
		SetActorTickEnabled(false);
		return;
	}

	FVector CurrentDestination = ExitSpline->GetLocationAtSplinePoint(CurrentSplineIndex, ESplineCoordinateSpace::World);
	CurrentDestination.Z = 0.f;
	FVector ActorLocation = GetActorLocation();
	ActorLocation.Z = 0.f;

	FVector ToDestination = (CurrentDestination - ActorLocation);

	static const float RadiusTolerance = 500.f;

	// Hit our destination. Go to the next point.
	if (ToDestination.SizeSquared2D() <= FMath::Square(RadiusTolerance))
	{
		CurrentSplineIndex++;
		return;
	}

	float DistanceToDestination = ToDestination.Size();

	ToDestination.Normalize();
	float ForwardDot = ToDestination | GetActorForwardVector();
	float RightDot = ToDestination | GetActorRightVector();

	// Set Throttle
	SetThrottle(ForwardDot > 0.f ? (bIsFrontBlockedByKiller ? 0.0f : 1.f) : (bIsRearBlockedByKiller ? 0.0f : -1.f));

	// Set Steering
	// If our current heading will get us within range of the destination, no need to adjust steering
	FVector ProjectedDestination = GetActorLocation() + GetActorForwardVector() * DistanceToDestination;
	ProjectedDestination.Z = 0.f;
	if ((ProjectedDestination - CurrentDestination).SizeSquared2D() <= FMath::Square(RadiusTolerance))
	{
		SetSteering(0.f);
	}
	else // Figure out how to steer
	{
		// Point is in front
		if (ForwardDot > 0.f)
		{
			// Positive is right. . .I think?
			SetSteering(RightDot > 0.f ? 1.f : -1.f);
		}
		else // Point is behind
		{
			// Steer in the opposite direction for reverse
			SetSteering(RightDot > 0.f ? -1.f : 1.f);
		}
	}
}

void ASCDriveableVehicle::OnForward(float Val)
{
	// Let's not screw ourselves with a lame looking low-impact collision
	if (Val > 0.f && bIsFrontBlockedByKiller)
		Val = 0.f;

	if (CounselorsInteractingWithVehicle.Num() > 0)
		Val = 0.f;

	if (bJasonFlippingBoat)
		Val = 0.f;

	if (bEscaping)
		return;

	ForwardVal = Val;

	SetThrottle(ForwardVal + ReverseVal);
}

void ASCDriveableVehicle::OnReverse(float Val)
{
	// Let's not screw ourselves with a lame looking low-impact collision
	if (Val < 0.f && bIsRearBlockedByKiller)
		Val = 0.f;

	if (CounselorsInteractingWithVehicle.Num() > 0)
		Val = 0.f;

	if (bJasonFlippingBoat)
		Val = 0.f;

	if (bEscaping)
		return;

	ReverseVal = Val;

	SetThrottle(ForwardVal + ReverseVal);
}

void ASCDriveableVehicle::SetThrottle(float val)
{
	ThrottleVal = val;

	if (IsLocallyControlled())
	{
		GetVehicleMovement()->SetThrottleInput(ThrottleVal);
		UpdateActiveCamera();
	}

	if (Role < ROLE_Authority)
	{
		SERVER_SetThrottle(val);
	}
	else
	{
		// Explicit call to OnRep for listen servers
		OnRep_Throttle();
	}
}

bool ASCDriveableVehicle::SERVER_SetThrottle_Validate(float val)
{
	return true;
}

void ASCDriveableVehicle::SERVER_SetThrottle_Implementation(float val)
{
	SetThrottle(val);
}

void ASCDriveableVehicle::OnRep_Throttle()
{
	OnThrottle(ThrottleVal);
}

void ASCDriveableVehicle::SetSteering(float val)
{
	if (FMath::IsNearlyEqual(SteeringVal, val))
		return;

	if (bJasonFlippingBoat)
		val = 0.f;

	// Track an internal steering value
	SteeringVal = val;

	if (IsLocallyControlled())
		GetVehicleMovement()->SetSteeringInput(val);

	// Need to set it on the server and replicate out for animation stuffs
	if (Role < ROLE_Authority)
	{
		SERVER_SetSteering(val);
	}
	else
	{
		OnRep_Steering();
	}
}

bool ASCDriveableVehicle::SERVER_SetSteering_Validate(float val)
{
	return true;
}

void ASCDriveableVehicle::SERVER_SetSteering_Implementation(float val)
{
	SetSteering(val);
}

void ASCDriveableVehicle::OnRep_Steering()
{
	OnSteering(SteeringVal);
}

void ASCDriveableVehicle::OnHandbrakeDown()
{
	if (bEscaping)
		return;

	bHandbrake = true;
	if (IsLocallyControlled())
		GetVehicleMovement()->SetHandbrakeInput(bHandbrake);

	if (Role < ROLE_Authority)
	{
		SERVER_OnHandbrakeDown();
	}

	// Explicit call to OnRep since we're changing it manually on the local controller...
	OnRep_Handbrake();
}

bool ASCDriveableVehicle::SERVER_OnHandbrakeDown_Validate()
{
	return true;
}

void ASCDriveableVehicle::SERVER_OnHandbrakeDown_Implementation()
{
	OnHandbrakeDown();
}

void ASCDriveableVehicle::OnHandbrakeUp()
{
	bHandbrake = false;
	if (IsLocallyControlled())
		GetVehicleMovement()->SetHandbrakeInput(bHandbrake);

	if (Role < ROLE_Authority)
	{
		SERVER_OnHandbrakeUp();
	}

	// Explicit call to OnRep since we're changing it manually on the local controller...
	OnRep_Handbrake();
}

bool ASCDriveableVehicle::SERVER_OnHandbrakeUp_Validate()
{
	return true;
}

void ASCDriveableVehicle::SERVER_OnHandbrakeUp_Implementation()
{
	OnHandbrakeUp();
}

void ASCDriveableVehicle::OnRep_Handbrake()
{
	OnHandbrake(bHandbrake);
}

void ASCDriveableVehicle::OnDriverExitVehicle()
{
	if (IsBeingSlammed())
		return;

	bLookBehindHeld = false;
	UpdateActiveCamera();

	if (Role < ROLE_Authority)
	{
		SERVER_OnDriverExitVehicle();
	}
	else
	{
		// This should keep the player from exiting after starting the vehicle
		if (bJustStarted)
		{
			bJustStarted = false;
			return;
		}

		if (FMath::Abs(GetVehicleMovement()->GetForwardSpeed()) > MinUseCarDoorSpeed)
			return;

		USCVehicleSeatComponent* DriverSeat = nullptr;
		for (USCVehicleSeatComponent* Seat : Seats)
		{
			if (Seat->IsDriverSeat())
			{
				DriverSeat = Seat;
				break;
			}
		}

		if (DriverSeat)
		{
			if (DriverSeat->IsSeatBlocked())
				return;

			// HACK: Due to the shitty way seats were implemented (fuck you LockInteractionLocally) we need to double check we can use the interaction here before we check
			// if we can use the interaction here. Previously the interaction would fale, but lock the interactable, causing killer to not be able to grab the driver. Which sucks.
			// The core problem is in UILLInteractableManagerComponent::CLIENT_LockInteraction, but changing how that locks now would be... dangerous.
			if ((DriverSeat->CanInteractWithBroadcast(DriverSeat->CounselorInSeat, FVector::ZeroVector, FRotator::ZeroRotator) & (int32)EILLInteractMethod::Press) != 0)
				DriverSeat->CounselorInSeat->GetInteractableManagerComponent()->AttemptInteract(DriverSeat, EILLInteractMethod::Press);
		}
	}
}

bool ASCDriveableVehicle::SERVER_OnDriverExitVehicle_Validate()
{
	return true;
}

void ASCDriveableVehicle::SERVER_OnDriverExitVehicle_Implementation()
{
	OnDriverExitVehicle();
}

void ASCDriveableVehicle::OnMeshOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role == ROLE_Authority)
	{
		if (APhysicsVolume* Volume = Cast<APhysicsVolume>(OtherActor))
		{
			if (Volume->bWaterVolume)
			{
				CurrentWaterVolume = Volume;
			}
		}
	}
}

void ASCDriveableVehicle::OnMeshOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Role == ROLE_Authority)
	{
		// Let's always hold onto our water volume if we're a boat
		if (VehicleType == EDriveableVehicleType::Car)
		{
			if (OtherActor == CurrentWaterVolume)
			{
				CurrentWaterVolume = nullptr;
			}
		}
	}
}

void ASCDriveableVehicle::OnVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bEscaping || bEscaped)
		return;

	if (OtherActor)
	{
		// Ignore Characters
		if (OtherActor->IsA<ASCCharacter>())
			return;

		// Ignore Projectiles
		if (OtherActor->IsA<ASCProjectile>())
			return;
	}

	float Speed = FMath::Abs(CachedSpeed);

	bool bPlayHeavy = false;
	if (Speed <= MinLightImpactSpeed)
		return;

	// Test forward against the impact normal
	FVector VehicleForward = GetActorForwardVector();
	float ImpactDot = FMath::Abs(Hit.ImpactNormal | VehicleForward);
	if (ImpactDot < FMath::Cos(FMath::DegreesToRadians(MaxImpactAngle)))
		return;

	bPlayHeavy = Speed > MinHeavyImpactSpeed;

	// Kill the car if we REALLY crashed
	if (bPlayHeavy)
	{
		if (Role == ROLE_Authority)
			SetStarted(false);
		PlayAnimMontage(VehicleSlammedMontage);
	}

	ImpactAudioComponent->SetWorldLocation(Hit.ImpactPoint);
	ImpactAudioComponent->SetSound(bPlayHeavy ? HeavyImpactCue : LightImpactCue);
	ImpactAudioComponent->Play();
}

void ASCDriveableVehicle::OnCheaterVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (Role < ROLE_Authority)
		return;

	if (bIsDisabled || !bIsStarted)
		return;

	// Make sure the vehicle is facing mostly up.
	if (FMath::Acos(GetActorUpVector() | FVector::UpVector) > FMath::DegreesToRadians(MaxAngleToKillCheaters))
		return;

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		if (Counselor->IsAlive() && !Counselor->IsInVehicle() && !IsBeingSlammed())
		{
			// Don't pass along the controller or actor, it's not their fault the idiot climbed on the car
			// Square the damage so that the counselor dies for sure please. if they have any damage mitigating perks they still need to die.
			Counselor->TakeDamage(10000.f, FDamageEvent(KillCounselorDamageType), nullptr, nullptr);
		}
	}
}

void ASCDriveableVehicle::OnKillerFrontCollisionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (Role < ROLE_Authority)
		return;

	if (bIsDisabled)
		return;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
	{
		KillersInFrontOfCar.AddUnique(Killer);
		bIsFrontBlockedByKiller = true;

		if (Driver && GetVehicleMovement()->GetForwardSpeed() > KillerSlamSpeed)
		{
			// Disable the car slam interact component immediately
			JasonInteractComponent->bIsEnabled = false;
			StartSlamJam(Killer);
		}
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		if (Counselor->IsAlive() && !Counselor->IsInVehicle() && !IsBeingSlammed())
		{
			if (CachedSpeed >= MinKillCounselorSpeed)
			{
				Counselor->TakeDamage(100.f, FDamageEvent(KillCounselorDamageType), Controller, this);
				PlayImpactSound(HeavyImpactCue, Counselor->GetActorLocation());
			}
			else
			{
				// Save this counselor for later, in case we speed up
				CounselorsInFrontOfCar.AddUnique(Counselor);
			}
		}
	}
	else if (ASCDestructibleActor* DestructibleActor = Cast<ASCDestructibleActor>(OtherActor))
	{
		if (ASCCounselorCharacter* DriverCounselor = Cast<ASCCounselorCharacter>(Driver))
			DestructibleActor->TakeDamage(100.f, FDamageEvent(), Controller, this);
	}
}

void ASCDriveableVehicle::OnKillerFrontCollisionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Role < ROLE_Authority)
		return;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
	{
		const int32 KillerIndex = KillersInFrontOfCar.Find(Killer);
		if (KillerIndex != INDEX_NONE)
		{
			KillersInFrontOfCar.RemoveAt(KillerIndex);

			if (KillersInFrontOfCar.Num() == 0)
			{
				bIsFrontBlockedByKiller = false;
			}
		}
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		CounselorsInFrontOfCar.Remove(Counselor);
	}
}

void ASCDriveableVehicle::OnKillerRearCollisionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (Role < ROLE_Authority)
		return;

	if (bIsDisabled)
		return;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
	{
		KillersBehindCar.AddUnique(Killer);
		bIsRearBlockedByKiller = true;

		if (Killer->IsStunnedBySweater())
			return;

		// Don't bump the killer when he's busy killing someone
		if (Killer->IsInSpecialMove())
			return;

		// If we're moving fast enough the killer will play a hit reaction and the vehicle and stall it out
		UWheeledVehicleMovementComponent* MovementComponent =  GetVehicleMovementComponent();
		if (Driver && MovementComponent->GetForwardSpeed() < -KillerSlamSpeed)
		{
			// Bump the killer
			Killer->TakeDamage(0.f, FDamageEvent(), Controller, this);

			SetStarted(false);
			MakeOnlyActiveCamera(this, GetFirstPersonCamera());
			PlayImpactSound(HeavyImpactCue, Killer->GetActorLocation());

			// If we're the killer, cancel shift/morph
			if (Killer->IsLocallyControlled())
			{
				Killer->CancelShift();
				Killer->CancelMorph();
			}

		}
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		if (Counselor->IsAlive() && !Counselor->IsInVehicle() && !IsBeingSlammed())
		{
			if (CachedSpeed <= -MinKillCounselorSpeed)
			{
				Counselor->TakeDamage(100.f, FDamageEvent(KillCounselorDamageType), Controller, this);
				PlayImpactSound(HeavyImpactCue, Counselor->GetActorLocation());
			}
			else
			{
				// Save this counselor for later, in case we speed up
				CounselorsBehindCar.AddUnique(Counselor);
			}
		}
	}
	else if (ASCDestructibleActor* DestructibleActor = Cast<ASCDestructibleActor>(OtherActor))
	{
		if (ASCCounselorCharacter* DriverCounselor = Cast<ASCCounselorCharacter>(Driver))
			DestructibleActor->TakeDamage(100.f, FDamageEvent(), DriverCounselor->GetCharacterController(), this);
	}
}

void ASCDriveableVehicle::OnKillerRearCollisionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Role < ROLE_Authority)
		return;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OtherActor))
	{
		const int32 KillerIndex = KillersBehindCar.Find(Killer);
		if (KillerIndex != INDEX_NONE)
		{
			KillersBehindCar.RemoveAt(KillerIndex);

			if (KillersBehindCar.Num() == 0)
			{
				bIsRearBlockedByKiller = false;
			}
		}
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OtherActor))
	{
		CounselorsBehindCar.Remove(Counselor);
	}
}

void ASCDriveableVehicle::MULTICAST_KillerSlamCar_Implementation(ASCKillerCharacter* Killer)
{
	// Track the thing
	bIsBeingSlammed = true;

	// If we're the killer, cancel shift/morph
	if (Killer->IsLocallyControlled())
	{
		Killer->CancelShift();
		Killer->CancelMorph();

		// If you're locally the killer you seem intent on ignoring the server's wish of rotating you
		// So we're going to rotate you AGAIN. Idiot.
		const FRotator WorldTargetRotation(GetActorRotation().Pitch, GetActorRotation().Yaw + 180.f, GetActorRotation().Roll);
		Killer->SetActorRotation(WorldTargetRotation);
	}

	Killer->GetCharacterMovement()->StopMovementImmediately();
	GetVehicleMovementComponent()->StopMovementImmediately();

	Killer->PlayAnimMontage(KillerSlamVehicleMontage);
	PlayAnimMontage(VehicleSlammedMontage);

	// Find the TRUE victim here... and give them a cool camera shot
	ASCCharacter* LocalCharacterInvolvedInThisHorribleCrash = nullptr;

	if (Killer->IsLocallyControlled())
	{
		LocalCharacterInvolvedInThisHorribleCrash = Killer;
	}
	else if (Driver && IsLocallyControlled())
	{
		LocalCharacterInvolvedInThisHorribleCrash = Cast<ASCCharacter>(Driver);
	}
	else
	{
		for (USCVehicleSeatComponent* Seat : Seats)
		{
			if (Seat->CounselorInSeat && Seat->CounselorInSeat->IsLocallyControlled())
			{
				LocalCharacterInvolvedInThisHorribleCrash = Seat->CounselorInSeat;
				break;
			}
		}
	}

	// Camera stuff
	if (LocalCharacterInvolvedInThisHorribleCrash)
	{
		MakeOnlyActiveCamera(this, SlamSplineCamera);
		SlamSplineCamera->ActivateCamera();
		LocalCharacterInvolvedInThisHorribleCrash->TakeCameraControl(this);

		// Turn heads back on
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(LocalCharacterInvolvedInThisHorribleCrash))
		{
			Counselor->HideFace(false);
		}

		// Don't move!
		if (AController* CharacterController = LocalCharacterInvolvedInThisHorribleCrash->GetController())
		{
			if (!CharacterController->IsMoveInputIgnored())
				CharacterController->SetIgnoreMoveInput(true);
		}

		// Return our camera after a minute
		FTimerDelegate Delegate;
		Delegate.BindLambda([this](ASCCharacter* Character)
		{
			if (IsValid(Character))
			{
				// Set our camera back and keep moving
				if (Character->IsKiller())
				{
					Character->ReturnCameraToCharacter(1.f, VTBlend_EaseInOut, 2.f, true, false);
				}
				else if (Driver != Character)
				{
					Character->ReturnCameraToCharacter(0.f, VTBlend_Linear, 0.f, true, false);
				}

				if (AController* CharacterController = Character->GetController())
					CharacterController->SetIgnoreMoveInput(false);

				// Turn heads back off and resume the driving state, if we're still in a seat
				if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
				{
					if (Counselor->GetCurrentSeat())
					{
						Counselor->OnStanceChanged(ESCCharacterStance::Driving);
						if (Counselor->IsPlayerControlled())
							Counselor->HideFace(true);
					}
				}
			}

			MakeOnlyActiveCamera(this, GetFirstPersonCamera());
		}, LocalCharacterInvolvedInThisHorribleCrash);

		GetWorldTimerManager().SetTimer(TimerHandle_LocalSlamFinish, Delegate, SlamSpline->Duration, false);
	}

	FTimerDelegate EndDelegate;
	EndDelegate.BindLambda([this]() { bIsBeingSlammed = false; });
	GetWorldTimerManager().SetTimer(TimerHandle_SlamFinish, EndDelegate, SlamSpline->Duration, false);

	if (Role == ROLE_Authority)
	{
		FTimerDelegate Delegate;
		Delegate.BindLambda([this]() { SetStarted(false); });

		GetWorldTimerManager().SetTimer(TimerHandle_SlamStall, Delegate, SlamSpline->Duration, false);
	}

	// Finally, hook them BPs
	OnVehicleSlammed.Broadcast();
}

void ASCDriveableVehicle::StartSlamJam(ASCKillerCharacter* Killer)
{
	// Don't do a slam if Jason is stunned by poorly knit sweaters
	if (Killer->IsStunnedBySweater())
		return;

	// Don't ask the killer to slam us when he's busy killing someone
	if (Killer->IsInSpecialMove())
		return;

	// Don't slam twice!
	if (IsBeingSlammed())
		return;

	// let's put this poor teen somewhere safe.
	if (ASCCounselorCharacter* GrabbedCounselor = Killer->GetGrabbedCounselor())
	{
		Killer->DropCounselor();
	}

	// If we're moving fast enough the killer will slam the vehicle and stall it out
	UWheeledVehicleMovementComponent* MovementComponent = GetVehicleMovementComponent();
	// First find the bone we want to move the killer to
	static const FName VehicleTarget_NAME(TEXT("target"));
	const int32 TargetBoneIndex = GetMesh()->GetBoneIndex(VehicleTarget_NAME);
	if (TargetBoneIndex == INDEX_NONE || VehicleSlammedMontage == nullptr)
		return;

	// This requires looking up the bone position inside the montage....
	UAnimInstance* VehicleAnimInstance = GetMesh()->GetAnimInstance();

	FCompactPose Pose;
	FBlendedCurve Curves;

	Pose.SetBoneContainer(&VehicleAnimInstance->GetRequiredBones());
//	Curves.InitFrom(VehicleAnimInstance->GetSkelMeshComponent()->GetCachedAnimCurveMappingNameUids()); // ILLFONIC: DGarcia: Commenting out from 4.18 integration. Might cause issues...

	// Get that first animation...
	UAnimSequenceBase* Sequence = VehicleSlammedMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference;
	Sequence->GetAnimationPose(Pose, Curves, FAnimExtractContext(0.f));

	// That was ugly, let's put that bone in world space now
	const FVector LocalTargetPosition = Pose.GetBones()[TargetBoneIndex].GetLocation();
	const FVector WorldTargetPosition = GetActorLocation() + GetActorRotation().RotateVector(LocalTargetPosition);
	const FRotator WorldTargetRotation(GetActorRotation().Pitch, GetActorRotation().Yaw + 180.f, GetActorRotation().Roll);

	// Alright, just slap the killer into that location
	Killer->SetActorLocation(WorldTargetPosition + FVector::UpVector * Killer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Killer->SetActorRotation(WorldTargetRotation);

	// COME ON AND SLAM
	Killer->GetCharacterMovement()->StopMovementImmediately();
	MovementComponent->StopMovementImmediately();
	MULTI_SetPhysicsState(0.0f);

	Killer->ClearAnyInteractions();

	// AND WELCOME TO THE JAM
	// https://youtu.be/Y-dMSstLDqM
	MULTICAST_KillerSlamCar(Killer);

	if (ASCPlayerState* PS = Killer->GetPlayerState())
	{
		PS->CarSlammed();
		PS->EarnedBadge(CarSlamBadge);
	}
}

void ASCDriveableVehicle::ToggleDriverMap()
{
	bShowDriverMap = !bShowDriverMap;
}

void ASCDriveableVehicle::StartTalking()
{
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
		PC->StartTalking();
}

void ASCDriveableVehicle::StopTalking()
{
	if (ASCPlayerController* PC = Cast <ASCPlayerController>(Controller))
		PC->StopTalking();
}

void ASCDriveableVehicle::OpenHood()
{
	PlayAnimMontage(OpenHoodAnim);
}

void ASCDriveableVehicle::CloseHood()
{
	PlayAnimMontage(CloseHoodAnim);
}

void ASCDriveableVehicle::EnableVehicleStarting()
{
	StartVehicleComponent->bIsEnabled = true;
	CalculateStartTime();
}

void ASCDriveableVehicle::CLIENT_SetCameraToCounselor_Implementation(ASCCounselorCharacter* Counselor)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->SetViewTarget(Counselor);
	}
}

void ASCDriveableVehicle::EjectCounselor(ASCCounselorCharacter* Counselor, bool bFromCrash)
{
	check(Role == ROLE_Authority);
	check(Counselor);

	if (Counselor == Driver)
	{
		if (bIsStarted)
		{
			if (Controller)
			{
				if (Counselor->GetOwner() != nullptr)
				{
					Controller->Possess(Counselor);
					Counselor->SetOwner(nullptr);
				}
			}
		}
	}

	Counselor->MULTICAST_EjectFromVehicle(bFromCrash);
}

FVector ASCDriveableVehicle::GetEmptyCapsuleLocation(const AActor* CounselorToIgnore/* = nullptr*/)
{
	static const FName NAME_EjectTrace(TEXT("EjectTrace"));
	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_EjectTrace, false, this);
	if (CounselorToIgnore)
		Params.AddIgnoredActor(CounselorToIgnore);

	for (UCapsuleComponent* Capsule : ExitCapsules)
	{
		TArray<AActor*> OverlapActors;
		Capsule->GetOverlappingActors(OverlapActors);
		bool bPerformTrace = true;
		for (AActor* OverlapActor : OverlapActors)
		{
			// Skip anything we're ignoring
			if (Params.GetIgnoredActors().Contains(OverlapActor->GetUniqueID()))
				continue;

			// Skip water volumes
			if (APhysicsVolume* PhysVol = Cast<APhysicsVolume>(OverlapActor))
				if (PhysVol->bWaterVolume)
					continue;

			bPerformTrace = false;
		}

		if (bPerformTrace)
		{
			// If we don't have a world, we should probably fuck off anyway.
			if (UWorld* World = GetWorld())
			{
				if (!World->LineTraceSingleByChannel(HitResult, GetActorLocation(), Capsule->GetComponentLocation(), ECC_Pawn, Params))
					return Capsule->GetComponentLocation();
			}
		}
	}

	// All Capsules are "blocked"
	return FVector::ZeroVector;
}

void ASCDriveableVehicle::OnLookBehindPressed()
{
	bLookBehindHeld = true;
	UpdateActiveCamera();
}

void ASCDriveableVehicle::OnLookBehindReleased()
{
	bLookBehindHeld = false;
	UpdateActiveCamera();
}
void ASCDriveableVehicle::UpdateActiveCamera()
{
	UCameraComponent* NewCam = ThrottleVal >= 0.f && !bLookBehindHeld ? GetFirstPersonCamera() : GetReverseCamera();
	if (NewCam && ActiveCamera != NewCam)
	{
		if (ActiveCamera)
			ActiveCamera->SetActive(false);

		ActiveCamera = NewCam;
		ActiveCamera->SetActive(true);
	}
}

bool ASCDriveableVehicle::CanEnterExitVehicle() const
{
	if (USCWheeledVehicleMovementComponent* Movement = Cast<USCWheeledVehicleMovementComponent>(GetVehicleMovement()))
	{
		return !(FMath::Abs(Movement->GetThrottleInput()) > 0.f || FMath::Abs(Movement->GetForwardSpeed()) > GetMinUseCarDoorSpeed());
	}

	return false;
}

void ASCDriveableVehicle::PlayAnimMontage(UAnimMontage* Montage)
{
	if (Montage)
	{
		if (USkeletalMeshComponent* SkelMesh = GetMesh())
		{
			if (UAnimInstance* AnimInst = SkelMesh->GetAnimInstance())
			{
				AnimInst->Montage_Play(Montage);
			}
		}
	}
}

void ASCDriveableVehicle::MULTICAST_PlayAnimMontage_Implementation(UAnimMontage* Montage)
{
	PlayAnimMontage(Montage);
}

void ASCDriveableVehicle::StopAnimMontage(UAnimMontage* Montage)
{
	if (Montage)
	{
		if (USkeletalMeshComponent* SkelMesh = GetMesh())
		{
			if (UAnimInstance* AnimInst = SkelMesh->GetAnimInstance())
			{
				const static FName ExitSection(TEXT("Exit"));
				if (Montage->GetSectionIndex(ExitSection) != INDEX_NONE)
					AnimInst->Montage_JumpToSection(ExitSection, Montage);
				else
					AnimInst->Montage_Stop(0.f, Montage);
			}
		}
	}
}

void ASCDriveableVehicle::MULTICAST_StopAnimMontage_Implementation(UAnimMontage* Montage)
{
	StopAnimMontage(Montage);
}

void ASCDriveableVehicle::StartHorn()
{
	if (Role < ROLE_Authority)
	{
		SERVER_PlayHorn(true);
	}
	else
	{
		MULTICAST_PlayHorn(true);
	}
}

void ASCDriveableVehicle::StopHorn()
{
	if (Role < ROLE_Authority)
	{
		SERVER_PlayHorn(false);
	}
	else
	{
		MULTICAST_PlayHorn(false);
	}
}

bool ASCDriveableVehicle::SERVER_PlayHorn_Validate(bool bPlay)
{
	return true;
}

void ASCDriveableVehicle::SERVER_PlayHorn_Implementation(bool bPlay)
{
	bPlay ? StartHorn() : StopHorn();
}

void ASCDriveableVehicle::MULTICAST_PlayHorn_Implementation(bool bPlay)
{
	if (bPlay)
	{
		HornAudioComponent->Play();
	}
	else
	{
		HornAudioComponent->FadeOut(0.1f, 0.f);
	}
}

void ASCDriveableVehicle::PlayVO(const FName VOName, ASCCounselorCharacter* Counselor)
{
	if (Role < ROLE_Authority)
	{
		SERVER_PlayVO(VOName, Counselor);
	}
	else
	{
		MULTICAST_PlayVO(VOName, Counselor);
	}
}

bool ASCDriveableVehicle::SERVER_PlayVO_Validate(const FName VOName, ASCCounselorCharacter* Counselor)
{
	return true;
}

void ASCDriveableVehicle::SERVER_PlayVO_Implementation(const FName VOName, ASCCounselorCharacter* Counselor)
{
	PlayVO(VOName, Counselor);
}

void ASCDriveableVehicle::MULTICAST_PlayVO_Implementation(const FName VOName, ASCCounselorCharacter* Counselor)
{
	if (Counselor && Counselor->VoiceOverComponent)
	{
		Counselor->VoiceOverComponent->PlayVoiceOver(VOName);
	}
}

void ASCDriveableVehicle::PlayImpactSound(USoundCue* ImpactSound, const FVector& ImpactLocation)
{
	check(Role == ROLE_Authority);

	MULTICAST_PlayImpactSound(ImpactSound, ImpactLocation);
}

void ASCDriveableVehicle::MULTICAST_PlayImpactSound_Implementation(USoundCue* ImpactSound, const FVector& ImpactLocation)
{
	ImpactAudioComponent->SetWorldLocation(ImpactLocation);
	ImpactAudioComponent->SetSound(ImpactSound);
	ImpactAudioComponent->Play();
}

void ASCDriveableVehicle::SetPassengerCameras(USCSplineCamera* Camera)
{
	for (const USCVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->CounselorInSeat && Seat->CounselorInSeat->IsLocallyControlled())
		{
			MakeOnlyActiveCamera(this, Camera);
			Seat->CounselorInSeat->TakeCameraControl(this);
			break;
		}
	}
}

void ASCDriveableVehicle::ReturnPassengerCameras()
{
	for (const USCVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->CounselorInSeat && Seat->CounselorInSeat->IsLocallyControlled())
		{
			Seat->CounselorInSeat->ReturnCameraToCharacter();
			break;
		}
	}
}

void ASCDriveableVehicle::ResetControlRotation()
{
	LocalPitch = 0.f;
	LocalYaw = 0.f;
}

bool ASCDriveableVehicle::IsPartRepaired(FString PartName)
{
	TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
	for (UActorComponent* Comp : RepairPoints)
	{
		if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
		{
			const FString PartNameStr = RepairPoint->GetName();
			if (PartNameStr.Equals(PartName))
			{
				return RepairPoint->IsRepaired();
			}
		}
	}
	return false;
}

bool ASCDriveableVehicle::IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const
{
	TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
	for (UActorComponent* Comp : RepairPoints)
	{
		if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
		{
			if (RepairPoint->GetRequiredPartClasses().Contains(Part))
			{
				AssociatedRepairComponent = RepairPoint;
				return !RepairPoint->IsRepaired();
			}
		}
	}

	return false;
}

void ASCDriveableVehicle::NextTrack()
{
	// If we have no songs setup, early out (e.g. the boat)
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

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_NextTrack))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_NextTrack);
	}

	PlaySong();
}

void ASCDriveableVehicle::PlaySong()
{
	TAssetPtr<USoundCue> SongToPlay = IsStreamerMode() ? StreamerModeSong : Songs[CurrentSong];
	if (SongToPlay.Get())
	{
		// Already loaded
		//DeferredPlayTrack();
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

void ASCDriveableVehicle::StartMusic(const TAssetPtr<USoundCue>& SongToPlay, float StartTime)
{
	if (ensure(SongToPlay.Get()))
	{
		RadioAudioComponent->SetSound(SongToPlay.Get());
		RadioAudioComponent->Play(IsStreamerMode() ? 0.f : StartTime);
	}
}

void ASCDriveableVehicle::StopMusic()
{
	GetWorldTimerManager().PauseTimer(TimerHandle_NextTrack);
	RadioAudioComponent->Stop();
}

bool ASCDriveableVehicle::IsStreamerMode()
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

void ASCDriveableVehicle::UpdateStreamerMode(bool bStreamerMode)
{
	if (RadioAudioComponent && RadioAudioComponent->IsPlaying())
	{
		RadioAudioComponent->Stop();
		
		if (!bStreamerMode)
		{
			CurrentSong = FMath::RandHelper(Songs.Num());
		}

		PlaySong();
	}
}
