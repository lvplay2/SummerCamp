// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCKillerAbilityWidget.h"
#include "SCPlayerController.h"
#include "ILLPlayerInput.h"

USCKillerAbilityWidget::USCKillerAbilityWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bCanEverTick = true;

	AvailableAbilities[(uint8) EKillerAbilities::Morph] = false;
	AvailableAbilities[(uint8) EKillerAbilities::Sense] = false;
	AvailableAbilities[(uint8) EKillerAbilities::Stalk] = false;
	AvailableAbilities[(uint8) EKillerAbilities::Shift] = false;

	ActiveAbilities[(uint8) EKillerAbilities::Morph] = false;
	ActiveAbilities[(uint8) EKillerAbilities::Sense] = false;
	ActiveAbilities[(uint8) EKillerAbilities::Shift] = false;
	ActiveAbilities[(uint8) EKillerAbilities::Stalk] = false;
}

void USCKillerAbilityWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		for (int i(0); i<(int)EKillerAbilities::MAX; ++i)
		{
			const EKillerAbilities CurrentAbility = (EKillerAbilities)i;
			const bool Available = Killer->IsAbilityAvailable(CurrentAbility);
			const bool Active = Killer->IsAbilityActive(CurrentAbility);

			switch (CurrentAbility)
			{
				case EKillerAbilities::Morph:
					MorphAmount = Killer->GetCurrentRechargeTimePercent(CurrentAbility);
					MorphUnlockAmount = Killer->GetAbilityUnlockPercent(CurrentAbility);
					break;
				case EKillerAbilities::Sense:
					SenseAmount = Killer->GetCurrentRechargeTimePercent(CurrentAbility);
					SenseUnlockAmount = Killer->GetAbilityUnlockPercent(CurrentAbility);
					break;
				case EKillerAbilities::Shift:
					ShiftAmount = Killer->GetCurrentRechargeTimePercent(CurrentAbility);
					ShiftUnlockAmount = Killer->GetAbilityUnlockPercent(CurrentAbility);
					break;
				case EKillerAbilities::Stalk:
					StalkAmount = Killer->GetCurrentRechargeTimePercent(CurrentAbility);
					StalkUnlockAmount = Killer->GetAbilityUnlockPercent(CurrentAbility);
					break;
			}

			if (ActiveAbilities[i] != Active)
			{
				if (Active)
					OnAbilityActive(CurrentAbility);
				else
					OnAbilityEnded(CurrentAbility);

				ActiveAbilities[i] = Active;
				break;
			}

			if (AvailableAbilities[i] != Available)
			{
				if (Available)
				{
					OnAbilityAvailable(CurrentAbility);
				}
				else
				{
					OnAbilityUsed(CurrentAbility);
				}

				AvailableAbilities[i] = Available;
				break;
			}
		}
	}
}

FLinearColor USCKillerAbilityWidget::GetAbilityIconColor(EKillerAbilities Ability) const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (Killer->IsAbilityUnlocked(Ability))
		{
			if (Killer->IsAbilityAvailable(Ability))
				return IconUsableColor;

			return IconUnusableColor;
		}
	}

	return IconLockedColor;
}

ESlateVisibility USCKillerAbilityWidget::GetWidgetVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (Killer->GetGrabbedCounselor() != nullptr)
			return ESlateVisibility::Collapsed;
	}

	return ESlateVisibility::HitTestInvisible;
}

FLinearColor USCKillerAbilityWidget::GetStalkIconColor() const
{
	return GetAbilityIconColor(EKillerAbilities::Stalk);
}

FLinearColor USCKillerAbilityWidget::GetSenseIconColor() const
{
	return GetAbilityIconColor(EKillerAbilities::Sense);
}

FLinearColor USCKillerAbilityWidget::GetMorphIconColor() const
{
	return GetAbilityIconColor(EKillerAbilities::Morph);
}

FLinearColor USCKillerAbilityWidget::GetShiftIconColor() const
{
	return GetAbilityIconColor(EKillerAbilities::Shift);
}

ESlateVisibility USCKillerAbilityWidget::GetStalkGlowVisibility() const
{
	if (ActiveAbilities[(uint8) EKillerAbilities::Stalk])
		return ESlateVisibility::HitTestInvisible;

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetSenseGlowVisibility() const
{
	if (ActiveAbilities[(uint8)EKillerAbilities::Sense])
		return ESlateVisibility::HitTestInvisible;

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetMorphGlowVisibility() const
{
	if (ActiveAbilities[(uint8) EKillerAbilities::Morph])
		return ESlateVisibility::HitTestInvisible;

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetShiftGlowVisibility() const
{
	if (ActiveAbilities[(uint8) EKillerAbilities::Shift])
		return ESlateVisibility::HitTestInvisible;

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetStalkActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		const bool Available = Killer->IsAbilityAvailable(EKillerAbilities::Stalk);
		const bool Powers = Killer->IsPowersEnabled();

		if (Available)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
			{
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
				{
					if (Input->IsUsingGamepad())
					{
						return Powers ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
					}
				}
			}

			return ESlateVisibility::HitTestInvisible;
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetSenseActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		const bool Available = Killer->IsAbilityAvailable(EKillerAbilities::Sense);
		const bool Powers = Killer->IsPowersEnabled();

		if (Available)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
			{
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
				{
					if (Input->IsUsingGamepad())
					{
						return Powers ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
					}
				}
			}

			return ESlateVisibility::HitTestInvisible;
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetMorphActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		const bool Available = Killer->IsAbilityAvailable(EKillerAbilities::Morph);
		const bool Powers = Killer->IsPowersEnabled();

		if (Available)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
			{
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
				{
					if (Input->IsUsingGamepad())
					{
						return Powers ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
					}
				}
			}

			return ESlateVisibility::HitTestInvisible;
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetShiftActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		const bool Available = Killer->IsAbilityAvailable(EKillerAbilities::Shift);
		const bool Powers = Killer->IsPowersEnabled();

		if (Available)
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
			{
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
				{
					if (Input->IsUsingGamepad())
					{
						return Powers ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
					}
				}
			}

			return ESlateVisibility::HitTestInvisible;
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCKillerAbilityWidget::GetPowersActionMapVisibility() const
{
	if (ASCKillerCharacter* Killer = Cast<ASCKillerCharacter>(GetOwningPlayerPawn()))
	{
		if (!Killer->IsPowersEnabled())
		{
			if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
			{
				if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
				{
					if (Input->IsUsingGamepad())
					{
						if (Killer->IsAbilityAvailable(EKillerAbilities::Morph) ||
							Killer->IsAbilityAvailable(EKillerAbilities::Sense) ||
							Killer->IsAbilityAvailable(EKillerAbilities::Shift) ||
							Killer->IsAbilityAvailable(EKillerAbilities::Stalk) )
						return ESlateVisibility::HitTestInvisible;
					}
				}
			}
		}
	}

	return ESlateVisibility::Collapsed;
}
