// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBridgeButton.h"
#include "SCGameMode.h"
#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCCounselorCharacter.h"
#include "SCPlayerState.h"
#include "SCGameState.h"

static const FName NAME_UseMaskParameter(TEXT("UseMask"));

ASCBridgeButton::ASCBridgeButton(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
	, DeactivationTimer(3.0f)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interact"));
	InteractComponent->SetupAttachment(RootComponent);
}

void ASCBridgeButton::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCBridgeButton, bTriggered);
}

void ASCBridgeButton::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InteractComponent->OnInteract.AddDynamic(this, &ASCBridgeButton::OnInteract);
}

void ASCBridgeButton::BeginPlay()
{
	Super::BeginPlay();

	DynamicMatDetachConsole = Mesh->CreateDynamicMaterialInstance(1, DetatchConsoleMaterial);
}

void ASCBridgeButton::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Press:

		InteractComponent->bIsEnabled = false;
		Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

		bTriggered = true;
		OnRep_ButtonTriggered();

		bool bAllTriggered = true;

		// See if all of our other buttons have been triggered
		for (ASCBridgeButton* Button : PairedButtons)
		{
			if (Button && !Button->GetIsTriggered())
			{
				bAllTriggered = false;
				break;
			}
		}

		if (bAllTriggered)
		{
			// Blast off the bridge...
			AttemptBridgeActivation.ExecuteIfBound();
		}
		else if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(BridgeButtonTimer, this, &ASCBridgeButton::OnResetTrigger, DeactivationTimer, false);
		}
		break;
	}
}

void ASCBridgeButton::OnRep_ButtonTriggered()
{
	DynamicMatDetachConsole->SetScalarParameterValue(NAME_UseMaskParameter, bTriggered ? 1.0f : 0.0f);
}

void ASCBridgeButton::OnResetTrigger()
{
	bTriggered = false;
	OnRep_ButtonTriggered();
	InteractComponent->bIsEnabled = true;
	return;
}

void ASCBridgeButton::KillButtonTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BridgeButtonTimer);
	}
}
