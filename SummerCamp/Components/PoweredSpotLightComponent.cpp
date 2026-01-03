// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "PoweredSpotLightComponent.h"

#include "PoweredPointLightComponent.h"

static const FName PoweredSpotLightGlow_NAME(TEXT("Glow"));

UPoweredSpotLightComponent::UPoweredSpotLightComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bStartPowered(true)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetHiddenInGame(false);
}

void UPoweredSpotLightComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterWithPowerPlant();

	OnActivateDelegate.AddDynamic(this, &UPoweredSpotLightComponent::OnActivate);
	OnDeactivateDelegate.AddDynamic(this, &UPoweredSpotLightComponent::OnDeactivate);

	InitialIntensity = Intensity;

	SetPowered(bStartPowered, nullptr);
	SetComponentTickEnabled(false);

	float Val = 0.0f;

	if (!IsRunningDedicatedServer())
	{
		// Find all our child components that are meshes so we can make them gloooowwwww
		TArray<USceneComponent*> Children;
		GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Child))
			{
				// Make all the materials dynamic so we can adjust the glow later...
				TArray<UMaterialInterface*> Materials = MeshComponent->GetMaterials();
				for (int32 i(0); i < Materials.Num(); ++i)
				{
					if (Materials[i] && Materials[i]->GetScalarParameterValue(PoweredSpotLightGlow_NAME, Val))
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
					if (MI && MI->GetScalarParameterValue(PoweredSpotLightGlow_NAME, Val))
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

void UPoweredSpotLightComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	// Unlikely to get here, but let's be safe
	if (IsValid(PowerCurve) == false)
	{
		SetComponentTickEnabled(false);
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

	SetIntensity(NewIntensity);

	const float NewGlow = 10.f * Value;
	for (UMaterialInstanceDynamic* DynamicMat : GlowMaterials)
	{
		DynamicMat->SetScalarParameterValue(PoweredSpotLightGlow_NAME, NewGlow);
	}

	// We're done updating if we've gone past our max time
	if (CurrentPowerTime >= MaxTime)
	{
		SetComponentTickEnabled(false);
		if (bIsApplyingPower)
			OnActivateDelegate.Broadcast();
		else
			OnDeactivateDelegate.Broadcast();

		PowerCurve = nullptr;
	}
}

void UPoweredSpotLightComponent::OnVisibilityChanged()
{
	Super::OnVisibilityChanged();

	SetIntensity(bVisible ? InitialIntensity : 0.f);

	const float GlowValue = bVisible ? 10.f : 0.f;

	for (UMaterialInstanceDynamic* DynamicMat : GlowMaterials)
	{
		DynamicMat->SetScalarParameterValue(PoweredSpotLightGlow_NAME, GlowValue);
	}
}

void UPoweredSpotLightComponent::SetPowered(bool bHasPower, UCurveFloat* Curve)
{
	ISCPoweredObject::SetPowered(bHasPower, Curve);

	// If we got a curve, let's do this overtime
	if (Curve)
	{
		SetComponentTickEnabled(true);

		bIsApplyingPower = bHasPower;
		CurrentPowerTime = 0.f;

		// Don't update the curve if a parent has already set it
		if (PowerCurve == nullptr)
			PowerCurve = Curve;

		// ... and tell our children to use the same curve as us
		TArray<USceneComponent*> Children;
		GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			if (UPoweredPointLightComponent* PointLightComponent = Cast<UPoweredPointLightComponent>(Child))
				PointLightComponent->PowerCurve = PowerCurve;

			else if (UPoweredSpotLightComponent* SpotLightComponent = Cast<UPoweredSpotLightComponent>(Child))
				SpotLightComponent->PowerCurve = PowerCurve;
		}

		return;
	}

	// No curve? Just turn the power on or off
	if (bHasPower)
		OnActivateDelegate.Broadcast();
	else
		OnDeactivateDelegate.Broadcast();
}

void UPoweredSpotLightComponent::OnDeactivate()
{
	SetVisibility(false);
}

void UPoweredSpotLightComponent::OnActivate()
{
	SetVisibility(true);
}
