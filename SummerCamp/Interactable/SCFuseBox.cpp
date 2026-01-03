// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFuseBox.h"

#include "SCCounselorAIController.h"
#include "SCGameMode.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCKillerCharacter.h"
#include "SCPlayerState.h"
#include "SCPowerGridVolume.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCSpecialMoveComponent.h"

// Sets default values
ASCFuseBox::ASCFuseBox(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LightMaterialIndex(1)
, bDestroyed(false)
, bStartDestroyed(false)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 15.0f;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FuseBoxMesh"));
	RootComponent = Mesh;

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->SetupAttachment(RootComponent);
	
	DestroyBox_SpecialMoveHandler = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("DestroyBox_SpecialMoveHandler"));
	DestroyBox_SpecialMoveHandler->SetupAttachment(RootComponent);
	DestroyBox_SpecialMoveHandler->bTakeCameraControl = false;

	DestructionDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("DestructionDriver"));
	DestructionDriver->SetNotifyName(TEXT("DestroyGenerator"));
}

void ASCFuseBox::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	// Bind Press interact delegates
	if (InteractComponent)
	{
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCFuseBox::CanInteractWith);
		InteractComponent->OnInteract.AddDynamic(this, &ASCFuseBox::OnInteract);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCFuseBox::OnHoldStateChanged);
	}

	DestroyBox_SpecialMoveHandler->SpecialStarted.AddDynamic(this, &ASCFuseBox::DestructionStarted);
	DestroyBox_SpecialMoveHandler->SpecialComplete.AddDynamic(this, &ASCFuseBox::NativeDestructionCompleted);
	DestroyBox_SpecialMoveHandler->SpecialAborted.AddDynamic(this, &ASCFuseBox::NativeDestructionAborted);

	if (DestructionDriver)
	{
		FAnimNotifyEventDelegate BeginDelegate;
		BeginDelegate.BindDynamic(this, &ASCFuseBox::DeactivateGrid);
		DestructionDriver->InitializeBegin(BeginDelegate);
	}

	if (Role == ROLE_Authority)
	{
		bDestroyed = bStartDestroyed;
	}
}

void ASCFuseBox::BeginPlay()
{
	Super::BeginPlay();

	// Only need to call this on the server, clients will handle via replication
	if (Role == ROLE_Authority)
	{
		MULTICAST_SetDestroyed(bDestroyed);

		if (PowerGrid)
		{
			PowerGrid->RequestDeactivateLights(bDestroyed, true);
			PowerGrid->SetFuseBox(this);
		}
	}
}

void ASCFuseBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASCFuseBox, bDestroyed, COND_InitialOnly);
}

int32 ASCFuseBox::CanInteractWith(AILLCharacter* Interactor, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	if (!Character)
		return 0;

	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (Killer->GetGrabbedCounselor())
			return 0;

		if (!bDestroyed)
			return (int32)EILLInteractMethod::Press;
	}
	else
	{
		if (bDestroyed)
			return (int32)EILLInteractMethod::Hold;
	}

	return 0;
}

void ASCFuseBox::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		if (Character->IsKiller() && bDestroyed == false)
		{
			DestroyBox_SpecialMoveHandler->ActivateSpecial(Character);
			SetOwner(Interactor);

			if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
			{
				GameMode->HandleScoreEvent(Character->PlayerState, DestroyScoreEvent);
			}
		}
		break;

	case EILLInteractMethod::Hold:
		SetDestroyed(false);

		Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

		if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
		{
			GameMode->HandleScoreEvent(Character->PlayerState, RepairScoreEvent);
		}

		if (ASCPlayerState* PS = Character->GetPlayerState())
			PS->GridPartRepaired();

		SetOwner(nullptr);
		break;
	}
}

void ASCFuseBox::OnHoldStateChanged(AActor* Interactor, EILLHoldInteractionState NewState)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	if (!Character)
		return;

	switch (NewState)
	{
	case EILLHoldInteractionState::Interacting:
		SetOwner(Interactor);
		break;

	case EILLHoldInteractionState::Canceled:
		Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		SetOwner(nullptr);
		break;
	}
}

void ASCFuseBox::DeactivateGrid(FAnimNotifyData NotifyData)
{
	NativeDestructionCompleted(nullptr);
}

void ASCFuseBox::DestructionStarted(ASCCharacter* Interactor)
{
	DestructionDriver->SetNotifyOwner(Interactor);
}

void ASCFuseBox::NativeDestructionCompleted(ASCCharacter* Interactor)
{
	if (GetOwner() && GetOwner()->Role == ROLE_Authority)
	{
		SetDestroyed(true);
		SetOwner(nullptr);
	}

	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	DestructionDriver->SetNotifyOwner(nullptr);
}

void ASCFuseBox::NativeDestructionAborted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
}

void ASCFuseBox::OnRep_Destroyed()
{
	if (bDestroyed)
		OnDeactivate();
	else
		OnActivate();

	Mesh->SetMaterial(LightMaterialIndex, bDestroyed ? BrokenLightMaterial : RepairedLightMaterial);

	if (PowerGrid)
		PowerGrid->RequestDeactivateLights(bDestroyed, true);
}

void ASCFuseBox::SetDestroyed(bool bInDestroyed)
{
	if (bDestroyed == bInDestroyed)
		return;

	MULTICAST_SetDestroyed(bInDestroyed);

	if (PowerGrid)
		PowerGrid->RequestDeactivateLights(bInDestroyed);
}

void ASCFuseBox::MULTICAST_SetDestroyed_Implementation(bool bInDestroyed)
{
	bDestroyed = bInDestroyed;

	if (bDestroyed)
		OnDeactivate();
	else
		OnActivate();

	Mesh->SetMaterial(LightMaterialIndex, bDestroyed ? BrokenLightMaterial : RepairedLightMaterial);
}
