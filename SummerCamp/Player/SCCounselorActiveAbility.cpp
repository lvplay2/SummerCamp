// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorActiveAbility.h"

#include "SCCounselorCharacter.h"
#include "SCGameState.h"
#include "SCKillerCharacter.h"


USCCounselorActiveAbility::USCCounselorActiveAbility(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bAOEAbility(true)
{
}

void USCCounselorActiveAbility::Activate(ASCCounselorCharacter* OwningCounselor)
{
	if (bUsed)
		return;

	bUsed = true;
	AbilityOwner = OwningCounselor;
	UWorld* World = OwningCounselor->GetWorld();

	if (AbilityVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, AbilityVFX, OwningCounselor->GetTransform());
	}

	if (OwningCounselor->IsLocallyControlled() && CounselorVO)
	{
		UGameplayStatics::PlaySound2D(World, CounselorVO);
	}

	if (bAOEAbility)
	{
		UClass* ClassType = AbilityType == EAbilityType::PamelasSweater ? ASCKillerCharacter::StaticClass() : ASCCounselorCharacter::StaticClass();
		ASCGameState* GS = Cast<ASCGameState>(World->GetGameState());
		const FVector OwnerLocation = OwningCounselor->GetActorLocation();
		const float AbilityRangeSq = FMath::Square(AbilityRange);

		for (ASCCharacter* Character : GS->PlayerCharacterList)
		{
			// Right class?
			if (!Character->IsA(ClassType))
				continue;

			// In range?
			const float DistSq = FVector::DistSquaredXY(OwnerLocation, Character->GetActorLocation());
			if (DistSq > AbilityRangeSq)
				continue;

			// Do it.
			if (bInstant)
			{
				InstantActivate(Character);
			}
			else if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(Character))
			{
				Counselor->ActivateAbility(this);
			}
		}
	}
	else
	{
		if (bInstant)
		{
			InstantActivate(OwningCounselor);
		}
		else
		{
			OwningCounselor->ActivateAbility(this);
		}
	}
}

bool USCCounselorActiveAbility::CanUseAbility() const
{
	// Couple safety checks
	if (HasBeenUsed() || !AbilityOwner)
		return false;

	// Tripping over ourselves
	if (AbilityOwner->bIsStumbling)
		return false;

	// Stunned
	if (AbilityOwner->IsStunned())
		return false;

	// In Combat Stance
	if (AbilityOwner->InCombatStance())
		return false;

	// Swimming
	if (AbilityOwner->CurrentStance == ESCCharacterStance::Swimming)
		return false;

	// Being help by a killer
	if (AbilityOwner->bIsGrabbed)
		return false;

	// Busy doing something else
	if (AbilityOwner->IsInSpecialMove())
		return false;

	if (AbilityOwner->IsInContextKill())
		return false;


	// Another safety check
	UWorld* World = AbilityOwner->GetWorld();
	if (!World)
		return false;

	switch (AbilityType)
	{
	case EAbilityType::PamelasSweater:
		{
			// Make sure we're in range
			if (ASCGameState* GameState = Cast<ASCGameState>(World->GetGameState()))
			{
				for (const ASCCharacter* Player : GameState->PlayerCharacterList)
				{
					if (const ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(Player))
					{
						// Can't activate on a killer doing a kill or currently morphing
						if (Killer->IsInContextKill() || Killer->IsMorphing())
							continue;

						const FVector KillerPos = Killer->GetActorLocation();
						const FVector AbilityPos = AbilityOwner->GetActorLocation();
						const float Distance = (KillerPos - AbilityPos).SizeSquared();

						if (Distance < FMath::Square(AbilityRange))
							return true;
					}
				}
			}

			// No killers nearby!
			return false;
		}
		break;
	}

	// We're probably fine.
	return true;
}

void USCCounselorActiveAbility::InstantActivate(ASCCharacter* Character)
{
	switch (AbilityType)
	{
	case EAbilityType::Healing:
		{
			ASCCounselorCharacter* const Counselor = Cast<ASCCounselorCharacter>(Character);
			const float CurrentHealth = Character->Health;
			float HealthToAdd = CurrentHealth * AbilityModifierValue;

			Counselor->Health = FMath::Clamp(CurrentHealth + HealthToAdd, CurrentHealth, Character->MaxHealth);
			Counselor->MULTICAST_RemoveGore();
		}
		break;
	case EAbilityType::Fear:
		{
			ASCCounselorCharacter* const Counselor = Cast<ASCCounselorCharacter>(Character);
			const float CurrentFear = Counselor->GetFear();
			float FearToRemove = CurrentFear * AbilityModifierValue;
			const float NewFear = FMath::Clamp(CurrentFear - FearToRemove, 0.0f, 100.0f);

			// Removed this because it wasn't going to work anyways and was going to cause problems with the fear quantization optimization.
			// If we decide abilities are going back in come talk to me about a proper way to do this. -DGarcia
			//Counselor->SetFear(NewFear);
		}
		break;
	case EAbilityType::PamelasSweater:
		{
			ASCKillerCharacter* const Killer = Cast<ASCKillerCharacter>(Character);
			Killer->StunnedBySweater(AbilityOwner);

			if (Killer->IsLocallyControlled() && JasonVO)
			{
				UGameplayStatics::PlaySound2D(Killer->GetWorld(), JasonVO);
			}
		}
		break;
	default:
		break;
	}
}
