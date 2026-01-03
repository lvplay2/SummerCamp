// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCEscapePod.h"
#include "SCRepairComponent.h"
#include "SCCharacter.h"
#include "SCGameMode.h"
#include "SCCounselorCharacter.h"
#include "SCInteractableManagerComponent.h"
#include "SCSpecialMoveComponent.h"


static const FName NAME_BarFullnessParameter(TEXT("Bar Fullness"));

ASCEscapePod::ASCEscapePod(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	RepairPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RepairPanel"));
	RepairPanel->SetupAttachment(Mesh);

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Press;
	InteractComponent->SetupAttachment(RootComponent);

	RepairComponent = CreateDefaultSubobject<USCRepairComponent>(TEXT("Repair"));
	RepairComponent->InteractMethods = (int32)EILLInteractMethod::Hold;
	RepairComponent->HoldTimeLimit = 15.0f;
	RepairComponent->SetupAttachment(RootComponent);

	BrokenSparksEmitter = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BrokenParticles"));
	BrokenSparksEmitter->SetupAttachment(Mesh);

	EscapeSpecialMove = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("EscapeSpecialMove"));
	EscapeSpecialMove->SetupAttachment(RootComponent);

	PodScreen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PodScreen"));
	PodScreen->SetupAttachment(Mesh);

	UsableTime = 120.0f;
	PodState = EEscapeState::EEscapeState_Disabled;
}

void ASCEscapePod::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCEscapePod, PodState);
}

void ASCEscapePod::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCEscapePod::OnInteract);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCEscapePod::HoldInteractStateChange);
		InteractComponent->bIsEnabled = false;
	}

	if (RepairComponent)
	{
		RepairComponent->OnInteract.AddDynamic(this, &ASCEscapePod::OnInteract);
	}

	InstanceMaterial = RepairPanel->CreateDynamicMaterialInstance(5, BaseMaterial);
	if (RepairComponent->bIsEnabled)
		PodScreen->SetMaterial(0, ChipRequiredMaterial);
}

void ASCEscapePod::BeginPlay()
{
	Super::BeginPlay();
}

void ASCEscapePod::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ActivationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ASCEscapePod::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InstanceMaterial && GetWorld())
	{
		const float ActiveTime = FMath::Clamp(GetWorldTimerManager().GetTimerElapsed(ActivationTimerHandle) / UsableTime, 0.f, 1.f);
		InstanceMaterial->SetScalarParameterValue(NAME_BarFullnessParameter, ActiveTime);
	}
}

bool ASCEscapePod::IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const
{
	if (PodState == EEscapeState::EEscapeState_Disabled)
	{
		if (RepairComponent && RepairComponent->GetRequiredPartClasses().Contains(Part))
		{
			AssociatedRepairComponent = RepairComponent;
			return !RepairComponent->IsRepaired();
		}
	}

	return false;
}

void ASCEscapePod::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch(InteractMethod)
	{
		case EILLInteractMethod::Hold:
		{
			PodState = EEscapeState::EEscapeState_Repaired;
			OnRep_PodState();

			Character->GetInteractableManagerComponent()->UnlockInteraction(RepairComponent);
			break;
		}
		case EILLInteractMethod::Press:
		{
			if (InteractComponent)
				InteractComponent->bIsEnabled = false;

			/*
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				Counselor->SetEscaped(nullptr, 0.0f);
			}
			*/
			PodState = EEscapeState::EEscapeState_Launched;
			EscapeSpecialMove->ActivateSpecial(Character);
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

			if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				GameMode->HandleScoreEvent(Character->PlayerState, RepairScoreEvent);
			}
			break;
		}
	}
}

void ASCEscapePod::HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
	{
		switch(NewState)
		{
			case EILLHoldInteractionState::Canceled:
			case EILLHoldInteractionState::Success:
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
				Character->GetInteractableManagerComponent()->UnlockInteraction(RepairComponent);
				break;
			}
		}
	}
}

void ASCEscapePod::OnRep_PodState()
{
	if (PodState == EEscapeState::EEscapeState_Repaired)
	{
		if (RepairComponent)
			RepairComponent->bIsEnabled = false;

		BrokenSparksEmitter->Deactivate();

		SetActorTickEnabled(true);
		if (GetWorld())
		{
			GetWorldTimerManager().SetTimer(ActivationTimerHandle, this, &ASCEscapePod::Native_ActivatePod, UsableTime);
		}
	}

	switch(PodState)
	{
	case EEscapeState::EEscapeState_Disabled:
		PodScreen->SetMaterial(0, ChipRequiredMaterial);
		break;

	case EEscapeState::EEscapeState_Repaired:
		PodScreen->SetMaterial(0, ChipInstalledMaterial);
		break;

	case EEscapeState::EEscapeState_Launched:
		PodScreen->SetMaterial(0, LaunchedMaterial);
		break;
	}
}

void ASCEscapePod::Native_ActivatePod()
{
	if (InteractComponent)
		InteractComponent->bIsEnabled = true;

	PodState = EEscapeState::EEscapeState_Active;

	ActivatePod();

	SetActorTickEnabled(false);
}

void ASCEscapePod::BreakPod()
{
	BrokenSparksEmitter->Activate();
	RepairComponent->bIsEnabled = false;
	InstanceMaterial = RepairPanel->CreateDynamicMaterialInstance(5, BrokenMaterial);
	PodScreen->SetMaterial(0, OutOfOrderMaterial);
}

float ASCEscapePod::GetRemainingActivationTime() const
{
	if (PodState == EEscapeState::EEscapeState_Active)
	{
		return 0.f;
	}
	else if (PodState == EEscapeState::EEscapeState_Repaired)
	{
		return GetWorldTimerManager().GetTimerRemaining(ActivationTimerHandle);
	}

	return -1.f;
}
