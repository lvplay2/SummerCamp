// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCTimerWidget.h"

#include "ILLGameBlueprintLibrary.h"

#include "SCPlayerState.h"
#include "SCGameState_SPChallenges.h"

USCTimerWidget::USCTimerWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bCanEverTick = true;
	bShowTimer = false;
	bShowPoliceTimer = false;

	TimerText = FText::FromString(TEXT("00:00"));
	PoliceTimerText = FText::FromString(TEXT("00:00"));
}

void USCTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (!GetWorld())
		return;

	if (ASCGameState* GS = Cast<ASCGameState>(GetWorld()->GetGameState()))
	{
		int32 RemainingTime = GS->RemainingTime;

		// If we're in single player we need to display the out of bounds timer rather than the game timer if it's active.
		if (ASCGameState_SPChallenges* SPGS = Cast<ASCGameState_SPChallenges>(GS))
		{
			RemainingTime = SPGS->OutOfBoundsTime > 0 ? SPGS->OutOfBoundsTime : RemainingTime;
		}

		if (GS->IsMatchInProgress())
		{
			TimerText = FText::FromString(UILLGameBlueprintLibrary::TimeSecondsToTimeString(RemainingTime, true));
			if (!bShowTimer && RemainingTime <= TimeToShow && RemainingTime >= 0)
			{
				bShowTimer = true;
				ShowGameTimer(true);
			}
			else if (bShowTimer && (RemainingTime < 0 || RemainingTime > TimeToShow))
			{
				bShowTimer = false;
				HideGameTimer();
			}

			if (GS->PoliceTimeRemaining > 0)
			{
				if (!bShowPoliceTimer)
				{
					ShowPoliceTimer();
					bShowPoliceTimer = true;
				}

				PoliceTimerText = FText::FromString(UILLGameBlueprintLibrary::TimeSecondsToTimeString(GS->PoliceTimeRemaining, true));
			}
			else if (bShowPoliceTimer)
			{
				bShowPoliceTimer = false;
				HidePoliceTimer();
			}
		}
	}
}

ESlateVisibility USCTimerWidget::GetWalkieTalkieVisibility() const
{
	if (GetOwningPlayer())
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(GetOwningPlayer()->PlayerState))
			return PS->HasWalkieTalkie() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	}

	return ESlateVisibility::Collapsed;
}
