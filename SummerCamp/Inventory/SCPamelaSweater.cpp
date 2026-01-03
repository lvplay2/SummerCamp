// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPamelaSweater.h"
#include "SCCounselorCharacter.h"
#include "SCInteractComponent.h"
#include "Animation/SkeletalMeshActor.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPamelaSweater"

ASCPamelaSweater::ASCPamelaSweater(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FriendlyName = LOCTEXT("FriendlyName", "Pamela Sweater");
	InventoryMax = 1;
}

void ASCPamelaSweater::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (SweaterAbilityClass)
	{
		SweaterAbility = NewObject<USCCounselorActiveAbility>(this, SweaterAbilityClass);
	}
}

int32 ASCPamelaSweater::CanInteractWith(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	int32 Flags = 0;
	const ASCCounselorCharacter* const Counselor = Cast<const ASCCounselorCharacter>(Character);
	if (Counselor && Counselor->GetIsFemale())
	{
		Flags = Super::CanInteractWith(Character, ViewLocation, ViewRotation);
	}

	return Flags;
}

void ASCPamelaSweater::PutOn()
{
	if (Role == ROLE_Authority)
	{
		SetActorHiddenInGame(true);
		SetLifeSpan(5.f);
	}
}

#undef LOCTEXT_NAMESPACE
