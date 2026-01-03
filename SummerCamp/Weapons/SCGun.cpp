// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGun.h"

#include "SCCounselorCharacter.h"
#include "SCImpactEffects.h"
#include "SCProjectile.h"
#include "SCPlayerState.h"
#include "SCPerkData.h"
#include "SCGameState.h"
#include "SCWindow.h"
#include "Animation/AnimInstance.h"

/*
				WARNING: THIS CLASS USES NET DORMANCY
	This means that if you need to update any replicated variables,
	logic, RPCs or other networking whatnot, you NEED to update the
	logic handling for dormancy.
*/

TAutoConsoleVariable<int32> CVarDebugGun(TEXT("sc.DebugGun"), 0,
	TEXT("Displays debug information for Guns.\n")
	TEXT(" 0: Off\n")
	TEXT(" 1: On"));

ASCGun::ASCGun(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, MaxSpreadAngle(15.f)
, NumPellets(5)
, bHasAmmunition(true)
, MuzzleFlashParicles(nullptr)
, ProjectileVelocity(2000.f)
, CounselorDamageMultiplier(200.f)
, FireDistance(2000.f)
, FireImpulse(250.f)
, FireMontage(nullptr)
{
	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle Location"));
	Muzzle->SetupAttachment(RootComponent);
}

void ASCGun::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCGun, bHasAmmunition);
	DOREPLIFETIME(ASCGun, bIsAiming);
}

void ASCGun::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_MeleeSwap);
	}

	Super::EndPlay(EndPlayReason);
}

int32 ASCGun::CanInteractWith(class AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (!bHasAmmunition)
		return 0;

	return Super::CanInteractWith(Character, ViewLocation, ViewRotation);
}

void ASCGun::OnDropped(ASCCharacter* DroppingCharacter, FVector AtLocation)
{
	Super::OnDropped(DroppingCharacter, AtLocation);

	// Stop aiming the gun so it doesn't get stuck in this state for the next person to pick it up
	bIsAiming = false;

	// if the gun is out of ammo, have it delete itself after 10 seconds
	if (!bHasAmmunition)
	{
		SetLifeSpan(10.0f);
	}
}

void ASCGun::EnableInteraction(bool bEnable)
{
	Super::EnableInteraction(bHasAmmunition ? bEnable : false);
}

void ASCGun::SetShimmerEnabled(const bool bEnabled)
{
	Super::SetShimmerEnabled(bHasAmmunition ? bEnabled : false);
}

bool ASCGun::Use(bool bFromInput)
{
	if (bHasAmmunition)
	{
		if (bIsAiming)
		{
			Fire();
		}
		return false;
	}
	
	return false;
}

UAnimSequenceBase* ASCGun::GetIdlePose(const USkeleton* Skeleton) const
{
	if (bHasAmmunition)
	{
		return LoadedIdle;
	}
	
	return Super::GetIdlePose(Skeleton);
}

void ASCGun::AimGun(bool Aiming)
{
	if (Role < ROLE_Authority)
	{
		SERVER_AimGun(Aiming);
	}

	if (bHasAmmunition || !Aiming)
	{
		bIsAiming = Aiming;
	}
}

bool ASCGun::SERVER_AimGun_Validate(bool Aiming)
{
	return true;
}

void ASCGun::SERVER_AimGun_Implementation(bool Aiming)
{
	AimGun(Aiming);
}

void ASCGun::Fire()
{
	if (!bHasAmmunition)
		return;

	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(GetOwner()))
	{
		HitActors.Empty();
		FVector ViewForward = CachedCameraForward.Vector();
		FVector CameraLocation = CachedCameraLocation;

		FCollisionQueryParams SC_TraceParams = FCollisionQueryParams(FName(TEXT("PelletTrace")), true, this);
		SC_TraceParams.bTraceComplex = false;
		SC_TraceParams.bTraceAsyncScene = true;
		SC_TraceParams.bReturnPhysicalMaterial = false;
		SC_TraceParams.AddIgnoredActor(Character);
		SC_TraceParams.IgnoreMask = ECC_Killer | ECC_Pawn;

		// attempt to ingore the killer if he cant take damage
		if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
		{
			if (GS->CurrentKiller)
			{
				if (!GS->CurrentKiller->CanTakeDamage())
					SC_TraceParams.AddIgnoredActor(GS->CurrentKiller);
			}
		}

		// create trace for each pellet
		for (uint8 pellet = 0; pellet < NumPellets; ++pellet)
		{
			float angle = FMath::FRandRange(0.f, MaxSpreadAngle);
			FVector pelletRay = ViewForward.RotateAngleAxis(angle, FVector::CrossProduct(FVector::UpVector, ViewForward));
			pelletRay = pelletRay.RotateAngleAxis(FMath::FRandRange(0.f, 360.f), ViewForward);

			float sinAngle =  FMath::DegreesToRadians(180.f - (90.f + FMath::Abs(angle)));
			float newDist = FireDistance / FMath::Sin(sinAngle);

			FHitResult ShotgunHit(ForceInit);

			if (ProjectileClass)
				FireProjectile(Character, pelletRay);

			if (ProjectileFireOnly)
				continue;

			GetWorld()->LineTraceSingleByChannel(ShotgunHit, CameraLocation, CameraLocation + (pelletRay * newDist), ECC_PhysicsBody, SC_TraceParams);

			if (CVarDebugGun.GetValueOnGameThread() > 0)
			{
				DrawDebugLine(GetWorld(), CameraLocation, CameraLocation + (pelletRay * newDist), FColor::Red, false, 6.f, 0, 2.f);
			}

			if (ShotgunHit.bBlockingHit)
			{
				if (!HitActors.Contains(ShotgunHit.GetActor()))
				{
					HitActors.Add(ShotgunHit.GetActor(), TArray<FVector>());
				}
				HitActors[ShotgunHit.GetActor()].Add(ShotgunHit.ImpactPoint);

				if (ASCWindow* HitWindow = Cast<ASCWindow>(ShotgunHit.GetActor()))
				{
					SC_TraceParams.AddIgnoredActor(HitWindow);

					// magic number: Any damage destroys the window. (even 0) 
					HitWindow->TakeDamage(1.f, FDamageEvent(), Character->Controller, Character);


					// Do another single line trace here
					// using the same data for this window hit.
					// "shooting through the window effect"
					GetWorld()->LineTraceSingleByChannel(ShotgunHit, CameraLocation, CameraLocation + (pelletRay * newDist), ECC_PhysicsBody, SC_TraceParams);

					if (ShotgunHit.bBlockingHit)
					{
						if (!HitActors.Contains(ShotgunHit.GetActor()))
						{
							HitActors.Add(ShotgunHit.GetActor(), TArray<FVector>());
						}
						HitActors[ShotgunHit.GetActor()].Add(ShotgunHit.ImpactPoint);
					}
				}
			

				// don't spawn ribbons if we're already launching projectiles.
				if (!ProjectileClass)
				{
					// Can't serialize nested containers, Make this a struct so we can pass all ribbon data at once.
					MULTICAST_SpawnRibbon(ShotgunHit.ImpactPoint);
					MULTICAST_SpawnPelletImpact(ShotgunHit);
				}
			}
			else
			{
				// don't spawn ribbons if we're already launching projectiles.
				if (!ProjectileClass)
				{
					MULTICAST_SpawnRibbon(CameraLocation + (pelletRay * newDist));
				}
			}
		}

		bool bShotCounselor = false;
		bool bShotJason = false;
		bool bStunnedJason = false;
		// do something with the hit actor list.
		for (const auto& Elem : HitActors)
		{
			if (ASCCharacter* HitCharacter = Cast<ASCCharacter>(Elem.Key))
			{
				const float AccumulatedDamage = WeaponStats.Damage * Elem.Value.Num();
				if (ASCCounselorCharacter* ShotCounselor = Cast<ASCCounselorCharacter>(HitCharacter))
				{
					// Insta-kill counselors
					ShotCounselor->TakeDamage(AccumulatedDamage * CounselorDamageMultiplier, FDamageEvent(), Character->Controller, Character);
					bShotCounselor = true;
				}
				else if (ASCKillerCharacter* ShotKiller = Cast<ASCKillerCharacter>(HitCharacter))
				{
					bShotJason = true;
					if (WeaponStats.WeaponStunClass->IsChildOf(USCStunDamageType::StaticClass()))
					{
						ShotKiller->TakeDamage(AccumulatedDamage, FDamageEvent(WeaponStats.WeaponStunClass), Character->Controller, Character);
						bStunnedJason = true;
					}
					else
					{
						ShotKiller->TakeDamage(AccumulatedDamage, FDamageEvent(), Character->Controller, Character);
					}
				}
			}
		}

		// Track use and hits
		if (ASCPlayerState* PS = Character->GetPlayerState())
		{
			uint8 StatFlags = (uint8)ESCItemStatFlags::Used;

			if (bShotCounselor)
				StatFlags |= (uint8)ESCItemStatFlags::HitCounselor;
			if (bShotJason)
				StatFlags |= (uint8)ESCItemStatFlags::HitJason;
			if (bStunnedJason)
				StatFlags |= (uint8)ESCItemStatFlags::Stun;

			PS->TrackItemUse(GetClass(), StatFlags);
		}

		MULTICAST_PlayGunAnim();

		if (!Character->GetInfiniteAmmo())
		{
			ForceNetUpdate();
			bHasAmmunition = false;
		}
	}

	bIsAiming = false;
}

void ASCGun::FireProjectile(ASCCharacter* Character, FVector InitialVelocity)
{
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Character;

		FVector CameraToMuzzle = CachedCameraLocation - Muzzle->GetComponentLocation();
		FVector SpawnLocation = CachedCameraLocation + CachedCameraForward.Vector() * CameraToMuzzle.Size();
		ASCProjectile* Projectile = World->SpawnActor<ASCProjectile>(ProjectileClass, SpawnLocation, Muzzle->GetComponentRotation(), SpawnParams);
		if (Projectile)
		{
			float LifeSpan = ProjectileLifespan;
			if (ASCPlayerState* PS = Character->GetPlayerState())
			{
				for (const USCPerkData* Perk : PS->GetActivePerks())
				{
					check(Perk);

					if (!Perk)
						continue;

					LifeSpan += Perk->FlareLifeSpanIncrease;
				}
			}

			Projectile->SetLifeSpan(LifeSpan);
			Projectile->InitVelocity(InitialVelocity.GetSafeNormal() * ProjectileVelocity);
		}
	}
}

void ASCGun::MULTICAST_PlayGunAnim_Implementation()
{
	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(GetOwner()))
	{
		// play aim montage.
		UAnimInstance* AnimInst = (Character->GetMesh() != nullptr) ? Character->GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInst && FireMontage)
		{
			float PlayLength = AnimInst->Montage_Play(FireMontage);
			if (APlayerController* PlayerController = Cast<APlayerController>(Character->Controller))
			{
				PlayerController->SetCinematicMode(false, true, false);
			}

			// If our cvar is set we don't want to auto drop the item.
			if (Character->GetInfiniteAmmo())
				return;

			// Note that we are now picking the item so that we can't interact while dropping.
			bIsPicking = true;

			if (Role == ROLE_Authority)
			{
				// Drop the gun if there's no more ammo.
				if (PlayLength > 0.f)
				{
					FTimerDelegate ThrowAway;
					ThrowAway.BindLambda([this](ASCCounselorCharacter* Counselor)
					{
						if (!IsValid(Counselor) || !Counselor->IsAlive())
							return;

						// TODO: Check if the counselor has more ammo.
						Counselor->SERVER_SetDroppingLargeItem(true);
					}, Character);

					GetWorldTimerManager().SetTimer(TimerHandle_MeleeSwap, ThrowAway, PlayLength - 0.05f, false);
				}
				else
				{
					if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
					{
						// TODO: Check if the counselor has more ammo.
						if (Counselor->IsAlive())
							Counselor->SERVER_SetDroppingLargeItem(true);
					}
				}
			}
		}
	}
}

void ASCGun::MULTICAST_SpawnRibbon_Implementation(const FVector& ImpactPoint)
{
	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(GetOwner()))
	{
		if (!Character->GetInfiniteAmmo())
		{
			ForceNetUpdate();
			bHasAmmunition = false;
		}
	}

	UParticleSystemComponent* Spawned = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),PelletRibbon, Muzzle->GetComponentTransform());
	if (Spawned)
	{
		Spawned->SetBeamSourcePoint(0, Muzzle->GetComponentLocation(), 0);
		Spawned->SetBeamTargetPoint(0, ImpactPoint, 0);
	}
	if (MuzzleFlashParicles == nullptr)
		MuzzleFlashParicles = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, Muzzle->GetComponentTransform());
}

void ASCGun::MULTICAST_SpawnPelletImpact_Implementation(const FHitResult& Impact)
{
	if (IsRunningDedicatedServer())
		return;

	if (UWorld* World = GetWorld())
	{
		USCImpactEffect* ImpactFX = NewObject<USCImpactEffect>(World, *FString::Printf(TEXT("%s_Impact"), *GetName()), RF_Transient, PelletImpactTemplate->GetDefaultObject());
		if (ImpactFX)
		{
			ImpactFX->SpawnFX(Impact, GetOwner(), GetClass(), false);
		}
	}
}
