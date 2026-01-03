// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFlare_Projectile.h"

#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCWeapon.h"

ASCFlare_Projectile::ASCFlare_Projectile(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FlareSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlareSpotLight"));
	FlareSpotLight->SetupAttachment(RootComponent);

	KillerEyeFlareParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("KillerEyeParticles"));
	KillerEyeFlareParticles->SetupAttachment(RootComponent);
	KillerEyeFlareParticles->bAutoActivate = false;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;
}

void ASCFlare_Projectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCFlare_Projectile, bAttachedToKiller);
}

void ASCFlare_Projectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &ASCFlare_Projectile::OnFlareBounce);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ASCFlare_Projectile::OnFlareStop);
}

void ASCFlare_Projectile::Destroyed()
{
	UWorld* World = GetWorld();
	if (ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr)
	{
		for (ASCCharacter* OuterCharacter : GameState->PlayerCharacterList)
		{
			if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OuterCharacter))
			{
				for (ASCCharacter* InnerCharacter : GameState->PlayerCharacterList)
				{
					if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(InnerCharacter))
					{
						Counselor->OnCharacterOutOfView(Killer);
					}
				}
			}
		}
	}

	Super::Destroyed();
}

void ASCFlare_Projectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (ASCGameState* GameState = World ? World->GetGameState<ASCGameState>() : nullptr)
	{
		for (ASCCharacter* OuterCharacter : GameState->PlayerCharacterList)
		{
			if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(OuterCharacter))
			{
				if ((GetActorLocation() - Killer->GetActorLocation()).SizeSquared2D() < FMath::Square(KillerMinDistance))
				{
					TriggerScoreEvent(FlareScoreEventClass);

					for (ASCCharacter* InnerCharacter : GameState->PlayerCharacterList)
					{
						if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(InnerCharacter))
						{
							Counselor->OnCharacterInView(Killer);
						}
					}
				}
				else
				{
					for (ASCCharacter* InnerCharacter : GameState->PlayerCharacterList)
					{
						if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(InnerCharacter))
						{
							Counselor->OnCharacterOutOfView(Killer);
						}
					}
				}
			}
		}
	}
}

void ASCFlare_Projectile::OnFlareBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (Role != ROLE_Authority)
		return;

	OnFlareStop(ImpactResult);
}

void ASCFlare_Projectile::OnFlareStop(const FHitResult& ImpactResult)
{
	if (Role != ROLE_Authority)
		return;

	if (ASCKillerCharacter* HitKiller = Cast<ASCKillerCharacter>(ImpactResult.GetActor()))
	{
		const FName KillerHeadBone = TEXT("head");
		const FName KillerEyeSocket = TEXT("eyeSocket_Left");
		if (ImpactResult.BoneName == KillerHeadBone)
		{
			// Attach the flare to his eyeball!!!
			ProjectileMovement->StopMovementImmediately();
			AttachToComponent(HitKiller->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, KillerEyeSocket);
			SetActorRelativeRotation(FQuat::Identity);
			HitKiller->TakeDamage(FlareHeavyDamage, FDamageEvent(FlareHeavyDamageType), GetInstigatorController(), GetInstigator());
			bAttachedToKiller = true;
			OnRep_AttachedToKiller();
		}
		else
		{
			HitKiller->TakeDamage(FlareLightDamage, FDamageEvent(FlareLightDamageType), GetInstigatorController(), GetInstigator());
		}

		// Track flare hits on Jason
		const ASCWeapon* OwningWeapon = Cast<ASCWeapon>(GetOwner());
		const ASCCounselorCharacter* InstigatingCounselor = Cast<ASCCounselorCharacter>(GetInstigator());
		if (OwningWeapon && InstigatingCounselor)
		{
			if (ASCPlayerState* PS = InstigatingCounselor->GetPlayerState())
			{
				PS->ShotJasonWithFlare(OwningWeapon->GetClass());
			}
		}

		TriggerScoreEvent(FlareStunScoreEventClass);
	}
}

void ASCFlare_Projectile::OnRep_AttachedToKiller()
{
	if (bAttachedToKiller)
	{
		ParticleSystem->SetActive(false);
		KillerEyeFlareParticles->SetActive(true);
	}
}
