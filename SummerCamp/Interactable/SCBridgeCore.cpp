// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBridgeCore.h"

#include "ILLCharacterAnimInstance.h"

#include "SCCounselorCharacter.h"
#include "SCDoor.h"
#include "SCGameState.h"
#include "SCInteractableManagerComponent.h"
#include "SCRepairComponent.h"
#include "SCRepairPart.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCSpecialMoveComponent.h"
#include "SCAnimDriverComponent.h"

ASCBridgeCore::ASCBridgeCore(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	BrokenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrokenMesh"));
	BrokenMesh->SetupAttachment(RootComponent);

	RepairedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RepairedMesh"));
	RepairedMesh->SetupAttachment(RootComponent);

	RepairComponent = CreateDefaultSubobject<USCRepairComponent>(TEXT("Interact"));
	RepairComponent->SetupAttachment(RootComponent);

	UseCoreRepairDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("UseCoreRepairDriver"));
	UseCoreRepairDriver->SetNotifyName(TEXT("UseCoreRepair"));
}

void ASCBridgeCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ASCBridgeCore::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (RepairComponent)
	{
		RepairComponent->OnInteract.AddDynamic(this, &ASCBridgeCore::OnInteract);
		RepairComponent->OnHoldStateChanged.AddDynamic(this, &ASCBridgeCore::OnHoldStateChanged);
	}

	if (UseCoreRepairDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCBridgeCore::OnCoreAnimTrigger);

		UseCoreRepairDriver->InitializeBegin(BeginDelegate);
	}
}

void ASCBridgeCore::BeginPlay()
{
	Super::BeginPlay();

	if (RepairedMesh)
		RepairedMesh->SetVisibility(false);
}

bool ASCBridgeCore::GetIsRepaired() const
{
	return RepairComponent->IsRepaired();
}

bool ASCBridgeCore::IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const
{
	if (RepairComponent && RepairComponent->GetRequiredPartClasses().Contains(Part))
	{
		AssociatedRepairComponent = RepairComponent;
		return !RepairComponent->IsRepaired();
	}

	return false;
}

void ASCBridgeCore::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Hold:
		if (UWorld* World = GetWorld())
		{
			if (ASCGameState* GS = World->GetGameState<ASCGameState>())
			{
				GS->bCoreRepaired = true;
			}
		}

		// remove the notify owner
		MULTICAST_UpdateNotifyState(nullptr, false);
		break;
	}
}

void ASCBridgeCore::OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState InteractState)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractState)
	{
		// We've begun interaction, tell the notify driver that this person is who we need to listen for.
	case EILLHoldInteractionState::Interacting:
		MULTICAST_UpdateNotifyState(Interactor, false);
		break;

		// ABORT!
	case EILLHoldInteractionState::Canceled:
		MULTICAST_UpdateNotifyState(Interactor, true);
		break;
	}
}

void ASCBridgeCore::OnCoreAnimTrigger(FAnimNotifyData NotifyData)
{
	//BrokenMesh->SetVisibility(false);
	RepairedMesh->SetVisibility(true);
	if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(NotifyData.CharacterMesh->GetOwner()))
	{
		Counselor->HideLargeItem(true);
	}
}

void ASCBridgeCore::MULTICAST_UpdateNotifyState_Implementation(AActor* Interactor, bool Canceled)
{
	UseCoreRepairDriver->SetNotifyOwner(Canceled ? nullptr : Interactor);

	// if we canceled the interaction we need to reset the mesh and held item state
	if (Canceled)
	{
		BrokenMesh->SetVisibility(true);
		RepairedMesh->SetVisibility(false);
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor))
		{
			Counselor->HideLargeItem(false);
		}
	}
}
