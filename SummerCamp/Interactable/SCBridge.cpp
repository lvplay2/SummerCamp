// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBridge.h"

#include "SCKillerCharacter.h"
#include "SCGameState_Hunt.h"
#include "SCRepairComponent.h"
#include "SCBridgeButton.h"

ASCBridge::ASCBridge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	JasonDetector = CreateDefaultSubobject<UBoxComponent>(TEXT("JasonDetector"));
	JasonDetector->SetupAttachment(RootComponent);

	bDetached = false;
}

void ASCBridge::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Bind the activation for our buttons.
	for (ASCBridgeButton* Button : BridgeButtons)
	{
		Button->AttemptBridgeActivation.BindDynamic(this, &ASCBridge::AttemptBridgeEscape);
	}
}

void ASCBridge::BeginPlay()
{
	Super::BeginPlay();

	// Register ourselves with the game state
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = World->GetGameState<ASCGameState>())
		{
			GS->Bridge = this;
		}
	}
}

void ASCBridge::AttemptBridgeEscape()
{
	bool bCanActivate = true;

	// Verify that the nav card and core have been repaired
	if (UWorld* World = GetWorld())
	{
		if (ASCGameState* GS = World->GetGameState<ASCGameState>())
		{
			bCanActivate &= GS->bNavCardRepaired && GS->bCoreRepaired;
		}
	}

	// Verify that Jason is not currently on the bridge
	{
		TArray<AActor*> Overlappers;
		JasonDetector->GetOverlappingActors(Overlappers, ASCKillerCharacter::StaticClass());

		bCanActivate &= Overlappers.Num() == 0;
	}

	if (bCanActivate)
	{
		MULTICAST_ActivateBridgeEscape();
	}

	// Either kill the timers or reset the buttons based on our outcome.
	for (ASCBridgeButton* Button : BridgeButtons)
	{
		bCanActivate ? Button->KillButtonTimer() : Button->OnResetTrigger();
	}
}

void ASCBridge::MULTICAST_ActivateBridgeEscape_Implementation()
{
	bDetached = true;
	BP_ActivateBridgeEscape();
}
