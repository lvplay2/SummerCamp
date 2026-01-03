// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCounselorInventoryWidget.h"
#include "SCPlayerController.h"
#include "ILLPlayerInput.h"
#include "SCCounselorCharacter.h"

USCCounselorInventoryWidget::USCCounselorInventoryWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bCanEverTick = true;
}

ESlateVisibility USCCounselorInventoryWidget::GetSlotVisibility(int ItemSlot) const
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				if (ASCCounselorCharacter* SCPlayer = Cast<ASCCounselorCharacter>(GetOwningPlayerPawn()))
				{
					if (SCPlayer->GetSmallItemIndex() == ItemSlot)
					{
						if (SCPlayer->GetCurrentSmallItem())
						{
							if (SCPlayer->GetCurrentSmallItem()->CanUse(true))
								return ESlateVisibility::HitTestInvisible;
						}
					}
				}
			}
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCCounselorInventoryWidget::GetSlotBackgroundVisibility(int ItemSlot) const
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				if (ASCCounselorCharacter* SCPlayer = Cast<ASCCounselorCharacter>(GetOwningPlayerPawn()))
				{
					if (SCPlayer->GetSmallItemIndex() == ItemSlot)
					{
						if (SCPlayer->GetCurrentSmallItem())
						{
							if (SCPlayer->GetCurrentSmallItem()->CanUse(true))
								return ESlateVisibility::HitTestInvisible;
						}
					}
				}
			}
			else
			{
				return ESlateVisibility::HitTestInvisible;
			}
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCCounselorInventoryWidget::GetSlot1UseVisibility() const
{
	return GetSlotVisibility(0);
}

ESlateVisibility USCCounselorInventoryWidget::GetSlot2UseVisibility() const
{
	return GetSlotVisibility(1);
}

ESlateVisibility USCCounselorInventoryWidget::GetSlot3UseVisibility() const
{
	return GetSlotVisibility(2);
}

ESlateVisibility USCCounselorInventoryWidget::GetSlot1BackgroundVisibility() const
{
	return GetSlotBackgroundVisibility(0);
}

ESlateVisibility USCCounselorInventoryWidget::GetSlot2BackgroundVisibility() const
{
	return GetSlotBackgroundVisibility(1);
}

ESlateVisibility USCCounselorInventoryWidget::GetSlot3BackgroundVisibility() const
{
	return GetSlotBackgroundVisibility(2);
}

ESlateVisibility USCCounselorInventoryWidget::GetCycleButtonsVisibility() const
{
	if (ASCPlayerController* PC = Cast<ASCPlayerController>(GetOwningPlayer()))
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
				return ESlateVisibility::HitTestInvisible;
		}
	}

	return ESlateVisibility::Collapsed;
}
