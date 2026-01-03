// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "PoweredPointLight.h"

#include "PoweredPointLightComponent.h"
#include "PoweredSpotLightComponent.h"

static const FName PoweredActorPointLightGlow_NAME(TEXT("Glow"));

APoweredPointLight::APoweredPointLight(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bStartPowered(true)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bHidden = false;

	CollisionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapVolume"));
	CollisionVolume->SetSphereRadius(1.f);
	CollisionVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionVolume->SetCollisionResponseToChannel(ECC_PowerdLight, ECR_Overlap);
	CollisionVolume->SetupAttachment(RootComponent);
}

void APoweredPointLight::BeginPlay()
{
	Super::BeginPlay();

	RegisterWithPowerPlant();

	InitialIntensity = GetLightComponent()->Intensity;

	SetPowered(bStartPowered, nullptr);
	SetActorTickEnabled(false);

	float Val = 0.0f;

	if (!IsRunningDedicatedServer())
	{
		// Find all our child components that are meshes so we can make them gloooowwwww
		TArray<USceneComponent*> ChildrenComponents;
		GetRootComponent()->GetChildrenComponents(true, ChildrenComponents);
		for (USceneComponent* Child : ChildrenComponents)
		{
			if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Child))
			{
				// Make all the materials dynamic so we can adjust the glow later...
				TArray<UMaterialInterface*> Materials = MeshComponent->GetMaterials();
				for (int32 i(0); i < Materials.Num(); ++i)
				{
					if (Materials[i] && Materials[i]->GetScalarParameterValue(PoweredActorPointLightGlow_NAME, Val))
					{
						GlowMaterials.Add(MeshComponent->CreateDynamicMaterialInstance(i, Materials[i]));
					}
				}

				continue;
			}

			if (UParticleSystemComponent* ParticleComponent = Cast<UParticleSystemComponent>(Child))
			{
				// make dynamic instance of materials on particle
				for (int32 i(0); i<ParticleComponent->GetNumMaterials(); ++i)
				{
					UMaterialInterface* MI = ParticleComponent->GetMaterial(i);
					// if this material has the glow parameter
					if (MI && MI->GetScalarParameterValue(PoweredActorPointLightGlow_NAME, Val))
					{
						// create a dynamic version of that material and store that shit
						GlowMaterials.Add(ParticleComponent->CreateDynamicMaterialInstance(i, MI));
					}
				}

				continue;
			}
		}
	}
}

void APoweredPointLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Unlikely to get here, but let's be safe
	if (IsValid(PowerCurve) == false)
	{
		SetActorTickEnabled(false);
		if (bIsApplyingPower)
			OnActivate();
		else
			OnDeactivate();

		return;
	}

	// Adjust the light intensity based on the float curve
	CurrentPowerTime += DeltaSeconds;

	float MinTime, MaxTime;
	PowerCurve->GetTimeRange(MinTime, MaxTime);

	const float Value = FMath::Max(0.f, PowerCurve->GetFloatValue(FMath::Clamp(CurrentPowerTime, MinTime, MaxTime)));
	const float NewIntensity = InitialIntensity * Value;

	GetLightComponent()->SetIntensity(NewIntensity);

	const float NewGlow = 10.f * Value;
	for (UMaterialInstanceDynamic* DynamicMat : GlowMaterials)
	{
		DynamicMat->SetScalarParameterValue(PoweredActorPointLightGlow_NAME, NewGlow);
	}

	// We're done updating if we've gone past our max time
	if (CurrentPowerTime >= MaxTime)
	{
		SetActorTickEnabled(false);
		if (bIsApplyingPower)
			OnActivate();
		else
			OnDeactivate();

		PowerCurve = nullptr;
	}
}

void APoweredPointLight::SetVisibility(bool bVisible)
{
	GetLightComponent()->SetVisibility(bVisible);
	GetLightComponent()->SetIntensity(bVisible ? InitialIntensity : 0.f);

	const float GlowValue = bVisible ? 10.f : 0.f;

	for (UMaterialInstanceDynamic* DynamicMat : GlowMaterials)
	{
		DynamicMat->SetScalarParameterValue(PoweredActorPointLightGlow_NAME, GlowValue);
	}
}

void APoweredPointLight::SetPowered(bool bHasPower, UCurveFloat* Curve)
{
	ISCPoweredObject::SetPowered(bHasPower, Curve);

	// If we got a curve, let's do this overtime
	if (Curve)
	{
		SetActorTickEnabled(true);

		PowerCurve = Curve;
		bIsApplyingPower = bHasPower;
		CurrentPowerTime = 0.f;

		// ... and tell our children to use the same curve as us
		TArray<USceneComponent*> ChildrenComponents;
		GetRootComponent()->GetChildrenComponents(true, ChildrenComponents);
		for (USceneComponent* Child : ChildrenComponents)
		{
			if (UPoweredPointLightComponent* ChildPointLightComponent = Cast<UPoweredPointLightComponent>(Child))
				ChildPointLightComponent->PowerCurve = PowerCurve;

			else if (UPoweredSpotLightComponent* ChildSpotLightComponent = Cast<UPoweredSpotLightComponent>(Child))
				ChildSpotLightComponent->PowerCurve = PowerCurve;
		}

		return;
	}

	// No curve? Just turn the power on or off
	if (bHasPower)
		OnActivate();
	else
		OnDeactivate();
}

void APoweredPointLight::OnActivate_Implementation()
{
	SetVisibility(true);
}

void APoweredPointLight::OnDeactivate_Implementation()
{
	SetVisibility(false);
}
