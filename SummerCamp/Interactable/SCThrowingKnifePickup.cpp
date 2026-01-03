// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCThrowingKnifePickup.h"

#include "SCInteractableManagerComponent.h"
#include "SCInteractComponent.h"
#include "SCSpecialMoveComponent.h"
#include "SCKillerCharacter.h"

#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"

ASCThrowingKnifePickup::ASCThrowingKnifePickup(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, StackSize(1.f)
{
	SpecialMoveAlignment = CreateDefaultSubobject<USCSpecialMoveComponent>(TEXT("AlignmentSpecialMove"));
	SpecialMoveAlignment->SetupAttachment(RootComponent);

	KnifePickupDriver = CreateDefaultSubobject<USCAnimDriverComponent>(TEXT("KnifePickupAnimDriver"));
	KnifePickupDriver->SetNotifyName(TEXT("KnifePickup"));
}

#if WITH_EDITOR
void ASCThrowingKnifePickup::CheckForErrors()
{
	Super::CheckForErrors();

	UWorld* World = GetWorld();
	if (!World)
		return;

	static const FName NAME_KnifeFloating(TEXT("KnifeFloating"));
	FCollisionQueryParams QueryParams(NAME_KnifeFloating);
	const FVector KnifeLocation = GetMesh()->GetComponentLocation();
	const FRotator TraceRotator(GetActorRotation() + FRotator(0.f, -90.f, 0.f));
	const FVector TraceDirection = TraceRotator.Vector();
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, KnifeLocation, KnifeLocation + TraceDirection * 16.f, ECollisionChannel::ECC_WorldStatic, QueryParams))
	{
		// No collision, nothing for the knife to stick into!
		FMessageLog MapCheck("MapCheck");
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} is floating! This looks weird.")), Arguments)))
			->AddToken(FMapErrorToken::Create(NAME_KnifeFloating));
	}
}
#endif // WITH_EDITOR

void ASCThrowingKnifePickup::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FAnimNotifyEventDelegate BeginNotify;
	BeginNotify.BindDynamic(this, &ASCThrowingKnifePickup::PerformPickup);
	KnifePickupDriver->InitializeBegin(BeginNotify);

	SpecialMoveAlignment->DestinationReached.AddDynamic(this, &ASCThrowingKnifePickup::OnPickupStart);
	SpecialMoveAlignment->SpecialComplete.AddDynamic(this, &ASCThrowingKnifePickup::OnPickupCompleted);
	SpecialMoveAlignment->SpecialAborted.AddDynamic(this, &ASCThrowingKnifePickup::OnPickupAborted);
}

int32 ASCThrowingKnifePickup::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	const int32 Flags = Super::CanInteractWith(Character, ViewLocation, ViewRotation);
	if (!Flags)
		return 0;

	if (const ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Character))
	{
		if (Killer->GetGrabbedCounselor())
			return 0;
	}

	return Flags;
}

void ASCThrowingKnifePickup::OnInteract(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Interactor))
	{
		SpecialMoveAlignment->ActivateSpecial(Killer);
	}
}

void ASCThrowingKnifePickup::PerformPickup(FAnimNotifyData NotifyData)
{
	if (Role == ROLE_Authority)
	{
		if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(NotifyData.CharacterMesh->GetOwner()))
			Killer->AddKnives(StackSize);

		SetLifeSpan(5.f);

		MULTICAST_DisablePickup();
	}
}

void ASCThrowingKnifePickup::OnPickupStart(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	ForceNetUpdate();
	KnifePickupDriver->SetNotifyOwner(Interactor);

	if (Role == ROLE_Authority)
		InteractComponent->bIsEnabled = false;
}

void ASCThrowingKnifePickup::OnPickupCompleted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	KnifePickupDriver->SetNotifyOwner(nullptr);
}

void ASCThrowingKnifePickup::OnPickupAborted(ASCCharacter* Interactor)
{
	if (Interactor && Interactor->IsLocallyControlled())
		Interactor->GetInteractableManagerComponent()->UnlockInteraction(InteractComponent);

	KnifePickupDriver->SetNotifyOwner(nullptr);

	if (Role == ROLE_Authority)
		InteractComponent->bIsEnabled = true;
}

void ASCThrowingKnifePickup::MULTICAST_DisablePickup_Implementation()
{
	GetMesh()->SetHiddenInGame(true);
}
