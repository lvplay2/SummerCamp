// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCFirstAid.h"

#include "ILLInventoryComponent.h"

#include "SCCharacter.h"
#include "SCGameMode.h"
#include "SCGameState.h"
#include "SCPerkData.h"
#include "SCPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCFirstAid"

ASCFirstAid::ASCFirstAid(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HealthRestoredPerUse = 50.0f;
	FriendlyName = LOCTEXT("FriendlyName", "First Aid");
}

bool ASCFirstAid::Use(bool bFromInput)
{
	if (UILLInventoryComponent* Inventory = GetOuterInventory())
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Inventory->GetCharacterOwner()))
		{
			if (CounselorHealed)
			{
				float HealthToRestore = HealthRestoredPerUse;
				float HealthRestoreModifier = 0.0f;
				if (ASCPlayerState* PS = Counselor->GetPlayerState())
				{
					for (const USCPerkData* Perk : PS->GetActivePerks())
					{
						check(Perk);

						if (!Perk)
							continue;

						HealthRestoreModifier += Perk->FirstAidHealingModifier;
					}
				}

				HealthToRestore += HealthToRestore * HealthRestoreModifier;

				CounselorHealed->Health = FMath::Clamp(CounselorHealed->Health + HealthToRestore, (float)CounselorHealed->Health, 100.0f);
				CounselorHealed->MULTICAST_RemoveGore();
				++TimesUsed;
				OnRep_NumUses();

				if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
				{
					GameMode->HandleScoreEvent(Counselor->PlayerState, UseFirstAidScoreEvent);
				}

				// Track first aid uses
				if (ASCPlayerState* PS = Counselor->GetPlayerState())
				{
					PS->UsedFirstAidSpray(GetClass());
					PS->EarnedBadge(CounselorHealed == Counselor ? HealedSelfBadge : HealedOtherBadge);
				}

				MULTICAST_SetCounselorHealed(nullptr);
			}
		}
	}

	return NumUses - TimesUsed <= 0;
}

bool ASCFirstAid::CanUse(bool bFromInput)
{
	// If we already have a counselor set then we're already attempting a heal, don't allow another
	if (bIsInUse)
		return false;

	bool bCanUse = false;

	if (UILLInventoryComponent* Inventory = GetOuterInventory())
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Inventory->GetCharacterOwner()))
		{
			bCanUse = Counselor->Health < 100.f;
			if (bCanUse)
				MULTICAST_SetCounselorHealed(Counselor);

			if (!bCanUse)
			{
				UWorld* World = GetWorld();
				if (ASCGameState* State = World ? World->GetGameState<ASCGameState>() : nullptr)
				{
					for (ASCCharacter* OtherCharacter : State->PlayerCharacterList)
					{
						if (ASCCounselorCharacter* OtherCounselor = Cast<ASCCounselorCharacter>(OtherCharacter))
						{
							// Skip ourselves
							if (Counselor == OtherCounselor)
								continue;

							// Can't heal counselors at full health or dead counselors
							if (OtherCounselor->Health >= 100.f || !OtherCounselor->IsAlive())
								continue;

							const FVector ToOtherCounselor = OtherCounselor->GetActorLocation() - Counselor->GetActorLocation();

							if (ToOtherCounselor.SizeSquared() <= FMath::Square(Counselor->FirstAidDistance))
							{
								if ((Counselor->GetActorForwardVector() | ToOtherCounselor.GetSafeNormal()) <= Counselor->FirstAidAngle)
								{
									bCanUse = true;
									MULTICAST_SetCounselorHealed(OtherCounselor);
								}
							}
						}
					}
				}
			}
		}
	}

	return bCanUse;
}

UAnimMontage* ASCFirstAid::GetUseItemMontage(const USkeleton* Skeleton) const
{
	if (UILLInventoryComponent* Inventory = GetOuterInventory())
	{
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Inventory->GetCharacterOwner()))
		{
			if (CounselorHealed != Counselor)
			{
				return HealOtherMontage;
			}
		}
	}

	return Super::GetUseItemMontage(Skeleton);
}

void ASCFirstAid::AddedToInventory(UILLInventoryComponent* Inventory)
{
	Super::AddedToInventory(Inventory);

	if (Role == ROLE_Authority)
	{
		if (Inventory)
		{
			// Update our NumUses each time a player picks this up. This way our NumUses - TimesUsed will always be accurate for each player.
			if (const ASCCharacter* Character = Cast<ASCCharacter>(Inventory->GetCharacterOwner()))
			{
				NumUses = 1;

				// Check perks to see if we get extra uses out of Med Spray
				if (const ASCPlayerState* PS = Character->GetPlayerState())
				{
					for (const USCPerkData* Perk : PS->GetActivePerks())
					{
						check(Perk);

						if (!Perk)
							continue;

						NumUses += Perk->ExtraFirstAidUses;
					}
				}
			}

			bPerkBonusApplied = true;
			OnRep_NumUses();
		}
	}
}

void ASCFirstAid::MULTICAST_SetCounselorHealed_Implementation(ASCCounselorCharacter* Counselor)
{
	CounselorHealed = Counselor;
}

#undef LOCTEXT_NAMESPACE
