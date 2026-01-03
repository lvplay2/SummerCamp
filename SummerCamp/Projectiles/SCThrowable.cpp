// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCThrowable.h"

#include "SCCounselorCharacter.h"
#include "SCImpactEffects.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCWindow.h"

ASCThrowable::ASCThrowable(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TimeToDestroy(5.0f)
{
	SetReplicates(true);
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	ThrowAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ThrowAudio"));
	ThrowAudioComponent->SetupAttachment(RootComponent);
}

void ASCThrowable::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ASCThrowable::OnStop);
}

void ASCThrowable::OnStop(const FHitResult& HitResult)
{
	bool bDestroyNow = false;

	if (Role == ROLE_Authority)
	{
		SetActorLocation(HitResult.ImpactPoint - GetActorForwardVector() * Root->GetScaledSphereRadius(), false, nullptr, ETeleportType::TeleportPhysics);
		AttachToComponent(HitResult.GetComponent(), FAttachmentTransformRules::KeepWorldTransform, HitResult.BoneName);
	}

	if (HitResult.GetActor())
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(HitResult.GetActor()))
		{
			bool validHit = true;
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				if (Counselor->IsInHiding())
					validHit = false;
			}

			if (validHit)
			{
				if (Role == ROLE_Authority)
					Character->TakeDamage(WeaponStats.Damage, FDamageEvent(WeaponStats.WeaponDamageClass), ThrownBy->Controller, ThrownBy);

				// Track hits
				if (ASCPlayerState* PS = ThrownBy->GetPlayerState())
					PS->ThrowableHitCounselor(GetClass());

				Character->MoveIgnoreActorAdd(this);

				if (ThrownBy && ThrownBy->IsLocallyControlled())
				{
					UGameplayStatics::PlaySound2D(GetWorld(), JasonImpactNoise);
				}
			}
		}
		else if (ASCWindow* Window = Cast<ASCWindow>(HitResult.GetActor()))
		{
			if (Role == ROLE_Authority)
			{
				Window->TakeDamage(WeaponStats.Damage, FDamageEvent(WeaponStats.WeaponDamageClass), ThrownBy->Controller, ThrownBy);
				bDestroyNow = true;
			}
		}
	}

	SetActorEnableCollision(false);

	if (Role == ROLE_Authority)
	{
		if (bDestroyNow)
			Destroy();
		else
			SetLifeSpan(TimeToDestroy);
	}

	// Spawn and play all the impact effects for this hit.
	UWorld* World = GetWorld();
	if (World)
	{
		//ASCImpactEffects* EffectActor = World->GetWorld()->SpawnActorDeferred<ASCImpactEffects>(ImpactTemplate, FTransform(HitResult.ImpactNormal.Rotation(), HitResult.ImpactPoint));
		//if (EffectActor)
		//{
		//	EffectActor->OwningWeapon = GetClass();
		//	EffectActor->SurfaceHit = HitResult;
		//	UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(HitResult.ImpactNormal.Rotation(), HitResult.ImpactPoint));
		//}
	}

	if (ParticleSystem)
	{
		ParticleSystem->KillParticlesForced();
		ParticleSystem->DeactivateSystem();
	}

	if (ThrowAudioComponent)
		ThrowAudioComponent->Stop();
}

void ASCThrowable::Use(AActor* Interactor)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
	{
		if (Character->IsLocallyControlled())
		{
			const FVector CamForward = Character->GetCachedCameraInfo().Rotation.Vector().GetSafeNormal();
			const FVector LaunchLocation = Character->GetCachedCameraInfo().Location + CamForward * 120.0f;
			SERVER_Use(Interactor, GetThrowVelocity(Interactor, true), LaunchLocation);
		}
	}
}

bool ASCThrowable::SERVER_Use_Validate(AActor* Interactor, FVector_NetQuantize NewVelocity, FVector_NetQuantize CameraLocation)
{
	return true;
}

void ASCThrowable::SERVER_Use_Implementation(AActor* Interactor, FVector_NetQuantize NewVelocity, FVector_NetQuantize LaunchLocation)
{
	MULTICAST_Use(Interactor, NewVelocity, LaunchLocation);
}

void ASCThrowable::MULTICAST_Use_Implementation(AActor* Interactor, FVector_NetQuantize NewVelocity, FVector_NetQuantize LaunchLocation)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
	{
		ThrownBy = Character;
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorLocation(LaunchLocation);
		InitVelocity(NewVelocity);

		UPrimitiveComponent* RootPrimitiveComponent = Cast<UPrimitiveComponent>(GetRootComponent());
		if (RootPrimitiveComponent)
		{
			RootPrimitiveComponent->IgnoreActorWhenMoving(ThrownBy, true);
		}

		if (ParticleSystem)
			ParticleSystem->ActivateSystem();

		if (ThrowAudioComponent)
			ThrowAudioComponent->Play();
	}
}

FVector ASCThrowable::GetThrowVelocity(AActor* Interactor, const bool bAsController)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
	{
		ThrownBy = Character;
		return Character->GetCachedCameraInfo().Rotation.Vector() * Character->GetThrowStrength();
	}

	return FVector::ZeroVector;
}
