// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBarrycade.h"

#include "ILLCharacterAnimInstance.h"

#include "SCCounselorCharacter.h"
#include "SCDumbleDoor.h"
#include "SCInteractableManagerComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

static const FName NAME_DoubleBarricadeRotation(TEXT("DoubleBarricade"));

ASCBarrycade::ASCBarrycade(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ForceFieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForceFieldMesh"));
	ForceFieldMesh->SetupAttachment(RootComponent);

	ActivateParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ActivateParticleSystem"));
	ActivateParticleSystem->SetupAttachment(RootComponent);
	DeactivateParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("DeactivateParticleSystem"));
	DeactivateParticleSystem->SetupAttachment(RootComponent);
	DestroyedParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("DestroyedParticleSystem"));
	DestroyedParticleSystem->SetupAttachment(RootComponent);
	
	ActivateParticleSystem2 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ActivateParticleSystem2"));
	ActivateParticleSystem2->SetupAttachment(RootComponent);
	DeactivateParticleSystem2 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("DeactivateParticleSystem2"));
	DeactivateParticleSystem2->SetupAttachment(RootComponent);
	DestroyedParticleSystem2 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("DestroyedParticleSystem2"));
	DestroyedParticleSystem2->SetupAttachment(RootComponent);
}

void ASCBarrycade::OnRep_Destroyed()
{
//	Mesh->SetVisibility(!bDestroyed);
	ForceFieldMesh->SetVisibility(!bDestroyed);
	if (LinkedDoor)
	{
		ASCDumbleDoor* ddoor = Cast<ASCDumbleDoor>(LinkedDoor);
		if (ddoor)
		{
			ddoor->OnBarricade(!bDestroyed);
		}
	}
	if (bDestroyed)
	{
		SetEnabled(false);
		DestroyedParticleSystem->SetActive(true, true);
		DestroyedParticleSystem2->SetActive(true, true);
		DeactivateParticleSystem->SetActive(false);
		DeactivateParticleSystem2->SetActive(false);

		if (DamagedAudio)
			UGameplayStatics::PlaySoundAtLocation(this, DamagedAudio, GetActorLocation());
	}
}


void ASCBarrycade::OnRep_Enabled()
{
//	FRotator NewRot = OriginalRotation;
//	NewRot.Pitch = bEnabled ? DOUBLEBARRICADE_ENABLED_ANGLE : DOUBLEBARRICADE_DISABLED_ANGLE;
	Mesh->SetRelativeRotation(bEnabled ? RotationOn : OriginalRotation, false, nullptr, ETeleportType::TeleportPhysics);

	ForceFieldMesh->SetVisibility(bEnabled);

	if (LinkedDoor)
	{
		ASCDumbleDoor* ddoor = Cast<ASCDumbleDoor>(LinkedDoor);
		if (ddoor)
		{
			ddoor->OnBarricade(bEnabled);
		}
	}

	if(!bEnabled && !bDestroyed)
	{
		DeactivateParticleSystem->SetActive(true, true);
		DeactivateParticleSystem2->SetActive(true, true);
	}

}


void ASCBarrycade::OnBarricadeAborted(ASCCharacter* Interactor)
{
	UseBarricadeDriver->SetNotifyOwner(nullptr);
	bIsBarricading = false;

	// If we're destroyed we'll want the barricade to be lowered, always.
	// Otherwise set the barricade back to where it was before we aborted the move.
	if (!bDestroyed)
	{
		//FRotator NewRot = OriginalRotation;
		//NewRot.Pitch = bEnabled ? DOUBLEBARRICADE_ENABLED_ANGLE : DOUBLEBARRICADE_DISABLED_ANGLE;
		//Mesh->SetRelativeRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
		Mesh->SetRelativeRotation(bEnabled ? RotationOn : OriginalRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else if (Role == ROLE_Authority)
	{
		// We're destroyed, force the barricade down all the way
		SetEnabled(true);
	}

	if (Interactor && Interactor->IsLocallyControlled())
	{
		if (LinkedDoor)
		{
			Interactor->GetInteractableManagerComponent()->UnlockInteraction(LinkedDoor->InteractComponent);
		}
	}
}

void ASCBarrycade::UseBarricadeDriverTick(FAnimNotifyData NotifyData)
{
	UILLCharacterAnimInstance* AnimInstance = Cast<UILLCharacterAnimInstance>(NotifyData.AnimInstance);
	if (!AnimInstance)
	{
		return;
	}
	FRotator NewRot = FMath::Lerp(OriginalRotation, RotationOn, FMath::Clamp(AnimInstance->GetUnblendedCurveValue(NAME_DoubleBarricadeRotation), 0.0f, 1.0f));
	//NewRot.Pitch = FMath::Lerp(DOUBLEBARRICADE_DISABLED_ANGLE, DOUBLEBARRICADE_ENABLED_ANGLE, FMath::Clamp(AnimInstance->GetUnblendedCurveValue(NAME_DoubleBarricadeRotation), 0.0f, 1.0f));
	Mesh->SetRelativeRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
}
