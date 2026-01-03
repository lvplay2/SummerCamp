// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCTrap.h"

#include "AI/Navigation/NavAreas/NavArea_Default.h"
#include "Animation/AnimMontage.h"

#include "NavAreas/SCNavArea_Trap.h"
#include "SCContextKillActor.h"
#include "SCCounselorAnimInstance.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCIndoorComponent.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCPocketKnife.h"
#include "SCPolicePhone.h"
#include "SCRadio.h"
#include "SCSpecialMoveComponent.h"
#include "SCSpecialMove_SoloMontage.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

namespace SCTrapNames
{
	static const FName NAME_TrapTrace(TEXT("TrapTrace"));
};

ASCTrap::ASCTrap(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.DoNotCreateDefaultSubobject(ItemMesh3PComponentName))
, TrapDamage(50.f)
, KillerDamageMultiplyer(1.f)
, PlacementDistance(35.f)
, TimelimitToPickup(1.f)
{
	bIsLarge = true;
	bCanArmerTrigger = true;
	bCanPickUp = true;

	// Need to tick for short periods of time to trap counselors getting out of vehicles
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh3P = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(TEXT("TrapMesh"));
	if (ItemMesh3P)
	{
		USkeletalMeshComponent* SkeleMesh = GetMesh<USkeletalMeshComponent>();
		SkeleMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
		SkeleMesh->bEnableUpdateRateOptimizations = true;
		SkeleMesh->bOnlyOwnerSee = false;
		SkeleMesh->bOwnerNoSee = true;
		SkeleMesh->bReceivesDecals = false;
		SkeleMesh->CastShadow = true;

		SkeleMesh->SetupAttachment(RootComponent);
		SkeleMesh->bNoSkeletonUpdate = true;
		SkeleMesh->bOwnerNoSee = false;
		SkeleMesh->bCastInsetShadow = true;

		SkeleMesh->SetCollisionProfileName(TEXT("NoCollision"));
	}

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASCTrap::OnBeginOverlap);
	CollisionSphere->SetupAttachment(GetMesh());

	PlacementCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PlacementCollision"));
	PlacementCollision->SetupAttachment(GetMesh());
	PlacementCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	PlacementCollision->AreaClass = UNavArea_Default::StaticClass();
	PlacementCollision->bDynamicObstacle = true;

	TriggeredAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("TriggeredAudio"));
	TriggeredAudio->SetupAttachment(RootComponent);
	TriggeredAudio->bAutoActivate = false;

	ArmingAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ArmingAudio"));
	ArmingAudio->SetupAttachment(RootComponent);
	ArmingAudio->bAutoActivate = false;

	InteractComponent->InteractMethods |= (int32)EILLInteractMethod::Hold;
	InteractComponent->SetupAttachment(GetMesh());

	ArmTrap_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("ArmTrap_SpecialMoveHandler"));
	ArmTrap_SpecialMoveHandler->SetupAttachment(RootComponent);
	ArmTrap_SpecialMoveHandler->bTakeCameraControl = false;

	DisarmTrap_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("DisarmTrap_SpecialMoveHandler"));
	DisarmTrap_SpecialMoveHandler->SetupAttachment(RootComponent);
	DisarmTrap_SpecialMoveHandler->bTakeCameraControl = false;

	DisarmDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DisarmDriver"));
	DisarmDriver->SetNotifyName(TEXT("TrapDisarmed"));

	bAlwaysRelevant = false;

	// for some reason traps need their offsets updated from the pivot to work right when dropped
	bShouldOffsetPivotOnDrop = true;
}

void ASCTrap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCTrap, bIsArmed);
}

void ASCTrap::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority && bStartArmed)
	{
		ForceNetUpdate();
		bIsArmed = true;

		// Make sure AI know it's not safe to walk here
		PlacementCollision->AreaClass = USCNavArea_Trap::StaticClass();
		UNavigationSystem::UpdateActorAndComponentsInNavOctree(*this);
	}

	InteractComponent->bCanHighlight = !bIsArmed;
	InteractComponent->bIsEnabled = true;

	if ((bIsArmed || bStartArmed) && ArmedAnimation)
	{
		GetMesh<USkeletalMeshComponent>()->bNoSkeletonUpdate = false;
		GetMesh<USkeletalMeshComponent>()->SetAnimation(ArmedAnimation);
		GetMesh<USkeletalMeshComponent>()->SetPosition(ArmedAnimation->GetPlayLength(), false);
		StopUpdatingSkeleton(ArmedAnimation->SequenceLength);
	}

	// shut it off here so that if begin play gets hit again because of relevency it wont try to re-arm a trap that shouldnt be
	bStartArmed = false;
}

void ASCTrap::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ArmTrap_SpecialMoveHandler->DestinationReached.AddDynamic(this, &ASCTrap::OnLocationReached);
	ArmTrap_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCTrap::OnTrapArmed);
	ArmTrap_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCTrap::OnTrapArmAborted);

	DisarmTrap_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCTrap::OnDisarmStarted);
	DisarmTrap_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCTrap::OnDisarmComplete);
	DisarmTrap_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCTrap::OnDisarmAborted);
	if (DisarmDriver)
	{
		FAnimNotifyEventDelegate Begin;
		Begin.BindDynamic(this, &ASCTrap::OnDisarmBeginDriver);
		DisarmDriver->InitializeBegin(Begin);
	}

}

void ASCTrap::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_PlaceTrap);
		GetWorldTimerManager().ClearTimer(TimerHandle_AnimationPlaying);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCTrap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Role == ROLE_Authority)
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(OverlappedCharacter))
		{
			// Counselor is finished exiting the vehicle, Trap him!
			if (!Counselor->PendingSeat)
			{
				TrapCharacter(Counselor);
				OverlappedCharacter = nullptr;
				SetActorTickEnabled(false);
			}
		}
		else
		{
			// In case the trap is under the car or something.
			// EndOverlap could be called and OverlappedCharacter will be set to null. Turn off the tick.
			SetActorTickEnabled(false);
		}
	}
}

void ASCTrap::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role == ROLE_Authority)
	{
		if (bIsArmed)
		{
			if (ASCCharacter* Character = Cast<ASCCharacter>(OtherActor))
			{
				// Make sure we're not on the other side of a wall or some shit
				FHitResult Hit;
				const FVector TraceStart = CollisionSphere->GetComponentLocation() + FVector::UpVector * Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				const FVector TraceEnd = Character->GetActorLocation();
				if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldDynamic, FCollisionQueryParams(SCTrapNames::NAME_TrapTrace, false, this)))
				{
					if (Hit.Actor != Character)
						return;
				}

				if (!bCanArmerTrigger)
				{
					if (Character == TrapArmer)
						return;
				}

				// Don't trigger if we're in a context kill
				if (Character->IsInContextKill())
					return;

				// Don't trigger if we're already trapped.
				if (Character->TriggeredTrap != nullptr)
					return;

				// Don't get trapped if we're currently stunned.
				if (Character->IsStunned())
					return;

				if (Character->IsKiller())
				{
					if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
					{
						// if the killer is shifting it should ignore traps
						if (Killer->IsShiftActive())
							return;

						// IF Jason is attempting to grab a counselor don't trigger the trap. //TODO: Setup a timer to test again or we'll be able to just stay on the trap!
						if (Killer->bIsPickingUpCounselor)
							return;
					}
				}
				else // Check perks for trap avoidance
				{
					if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
					{
						// Grabbed counselors can't trigger traps
						if (Counselor->bIsGrabbed)
							return;

						// Don't trigger the trap if they are in a vehicle over it
						if (Counselor->IsInVehicle())
							return;

						// We don't trigger traps for counselors leaving vehicles until they're all the way out
						// See ASCTrap::Tick(...)
						if (Counselor->PendingSeat && Counselor->bLeavingSeat)
						{
							if (!OverlappedCharacter)
							{
								OverlappedCharacter = Counselor;
								SetActorTickEnabled(true); // Turn the tick on! Shut up, Luke!
							}

							return;
						}
					}
				}

				TrapCharacter(Character);
			}
		}
	}
}

void ASCTrap::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Role == ROLE_Authority)
	{
		if (OverlappedCharacter == OtherActor)
			OverlappedCharacter = nullptr;
	}
}

void ASCTrap::TrapCharacter(ASCCharacter* Character)
{
	check(Role == ROLE_Authority);

	if (!Character)
		return;

	FVector FootLocation = FVector::ZeroVector;
	const int32 CaughtFoot = Character->TriggerTrap(this, FootLocation);
	const bool IsSprinting = Character->IsSprinting();

	// Bear trap rules all! 
	// This needs to be after trigger trap so that when we clearinteractions, 
	// which triggers the VaultAbort handler, we can therefore know whether or not we were in a special move
	// that got aborted due to the....vault abort. 
	Character->ClearAnyInteractions();

	AController* ArmerController = TrapArmer ? TrapArmer->GetController() : nullptr;

	// make sure counselors dont get penalized for killing another player with a bear trap
	// since its too hard to keep people from steppin in your traps
	if (TrapArmer != Character)
	{
		if (TrapArmer && !TrapArmer->IsKiller())
		{
			if (Character && !Character->IsKiller())
				ArmerController = nullptr;
		}
	}

	const float Damage = TrapDamage * (Character->IsKiller() ? KillerDamageMultiplyer : 1.f);

	switch (CaughtFoot)
	{
		// Left foot
	case 0:
		Character->TakeDamage(Damage, FDamageEvent(IsSprinting ? TrapStunLFSprintingClass : TrapStunLFClass), ArmerController, this);
		break;

		// Right foot
	case 1:
		Character->TakeDamage(Damage, FDamageEvent(IsSprinting ? TrapStunRFSprintingClass : TrapStunRFClass), ArmerController, this);
		break;

	default:
		UE_LOG(LogSC, Warning, TEXT("No valid foot was found for overlap with %s and %s"), *GetName(), *Character->GetName());
		break;
	}

	// Lets see if the armer should get points for trapping someone
	if (TrapArmer && Character && TrapArmer != Character && (TrapArmer->IsKiller() || Character->IsKiller()))
	{
		if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
		{
			GameMode->HandleScoreEvent(TrapArmer->PlayerState, TrapScoreEvent);
		}

		if (ASCPlayerState* PS = TrapArmer->GetPlayerState())
			PS->TrappedPlayer(GetClass());
	}

	ForceNetUpdate();
	bIsArmed = false;
	InteractComponent->bCanHighlight = bCanPickUp;
 

	MULTICAST_OnTriggered(FootLocation);
}

void ASCTrap::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Hold:
		ArmTrap(Interactor);
		break;

	default:
		Super::OnInteract(Interactor, InteractMethod);
		break;
	}
}

int32 ASCTrap::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	// We're attached to someone, you can't use this trap
	if (GetAttachParentActor())
		return 0;

	if (Character->IsInSpecialMove())
		return 0;

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_PlaceTrap))
		return 0;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (!bCanPickUp)
	{
		// add counselor check to disarm jason traps here
		if (bIsArmed)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				const bool bHasPocketKnife = Counselor->HasItemInInventory(ASCPocketKnife::StaticClass());

				if (Counselor->IsCrouching() && bHasPocketKnife)
					return (int32) EILLInteractMethod::Hold;
			}
		}

		return 0;
	}

	if (CurrentTime - Escape_Timestamp <= TimelimitToPickup)
		return 0;

	if (ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character))
	{
		if (SCCharacter->TriggeredTrap != nullptr)
			return 0;
	}

	int32 Flags = Super::CanInteractWith(Character, ViewLocation, ViewRotation);

	if (GetOwner() && GetOwner() != Character && bIsPlacing)
		return 0;

	if (bIsArming)
		return 0;

	// don't allow picking up the trap if it's armed
	if (bIsArmed)
		Flags &= ~(int32)EILLInteractMethod::Press;

	TArray<AActor*> PhoneOverlaps;
	GetOverlappingActors(PhoneOverlaps, ASCPolicePhone::StaticClass());

	// Don't allow disarming or arming if the player height difference is too great
	const float CharacterBase = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	if (FMath::Abs(CharacterBase - GetActorLocation().Z) <= 50.f && PhoneOverlaps.Num() == 0)
		Flags |= (int32)EILLInteractMethod::Hold;

	return Flags;
}

void ASCTrap::ArmTrap(AActor* Interactor)
{
	// Don't double arm our trap
	if (bIsArming)
		return;

	if (Role < ROLE_Authority)
	{
		SERVER_ArmTrap(Interactor);
	}
	else
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
		{
			// Rotate trap to face character
			FRotator NewRotation = FRotator::ZeroRotator;
			NewRotation.Yaw = (GetActorLocation() - Character->GetActorLocation()).Rotation().Yaw;
			MULTICAST_SetArmedRotation(NewRotation);

			// if trap isnt armed, attempt to arm it
			if (!bIsArmed)
			{
				TrapArmer = Character;

				// Move player to trap to arm
				bIsArming = true;

				ArmTrap_SpecialMoveHandler->ActivateSpecial(Character);

				if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
				{
					GameMode->HandleScoreEvent(Character->PlayerState, PlaceTrapScoreEvent);
				}

				// Track arming
				if (ASCPlayerState* PS = Character->GetPlayerState())
				{
					PS->TrackItemUse(GetClass(), (uint8)ESCItemStatFlags::Used);
				}

				// Make sure AI know it's not safe to walk here
				PlacementCollision->AreaClass = USCNavArea_Trap::StaticClass();
				UNavigationSystem::UpdateActorAndComponentsInNavOctree(*this);
			}
			else // disarm the trap
			{
				DisarmDriver->SetNotifyOwner(Interactor);
				DisarmTrap_SpecialMoveHandler->ActivateSpecial(Character);
				Character->GetInteractableManagerComponent()->LockInteraction(InteractComponent, EILLInteractMethod::Hold);

				// Make sure AI know it's safe to walk here again
				PlacementCollision->AreaClass = UNavArea_Default::StaticClass();
				UNavigationSystem::UpdateActorAndComponentsInNavOctree(*this);
			}

			// Track trap use
			if (ASCPlayerState* PS = Character->GetPlayerState())
			{
				PS->TrackItemUse(GetClass(), (uint8)ESCItemStatFlags::Used);
			}

			SetOwner(nullptr);

			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
				Counselor->HideLargeItem(true);
		}
		else
		{
			bIsArmed = true;
		}
	}
}

bool ASCTrap::SERVER_ArmTrap_Validate(AActor* Interactor)
{
	return true;
}

void ASCTrap::SERVER_ArmTrap_Implementation(AActor* Interactor)
{
	ArmTrap(Interactor);
}

void ASCTrap::MULTICAST_OnTriggered_Implementation(FVector TriggeredLocation, bool bNotifyOwner)
{
	// Since OnTriggered is also called when a trap is disarmed, it would not have anything to attach to (no victim)
	// Only use relative location when attached to a character...
	if (GetAttachParentActor() != nullptr)
	{
		// Using ZeroVector because the trap is snapped directly to the 
		// skeleton socket. (SetActorLocation would set it with an offset)
		SetActorRelativeLocation(FVector::ZeroVector);
	}

	GetMesh<USkeletalMeshComponent>()->Stop();
	GetMesh<USkeletalMeshComponent>()->bNoSkeletonUpdate = false;
	GetMesh<USkeletalMeshComponent>()->PlayAnimation(TriggeredAnimation, false);
	StopUpdatingSkeleton(TriggeredAnimation->SequenceLength);
	InteractComponent->bCanHighlight = bCanPickUp;

	TriggeredAudio->Play();

	if (bNotifyOwner)
	{
		// If the owner is a killer and locally controlled the player stepped in a killer trap. Lets alert the killer.
		ASCKillerCharacter* KillerCharacter = Cast<ASCKillerCharacter>(GetOwner());
		if (KillerCharacter && KillerCharacter->IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(GetWorld(), KillerAlertAudio);
		}
	}

	OnTrapTriggered();

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
			GameState->TrapTriggered(this, Cast<ASCCharacter>(GetAttachParentActor()), /*bIsDisarmed=*/false);
	}

	// "Abort Disarm" does not get called when a trap is triggered during disarming, so this is the 2nd best place.
	// This block is intended for clearing up the counselor's disarm animation. (Which ClearAnyInteractions takes care of)
	if (ASCCharacter * DisArmer = DisarmTrap_SpecialMoveHandler->GetInteractor())
	{
		DisArmer->ClearAnyInteractions();
	}

	// Make sure AI know it's safe to walk here again
	PlacementCollision->AreaClass = UNavArea_Default::StaticClass();
	UNavigationSystem::UpdateActorAndComponentsInNavOctree(*this);
}

bool ASCTrap::Use(bool bFromInput)
{
	if (bIsPlacing)
		return false;

	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(GetOwner()))
	{
		if (Character->bInWater)
			return false;

		if (CounselorPlaceMontage)
		{
			bIsPlacing = true;
			const FVector CharacterForward = Character->GetActorForwardVector();

			FCollisionQueryParams TraceParams(SCTrapNames::NAME_TrapTrace);
			TraceParams.AddIgnoredActor(this);
			TraceParams.AddIgnoredActor(Character);

			// Trace in front of the character to see if we'll be trying to place the trap in a wall.
			FHitResult Hit;
			
			const float Dist = PlacementDistance + PlacementCollision->GetCollisionShape().GetSphereRadius();
			if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + CharacterForward * Dist, ECollisionChannel::ECC_Visibility, TraceParams))
			{
				bIsPlacing = false;
				return false;
			}

			FHitResult HitResult;
			if (!IsSpaceClear(HitResult))
			{
				bIsPlacing = false;
				return false;
			}

			MULTICAST_PlayPlacement(Character);

			GetWorldTimerManager().SetTimer(TimerHandle_PlaceTrap, this, &ASCTrap::PlaceTrap, CounselorPlaceMontage->GetSectionLength(0), false);
		}
		else
		{
			PlaceTrap();
		}

		// Using the trap should prevent interaction with other objects
		Character->GetInteractableManagerComponent()->LockInteraction(InteractComponent, EILLInteractMethod::Hold);
	}

	return false;
}

void ASCTrap::MULTICAST_PlayPlacement_Implementation(ASCCharacter* Character)
{
	Character->PlayAnimMontage(CounselorPlaceMontage);
	InteractComponent->bCanHighlight = false;
}

// Helper for our line traces (up and down!)
// Param is auto so we can take either FHitResult or FOverlapResult
template<typename TType>
static bool TrapValidateHitResult(const TType& Hit)
{
	if (!Hit.bBlockingHit)
		return false;

	if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
	{
		// We didn't actually want to be blocked by this trace
		if (HitComponent->GetCollisionResponseToChannel(ECC_WorldDynamic) != ECollisionResponse::ECR_Block &&
			HitComponent->GetCollisionResponseToChannel(ECC_WorldStatic) != ECollisionResponse::ECR_Block)
			return false;
	}

	return true;
};

bool ASCTrap::IsSpaceClear(FHitResult& OutBestCollision) const
{
	ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(GetOwner());
	if (!Character)
		return false;

	const FVector CharacterForward = Character->GetActorForwardVector();

	const FVector CharacterBase = Character->GetActorLocation() - FVector::UpVector * Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector StartTrace = CharacterBase + CharacterForward * PlacementDistance + FVector::UpVector * 25.f;
	const FVector EndTrace = StartTrace - FVector::UpVector * 50.f;

	FCollisionQueryParams TraceParams(SCTrapNames::NAME_TrapTrace);
	TraceParams.AddIgnoredActor(this);
	TraceParams.AddIgnoredActor(Character);

	FCollisionObjectQueryParams TraceChannels;
	TraceChannels.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
	TraceChannels.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);

	OutBestCollision.Reset(0.f, false);

	TArray<FHitResult> HitResults;

	if (GetWorld()->LineTraceMultiByObjectType(HitResults, StartTrace, EndTrace, TraceChannels, TraceParams))
	{
		for (const FHitResult& HitResult : HitResults)
		{
			if (!TrapValidateHitResult(HitResult))
				continue;

			if (AActor* HitActor = HitResult.GetActor())
			{
				// If we hit one of these classes, we will return false. This DO NOT allow traps to be placed on them.
				static const UClass* BlockingActorClasses[] = {
					ASCTrap::StaticClass(), // Can't stack traps
					APhysicsVolume::StaticClass() // Can't place traps on physics volumes.
				};

				for (const UClass* BlockClass : BlockingActorClasses)
				{
					if (HitActor->IsA(BlockClass))
					{
						return false;
					}
				}
			}

			// Our trace didn't go anywhere! We're probably inside of this actor
			if (HitResult.Time <= KINDA_SMALL_NUMBER)
				return false;

			// This collision result is a fine place for a trap!
			OutBestCollision = HitResult;
			break;
		}
	}

	// No collision? No floating!
	if (OutBestCollision.Location.IsZero())
	{
		return false;
	}

	// Test above (75cm from the ground (start is already 25cm up)), to make sure we're not getting placed under a table or something dumb
	HitResults.Empty();
	if (GetWorld()->LineTraceMultiByObjectType(HitResults, StartTrace, StartTrace + FVector::UpVector * 50.f, TraceChannels, TraceParams))
	{
		for (const FHitResult& HitResult : HitResults)
		{
			if (!TrapValidateHitResult(HitResult))
				continue;

			// Any collision when tracing up is bad
			return false;
		}
	}

	// Test for overlapping actors
	// Cutting our extent down a LOT since we want to be able to place more often then not, mostly we're looking for objects we're inside of
	const FCollisionShape CollisionTestSphere = FCollisionShape::MakeSphere(CollisionSphere->GetScaledSphereRadius());
	const FRotator TestRotation(0.f, (OutBestCollision.ImpactPoint - Character->GetActorLocation()).Rotation().Yaw, 0.f);
	const FVector TestLocation = OutBestCollision.Location + FVector::UpVector * CollisionSphere->GetScaledSphereRadius();
	TArray<FOverlapResult> Overlaps;
	if (GetWorld()->OverlapMultiByObjectType(Overlaps, TestLocation, FQuat(TestRotation), TraceChannels, CollisionTestSphere, TraceParams))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (!TrapValidateHitResult(Overlap) || Overlap.GetActor() == OutBestCollision.GetActor())
				continue;

			// Useful debug draw for figuring out what overlaps are preventing us from placing this trap
			/*DrawDebugBox(GetWorld(), TestLocation, CollisionBox.GetExtent(), FQuat(TestRotation), FColor::Red, true, 2.f);
			DrawDebugString(GetWorld(), TestLocation + FVector::UpVector * Extent.Z, FString::Printf(TEXT("Failed on %s"), *GetNameSafe(Overlap.GetActor())), nullptr, FColor::Red, 2.f);
			DrawDebugPoint(GetWorld(), TestLocation, 5.f, FColor::Red, true, 2.f);*/

			// We're overlapping something that doesn't like us!
			return false;
		}
	}

	return true;
}

void ASCTrap::PlaceTrap()
{
	ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(GetOwner());
	if (!Character)
		return;

	bIsPlacing = false;

	FHitResult HitResult;
	if (IsSpaceClear(HitResult))
	{
		// Remove the item from our inventory
		ASCItem* EquippedItem = Character->GetEquippedItem();
		if (EquippedItem == this)
		{
			Character->ClearEquippedItem();
		}

		FRotator NewRotation = FRotator::ZeroRotator;
		NewRotation.Yaw = (HitResult.ImpactPoint - Character->GetActorLocation()).Rotation().Yaw;

		DetachFromCharacter();
		bIsPicking = false;
		EnableInteraction(true);

		SetActorLocationAndRotation(HitResult.ImpactPoint, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

		SetOwner(nullptr);

		// Auto-arm if requested
		if (Character->IsLargeItemPressHeld())
		{
			SetOwner(Character); // OnDropped clears the owner, but we may need this for RPCs. If we get rid of the RPCs though, stop owner mangling!
			Character->GetInteractableManagerComponent()->LockInteraction(InteractComponent, EILLInteractMethod::Hold);
			ArmTrap(Character);
		}
		else
		{
			// We're done placing, free up our interaction
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			//InteractComponent->bCanHighlight = true;
		}
	}
}

void ASCTrap::OwningCharacterChanged(ASCCharacter* OldOwner)
{
	Super::OwningCharacterChanged(OldOwner);

	// Traps are finicky, let's just leave them awake all the time
	if (Role == ROLE_Authority)
	{
		SetNetDormancy(DORM_Awake);
	}
}

void ASCTrap::OnLocationReached(ASCCharacter* Interactor)
{
	// play trap arm effects
	GetMesh<USkeletalMeshComponent>()->bNoSkeletonUpdate = false;
	GetMesh<USkeletalMeshComponent>()->PlayAnimation(ArmedAnimation, false);
	StopUpdatingSkeleton(ArmedAnimation->SequenceLength);

	if (Interactor)
	{
		InteractComponent->bCanHighlight = Interactor->IsLocallyControlled() && bCanPickUp;
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		{
			ASCItem* EquippedItem = Interactor->GetEquippedItem();
			if (EquippedItem && EquippedItem != this)
				Counselor->HideLargeItem(true);
		}
	}

	ArmingAudio->Play();
}

void ASCTrap::OnTrapArmed(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	ForceNetUpdate();
	bIsArmed = true;
	bIsArming = false;

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		Counselor->HideLargeItem(false);
}

void ASCTrap::OnTrapDisarmed(ASCCharacter* Interactor)
{
	ForceNetUpdate();
	bIsArmed = false;
	TrapArmer = nullptr;

	bool bNotifyJason = true;

	if (Interactor)
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

		// this was a jason trap, remove the players pocket knife
		if (!bCanPickUp)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
			{
				Counselor->RemoveSmallItemOfType(ASCPocketKnife::StaticClass());
				bNotifyJason = false;
			}
		}
	}

	// Play triggered effect
	MULTICAST_OnTriggered(GetActorLocation(), bNotifyJason);
}

void ASCTrap::OnDisarmBeginDriver(FAnimNotifyData NotifyData)
{
	if (Role == ROLE_Authority)
		OnTrapDisarmed(Cast<ASCCharacter>(DisarmDriver->GetNotifyOwner()));

	DisarmDriver->SetNotifyOwner(nullptr);
}

void ASCTrap::OnTrapArmAborted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	ForceNetUpdate();
	bIsArming = false;
	bIsArmed = false;

	
	if (Role == ROLE_Authority)
	{
		// Did we rotate far enough to toggle being open/closed?
		if (Interactor->GetActiveSCSpecialMove()->GetTimePercentage() > 0.5f)
		{
			MULTICAST_OnTriggered(GetActorLocation());
		}
		else
		{
			// Reset the animation if we didn't rotate far enough (otherwise the arming animation continues to its end, even though the trap isn't armed) 
			GetMesh<USkeletalMeshComponent>()->SetAnimation(ArmedAnimation);
			GetMesh<USkeletalMeshComponent>()->SetPosition(0, false);
		}
	}

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		Counselor->HideLargeItem(false);
}

void ASCTrap::OnDisarmStarted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->LockInteraction(InteractComponent, EILLInteractMethod::Hold);

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		Counselor->HideLargeItem(true);
}

void ASCTrap::OnDisarmComplete(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		Counselor->HideLargeItem(false);

	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
			GameState->TrapTriggered(this, Interactor, /*bIsDisarmed=*/true);
	}
}

void ASCTrap::OnDisarmAborted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		Counselor->HideLargeItem(false);
}

void ASCTrap::MULTICAST_PlayEscape_Implementation()
{
	GetMesh<USkeletalMeshComponent>()->bNoSkeletonUpdate = false;
	GetMesh<USkeletalMeshComponent>()->PlayAnimation(EscapedAnimation, false);
	StopUpdatingSkeleton(EscapedAnimation->SequenceLength);

	Escape_Timestamp = GetWorld()->GetTimeSeconds();
}

void ASCTrap::MULTICAST_SetArmedRotation_Implementation(const FRotator NewRotation)
{
	SetActorRotation(NewRotation, ETeleportType::TeleportPhysics);
}

void ASCTrap::StopUpdatingSkeleton(const float Delay)
{
	USkeletalMeshComponent* SkeleMesh = GetMesh<USkeletalMeshComponent>();
	if (Delay <= 0.f)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_AnimationPlaying);
		TurnOffSkeletalUpdates();
	}
	else
	{
		GetWorldTimerManager().SetTimer(TimerHandle_AnimationPlaying, this, &ASCTrap::TurnOffSkeletalUpdates, Delay, false);
	}
}

void ASCTrap::TurnOffSkeletalUpdates()
{
	if (USkeletalMeshComponent* SkeleMesh = GetMesh<USkeletalMeshComponent>())
	{
		SkeleMesh->bNoSkeletonUpdate = true;
	}
}
