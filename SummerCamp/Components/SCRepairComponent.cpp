// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCRepairComponent.h"
#include "SCInteractableManagerComponent.h"

#include "SCCounselorCharacter.h"
#include "SCKillerCharacter.h"

USCRepairComponent::USCRepairComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InteractMethods = (int32)EILLInteractMethod::Hold;
	HoldTimeLimit = 15.f;
	ActorIgnoreList.Add(ASCKillerCharacter::StaticClass());
}

void USCRepairComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USCRepairComponent, bIsRepaired);
}

void USCRepairComponent::OnInteractBroadcast(AActor* Interactor, EILLInteractMethod InteractMethod)
{
	ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Interactor);
	if (!Counselor)
		return;

	Counselor->GetInteractableManagerComponent()->UnlockInteraction(this);

	switch (InteractMethod)
	{
		case EILLInteractMethod::Hold:
		{
			bool bSuccess = false;
			int32 index = 0;
			for (; index < RequiredPartClasses.Num(); ++index)
			{
				TSubclassOf<ASCRepairPart>& PartClass = RequiredPartClasses[index];

				// Use Large Item first if we can
				if (ASCItem* EquippedItem = Counselor->GetEquippedItem())
				{
					if (EquippedItem->IsA(PartClass))
					{
						Counselor->SERVER_UseCurrentLargeItem(false);
						bSuccess = true;
						break;
					}

				}
				// Didn't use a large item, check if we have an appropriate small item
				for (ASCItem* Item : Counselor->GetSmallItems())
				{
					if (Item->IsA(PartClass))
					{
						Counselor->UseSmallRepairPart(Cast<ASCRepairPart>(Item));
						bSuccess = true;
						break;
					}
				}

				// Used a small item
				if (bSuccess)
					break;
			}

			if (bSuccess)
			{
				RequiredPartClasses.RemoveAt(index);
			}

			if (RequiredPartClasses.Num() <= 0)
			{
				GetOwner()->ForceNetUpdate();
				bIsEnabled = false;
				bIsRepaired = true;
				// For listen servers. #LANThings
				OnRep_IsRepaired();
			}
		}
		break;
	}

	Super::OnInteractBroadcast(Interactor, InteractMethod);
}

int32 USCRepairComponent::CanInteractWithBroadcast(AILLCharacter* Character, const FVector& ViewLocation, const FRotator& ViewRotation)
{
	if (Super::CanInteractWithBroadcast(Character, ViewLocation, ViewRotation))
	{
		// No parts needed!
		if (RequiredPartClasses.Num() == 0)
			return InteractMethods;

		// Check to see if we have any valid parts
		for (const auto& PartClass : RequiredPartClasses)
		{
			if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				// Large item
				if (ASCItem* EquippedItem = Counselor->GetEquippedItem())
				{
					if (EquippedItem->IsA(PartClass))
					{
						return InteractMethods;
					}
				}

				// Small items
				if (Counselor->HasSmallRepairPart(PartClass))
				{
					return InteractMethods;
				}
			}
		}
	}


	// Don't have the parts we need, can't repair!
	return 0;
}

void USCRepairComponent::ForceRepair()
{
	check(GetOwnerRole() == ROLE_Authority);

	GetOwner()->ForceNetUpdate();
	bIsRepaired = true;
	bIsEnabled = false;
	OnRep_IsRepaired();
}

void USCRepairComponent::OnRep_IsRepaired()
{
	OnRepair.Broadcast();
}
