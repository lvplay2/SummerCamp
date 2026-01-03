// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNoiseMaker_Projectile.h"
#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"
#include "SCNoiseMaker.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"

ASCNoiseMaker_Projectile::ASCNoiseMaker_Projectile(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, ActivationTime(3.f)
, NoiseTimer(10.f)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->bAutoActivate = false;
	Audio->SetupAttachment(RootComponent);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->bNoSkeletonUpdate = true;
	Mesh->bEnableUpdateRateOptimizations = true;
	Mesh->SetTickGroup(TG_PostPhysics);

	BlastComponent = CreateDefaultSubobject<USphereComponent>(TEXT("BlastRadius"));
	BlastComponent->SetupAttachment(RootComponent);
	BlastComponent->SetSphereRadius(15.f);
}

void ASCNoiseMaker_Projectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCNoiseMaker_Projectile, bActivated);
}

void ASCNoiseMaker_Projectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ASCCounselorCharacter* ThrowingCharacter = Cast<ASCCounselorCharacter>(Instigator);
	if (ASCPlayerState* PS = ThrowingCharacter ? ThrowingCharacter->GetPlayerState() : nullptr)
	{
		float RadiusModifier = 0.0f;
		for (const USCPerkData* Perk : PS->GetActivePerks())
		{
			check(Perk);

			if (!Perk)
				continue;

			RadiusModifier += Perk->FirecrackerStunRadiusModifier;
		}

		float BlastRadius = BlastComponent->GetCollisionShape().GetSphereRadius();
		BlastRadius += BlastRadius * RadiusModifier;

		BlastComponent->SetSphereRadius(BlastRadius);
	}

	BlastComponent->OnComponentBeginOverlap.AddDynamic(this, &ASCNoiseMaker_Projectile::OnOverlap);
	BlastComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASCNoiseMaker_Projectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// if the firework goes underwater, let it die
	if (APhysicsVolume* Volume = Cast<APhysicsVolume>(OtherActor))
	{
		if (Volume->bWaterVolume && !IsPendingKill())
		{
			const float WaterZ = Volume->GetActorLocation().Z + Volume->GetSimpleCollisionHalfHeight();
			if (GetActorLocation().Z <= WaterZ)
			{
				ParticleSystem->KillParticlesForced();
				Audio->FadeOut(0.5f, 0.f);
				Mesh->SetVisibility(false);
				Destroy();

				ActivationTime = 0.f;
				NoiseTimer = 0.f;
				bActivated = false;
				return;
			}
		}
	}

	if (Role == ROLE_Authority)
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(OtherActor))
		{
			if (DamagedCharacters.Contains(Character))
				return;

			// Do a trace from the cracker to the hit character.
			UWorld* World = GetWorld();
			if (World)
			{
				FHitResult HitResult;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
				QueryParams.AddIgnoredActor(Character);
				QueryParams.bTraceComplex = true;
				FVector StartPoint = GetActorLocation() + FVector::UpVector * Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				const FVector EndPoint = Character->GetActorLocation();

				while (World->LineTraceSingleByChannel(HitResult, StartPoint, EndPoint, ECollisionChannel::ECC_Visibility, QueryParams))
				{
					if (HitResult.GetActor()->IsA<ASCCharacter>())
					{
						// We hit a character, that's OK.
						QueryParams.AddIgnoredActor(HitResult.GetActor());
						StartPoint = HitResult.ImpactPoint;
					}
					else
					{
						// We hit something else. Don't process the hit.
						return;
					}
				}
			}

			ASCCounselorCharacter* ThrowingCharacter = Cast<ASCCounselorCharacter>(Instigator);
			ASCPlayerState* PS = ThrowingCharacter ? ThrowingCharacter->GetPlayerState() : nullptr;
			ASCNoiseMaker* OwnerFirecracker = Cast<ASCNoiseMaker>(GetOwner());

			DamagedCharacters.AddUnique(Character);
			if (Character->IsKiller())
			{
				if (KillerStunType)
					Character->TakeDamage(KillerDamage, FDamageEvent(KillerStunType), Instigator->Controller, Instigator);

				if (PS && OwnerFirecracker)
				{
					PS->HitJasonWithFirecracker(OwnerFirecracker->GetClass());
				}

				TriggerScoreEvent(StunJasonScoreEventClass);
			}
			else
			{
				if (CounselorStunType)
					Character->TakeDamage(CounselorDamage, FDamageEvent(CounselorStunType), Instigator->Controller, Instigator);

				if (PS && OwnerFirecracker)
					PS->TrackItemUse(OwnerFirecracker->GetClass(), (uint8) ESCItemStatFlags::HitCounselor);
			}
		}
	}
}

void ASCNoiseMaker_Projectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bActivated)
	{
		ParticleSystem->SetWorldRotation(FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		if (ActivationTime > 0.f)
		{
			ActivationTime -= DeltaSeconds;
			if (ActivationTime <= 0.f)
			{
				ParticleSystem->Activate();
				Audio->Play();
				BlastComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}
		else
		{
			NoiseTimer -= DeltaSeconds;
			if (NoiseTimer <= 0.f)
			{
				if (!IsPendingKill())
				{
					ParticleSystem->Deactivate();
					Audio->FadeOut(0.25f, 0.f);
					Mesh->SetVisibility(false);
					Destroy();
				}
			}
		}
	}
}

void ASCNoiseMaker_Projectile::InitVelocity(const FVector& Direction)
{
	Super::InitVelocity(Direction);

	bActivated = true;

	NetUpdateFrequency = 60.f;
}
