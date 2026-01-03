// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCInteractWidget.h"

#include "Kismet/GameplayStatics.h"
#include "TextWidgetTypes.h"
#include "TextBlock.h"

#include "SCCharacter.h"
#include "SCInteractComponent.h"
#include "SCInteractableManagerComponent.h"
#include "CanvasPanel.h"
#include "CanvasPanelSlot.h"

USCInteractWidget::USCInteractWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LinkedInteractable(nullptr)
{
	bCanEverTick = true;
}

void USCInteractWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!LinkedInteractable)
		return;

	if (DynamicHoldMaterial)
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn()))
		{
			const float interactPercent = Character->GetInteractTimePercent();
			DynamicHoldMaterial->SetScalarParameterValue(TEXT("Alpha"), interactPercent);
		}
	}
}

void USCInteractWidget::UpdatePosition()
{
	if (!LinkedInteractable)
		return;

	if (APlayerController* PC = Cast<APlayerController>(GetOwningPlayer()))
	{
		FVector2D ScreenLocation;
		if (bDrawInExactScreenSpace)
		{
			ScreenLocation = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY()) * ScreenLocationRatio;
			// Center it, bitch
			ScreenLocation.X -= GetDesiredSize().X * 0.5f;
			ScreenLocation.Y -= GetDesiredSize().Y * 0.5f;
			SetRenderTranslation(ScreenLocation);
			SetVisibility(ESlateVisibility::Visible);
			return;
		}
		else if (UGameplayStatics::ProjectWorldToScreen(PC, LinkedInteractable->GetComponentLocation(), ScreenLocation))
		{
			// Center it, bitch
			ScreenLocation.X -= GetDesiredSize().X * 0.5f;
			ScreenLocation.Y -= GetDesiredSize().Y * 0.5f;
			SetRenderTranslation(ScreenLocation);
			SetVisibility(ESlateVisibility::Visible);
			return;
		}
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

ESlateVisibility USCInteractWidget::ShowHoldRing() const
{
	if (LinkedInteractable && bBestInteractable)
	{
		if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn()))
		{
			if (!Character->InteractTimerPassedMinHoldTime())
				return ESlateVisibility::Collapsed;

			if (LinkedInteractable->Pips.Num() == 0 && CanHoldInteract())
				return ESlateVisibility::Visible;
		}
	}

	return ESlateVisibility::Collapsed;
}

ESlateVisibility USCInteractWidget::ShowHoldRingBackground() const
{
	if (LinkedInteractable && bBestInteractable)
	{
		if (CanHoldInteract())
			return ESlateVisibility::Visible;
	}

	return ESlateVisibility::Collapsed;
}

bool USCInteractWidget::CanHoldInteract() const
{
	if (LinkedInteractable)
	{
		int32 Flags = (int32)LinkedInteractable->InteractMethods;
		if (ASCCharacter* Character = Cast<ASCCharacter>(GetOwningPlayerPawn()))
		{
			if (Character->GetInteractableManagerComponent()->GetBestInteractableComponent() == LinkedInteractable)
			{
				Flags = Character->GetInteractableManagerComponent()->GetBestInteractableMethodFlags();
				return (Flags & (int32) EILLInteractMethod::Hold) == (int32) EILLInteractMethod::Hold;
			}
		}
	}

	return false;
}


void USCInteractWidget::CleanUpWidget()
{
	// notify the blueprint side that we should clean up anything
	OnCleanUpWidget();
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromViewport();
	MarkPendingKill();
}
