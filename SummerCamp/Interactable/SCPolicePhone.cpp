// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPolicePhone.h"

#include "SCCounselorAIController.h"
#include "SCCounselorCharacter.h"
#include "SCPoliceSpawner.h"
#include "SCGameMode.h"
#include "SCPlayerState.h"
#include "SCSpecialMoveComponent.h"
#include "SCSpecialMove_SoloMontage.h"
#include "SCInteractableManagerComponent.h"
#include "SCStatusIconComponent.h"
#include "SCVoiceOverComponent.h"
#include "SCGameState_Paranoia.h"

// Sets default values
ASCPolicePhone::ASCPolicePhone(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bInRange(false)
{
	bReplicates = true;
	NetUpdateFrequency = 30.0f;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->bEnableUpdateRateOptimizations = true;
	RootComponent = ItemMesh;

	InteractComponent = CreateDefaultSubobject<USCInteractComponent>(TEXT("Interactable"));
	InteractComponent->InteractMethods = (int32)EILLInteractMethod::Hold;
	InteractComponent->HoldTimeLimit = 5.f;
	InteractComponent->bIsEnabled = true;
	InteractComponent->SetupAttachment(RootComponent);

	// Tick is activated while a player has picked up a phone. This is for polling the player for their connected state.
	// Needed to visually "hang up the phone" if the player disconnects while using it.
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = true;

	DisabledIcon = CreateDefaultSubobject<USCStatusIconComponent>(TEXT("DisabledIcon"));
	DisabledIcon->SetupAttachment(RootComponent);
}

void ASCPolicePhone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPolicePhone, CurrentInteractor);
}

void ASCPolicePhone::BeginPlay()
{
	Super::BeginPlay();

	if (ASCGameState_Paranoia* GS = Cast<ASCGameState_Paranoia>(GetWorld()->GetGameState()))
	{
		Enable(false);
		SetActorTickEnabled(false);
		if (DisabledIcon)
			DisabledIcon->SetVisibility(false);
	}
}

void ASCPolicePhone::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (InteractComponent)
	{
		InteractComponent->OnInteract.AddDynamic(this, &ASCPolicePhone::OnInteract);
		InteractComponent->OnHoldStateChanged.AddDynamic(this, &ASCPolicePhone::HoldInteractStateChange);
		InteractComponent->OnCanInteractWith.BindDynamic(this, &ASCPolicePhone::CanInteractWith);

		InteractComponent->OnInRange.AddDynamic(this, &ASCPolicePhone::OnInRange);
		InteractComponent->OnOutOfRange.AddDynamic(this, &ASCPolicePhone::OnOutOfRange);
	}
}

void ASCPolicePhone::Enable(const bool bEnabled)
{
	bIsEnabled = bEnabled;
	InteractComponent->bIsEnabled = bEnabled;

	if (bIsEnabled && bInRange && DisabledIcon)
	{
		DisabledIcon->HideIcon();
		bInRange = false;
	}

	UpdateShimmer();
}

void ASCPolicePhone::Tick(float DeltaTime)
{
	// NOTE: Tick is only active when a user is using the phone
	Super::Tick(DeltaTime);

	if (!CurrentInteractor || !CurrentInteractor->bIsPossessed)
	{
		// Visually hang up the phone if the caller has disconnected from the server. 
		MULTICAST_PlayPhoneAnimation(UsePhoneFailedAnimation);
		CurrentInteractor = nullptr;
		SetActorTickEnabled(false);
	}
}

void ASCPolicePhone::UpdateShimmer()
{
	if (IsRunningDedicatedServer())
		return;

	if (!PhoneMat)
		PhoneMat = ItemMesh->CreateDynamicMaterialInstance(0, ItemMesh->GetMaterial(0));

	float ShimmerValue = bIsEnabled ? 1.f : 0.f;
	if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld()))
	{
		if (AILLCharacter* LocalPlayer = Cast<AILLCharacter>(PC->AcknowledgedPawn))
		{
			if (!InteractComponent->IsInteractionAllowed(LocalPlayer))
				ShimmerValue = 0.f;
		}
	}

	static const FName Shimmer_NAME(TEXT("Shimmer"));
	PhoneMat->SetScalarParameterValue(Shimmer_NAME, ShimmerValue);
}

void ASCPolicePhone::OnInteract_Implementation(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCharacter* Character = Cast<ASCCharacter>(Interactor);
	check(Character);

	switch (InteractMethod)
	{
	case EILLInteractMethod::Hold:
		if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
		{
			GameMode->StartPoliceTimer(Character->PlayerState);
			GameMode->HandleScoreEvent(Character->PlayerState, CallPoliceScoreEvent);
		}

		if (ASCPlayerState* PS = Character->GetPlayerState())
		{
			PS->CalledPolice();
		}

		MULTICAST_PlayPhoneAnimation(UsePhoneSuccessAnimation);

		ForceNetUpdate();
		Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
		InteractComponent->bIsEnabled = false;

		SetActorTickEnabled(false);
		CurrentInteractor = nullptr;
		break;
	}
}

int32 ASCPolicePhone::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (!bIsEnabled)
		return 0;

	// no phone boxes in paranoia mode
	if (ASCGameState_Paranoia* GS = Cast<ASCGameState_Paranoia>(GetWorld()->GetGameState()))
		return 0;

	return InteractComponent->InteractMethods;
}

void ASCPolicePhone::HoldInteractStateChange(AActor* Interactor, EILLHoldInteractionState NewState)
{
	// Use the phone, call the cops! Hide your wife, hide your kids!
	switch (NewState)
	{
	case EILLHoldInteractionState::Interacting:
		// Enable Tick to check for cases of disconnected players
		// (To play the phone-hang-up animation)
		SetActorTickEnabled(true);

		CurrentInteractor = Cast<ASCCharacter>(Interactor);
		MULTICAST_PlayPhoneAnimation(UsePhoneAnimation);
		break;

	case EILLHoldInteractionState::Canceled:
		// When the phone is hung up, no longer need to check for 
		// disconnected phone users.
		SetActorTickEnabled(false);

		if (CurrentInteractor)
		{
			CurrentInteractor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			CurrentInteractor->VoiceOverComponent->StopVoiceOver();

			MULTICAST_PlayPhoneAnimation(UsePhoneFailedAnimation);
		}
		else if (ASCCharacter* Character = Cast<ASCCharacter>(Interactor))
		{
			Character->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);
			Character->VoiceOverComponent->StopVoiceOver();
		}

		CurrentInteractor = nullptr;
		break;
	}
}

void ASCPolicePhone::MULTICAST_PlayPhoneAnimation_Implementation(UAnimationAsset* PlayAnim)
{
	ItemMesh->PlayAnimation(PlayAnim, false);
}

void ASCPolicePhone::OnInRange(AActor* Interactor)
{
	if (bIsEnabled)
		return;

	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(Interactor))
	{
		if (Character->IsLocallyControlled() && Character->IsPlayerControlled())
		{
			if (DisabledIcon)
				DisabledIcon->ShowIcon();

			bInRange = true;
		}
	}
}

void ASCPolicePhone::OnOutOfRange(AActor* Interactor)
{
	if (bIsEnabled)
		return;

	if (ASCCounselorCharacter* Character = Cast<ASCCounselorCharacter>(Interactor))
	{
		if (Character->IsLocallyControlled() && Character->IsPlayerControlled())
		{
			if (DisabledIcon)
				DisabledIcon->HideIcon();

			bInRange = false;
		}
	}
}

void ASCPolicePhone::CancelPhoneCall()
{
	if (CurrentInteractor && (CurrentInteractor->IsLocallyControlled() || Role == ROLE_Authority))
	{
		CurrentInteractor->ClearAnyInteractions();
	}
}
