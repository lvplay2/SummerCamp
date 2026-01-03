// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCImpactEffects.h"

#include "ILLGameInstance.h"

#include "SCCharacterBodyPartComponent.h"
#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"
#include "SCWeapon.h"

TAutoConsoleVariable<int32> CVarDebugImpactEffects(
	TEXT("sc.DebugImpactEffects"),
	0,
	TEXT("Displays debug information for impact effects.\n")
	TEXT(" 0: None\n")
	TEXT(" 1: Display debug info"));

USCImpactEffect::USCImpactEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UWorld* USCImpactEffect::GetWorld() const
{
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void USCImpactEffect::SpawnFX(const FHitResult& Hit, const AActor* Instigator, const TSubclassOf<ASCWeapon> WeaponClass, const bool bIsHeavyImpact)
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance());
	if (!GI)
		return;

	if (IsRunningDedicatedServer())
		return;

	ImpactLocation = Hit.ImpactPoint;
	ImpactRotation = Hit.ImpactNormal.Rotation();

	EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

	// Spawn Particles
	PendingParticleSystem = GetParticleSystem(SurfaceType, Hit.GetActor());
	if (PendingParticleSystem.Get())
	{
		DeferredSpawnParticleSystem();
	}
	else if (!PendingParticleSystem.IsNull())
	{
		GI->StreamableManager.RequestAsyncLoad(PendingParticleSystem.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredSpawnParticleSystem));

		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::White, FString::Printf(TEXT("Loading Particle System: %s"), *PendingParticleSystem.ToString()), false);
	}
	else
	{
		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString(TEXT("No Particle System to Spawn!")), false);
	}

	// Play Sound
	PendingSoundCue = GetSoundCue(SurfaceType, Hit.GetActor(), bIsHeavyImpact);
	if (PendingSoundCue.Get())
	{
		DeferredPlaySoundCue();
	}
	else if (!PendingSoundCue.IsNull())
	{
		GI->StreamableManager.RequestAsyncLoad(PendingSoundCue.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::DeferredPlaySoundCue));

		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::White, FString::Printf(TEXT("Loading Sound Cue: %s"), *PendingSoundCue.ToString()), false);
	}
	else
	{
		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString(TEXT("No Sound Cue to Play!")), false);
	}

	// Spawn Decal
	const FDecalData& Decal = GetDecal(WeaponClass, SurfaceType, Hit);
	if (Decal.DecalMaterial)
	{
		FRotator SpawnRotation = (-Hit.ImpactNormal).Rotation();
		if (Decal.bApplyRandomRotation)
			SpawnRotation.Roll = FMath::RandRange(-180.f, 180.f);

		auto SpawnDecal = [](const FHitResult& HitResult, UMaterialInterface* DecalMaterial, FVector DecalSize, FRotator DecalRotation, float DecalLifeSpan, float DecalFadeScreenSize)
		{
			UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(DecalMaterial, DecalSize, HitResult.GetComponent(), HitResult.BoneName, HitResult.ImpactPoint, DecalRotation, EAttachLocation::KeepWorldPosition, DecalLifeSpan);
			if (DecalComp)
			{
				DecalComp->FadeScreenSize = DecalFadeScreenSize;
			}

			if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, FString::Printf(TEXT("Spawned Decal: %s"), *DecalMaterial->GetPathName()), false);

			return DecalComp;
		};

		const UDecalComponent* SpawnedDecal = SpawnDecal(Hit, Decal.DecalMaterial, Decal.DecalSize, SpawnRotation, Decal.LifeSpan, Decal.FadeScreenSize);

		auto PerformRayTrace = [&](FHitResult& HitResult, const bool bUseInstigatorForwardVector, const FVector Direction, const float Length)
		{
			const FVector Start = SpawnedDecal ? SpawnedDecal->GetComponentLocation() : FVector(Hit.Location);
			const FVector RotatedDir = bUseInstigatorForwardVector && Instigator ? Instigator->GetActorForwardVector().Rotation().RotateVector(Direction) : (Hit.ImpactNormal).Rotation().RotateVector(Direction);
			const FVector End = Start + RotatedDir * Length;

			static const FName NAME_SplashDecalTrace(TEXT("SplashDecalTrace"));
			FCollisionQueryParams QueryParams(NAME_SplashDecalTrace, true, Hit.GetActor());
			bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldDynamic, QueryParams);

			if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
				DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 5.f);

			return bHit;
		};

		// Spawn Splash decal(s)
		for (const FSplashDecalData& SplashData : Decal.SplashDecals)
		{
			if (SplashData.Decals.Num() > 0)
			{
				FHitResult HitResult;
				if (PerformRayTrace(HitResult, SplashData.bUseInstigatorForwardVector, SplashData.RayDirection, SplashData.RayLength))
				{
					SpawnRotation = (-HitResult.ImpactNormal).Rotation();
					if (SplashData.bApplyRandomRotation)
						SpawnRotation.Roll = FMath::RandRange(-180.f, 180.f);

					UMaterialInterface* SplashDecal = SplashData.Decals[FMath::RandHelper(SplashData.Decals.Num())];
					SpawnDecal(HitResult, SplashDecal, SplashData.DecalSize, SpawnRotation, SplashData.LifeSpan, SplashData.FadeScreenSize);

					if (Decal.bSingleSplashDecal)
						break;
				}
			}
		}
	}
}

TAssetPtr<UParticleSystem> USCImpactEffect::GetParticleSystem(TEnumAsByte<EPhysicalSurface> SurfaceType, AActor* HitActor) const
{
	if (HitActor)
	{
		if (HitActor->IsA<ASCKillerCharacter>())
		{
			return JasonFX;
		}
		else if (HitActor->IsA<ASCCounselorCharacter>())
		{
			return CounselorFX;
		}
	}

	switch (SurfaceType)
	{
	case SC_SURFACE_WoodHeavy:		return WoodHeavyFX;
	case SC_SURFACE_WoodMedium:		return WoodMediumFX;
	case SC_SURFACE_WoodLight:		return WoodLightFX;
	case SC_SURFACE_WoodHollow:		return WoodHollowFX;
	case SC_SURFACE_Sand:			return SandFX;
	case SC_SURFACE_Gravel:			return GravelFX;
	case SC_SURFACE_DirtDry:		return DirtDryFX;
	case SC_SURFACE_DirtWet:		return DirtWetFX;
	case SC_SURFACE_Mud:			return MudFX;
	case SC_SURFACE_Grass:			return GrassFX;
	case SC_SURFACE_LeafLitter:		return LeafLitterFX;
	case SC_SURFACE_WaterDeep:		return WaterDeepFX;
	case SC_SURFACE_WaterShallow:	return WaterShallowFX;
	case SC_SURFACE_MetalThick:		return MetalThickFX;
	case SC_SURFACE_MetalMedium:	return MetalMediumFX;
	case SC_SURFACE_MetalLight:		return MetalLightFX;
	case SC_SURFACE_MetalHollow:	return MetalHollowFX;
	case SC_SURFACE_Rock:			return RockFX;
	case SC_SURFACE_LargeFoliage:	return LargeFoliageFX;
	case SC_SURFACE_SmallFoliage:	return SmallFoliageFX;
	case SC_SURFACE_Cloth:			return ClothFX;
	case SC_SURFACE_WoodDoor:		return WoodDoorFX;
	}

	return DefaultFX;
}

void USCImpactEffect::DeferredSpawnParticleSystem()
{
	if (PendingParticleSystem.Get())
	{
		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, FString::Printf(TEXT("Spawning Particle System: %s"), *PendingParticleSystem.Get()->GetPathName()), false);

		UGameplayStatics::SpawnEmitterAtLocation(this, PendingParticleSystem.Get(), ImpactLocation, ImpactRotation);
	}
	else
	{
		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString::Printf(TEXT("Tried to Spawn Particle System %s, but it Wasn't Loaded!"), *PendingParticleSystem.ToString()), false);
	}
}

TAssetPtr<USoundCue> USCImpactEffect::GetSoundCue(TEnumAsByte<EPhysicalSurface> SurfaceType, AActor* HitActor, bool HeavyImpact /*= false*/) const
{
	if (HitActor)
	{
		if (const ASCCharacter* HitCharacter = Cast<ASCCharacter>(HitActor))
		{
			if (HitCharacter->IsBlocking())
			{
				return BlockingSound;
			}
		}
	}

	switch (SurfaceType)
	{
	case SC_SURFACE_WoodHeavy:		return WoodHeavySound;
	case SC_SURFACE_WoodMedium:		return WoodMediumSound;
	case SC_SURFACE_WoodLight:		return WoodLightSound;
	case SC_SURFACE_WoodHollow:		return WoodHollowSound;
	case SC_SURFACE_Sand:			return SandSound;
	case SC_SURFACE_Gravel:			return GravelSound;
	case SC_SURFACE_DirtDry:		return DirtDrySound;
	case SC_SURFACE_DirtWet:		return DirtWetSound;
	case SC_SURFACE_Mud:			return MudSound;
	case SC_SURFACE_Grass:			return GrassSound;
	case SC_SURFACE_LeafLitter:		return LeafLitterSound;
	case SC_SURFACE_WaterDeep:		return WaterDeepSound;
	case SC_SURFACE_WaterShallow:	return WaterShallowSound;
	case SC_SURFACE_MetalThick:		return MetalThickSound;
	case SC_SURFACE_MetalMedium:	return MetalMediumSound;
	case SC_SURFACE_MetalLight:		return MetalLightSound;
	case SC_SURFACE_MetalHollow:	return MetalHollowSound; 
	case SC_SURFACE_Rock:			return RockSound;
	case SC_SURFACE_LargeFoliage:	return LargeFoliageSound;
	case SC_SURFACE_SmallFoliage:	return SmallFoliageSound;
	case SC_SURFACE_Cloth:			return ClothSound;
	case SC_SURFACE_WoodDoor:		return WoodDoorSound;
	}

	return HeavyImpact ? DefaultSoundHeavy : DefaultSound;
}

void USCImpactEffect::DeferredPlaySoundCue()
{
	if (PendingSoundCue.Get())
	{
		if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Playing Impact Sound: %s"), *PendingSoundCue.Get()->GetPathName()));
		}

		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PendingSoundCue.Get(), ImpactLocation);
	}
	else if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Tried to Play Impact Sound %s, but it Wasn't Loaded!"), *PendingSoundCue.ToString()));
	}
}

const FDecalData& USCImpactEffect::GetDecal(const TSubclassOf<ASCWeapon> WeaponClass, TEnumAsByte<EPhysicalSurface> SurfaceType, const FHitResult& Hit) const
{
	auto DecalHelper = [&](const FDecalData& Decal) -> const FDecalData&
	{
		if (Decal.DecalMaterial)
		{
			return Decal;
		}
	
		return DefaultDecal;
	};

	if (Hit.GetActor())
	{
		if (Hit.GetActor()->IsA<ASCKillerCharacter>())
		{
			return JasonDecal;
		}
		else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Hit.GetActor()))
		{
			if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, *FString::Printf(TEXT("Hit Counselor Bone: %s. Trying to apply blood"), *Hit.BoneName.ToString()));
			}

			if (!Counselor->IsBlocking())
			{
				if (USCCharacterBodyPartComponent* HitLimb = Counselor->GetBodyPartWithBone(Hit.BoneName))
				{
					// Get gore data from hit limb
					FGoreMaskData LimbGore;
					HitLimb->GetDamageGoreInfo(WeaponClass, LimbGore);

					// Apply gore to the hit counselor
					Counselor->ApplyGore(LimbGore);

					if (CVarDebugImpactEffects.GetValueOnGameThread() > 0)
					{
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *FString::Printf(TEXT("Applied blood to %s."), *HitLimb->GetName()));
					}
				}
			}

			return CounselorDecal;
		}
	}

	switch (SurfaceType)
	{
	case SC_SURFACE_WoodHeavy:		return DecalHelper(WoodHeavyDecal);
	case SC_SURFACE_WoodMedium:		return DecalHelper(WoodMediumDecal);
	case SC_SURFACE_WoodLight:		return DecalHelper(WoodLightDecal);
	case SC_SURFACE_WoodHollow:		return DecalHelper(WoodHollowDecal);
	case SC_SURFACE_Sand:			return DecalHelper(SandDecal);
	case SC_SURFACE_Gravel:			return DecalHelper(GravelDecal);
	case SC_SURFACE_DirtDry:		return DecalHelper(DirtDryDecal);
	case SC_SURFACE_DirtWet:		return DecalHelper(DirtWetDecal);
	case SC_SURFACE_Mud:			return DecalHelper(MudDecal);
	case SC_SURFACE_Grass:			return DecalHelper(GrassDecal);
	case SC_SURFACE_LeafLitter:		return DecalHelper(LeafLitterDecal);
	case SC_SURFACE_WaterDeep:		return DecalHelper(WaterDeepDecal);
	case SC_SURFACE_WaterShallow:	return DecalHelper(WaterShallowDecal);
	case SC_SURFACE_MetalThick:		return DecalHelper(MetalThickDecal);
	case SC_SURFACE_MetalMedium:	return DecalHelper(MetalMediumDecal);
	case SC_SURFACE_MetalLight:		return DecalHelper(MetalLightDecal);
	case SC_SURFACE_MetalHollow:	return DecalHelper(MetalHollowDecal);
	case SC_SURFACE_Rock:			return DecalHelper(RockDecal);
	case SC_SURFACE_LargeFoliage:	return DecalHelper(LargeFoliageDecal);
	case SC_SURFACE_SmallFoliage:	return DecalHelper(SmallFoliageDecal);
	case SC_SURFACE_Cloth:			return DecalHelper(ClothDecal);
	case SC_SURFACE_WoodDoor:		return DecalHelper(WoodDoorDecal);
	}

	return DefaultDecal;
}
