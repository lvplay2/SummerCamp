// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPhoneJunctionBox.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCGameMode.h"
#include "SCInteractableManagerComponent.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCPolicePhone.h"
#include "SCRepairComponent.h"
#include "SCRepairPart.h"
#include "SCSpecialMoveComponent.h"
#include "SCGameState_Paranoia.h"

// Sets default values
ASCPhoneJunctionBox::ASCPhoneJunctionBox(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LightMaterialIndex(1)
, bIsBroken(false)
{
	bReplicates = true;
	NetUpdateFrequency = 30.0f;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Hold;
	InteractComponent->HoldTimeLimit = 5.f;
	InteractComponent->SetupAttachment(RootComponent);

	DestroyBox_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("DestroyBox_SpecialMoveHandler"));
	DestroyBox_SpecialMoveHandler->SetupAttachment(RootComponent);
	DestroyBox_SpecialMoveHandler->bTakeCameraControl = false;

	DestroyedEffectLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ParticleLocation"));
	DestroyedEffectLocation->SetupAttachment(RootComponent);

	DestructionDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DestructionDriver"));
	DestructionDriver->SetNotifyName("DestroyGenerator");
}

void ASCPhoneJunctionBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPhoneJunctionBox, bHasFuse);
	DOREPLIFETIME(ASCPhoneJunctionBox, bIsBroken);
}

void ASCPhoneJunctionBox::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCPhoneJunctionBox::OnInteract);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCPhoneJunctionBox::HoldInteractStateChange);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCPhoneJunctionBox::CanInteractWith);
		InteractComponent->bIsEnabled = false; // Disable until the fuse is applied to the box
	}

	DestroyBox_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCPhoneJunctionBox::DestroyBoxStarted);
	DestroyBox_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCPhoneJunctionBox::DestroyBox);
	DestroyBox_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCPhoneJunctionBox::DestroyBoxAborted);

	if (DestructionDriver)
	{
		FAnimNotifyEventDelegate NotifyDestroyDelegate;
		NotifyDestroyDelegate.BindDynamic(this, &ASCPhoneJunctionBox::DestroyBoxNotify);
		DestructionDriver->InitializeBegin(NotifyDestroyDelegate);
	}

	if (Role == ROLE_Authority)
	{
		DestroyBox(nullptr);
	}

	// Loop through repair components and subscribe to its OnInteract
	TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
	for (UActorComponent* Comp : RepairPoints)
	{
		if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
		{
			RepairPoint->OnInteract.AddDynamic(this, &ASCPhoneJunctionBox::OnInteract);
		}
	}
}

int32 ASCPhoneJunctionBox::CanInteractWith_Implementation(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	int32 Flags = 0;

	// no phone boxes in paranoia mode
	if (ASCGameState_Paranoia* GS = Cast<ASCGameState_Paranoia>(GetWorld()->GetGameState()))
		return 0;

	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		if (GS->HasPoliceCalledSFXPlayed)
		{
			ForceNetUpdate();
			InteractComponent->bIsEnabled = false;
			return 0;
		}
	}

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (!bIsBroken && !Killer->GetGrabbedCounselor())
		{
			Flags |= (int32)EILLInteractMethod::Press;
		}
	}
	else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
	{
		if (bIsBroken)
		{
			Flags |= (int32)EILLInteractMethod::Hold;
		}
	}

	return Flags;
}

void ASCPhoneJunctionBox::OnInteract_Implementation(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
		{
			// break this fucker
			DestroyBox_SpecialMoveHandler->ActivateSpecial(Killer);
		}
		break;
	case EILLInteractMethod::Hold:
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		{
			if (!bHasFuse)
			{
				// Apply fuse and reenable the default interact component in case the box needs to be repaired again without the fuse
				ForceNetUpdate();
				bHasFuse = true;
				InteractComponent->bIsEnabled = true;
			}

			bIsBroken = false;
			OnRep_IsBroken();

			if (Counselor->IsAbilityActive(EAbilityType::Repair))
			{
				Counselor->RemoveAbility(EAbilityType::Repair);
			}

			if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				GameMode->HandleScoreEvent(Counselor->PlayerState, RepairScoreEvent);
			}

			if (ASCPlayerState* PS = Counselor->GetPlayerState())
				PS->GridPartRepaired();

			Counselor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

			// Need to also unlock any repair points
			TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
			for (UActorComponent* Comp : RepairPoints)
			{
				if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
				{
					Counselor->GetInteractableManagerComponent()->UnlockInteraction(RepairPoint);
				}
			}
		}
		break;
	}
}

void ASCPhoneJunctionBox::HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState)
{
	if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
	{
		switch (NewState)
		{
			case EILLHoldInteractionState::Canceled:
			case EILLHoldInteractionState::Success:
			{
				Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

				// Need to also unlock any repair points
				TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
				for (UActorComponent* Comp : RepairPoints)
				{
					if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
					{
						Character->GetInteractableManagerComponent()->UnlockInteraction(RepairPoint);
					}
				}
			}
			break;
		}
	}
}

bool ASCPhoneJunctionBox::IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const
{
	if (!bHasFuse)
	{
		TArray<UActorComponent*> RepairPoints = GetComponentsByClass(USCRepairComponent::StaticClass());
		for (UActorComponent* Comp : RepairPoints)
		{
			if (USCRepairComponent* RepairPoint = Cast<USCRepairComponent>(Comp))
			{
				if (RepairPoint->GetRequiredPartClasses().Contains(Part))
				{
					AssociatedRepairComponent = RepairPoint;
					return !RepairPoint->IsRepaired();
				}
			}
		}
	}

	return false;
}

bool ASCPhoneJunctionBox::IsPoliceCalled() const
{
	if (ConnectedPhone)
	{
		return ConnectedPhone->IsPoliceCalled(!bIsBroken);
	}

	return false;
}

void ASCPhoneJunctionBox::DestroyBoxNotify(FAnimNotifyData NotifyData)
{
	DestroyBox(nullptr);
}

void ASCPhoneJunctionBox::DestroyBoxStarted(ASCCharacter* Interactor)
{
	DestructionDriver->SetNotifyOwner(Interactor);
}

void ASCPhoneJunctionBox::DestroyBox(ASCCharacter* Interactor)
{
	DestructionDriver->SetNotifyOwner(nullptr);

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	if (Role == ROLE_Authority && bIsBroken == false)
	{
		bIsBroken = true;
		OnRep_IsBroken();
	}
}

void ASCPhoneJunctionBox::DestroyBoxAborted(ASCCharacter* Interactor)
{
	DestructionDriver->SetNotifyOwner(nullptr);

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

void ASCPhoneJunctionBox::OnRep_IsBroken()
{
	if (Mesh)
		Mesh->SetMaterial(LightMaterialIndex, bIsBroken ? BrokenLightMaterial : RepairedLightMaterial);

	if (ConnectedPhone)
		ConnectedPhone->Enable(!bIsBroken);

	if (bIsBroken)
	{
		if (ConnectedPhone)
			ConnectedPhone->CancelPhoneCall();

		DamagedEffect = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DestroyedEffect, DestroyedEffectLocation->GetComponentTransform(), false);
		OnPhoneBoxDestroyed();
	}
	else
	{
		OnPhoneBoxRepaired();
		if (DamagedEffect != nullptr)
		{
			DamagedEffect->DestroyComponent();
			DamagedEffect = nullptr;
		}
	}
}
