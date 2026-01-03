// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLUserWidget.h"

#include "Engine/Console.h"
#include "Input/HittestGrid.h"
#include "UMG.h"
#include "WidgetLayoutLibrary.h"

#include "ILLGameViewportClient.h"
#include "ILLHUD.h"
#include "ILLPlayerInput.h"

DEFINE_LOG_CATEGORY(LogILLWidget);

// Threshold for input axis to pass for a single navigation move
#define NAVIGATION_MOVE_DELTA .5f

// Minimum axis delta for a repeated axis input
#define NAVIGATION_REPEAT_DELTA .75f

// Widget edge distance epsilon
#define NAVIGATION_EPSILON .5f

// Frame counter when we have a foreground menu
static uint8 GForegroundFrameCounter;

// Last frame that an input was processed on, to prevent button mash
static uint8 GLastInputForegroundFrameCount;

UILLUserWidget::UILLUserWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MenuZOrder = 100;
	bTopMostMenu = true;
	bAllowGamepadNavigation = true;
	DeferNavigationTime = 0.1f;

	// Bumping up default priority so that we take precedence over the menu input which would otherwise block our input
	Priority = 150;
}

UWorld* UILLUserWidget::GetWorld() const
{
	if (!bWorldForbidden)
	{
		return Super::GetWorld();
	}

	return nullptr;
}

void UILLUserWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	
#if WITH_EDITOR
	if (IsDesignTime())
	{
		OnSyncDesignTime();
	}
#endif
}

void UILLUserWidget::RemoveFromParent()
{
	// Cleanup the input blocker
	if (SilentSlatePreprocessor.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GI = World->GetGameInstance())
			{
				if (UILLGameViewportClient* VPC = Cast<UILLGameViewportClient>(GI->GetGameViewportClient()))
				{
					VPC->RemoveSlatePreprocessor(SilentSlatePreprocessor);
				}
			}
		}
	}

	const bool bWasVisible = IsInViewport() || GetParent();

	Super::RemoveFromParent();
	
#if WITH_EDITOR
	if (!IsDesignTime())
#endif
	if (CanSafelyRouteEvent() && bWasVisible)
	{
		// Trigger the event locally and broadcast the removal to all child ILLUserWidgets
		RecursiveBroadcastRemoval(this);
	}
}

void UILLUserWidget::RecursiveBroadcastRemoval(UILLUserWidget* FromWidget)
{
	if (FromWidget->CanSafelyRouteEvent())
	{
		if (bIsNavigable && GetOwningPlayer())
		{
			if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(GetOwningPlayer()->PlayerInput))
			{
				// No longer listen for gamepad usage changes
				Input->OnUsingGamepadChanged.RemoveDynamic(this, &UILLUserWidget::OnUsingGamepadChanged);
			}
		}

		// Trigger the event locally
		FromWidget->OnRemovedFromParentBroadcast();
		if (FromWidget->OnRemovedFromParent.IsBound())
		{
			FromWidget->OnRemovedFromParent.Broadcast(this);
		}

		// Recurse
		if (FromWidget->WidgetTree)
		{
			FromWidget->WidgetTree->ForEachWidget([&](UWidget* Widget)
			{
				if (UILLUserWidget* UserWidget = Cast<UILLUserWidget>(Widget))
				{
					RecursiveBroadcastRemoval(UserWidget);
				}
			});
		}
	}
}

bool UILLUserWidget::Initialize()
{
	if (Super::Initialize())
	{
		if (bIsNavigable && GetOwningPlayer())
		{
			if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(GetOwningPlayer()->PlayerInput))
			{
				// Listen for gamepad usage changes
				Input->OnUsingGamepadChanged.AddDynamic(this, &UILLUserWidget::OnUsingGamepadChanged);

				// Fake an update now
				// This ensures that the cursor is suppressed when a new menu is opened
				OnUsingGamepadChanged(Input->IsUsingGamepad());
			}
		}

		return true;
	}

	return false;
}

void UILLUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsNavigable || !bEnableNavigation || !FSlateApplication::IsInitialized())
	{
		ForegroundFrameTime = 0.f;
		return;
	}

	// Invalidate NavigableWidgetCache
	bNavigationCacheInvalidated = true;

	// Do nothing while the console is up
	UConsole* ViewportConsole = (GEngine && GEngine->GameViewport) ? GEngine->GameViewport->ViewportConsole : nullptr;
	if (ViewportConsole && ViewportConsole->ConsoleState != NAME_None)
	{
		ForegroundFrameTime = 0.f;
		return;
	}

	// Now to check for focus...
	TSharedPtr<SWidget> ThisWidget = GetCachedWidget();
	if (!ThisWidget.IsValid())
	{
		ForegroundFrameTime = 0.f;
		return;
	}

	// Die out if our window is not the foreground window
	TSharedPtr<SWindow> OurWindow = FSlateApplication::Get().FindWidgetWindow(ThisWidget.ToSharedRef());
	if (!OurWindow.IsValid() || !OurWindow->GetNativeWindow()->IsForegroundWindow())
	{
		ForegroundFrameTime = 0.f;
		return;
	}

	++GForegroundFrameCounter;
	ForegroundFrameTime += InDeltaTime;
	if (ForegroundFrameTime < DeferNavigationTime)
		return;

	// Update CachedWidgetRect
	CachedWidgetRect = BoundingRectForGeometry(MyGeometry);

	// Assign keyboard focus to the widget under the mouse
	// This allows ENTER/A etc events to be sent directly to the keyboard-focusable Slate widget within in
	SyncKeyboardWithMouseFocus();

	// If there still is no keyboard focus, assign it to default or top-left most widget
	ConditionalAttemptDefaultNavigation(false);
}

void UILLUserWidget::OnInputAction(FOnInputAction Callback)
{
	if (bTransitioningOut)
	{
		UE_LOG(LogILLWidget, Warning, TEXT("OnInputAction ignored due to transition out!"));
	}
	else if (GLastInputForegroundFrameCount == GForegroundFrameCounter)
	{
		UE_LOG(LogILLWidget, Warning, TEXT("OnInputAction ignored due to button mash!"));
	}
	else
	{
		GLastInputForegroundFrameCount = GForegroundFrameCounter;
		Super::OnInputAction(Callback);
	}
}

APawn* UILLUserWidget::GetOwningViewTargetPawn() const
{
	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (APawn* ViewPawn = Cast<APawn>(OwningPC->GetViewTarget()))
		{
			return ViewPawn;
		}
	}

	return GetOwningPlayerPawn();
}

AILLPlayerController* UILLUserWidget::GetOwningILLPlayer() const
{
	return Cast<AILLPlayerController>(GetOwningPlayer());
}

AILLHUD* UILLUserWidget::GetOwningPlayerILLHUD() const
{
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		return Cast<AILLHUD>(OwningPlayer->GetHUD());
	}

	return nullptr;
}

void UILLUserWidget::PlayTransitionOut(TSubclassOf<UILLUserWidget> TransitioningTo/* = nullptr*/)
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::PlayTransitionOut: TransitioningTo: %s Block input: %s"), *GetName(), TransitioningTo ? *TransitioningTo->GetFullName() : TEXT("NULL"), bBlockInputForTransitionOut ? TEXT("true") : TEXT("false"));

	TransitionTarget = TransitioningTo;
	bTransitioningOut = true;

	if (bBlockInputForTransitionOut)
	{
		// Spawn an input blocker
		if (!SilentSlatePreprocessor.IsValid())
		{
			if (UWorld* World = GetWorld())
			{
				if (UGameInstance* GI = World->GetGameInstance())
				{
					if (UILLGameViewportClient* VPC = Cast<UILLGameViewportClient>(GI->GetGameViewportClient()))
					{
						SilentSlatePreprocessor = MakeShareable(new FILLNULLSlateViewportInputProcessor());
						VPC->PushSlatePreprocessor(SilentSlatePreprocessor);
					}
				}
			}
		}
	}

	// Trigger animations
	OnPlayTransitionOut(TransitioningTo);

	// Broadcast OnParentTransitioningOut
	RecursiveBroadcastTransitionOut(this);
}

void UILLUserWidget::RecursiveBroadcastTransitionOut(UILLUserWidget* FromWidget)
{
	if (FromWidget->CanSafelyRouteEvent() && FromWidget->WidgetTree)
	{
		FromWidget->WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (UILLUserWidget* UserWidget = Cast<UILLUserWidget>(Widget))
			{
				UserWidget->bTransitioningOut = true; // HACK: Assign this to true too
				UserWidget->OnParentTransitioningOut();
				RecursiveBroadcastTransitionOut(UserWidget);
			}
		});
	}
}

void UILLUserWidget::OnPlayTransitionOut_Implementation(TSubclassOf<UILLUserWidget> TransitioningTo)
{
	OnTransitionOutComplete();
}

void UILLUserWidget::OnTransitionOutComplete()
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::OnTransitionOutComplete"), *GetName());

	if (OnTransitionCompleteOverride.IsBound())
	{
		OnTransitionCompleteOverride.Execute(this);
	}
	else
	{
		OnTransitionComplete.Broadcast(this);

		// Self-destruct
		RemoveFromParent();
	}
}

bool UILLUserWidget::MenuInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (!bIsNavigable)
	{
		return false;
	}

	// Determine the direction to navigate
	EUINavigation NavigationType = EUINavigation::Invalid;
	if (Key == EKeys::Gamepad_LeftX)
	{
		if (LastAxisDeltas.X < NAVIGATION_MOVE_DELTA && Delta >= NAVIGATION_MOVE_DELTA)
		{
			AttemptNavigation(EUINavigation::Right, bGamepad, true);
		}
		else if (LastAxisDeltas.X > -NAVIGATION_MOVE_DELTA && Delta <= -NAVIGATION_MOVE_DELTA)
		{
			AttemptNavigation(EUINavigation::Left, bGamepad, true);
		}

		LastAxisDeltas.X = Delta;
		return true;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		if (LastAxisDeltas.Y < NAVIGATION_MOVE_DELTA && Delta >= NAVIGATION_MOVE_DELTA)
		{
			AttemptNavigation(EUINavigation::Up, bGamepad, true);
		}
		else if (LastAxisDeltas.Y > -NAVIGATION_MOVE_DELTA && Delta <= -NAVIGATION_MOVE_DELTA)
		{
			AttemptNavigation(EUINavigation::Down, bGamepad, true);
		}

		LastAxisDeltas.Y = Delta;
		return true;
	}

	return false;
}

bool UILLUserWidget::MenuInputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	if (!bIsNavigable)
	{
		return false;
	}

	if (EventType != EInputEvent::IE_Pressed && EventType != EInputEvent::IE_Repeat)
	{
		return false;
	}

	// Determine the direction to navigate
	EUINavigation NavigationType = EUINavigation::Invalid;
	if (Key == EKeys::Gamepad_LeftStick_Up)
	{
		if (EventType == EInputEvent::IE_Repeat && LastAxisDeltas.Y >= NAVIGATION_REPEAT_DELTA)
			NavigationType = EUINavigation::Up;
	}
	else if (Key == EKeys::Gamepad_LeftStick_Down)
	{
		if (EventType == EInputEvent::IE_Repeat && LastAxisDeltas.Y <= -NAVIGATION_REPEAT_DELTA)
			NavigationType = EUINavigation::Down;
	}
	else if (Key == EKeys::Gamepad_LeftStick_Right)
	{
		if (EventType == EInputEvent::IE_Repeat && LastAxisDeltas.X >= NAVIGATION_REPEAT_DELTA)
			NavigationType = EUINavigation::Right;
	}
	else if (Key == EKeys::Gamepad_LeftStick_Left)
	{
		if (EventType == EInputEvent::IE_Repeat && LastAxisDeltas.X <= -NAVIGATION_REPEAT_DELTA)
			NavigationType = EUINavigation::Left;
	}
	else if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
	{
		NavigationType = EUINavigation::Up;
	}
	else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
	{
		NavigationType = EUINavigation::Down;
	}
	else if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left)
	{
		NavigationType = EUINavigation::Left;
	}
	else if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right)
	{
		NavigationType = EUINavigation::Right;
	}
	else if (Key == EKeys::Tab)
	{
		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			UPlayerInput* Input = OwningPlayer->PlayerInput;
			if (!Input->IsCtrlPressed() && !Input->IsAltPressed() && !Input->IsCmdPressed())
			{
				NavigationType = Input->IsShiftPressed() ? EUINavigation::Previous : EUINavigation::Next;
			}
		}
	}

	if (NavigationType == EUINavigation::Invalid)
	{
		return false;
	}

	// Now navigate
	AttemptNavigation(NavigationType, bGamepad, true);
	return true;
}

bool UILLUserWidget::NavigateToNamedWidget(FName WidgetName)
{
	if (WidgetName.IsNone())
		return false;
	
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::NavigateToNamedWidget: %s"), *GetName(), *WidgetName.ToString());

	// Filter navigable widgets for WidgetName
	TNavigableWidgetList MatchingNavigableWidgets;
	MatchingNavigableWidgets.Empty(NavigableWidgetCache.Num());
	RecursiveCollectNavigableWidgets(this, MatchingNavigableWidgets, WidgetName);

	// Navigate to the first one found
	if (MatchingNavigableWidgets.Num() > 0)
	{
		NavigateToWidget(MatchingNavigableWidgets[0], false, false);
		return true;
	}

	return false;
}

void UILLUserWidget::ConditionalAttemptDefaultNavigation(const bool bGamepadCheck)
{
	ConditionalUpdateNavigationCache();

	// Attempt navigation immediately if the current keyboard focus is not set
	SNavigableWidgetItem KeyboardFocus = FindKeyboardFocusedNavigableWidget();
	if (!KeyboardFocus.SlateWidget.IsValid())
	{
		// Attempt to navigate to the first best widget
		AttemptDefaultNavigation();
	}
	else if (bGamepadCheck && !KeyboardFocus.bAllowedForGamepad)
	{
		// Re-navigate if we know we're using a gamepad and our current focus is forbidden for that
		AttemptDefaultNavigation();
	}

	// If we still have no keyboard focus
	if (MyWidget.IsValid())
	{
		TSharedPtr<SWidget> ThisWidget = MyWidget.Pin();
		KeyboardFocus = FindKeyboardFocusedNavigableWidget();
		if (!KeyboardFocus.SlateWidget.IsValid())
		{
			// And we have no focused descendants
			if (!FSlateApplication::Get().HasFocusedDescendants(ThisWidget.ToSharedRef()))
			{
				// Focus the menu itself, so that input action bindings can still work
				if (bAllUserFocus)
					FSlateApplication::Get().SetAllUserFocus(ThisWidget, EFocusCause::Navigation);
				else
					FSlateApplication::Get().SetKeyboardFocus(ThisWidget, EFocusCause::Navigation);
			}
		}
	}
}

void UILLUserWidget::AttemptDefaultNavigation()
{
	UE_LOG(LogILLWidget, VeryVerbose, TEXT("%s::AttemptDefaultNavigation: DefaultFocusWidgetName: %s"), *GetName(), *DefaultFocusWidgetName.ToString());

	if (!NavigateToNamedWidget(RequestedDefaultNavigation))
	{
		if (!NavigateToNamedWidget(DefaultFocusWidgetName))
		{
			// Fallback to TAB-style navigation
			AttemptNavigation(EUINavigation::Next, FSlateRect(), false);
		}
	}
}

void UILLUserWidget::AttemptNavigation(const EUINavigation NavigationType, const bool bForGamepad, const bool bForgeCursorMove/* = false*/)
{
	// Build a SourceRect from either the CurrentFocusWidget or just the current cursor position
	FSlateRect SourceRect;
	SNavigableWidgetItem CurrentFocusWidget = FindKeyboardFocusedNavigableWidget();
	if (CurrentFocusWidget.SlateWidget.IsValid())
	{
		SourceRect = CurrentFocusWidget.Rect;
	}
	else
	{
		const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
		SourceRect = FSlateRect(CursorPosition, CursorPosition);
	}

	// Navigate!
	AttemptNavigation(NavigationType, SourceRect, bForGamepad, bForgeCursorMove);
}

void UILLUserWidget::AttemptNavigation(const EUINavigation NavigationType, FSlateRect SourceRect, bool bForGamepad, const bool bForgeCursorMove/* = false*/)
{
	// Rebuild navigation if needed
	ConditionalUpdateNavigationCache();

	// Auto-detect if this is for gamepad navigation
	if (!bForGamepad)
	{
		if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(GetOwningPlayer()->PlayerInput))
		{
			if (Input->IsUsingGamepad())
			{
				bForGamepad = true;
			}
		}
	}

	SNavigableWidgetItem WidgetToFocus;
	const float MinimumHorizontalDistance = (SourceRect.Right-SourceRect.Left) * -0.5f;
	const float MinimumVerticalDistance = (SourceRect.Bottom-SourceRect.Top) * -0.5f;
	if (NavigationType == EUINavigation::Left || NavigationType == EUINavigation::Right || NavigationType == EUINavigation::Next || NavigationType == EUINavigation::Previous)
	{
		auto TieBreaker = [&](const SNavigableWidgetItem& WidgetEntry) -> float
		{
			// Break distance ties by comparing the source delta to the middle of WidgetEntry
			const float SourceCenter = (SourceRect.Bottom+SourceRect.Top)*.5f;
			const float WidgetCenter = (WidgetEntry.Rect.Bottom+WidgetEntry.Rect.Top)*.5f;
			return (SourceCenter > WidgetCenter) ? SourceCenter - WidgetCenter : WidgetCenter - SourceCenter;
		};
		auto DistanceEval = [&](const SNavigableWidgetItem& WidgetEntry) -> float
		{
			if (NavigationType == EUINavigation::Right || NavigationType == EUINavigation::Next)
				return WidgetEntry.Rect.Left - SourceRect.Right;
			return SourceRect.Left - WidgetEntry.Rect.Right;
		};

		// Scan for collision along the movement direction
		WidgetToFocus = ClosestWidgetEvaluator(bForGamepad, MinimumHorizontalDistance,
			[&](const SNavigableWidgetItem& WidgetEntry) -> float
			{
				if (WidgetEntry.Rect.Bottom < SourceRect.Top || WidgetEntry.Rect.Top > SourceRect.Bottom)
					return FLT_MAX;

				return DistanceEval(WidgetEntry);
			}, TieBreaker);

		if (!WidgetToFocus.SlateWidget.IsValid())
		{
			if (NavigationType == EUINavigation::Next)
			{
				// Special case handling of TAB
				WidgetToFocus = ClosestWidgetEvaluator(bForGamepad, 0.f,
					[&](const SNavigableWidgetItem& WidgetEntry) -> float
					{
						// Find the widget closest to the top-left below our SourceRect
						if (WidgetEntry.Rect.Top >= SourceRect.Bottom)
							return WidgetEntry.Rect.Top - SourceRect.Bottom;

						return FLT_MAX;
					}, [&](const SNavigableWidgetItem& WidgetEntry) -> float
					{
						// Tie break on the distance of the left edge to the left side of the screen
						return WidgetEntry.Rect.Left - CachedWidgetRect.Left;
					});
			}
			else if (NavigationType == EUINavigation::Previous)
			{
				// Special case handling of Shift+TAB
				WidgetToFocus = ClosestWidgetEvaluator(bForGamepad, 0.f,
					[&](const SNavigableWidgetItem& WidgetEntry) -> float
					{
						// Find the widget closest to the top-right above our SourceRect
						if (WidgetEntry.Rect.Bottom <= SourceRect.Top)
							return SourceRect.Top - WidgetEntry.Rect.Bottom;

						return FLT_MAX;
					}, [&](const SNavigableWidgetItem& WidgetEntry) -> float
					{
						// Tie break on the distance of the right edge to the right side of the screen
						return CachedWidgetRect.Right - WidgetEntry.Rect.Right;
					});
			}
			else
			{
				// Simply find the closest edge in that direction
				WidgetToFocus = ClosestWidgetEvaluator(bForGamepad, MinimumHorizontalDistance, DistanceEval, TieBreaker);
			}
		}
	}
	else if (NavigationType == EUINavigation::Up || NavigationType == EUINavigation::Down)
	{
		auto TieBreaker = [&](const SNavigableWidgetItem& WidgetEntry) -> float
		{
			// Break distance ties by comparing the source delta to the middle of WidgetEntry
			const float SourceMiddle = (SourceRect.Left+SourceRect.Right)*.5f;
			const float WidgetMiddle = (WidgetEntry.Rect.Left+WidgetEntry.Rect.Right)*.5f;
			return (SourceMiddle > WidgetMiddle) ? SourceMiddle - WidgetMiddle : WidgetMiddle - SourceMiddle;
		};
		auto DistanceEval = [&](const SNavigableWidgetItem& WidgetEntry) -> float
		{
			if (NavigationType == EUINavigation::Down)
				return WidgetEntry.Rect.Top - SourceRect.Bottom;
			return SourceRect.Top - WidgetEntry.Rect.Bottom;
		};

		// Scan for collision along the movement direction
		WidgetToFocus = ClosestWidgetEvaluator(bForGamepad, MinimumVerticalDistance,
			[&](const SNavigableWidgetItem& WidgetEntry) -> float
			{
				if (WidgetEntry.Rect.Right < SourceRect.Left || WidgetEntry.Rect.Left > SourceRect.Right)
					return FLT_MAX;

				return DistanceEval(WidgetEntry);
			}, TieBreaker);

		if (!WidgetToFocus.SlateWidget.IsValid())
		{
			// Simply find the closest edge in that direction
			WidgetToFocus = ClosestWidgetEvaluator(bForGamepad, MinimumVerticalDistance, DistanceEval, TieBreaker);
		}
	}

	// Now perform the actual navigation
	NavigateToWidget(WidgetToFocus, bForGamepad, bForgeCursorMove);
}

void UILLUserWidget::ConditionalUpdateNavigationCache()
{
	if (bNavigationCacheInvalidated)
	{
		NavigableWidgetCache.Empty(NavigableWidgetCache.Num());
		RecursiveCollectNavigableWidgets(this, NavigableWidgetCache);
		bNavigationCacheInvalidated = false;
	}
}

UILLUserWidget::SNavigableWidgetItem UILLUserWidget::FindKeyboardFocusedNavigableWidget()
{
	TSharedPtr<SWidget> KeyboardFocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (KeyboardFocusedWidget.IsValid())
	{
		ConditionalUpdateNavigationCache();

		// Ensure the found widget is considered navigable
		for (const auto& WidgetEntry : NavigableWidgetCache)
		{
			if (WidgetEntry.SlateWidget == KeyboardFocusedWidget)
			{
				return WidgetEntry;
			}
		}

		// Fallback to whatever widget reports having it...
		// This is only done for the case of UEditableTextBox, which contains an SEditableTextBox which contains a SEditableText that is NOT a child you can query! Yay Slate!
		for (const auto& WidgetEntry : NavigableWidgetCache)
		{
			if (WidgetEntry.SlateWidget->HasKeyboardFocus())
			{
				return WidgetEntry;
			}
		}
	}

	return SNavigableWidgetItem();
}

void UILLUserWidget::NavigateToWidget(const SNavigableWidgetItem& WidgetToFocus, bool bForGamepad, const bool bForgeCursorMove)
{
	UE_LOG(LogILLWidget, VeryVerbose, TEXT("%s::NavigateToWidget: WidgetToFocus: %s"), *GetName(), WidgetToFocus.UMGWidget ? *WidgetToFocus.UMGWidget->GetFullName() : TEXT("NULL"));

	if (WidgetToFocus.SlateWidget.IsValid())
	{
		// Scroll this widget into view if it's in a scrollbox
		if (WidgetToFocus.WithinScrollBox)
		{
			WidgetToFocus.WithinScrollBox->ScrollWidgetIntoView(WidgetToFocus.UMGWidget);
		}

		if (bForgeCursorMove)
		{
			// Fake a navigation move over the widget
			FSlateApplication::Get().ContriveMouseNavigationTo(WidgetToFocus.Rect.GetCenter());
		}
		else if (bForGamepad)
		{
			// Gamepad input received, get rid of the cursor
			FSlateApplication::Get().SuppressMouseUntilMove();
		}

		// Assign keyboard focus to the widget as well
		if (bAllUserFocus)
			FSlateApplication::Get().SetAllUserFocus(WidgetToFocus.SlateWidget, EFocusCause::Navigation);
		else
			FSlateApplication::Get().SetKeyboardFocus(WidgetToFocus.SlateWidget, EFocusCause::Navigation);
	}
}

void UILLUserWidget::OnUsingGamepadChanged(bool bUsingGamepad)
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::OnUsingGamepadChanged: bUsingGamepad: %s"), *GetName(), bUsingGamepad ? TEXT("true") : TEXT("false"));

	if (bEnableNavigation && bIsNavigable && bUsingGamepad)
	{
		// Using a gamepad, make sure the cursor is hidden
		FSlateApplication::Get().SuppressMouseUntilMove();
	}

	// Update navigation cache if needed
	ConditionalUpdateNavigationCache();

	// Attempt to navigate to the first best widget
	ConditionalAttemptDefaultNavigation(bUsingGamepad);
}

void UILLUserWidget::InternalRecursiveCollectNavigableWidgets(UILLUserWidget* FromWidget, TNavigableWidgetList& OutNavigableWidgets, FName WithinWidgetNamed, UScrollBox* LastScrollBox)
{
	if (!FromWidget->GetIsEnabled())
	{
		return;
	}

	// FIXME: Find a better way to see if any of our parents are invisible and ignore these widgets
	if (LastScrollBox && !LastScrollBox->IsVisible())
	{
		return;
	}

	FromWidget->WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		if (Widget->IsA(UScrollBox::StaticClass()))
		{
			LastScrollBox = Cast<UScrollBox>(Widget);
		}
		// Check for one of our standard controls accepted for navigation
		else if (Widget->IsA(UButton::StaticClass()) || Widget->IsA(UCheckBox::StaticClass()) || Widget->IsA(USlider::StaticClass()) || Widget->IsA(UEditableText::StaticClass()) || Widget->IsA(UEditableTextBox::StaticClass()))
		{
			if (!WithinWidgetNamed.IsNone() && Widget->GetFName() != WithinWidgetNamed)
			{
				// Do not add if our name doesn't match directly
				return;
			}

			TSharedPtr<SWidget> CachedWidget = Widget->GetCachedWidget();
			if (CachedWidget.IsValid() && CachedWidget->IsEnabled() && CachedWidget->SupportsKeyboardFocus())
			{
				const EVisibility& LastPaintVisibility = CachedWidget->GetLastPaintVisibility();
				if (LastPaintVisibility.IsHitTestVisible())
				{
					SNavigableWidgetItem Entry;
					Entry.UMGWidget = Widget;
					Entry.SlateWidget = CachedWidget;
					Entry.WithinScrollBox = LastScrollBox;
					Entry.Geometry = CachedWidget->GetCachedGeometry();
					Entry.Rect = BoundingRectForGeometry(Entry.Geometry);
					Entry.bAllowedForGamepad = FromWidget->bAllowGamepadNavigation;
					OutNavigableWidgets.Add(Entry);
				}
			}
		}
		else if (UILLUserWidget* UserWidget = Cast<UILLUserWidget>(Widget))
		{
			if (!WithinWidgetNamed.IsNone() && Widget->GetFName() == WithinWidgetNamed)
			{
				// Clear the WithinWidgetNamed filter for subsequent recursions, since they all technically match
				InternalRecursiveCollectNavigableWidgets(UserWidget, OutNavigableWidgets, FName(), LastScrollBox);
			}
			else
			{
				// Dive into this child user widget
				InternalRecursiveCollectNavigableWidgets(UserWidget, OutNavigableWidgets, WithinWidgetNamed, LastScrollBox);
			}
		}
	});
}

void UILLUserWidget::RecursiveCollectNavigableWidgets(UILLUserWidget* FromWidget, TNavigableWidgetList& OutNavigableWidgets, FName WithinWidgetNamed/* = FName()*/)
{
	InternalRecursiveCollectNavigableWidgets(FromWidget, OutNavigableWidgets, WithinWidgetNamed, nullptr);
}

void UILLUserWidget::SyncKeyboardWithMouseFocus()
{
	// Skip when there is no cursor
	if (!FSlateApplication::Get().GetPlatformCursor().IsValid()
	|| FSlateApplication::Get().GetPlatformCursor()->GetType() == EMouseCursor::None)
	{
		return;
	}

	// Update NavigableWidgetCache
	ConditionalUpdateNavigationCache();

	// Look for a widget under the cursor
	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	for (const auto& WidgetEntry : NavigableWidgetCache)
	{
		if (WidgetEntry.Rect.ContainsPoint(CursorPosition))
//		if (WidgetEntry.Geometry.IsUnderLocation(CursorPosition)) pjackson: Above line is a likely fix for the offset navigation issue
		{
			// Only give keyboard focus when the widget reports it doesn't have it
			// This saves on hammering focus obviously, but also fixes editable text not being selectable with the mouse and the caret frequently being forced to the end of the line
			if (!WidgetEntry.SlateWidget->HasKeyboardFocus())
			{
				// Assign keyboard focus to it
				if (bAllUserFocus)
					FSlateApplication::Get().SetAllUserFocus(WidgetEntry.SlateWidget, EFocusCause::Navigation);
				else
					FSlateApplication::Get().SetKeyboardFocus(WidgetEntry.SlateWidget, EFocusCause::Navigation);
			}
			break;
		}
	}
}

template<typename TDistanceFunc, typename TTieBreakFunc>
UILLUserWidget::SNavigableWidgetItem UILLUserWidget::ClosestWidgetEvaluator(const bool bForGamepad, const float MinimumDistance, TDistanceFunc DistanceFunc, TTieBreakFunc TieBreakFunc)
{
	float BestDistance = FLT_MAX;
	float TieEval = FLT_MAX;
	SNavigableWidgetItem Result;
	for (const SNavigableWidgetItem& WidgetEntry : NavigableWidgetCache)
	{
		// Ignore off-screen widgets
		if((WidgetEntry.Rect.Right < CachedWidgetRect.Left && WidgetEntry.Rect.Left < CachedWidgetRect.Left) // Off-screen left
		|| (WidgetEntry.Rect.Left > CachedWidgetRect.Right && WidgetEntry.Rect.Right > CachedWidgetRect.Right) // Off-screen right
		|| (WidgetEntry.Rect.Bottom < CachedWidgetRect.Top && WidgetEntry.Rect.Top < CachedWidgetRect.Top) // Off-screen top
		|| (WidgetEntry.Rect.Top > CachedWidgetRect.Bottom && WidgetEntry.Rect.Bottom > CachedWidgetRect.Bottom)) // Off-screen bottom
		{
			continue;
		}

		// Check if navigation is allowed
		if (bForGamepad && !WidgetEntry.bAllowedForGamepad)
		{
			continue;
		}

		// Determine the distance to the widget and compare it against the best
		const float WidgetDistance = DistanceFunc(WidgetEntry);
		if (WidgetDistance > MinimumDistance)
		{
			if (BestDistance < NAVIGATION_EPSILON && WidgetDistance < NAVIGATION_EPSILON)
			{
				// Both have a negative distance, always perform a tie break on that
				if (TieBreakFunc(WidgetEntry) < TieEval)
				{
					BestDistance = WidgetDistance;
					TieEval = TieBreakFunc(WidgetEntry);
					Result = WidgetEntry;
				}
			}
			else if (BestDistance > WidgetDistance)
			{
				BestDistance = WidgetDistance;
				TieEval = TieBreakFunc(WidgetEntry);
				Result = WidgetEntry;
			}
			else if (BestDistance+NAVIGATION_EPSILON > WidgetDistance)
			{
				// Baseline comparison very close, tie break
				if (TieBreakFunc(WidgetEntry) < TieEval)
				{
					BestDistance = WidgetDistance;
					TieEval = TieBreakFunc(WidgetEntry);
					Result = WidgetEntry;
				}
			}
		}
	}

	return Result;
}

void UILLUserWidget::EmitButtonTip(const FText& TipText)
{
	RecursiveBroadcastButtonTip(this, TipText);
}

void UILLUserWidget::RecursiveBroadcastButtonTip(UILLUserWidget* FromWidget, const FText& TipText)
{
	if (FromWidget->CanSafelyRouteEvent())
	{
		// Trigger the event locally
		FromWidget->OnButtonTipEmitted(TipText);

		// Recurse
		if (FromWidget->WidgetTree)
		{
			FromWidget->WidgetTree->ForEachWidget([&](UWidget* Widget)
			{
				if (UILLUserWidget* UserWidget = Cast<UILLUserWidget>(Widget))
				{
					RecursiveBroadcastButtonTip(UserWidget, TipText);
				}
			});
		}
	}
}

void UILLUserWidget::EmitButtonTipLeft(const FText& TipText) // CLEANUP: pjackson: Fucking no. Stop touching IGF if you are going to do this.
{
	RecursiveBroadcastButtonTipLeft(this, TipText);
}

void UILLUserWidget::RecursiveBroadcastButtonTipLeft(UILLUserWidget* FromWidget, const FText& TipText)
{
	if (FromWidget->CanSafelyRouteEvent())
	{
		// Trigger the event locally
		FromWidget->OnButtonTipLeftEmitted(TipText);

		// Recurse
		if (FromWidget->WidgetTree)
		{
			FromWidget->WidgetTree->ForEachWidget([&](UWidget* Widget)
			{
				if (UILLUserWidget* UserWidget = Cast<UILLUserWidget>(Widget))
				{
					RecursiveBroadcastButtonTipLeft(UserWidget, TipText);
				}
			});
		}
	}
}

void UILLUserWidget::OnInputAxis(float AxisValue, FILLOnInputAxis Callback)
{
	if (GetIsEnabled())
	{
		Callback.ExecuteIfBound(AxisValue);
	}
}

void UILLUserWidget::ListenForInputAxis(FName ActionName, bool bConsume, FILLOnInputAxis Callback)
{
	if (!InputComponent)
	{
		InitializeInputComponent();
	}

	if (InputComponent)
	{
		FInputAxisBinding NewBinding(ActionName);
		NewBinding.bConsumeInput = bConsume;
		NewBinding.AxisDelegate.GetDelegateForManualSet().BindUObject(this, &ThisClass::OnInputAxis, Callback);

		InputComponent->AxisBindings.Add(NewBinding);
	}
}

UObject* UILLUserWidget::DoStaticLoadObj(TSoftObjectPtr<UObject> Asset) {
	FSoftObjectPath SoftObjectPath = Asset.ToSoftObjectPath();
	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> Handle;

	Handle = StreamableManager.RequestSyncLoad(SoftObjectPath);

	return SoftObjectPath.ResolveObject();


}
