// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBridgeNavUnit.h"

#include "SCCounselorCharacter.h"
#include "SCGameState_Hunt.h"
#include "SCRepairComponent.h"
#include "SCRepairPart.h"

ASCBridgeNavUnit::ASCBridgeNavUnit(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	RepairComponent = CreateDefaultSubobject<USCRepairComponent>(TEXT("Interact"));
	RepairComponent->SetupAttachment(RootComponent);
}

void ASCBridgeNavUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ASCBridgeNavUnit::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (RepairComponent)
	{
		RepairComponent->OnInteract.AddDynamic(this, &ASCBridgeNavUnit::OnInteract);
	}
}

void ASCBridgeNavUnit::BeginPlay()
{
	Super::BeginPlay();
}

bool ASCBridgeNavUnit::GetIsRepaired() const
{
	return RepairComponent->IsRepaired();
}

bool ASCBridgeNavUnit::IsRepairPartNeeded(TSubclassOf<ASCRepairPart> Part, USCRepairComponent*& AssociatedRepairComponent) const
{
	if (RepairComponent && RepairComponent->GetRequiredPartClasses().Contains(Part))
	{
		AssociatedRepairComponent = RepairComponent;
		return !RepairComponent->IsRepaired();
	}

	return false;
}

void ASCBridgeNavUnit::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
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
				GS->bNavCardRepaired = true;
			}
		}
		break;
	}
}
