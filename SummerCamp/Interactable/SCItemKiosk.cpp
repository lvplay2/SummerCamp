// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCItemKiosk.h"
#include "SCInteractComponent.h"
#include "SCInteractableManagerComponent.h"
#include "SCSpecialMoveComponent.h"
#include "SCCharacter.h"
#include "SCItem.h"

ASCItemKiosk::ASCItemKiosk(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	InteractionPoint = CreateDefaultSubobject<USCInteractComponent>(TEXT("InteractionPoint"));
	InteractionPoint->SetupAttachment(RootComponent);

	UseSpecialMove = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("UseSpecialMove"));
	UseSpecialMove->SetupAttachment(RootComponent);
	UseSpecialMove->bTakeCameraControl = false;
}

void ASCItemKiosk::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractionPoint)
	{
		InteractionPoint->OnInteract.AddDynamic(this, &ASCItemKiosk::OnInteract);
		InteractionPoint->OnCanInteractWith.BindDynamic(this, &ASCItemKiosk::CanInteractWith);
	}

	if (UseSpecialMove)
	{
		UseSpecialMove->SpecialAborted.AddDynamic(this, &ASCItemKiosk::OnUseAborted);
		UseSpecialMove->SpecialComplete.AddDynamic(this, &ASCItemKiosk::OnUsedComplete);
	}
}

void ASCItemKiosk::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:
		UseSpecialMove->ActivateSpecial(Character);
		break;
	}
}

int32 ASCItemKiosk::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	ASCCharacter* SCCharacter = Cast<ASCCharacter>(Character);

	int32 InventoryMax = 0;
	if (ASCItem* DefaultItem = Cast<ASCItem>(ItemClass->GetDefaultObject()))
		InventoryMax = DefaultItem->GetInventoryMax();

	if (InventoryMax > 0 && InventoryMax <= SCCharacter->NumberOfItemTypeInInventory(ItemClass))
		return 0;

	if (SCCharacter->IsInSpecialMove())
		return 0;

	return (int32) EILLInteractMethod::Press;
}

void ASCItemKiosk::OnUseAborted(ASCCharacter* Interactor)
{
	// Prevent getting an item if you get knocked out of interaction before finishing the pickup animation

	// Unlock interaction
	if (Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractionPoint);
	}
}

void ASCItemKiosk::OnUsedComplete(ASCCharacter* Interactor)
{
	if (!Interactor)
		return;

	if (Role == ROLE_Authority)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;

		Interactor->PickingItem = GetWorld()->SpawnActor<ASCItem>(ItemClass, SpawnParams);
		Interactor->PickingItem->bIsPicking = true;
		Interactor->AddOrSwapPickingItem();
	}

	if (Interactor->IsLocallyControlled())
	{
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractionPoint);
	}
}
