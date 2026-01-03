// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFlareGun.h"

#include "ILLInventoryComponent.h"
#include "SCCounselorCharacter.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"
#include "SCWeapon.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCFlareGun"

ASCFlareGun::ASCFlareGun(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	FriendlyName = LOCTEXT("FriendlyName", "Flare Gun");
}

void ASCFlareGun::AttachToCharacter(bool AttachToBody, FName OverrideSocket/* = NAME_None*/)
{
	if (FlareGunWeaponClass)
	{
		if (ASCCounselorCharacter* CharacterOwner = Cast<ASCCounselorCharacter>(GetCharacterOwner()))
		{
			if (CharacterOwner->Role != ROLE_Authority)
				return;

			if (UWorld* World = GetWorld())
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = CharacterOwner;

				ASCWeapon* FlareGun = World->SpawnActor<ASCWeapon>(FlareGunWeaponClass, GetActorLocation(), GetActorRotation(), SpawnParams);
				if (FlareGun)
				{
					CharacterOwner->PickingItem = FlareGun;
					CharacterOwner->PickingItem->bIsPicking = true;
					CharacterOwner->AddOrSwapPickingItem();
					CharacterOwner->OnForceDestroyItem(this);
					FlareGun->EnableInteraction(false);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
