// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWeapon.h"

#include "Animation/AnimMontage.h"

#include "ILLInventoryComponent.h"

#include "SCAnimInstance.h"
#include "SCCharacter.h"
#include "SCCounselorCharacter.h"
#include "SCDestructableWall.h"
#include "SCDoor.h"
#include "SCImpactEffects.h"
#include "SCInteractComponent.h"
#include "SCPlayerState.h"
#include "SCWindow.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCWeapon"

TAutoConsoleVariable<float> CVarAttackRotationIteration(TEXT("sc.AttackRotationIteration"), 15.f, TEXT("Max number of degrees rotation between frames to allow before requiring extra traces"));

// Cap on the number of traces to perform in a frame so we don't murder our frame rate, just our friends
#define MAX_WEAPON_TRACE_ITERATIONS 9

ASCWeapon::ASCWeapon(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bIsLarge = true;

	FriendlyName = LOCTEXT("FriendlyName", "Weapon");

	HandAttachSocket = TEXT("WeaponSocket");

	KillVolume = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("DamageVolume"));
	KillVolume->SetupAttachment(GetMesh());
}

void ASCWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CurrentDurability = WeaponStats.Durability;
}

void ASCWeapon::OnDropped(ASCCharacter* DroppingCharacter, FVector AtLocation /*= FVector::ZeroVector*/)
{
	// Disable collision when dropping so weapon rest ON the ground
	const float PrevRadius = KillVolume->GetUnscaledCapsuleRadius();
	KillVolume->SetCapsuleRadius(0.f, false);
	Super::OnDropped(DroppingCharacter, AtLocation);
	KillVolume->SetCapsuleRadius(PrevRadius, false);
}

void ASCWeapon::OwningCharacterChanged(ASCCharacter* OldOwner)
{
	Super::OwningCharacterChanged(OldOwner);

	if (OwningCharacter)
	{
		if (ASCCounselorCharacter* NewCounselor = Cast<ASCCounselorCharacter>(OwningCharacter))
			CurrentDurability = FMath::Max(CurrentDurability * NewCounselor->GetWeaponDurabilityModifier(), 1.f);

		KillVolume->IgnoreActorWhenMoving(OwningCharacter, true);
		OwningCharacter->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
	}
	else if (OldOwner)
	{
		if (ASCCounselorCharacter* OldCounselor = Cast<ASCCounselorCharacter>(OldOwner))
			CurrentDurability = FMath::Max(CurrentDurability / OldCounselor->GetWeaponDurabilityModifier(), 1.f);

		KillVolume->IgnoreActorWhenMoving(OldOwner, false);
		OldOwner->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	}
}

void ASCWeapon::GetSweepInfo(TArray<FHitResult>& OutResults, const FVector& PreviousLocation, const FRotator& PreviousRotation, const TArray<AActor*>& ActorsToIgnore/* = {}*/, const TArray<UPrimitiveComponent*>& ComponentsToIgnore/* = {}*/) const
{
	UWorld* World = GetWorld();
	if (!World || !KillVolume)
		return;

	AActor* OwningActor = GetOwner();

	static const FName NAME_SweepTag(TEXT("WeaponSweepTrace"));
	FCollisionQueryParams QueryParams(NAME_SweepTag, true);
	QueryParams.bReturnPhysicalMaterial = true;
	QueryParams.bReturnFaceIndex = true;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningActor);
	QueryParams.AddIgnoredActors(ActorsToIgnore);
	QueryParams.AddIgnoredComponents(ComponentsToIgnore);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Camera, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Boat, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_DynamicContextBlocker, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Foliage, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Killer, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Visibility, ECR_Ignore);

	OutResults.Empty();

	// Figure out how many traces we're going to do for this attack
	int32 Iteration = 0;
	int32 AttackTraceIterations = (int32)FMath::Max(1.f, (KillVolume->GetComponentLocation() - PreviousLocation).Size() / (KillVolume->GetScaledCapsuleRadius() * 2.f));
	AttackTraceIterations = (int32)FMath::Max((float)AttackTraceIterations, FMath::FindDeltaAngleDegrees(KillVolume->GetComponentRotation().Yaw, PreviousRotation.Yaw) / CVarAttackRotationIteration.GetValueOnGameThread());
	AttackTraceIterations = FMath::Min(AttackTraceIterations, MAX_WEAPON_TRACE_ITERATIONS);

#if !UE_BUILD_SHIPPING
	if (CVarDebugCombat.GetValueOnGameThread() >= 1)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, FString::Printf(TEXT("Performing %i Attack iterations"), AttackTraceIterations));
	}
#endif // !UE_BUILD_SHIPPING

	while (++Iteration <= AttackTraceIterations)
	{
		// Get the sub-position for this iteration of the swing
		const float Delta = (float)Iteration / (float)AttackTraceIterations;
		const FRotator Rotation = FMath::Lerp(PreviousRotation, KillVolume->GetComponentRotation(), Delta);
		const FVector Start = FMath::Lerp(PreviousLocation, KillVolume->GetComponentLocation(), Delta);

		// Trace the position of the weapon
		FHitResult Hit;
		if (World->SweepSingleByChannel(Hit, Start, KillVolume->GetComponentLocation(), Rotation.Quaternion(), ECC_WorldDynamic, KillVolume->GetCollisionShape(), QueryParams, ResponseParams))
			OutResults.Add(Hit);

		// Trace from the weapon back to the owner to see if we're swinging around a character (they're inside our arc)
		if (OwningActor)
		{
			if (World->LineTraceSingleByChannel(Hit, Start, OwningActor->GetActorLocation(), ECC_WorldDynamic, QueryParams, ResponseParams))
				OutResults.Add(Hit);
		}

#if !UE_BUILD_SHIPPING
		if (CVarDebugCombat.GetValueOnGameThread() >= 1)
		{
			DrawDebugCapsule(World, Start, KillVolume->GetCollisionShape().GetCapsuleHalfHeight(), KillVolume->GetCollisionShape().GetCapsuleRadius(), Rotation.Quaternion(), FColor::Red, false, 5.f);
			if (OwningActor)
				DrawDebugLine(World, Start, OwningActor->GetActorLocation(), FColor::Red, false, 5.f);
		}
#endif // !UE_BUILD_SHIPPING
	}
}

bool ASCWeapon::CanGetStuckIn(const AActor* HitActor, const float PenetrationDepth, const float ImpactAngle, const TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	// Check stuck chance to start
	const int StuckChance = FMath::RandRange(0, 100);
	if (StuckChance >= WeaponStats.StuckChance)
		return false;

	// Check penetration depth
	if (PenetrationDepth < GetMinimumStuckPenetration())
		return false;

	// Check impact angle
	if (ImpactAngle > GetMaximumStuckAngle())
		return false;

	// Actor specific checks
	if (HitActor)
	{
		if (const ASCDoor* Door = Cast<ASCDoor>(HitActor))
		{
			return false;
		}
		else if (const ASCDestructableWall* Wall = Cast<ASCDestructableWall>(HitActor))
		{
			return false;
		}
		else if (HitActor->IsA<ASCWindow>())
		{
			return false;
		}
		else if (HitActor->IsA<ASCCharacter>())
		{
			return false;
		}
	}

	// Check if this is a surface type this weapon can get stuck in
	return WeaponStats.StuckSurfaceTypes.Contains(SurfaceType);
}

void ASCWeapon::WearDurability(bool FromAttack)
{
	CurrentDurability -= (FromAttack ? WeaponStats.AttackingWear : WeaponStats.BlockingWear);

	if (CurrentDurability <= 0.0f)
	{
		BreakWeapon();
	}
}

FAttackMontage ASCWeapon::GetBestAsset(const USkeleton* Skeleton, const bool bIsFemale, const TArray<FAttackMontage>& MontageArray) const
{
	FAttackMontage AttackMontage = FAttackMontage(); // Default in case no match is found
	for (const FAttackMontage& Montage : MontageArray)
	{
		if (Montage.AttackMontage && Montage.AttackMontage->GetSkeleton() == Skeleton && (Montage.RecoilMontage == nullptr || Montage.RecoilMontage->GetSkeleton() == Skeleton))
		{
			if (Montage.bIsFemaleMontages == bIsFemale)
			{
				return Montage;
			}
			else
			{
				AttackMontage = Montage; // Close match
			}
		}
	}

	// Return the closest match
	return AttackMontage;
}

void ASCWeapon::DestroyWeapon()
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwner()))
	{
		Character->DropEquippedItem();
		if (Character->InCombatStance())
			Character->CLIENT_SetCombatStance(false);
	}
	Destroy();
}

void ASCWeapon::BreakWeapon()
{
	if (Role < ROLE_Authority)
	{
		SERVER_BreakWeapon();
	}
	else
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwner()))
		{
			Character->ClearEquippedItem();
			if (Character->InCombatStance())
			{
				Character->CLIENT_SetCombatStance(false);
			}
		}

		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		MULTICAST_BreakWeapon();
	}
}

bool ASCWeapon::SERVER_BreakWeapon_Validate()
{
	return true;
}

void ASCWeapon::SERVER_BreakWeapon_Implementation()
{
	BreakWeapon();
}

void ASCWeapon::MULTICAST_BreakWeapon_Implementation()
{
	if (!GetOwner())
		return;

	GetMesh()->SetHiddenInGame(true);

	FVector SpawnLocation = GetOwner()->GetActorLocation();
	for (UStaticMesh* Part : BrokenParts)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;
		if (AStaticMeshActor* BrokenMesh = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnParams))
		{
			BrokenMesh->SetMobility(EComponentMobility::Movable);
			BrokenMesh->SetActorLocation(SpawnLocation);
			BrokenMesh->GetStaticMeshComponent()->SetStaticMesh(Part);
			BrokenMesh->SetActorEnableCollision(true);
			BrokenMesh->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
			BrokenMesh->GetStaticMeshComponent()->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
			BrokenMesh->GetStaticMeshComponent()->SetSimulatePhysics(true);
			BrokenMesh->SetLifeSpan(15.0f);
			SpawnLocation += FVector(0.f, 0.f, -0.5f);
		}
	}

	ForceNetUpdate();
	InteractComponent->bIsEnabled = false;
	SetActorTickEnabled(false);
	SetLifeSpan(1.0f);
}

void ASCWeapon::SpawnHitEffect(const FHitResult& Hit)
{
	if (Role < ROLE_Authority)
	{
		SERVER_SpawnHitEffect(Hit);
	}
	else
	{
		if (Hit.ImpactPoint != FVector::ZeroVector)
		{
			MULTICAST_SpawnHitEffect(Hit);
		}
	}
}

bool ASCWeapon::SERVER_SpawnHitEffect_Validate(const FHitResult& Hit)
{
	return true;
}

void ASCWeapon::SERVER_SpawnHitEffect_Implementation(const FHitResult& Hit)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwner()))
	{
		Character->MakeStealthNoise(.25f, Character, Hit.Location);
	}

	SpawnHitEffect(Hit);
}

void ASCWeapon::MULTICAST_SpawnHitEffect_Implementation(const FHitResult& Hit)
{
	if (IsRunningDedicatedServer())
		return;

	// Spawn and play all the impact effects for this hit.
	UWorld* World = GetWorld();
	if (Hit.GetActor() && World)
	{
		USCImpactEffect* ImpactFX = NewObject<USCImpactEffect>(World, *FString::Printf(TEXT("%s_Impact"), *GetName()), RF_Transient, ImpactEffectClass->GetDefaultObject());
		if (ImpactFX)
		{
			bool bIsHeavyImpact = false;
			if (ASCCharacter* CharacterOwner = Cast<ASCCharacter>(GetOwner()))
			{
				if (USkeletalMeshComponent* OwnerMesh = CharacterOwner->GetMesh())
				{
					if (USCAnimInstance* AnimInst = Cast<USCAnimInstance>(OwnerMesh->GetAnimInstance()))
					{
						bIsHeavyImpact = AnimInst->IsHeavyAttack();
					}
				}
			}

			ImpactFX->SpawnFX(Hit, GetOwner(), GetClass(), bIsHeavyImpact);
		}
	}
}

bool ASCWeapon::Use(bool bFromInput)
{
	// Weapons attack on use
	if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwner()))
	{
		if (Character->CanAttack())
		{
			Character->OnAttack();

			if (ASCPlayerState* PS = Character->GetPlayerState())
			{
				PS->TrackItemUse(GetClass(), (uint8)ESCItemStatFlags::Used);
			}
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
