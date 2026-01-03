// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLRadialSelectWidget.h"

#include "Image.h"

#include "ILLPlayerController.h"
#include "ILLPlayerInput.h"
#include "ILLLocalPlayer.h"
#include "ILLInputSettingsSaveGame.h"

// Widget edge distance epsilon
#define NAVIGATION_EPSILON_MOUSE 15.f
#define NAVIGATION_EPSILON_CONTROLLER .5f

UILLRadialSelectWidget::UILLRadialSelectWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, NumSections(8)
, ConfirmInputName(TEXT("Confirm"))
, CancelInputName(TEXT("Cancel"))
, NavigationLeftXName(TEXT("LeftXNavigate"))
, NavigationLeftYName(TEXT("LeftYNavigate"))
, NavigationRightXName(TEXT("RightXNavigate"))
, NavigationRightYName(TEXT("RightYNavigate"))
, CurrentHighlightSection(INDEX_NONE)
{
}

void UILLRadialSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (AILLPlayerController* PC = GetOwningILLPlayer())
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				// Handle zero vector
				if (StickPos.IsNearlyZero(NAVIGATION_EPSILON_CONTROLLER))
				{
					// Hide the highlight
					bShowHighlight = false;
					CurrentHighlightSection = INDEX_NONE;
					OnStickIsNeutral();
				}
				else
				{
					CalculateAndUpdateHighlightRotation(StickPos.GetSafeNormal());
				}
			}
		}
	}
}

FReply UILLRadialSelectWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (AILLPlayerController* PC = GetOwningILLPlayer())
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(PC->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				return FReply::Unhandled();
			}
		}
	}

	FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	FVector2D Center = InGeometry.GetLocalSize() * 0.5f;
	FVector2D CenterToMouse = LocalPosition - Center;

	if (CenterToMouse.IsNearlyZero(NAVIGATION_EPSILON_MOUSE))
	{
		bShowHighlight = false;
		CurrentHighlightSection = INDEX_NONE;
	}
	else
	{

		// Check if the cursor is outside the widget bounds
		float RadiusSq = FMath::Square(WidgetDiameter * 0.5f);
		float SizeSq = CenterToMouse.SizeSquared();
		if (SizeSq >= RadiusSq)
		{
			bShowHighlight = false;
			CurrentHighlightSection = INDEX_NONE;
		}
		else
		{
			CalculateAndUpdateHighlightRotation(CenterToMouse.GetSafeNormal());
		}
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UILLRadialSelectWidget::OnConfirmPressed()
{
	if (CurrentHighlightSection != INDEX_NONE)
	{
		OnSelect(CurrentHighlightSection);
	}
}

void UILLRadialSelectWidget::OnCancelPressed()
{
	OnCancel();
}

void UILLRadialSelectWidget::OnNavigateX(float AxisValue)
{
	StickPos.X = AxisValue;
}

void UILLRadialSelectWidget::OnNavigateY(float AxisValue)
{
	float Invert = -1.0f;

	// FIXME: Make this more flexible
	// Only use inversion for look when using the stick for looking
	if (bUseRightStick)
	{
		UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(GetOwningLocalPlayer());
		if (UILLInputSettingsSaveGame* InputSettings = LocalPlayer->GetLoadedSettingsSave<UILLInputSettingsSaveGame>())
		{
			if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(GetOwningILLPlayer()->PlayerInput))
			{
				if (InputSettings->bInvertedControllerLook && Input->IsUsingGamepad())
					Invert = 1.0f;
			}
		}
	}

	StickPos.Y = AxisValue * Invert;
}

void UILLRadialSelectWidget::CalculateAndUpdateHighlightRotation(FVector2D InVector)
{
	// Rotate the "Up" vector if bOffsetRotation is true
	FVector2D Up(0.f, -1.f);
	FVector2D UpRotated = Up.GetRotated(-360.f / NumSections * 0.5f);
	float DotUp = FVector2D::DotProduct(bOffsetRotation ? UpRotated : Up, InVector);

	// Half-space test for left/right
	FVector2D Right(1.f, 0.f);
	FVector2D RightRotated = Right.GetRotated(-360.f / NumSections * 0.5f);
	float DotRight = FVector2D::DotProduct(bOffsetRotation ? RightRotated : Right, InVector);

	float AngleBetween = FMath::RadiansToDegrees(FMath::Acos(DotUp));
	// Put the angle into (0, 360)
	if (DotRight < 0.f)
	{
		AngleBetween = 360.f - AngleBetween;
	}

	// Snap the rotation based on our number of sections
	float SectionWidth = 360.f / NumSections;
	int32 LastHighlightSection = CurrentHighlightSection;
	CurrentHighlightSection = AngleBetween / SectionWidth;

	if (LastHighlightSection != CurrentHighlightSection)
	{
		// Put rotation in (-180, 180)  scope
		float Rotation = FMath::UnwindDegrees(SectionWidth * CurrentHighlightSection);

		// Show and Rotate the highlight
		bShowHighlight = true;
		OnUpdateHighlightRotation(Rotation);
	}
}
