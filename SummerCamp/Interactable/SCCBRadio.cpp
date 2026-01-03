// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCBRadio.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode_Hunt.h"
#include "SCInteractableManagerComponent.h"
#include "SCPlayerState.h"
#include "SCVoiceOverComponent.h"

ASCCBRadio::ASCCBRadio(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 15.0f;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Hold;
	InteractComponent->HoldTimeLimit = 5.0f;
	InteractComponent->SetupAttachment(RootComponent);
}

void ASCCBRadio::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCCBRadio, bHunterCalled);
	DOREPLIFETIME(ASCCBRadio, InteractingCharacter);
}

void ASCCBRadio::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCCBRadio::OnInteract);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCCBRadio::HoldInteractStateChange);
	}
}

void ASCCBRadio::BeginPlay()
{
	Super::BeginPlay();

	RegisterWithPowerPlant();

	if (bUseDynamicMaterial && !IsRunningDedicatedServer())
	{
		TArray<UMaterialInterface*> Materials = Mesh->GetMaterials();
		for (int32 i(0); i < Materials.Num(); ++i)
		{
			Mesh->CreateDynamicMaterialInstance(i, Materials[i]);
		}
	}
}

void ASCCBRadio::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PowerCurve == nullptr)
	{
		SetActorTickEnabled(false);
		return;
	}

	// OPTIMIZE: This is a very inefficient way to animate this material parameter and should be done entirely material-side

	static const FName NAME_Glow(TEXT("Glow"));

	// Adjust the light intensity based on the float curve
	CurrentPowerTime += DeltaSeconds;

	float MinTime, MaxTime;
	PowerCurve->GetTimeRange(MinTime, MaxTime);
	const float Value = FMath::Max(0.f, PowerCurve->GetFloatValue(FMath::Clamp(CurrentPowerTime, MinTime, MaxTime)));

	if (!IsRunningDedicatedServer())
	{
		const float NewGlow = 10.f * Value;
		for (UMaterialInterface* Material : Mesh->GetMaterials())
		{
			if (UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(Material))
			{
				DynamicMat->SetScalarParameterValue(NAME_Glow, NewGlow);
			}
		}
	}

	// We're done updating if we've gone past our max time
	if (CurrentPowerTime >= MaxTime)
	{
		CurrentPowerTime = 0.f;
		PowerCurve = nullptr;
		SetActorTickEnabled(false);
	}
}

void ASCCBRadio::SetPowered(bool bHasPower, UCurveFloat* Curve)
{
	ISCPoweredObject::SetPowered(bHasPower, Curve);

	if (InteractingCharacter)
	{
		if (InteractingCharacter->IsLocallyControlled())
		{
			InteractingCharacter->ClearAnyInteractions();
		}
	}

	if (!bHunterCalled)
	{
		ForceNetUpdate();
		InteractComponent->bIsEnabled = bHasPower;
	}

	if (bUseDynamicMaterial)
	{
		if (Curve)
		{
			SetActorTickEnabled(true);
			SetPowerCurve(Curve);
		}
		else if (!IsRunningDedicatedServer())
		{
			const float NewGlow = bHasPower ? 10.f : 0.f;
			for (UMaterialInterface* Material : Mesh->GetMaterials())
			{
				if (UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(Material))
				{
					DynamicMat->SetScalarParameterValue(TEXT("Glow"), NewGlow);
				}
			}
		}
	}
}

int32 ASCCBRadio::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& Rotation)
{
	if (InteractingCharacter || bHunterCalled)
		return 0;

	return InteractComponent->InteractMethods;
}

void ASCCBRadio::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	// This will only be called from completing the hold interaction, and always on the server. No safety checks needed.
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
		case EILLInteractMethod::Hold:
		{
			// TODO: Make this gamemode agnostic
			if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				GameMode->StartHunterTimer();
				GameMode->HandleScoreEvent(Character->PlayerState, CallHunterScoreEvent);
			}

			if (ASCPlayerState* PS = Character->GetPlayerState())
			{
				PS->CalledHunter();
			}

			ForceNetUpdate();
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			InteractingCharacter = nullptr;
			InteractComponent->bIsEnabled = false;
			bHunterCalled = true;
		}
		break;
	}

}

void ASCCBRadio::HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState)
{
	switch (NewState)
	{
		case EILLHoldInteractionState::Interacting:
		{
			if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
			{
				InteractingCharacter = Character;
			}
		}
		break;

		case EILLHoldInteractionState::Canceled:
		{
			if (InteractingCharacter)
			{
				InteractingCharacter->VoiceOverComponent->StopVoiceOver();
				InteractingCharacter->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
				InteractingCharacter = nullptr;
			}
			else if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
			{
				Character->VoiceOverComponent->StopVoiceOver();
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			}
		}
		break;
	}
}
