// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPocketKnife.h"

#include "ILLInventoryComponent.h"

#include "SCCounselorCharacter.h"
#include "SCPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCPocketKnife"

ASCPocketKnife::ASCPocketKnife(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FriendlyName = LOCTEXT("FriendlyName", "Pocket Knife");
}

bool ASCPocketKnife::ShouldAttach(bool bFromInput)
{
	return !bFromInput;
}

bool ASCPocketKnife::CanUse(bool bFromInput)
{
	return !bFromInput;
}

bool ASCPocketKnife::Use(bool bFromInput)
{
	// Get the owner counselor and 'un-grab' them
	if (UILLInventoryComponent* Inventory = GetOuterInventory())
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Inventory->GetCharacterOwner()))
		{
			Counselor->EndStun();

			if (ASCPlayerState* PS = Counselor->GetPlayerState())
			{
				PS->PocketKnifeEscape(GetClass());
				PS->EarnedBadge(Counselor->PocketKnifeEscapeBadge);
			}
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
