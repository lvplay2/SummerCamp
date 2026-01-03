// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Framework/Application/IInputProcessor.h"
#include "ILLUserWidget.generated.h"

class UScrollBox;

class AILLHUD;
class AILLPlayerController;
class UILLWidgetComponent;

ILLGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogILLWidget, Log, All);

/** @return Screen-space Slate rect for WidgetGeometry. */
FORCEINLINE FSlateRect BoundingRectForGeometry(const FGeometry& WidgetGeometry)
{
	return TransformRect(
		Concatenate(
			Inverse(WidgetGeometry.GetAccumulatedLayoutTransform()),
			WidgetGeometry.GetAccumulatedRenderTransform()
		),
		FSlateRotatedRect(WidgetGeometry.GetLayoutBoundingRect())
	).ToBoundingRect();
}

/**
* @class FILLNULLSlateViewportInputProcessor
*/
class FILLNULLSlateViewportInputProcessor
	: public IInputProcessor
{
public:
	FILLNULLSlateViewportInputProcessor() {}
	virtual ~FILLNULLSlateViewportInputProcessor() {}

	// Begin IInputProcessor interface
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) { return true; }
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) { return true; }
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) { return true; }
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) { return true; }
	// End IInputProcessor interface
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FILLOnInputAxis, float, AxisValue);

/**
 * @class UILLUserWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLUserWidget
: public UUserWidget
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual UWorld* GetWorld() const override;
	// End UObject interface

	// Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	virtual void RemoveFromParent() override;
	// End UWidget Interface

	// Begin UUserWidget interface
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void OnInputAction(FOnInputAction Callback) override;
	// End UUserWidget interface

	// Disallow this widget from calling Super::GetWorld? Useful for banning widgets from World use (loading screens)
	bool bWorldForbidden;

protected:
	/**
	 * Callback delegate for when a property on this widget changes during design time, allowing custom button text etc. to update.
	 * NOTE: Be CAREFUL with this. If you change a property on a widget using this event, then it will be the default the next time the widget is saved.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface", meta=(BlueprintProtected))
	void OnSyncDesignTime();

	/** Called locally then broadcasted to all child ILLUserWidgets after RemoveFromParent is called. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface", meta=(BlueprintProtected))
	void OnRemovedFromParentBroadcast();

	/** Calls OnRemovedFromParentBroadcast on FromWidget then on any UILLUserWidget in it's WidgetTree, recursively. */
	void RecursiveBroadcastRemoval(UILLUserWidget* FromWidget);

	//////////////////////////////////////////////////
	// Additional optional contexts
public:
	/** @return View target pawn or the owning player pawn. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Player")
	APawn* GetOwningViewTargetPawn() const;

	/** cbrungardt TODO - need to move this into an MHUserWidget or something. This fixes a crash though.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "User Interface", meta = (BlueprintProtected))
	void OnLoadingScreenDetails(const FText& MapName, UTexture2D* MapImage, UTexture2D* TopDownMap, const FText& GameMode, const FText& DiffficultyMode, bool IsSurvivalMode);

	/** @return OwningPlayer casted up to ILLPlayerController. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	AILLPlayerController* GetOwningILLPlayer() const;

	/** @return ILLHUD instance for our OwningPlayer. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	AILLHUD* GetOwningPlayerILLHUD() const;

	/** Called from the ILLWidgetComponent when the local player is in range of the component. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "WidgetComponent")
	void OnLocalCharacterInRange();

	/** Called from the ILLWidgetComponent when the local player is out of range of the component. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "WidgetComponent")
	void OnLocalCharacterOutOfRange();

	// Actor this widget is associated with
	UPROPERTY(BlueprintReadOnly, Category="User Interface")
	AActor* ActorContext;

	// Widget component this is associated with
	UPROPERTY(BlueprintReadOnly, Category="User Interface")
	UILLWidgetComponent* ComponentContext;
	
	//////////////////////////////////////////////////
	// Transitions
public:
	/**
	 * Sets up input blocking when bBlockInputForTransitionOut is true, then triggers OnPlayTransitionOut so UMG can play a transition-out animation.
	 * NOTE: Must call to OnTransitionOutComplete when animation completes or end up in a stuck state!
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Transition")
	void PlayTransitionOut(TSubclassOf<UILLUserWidget> TransitioningTo = nullptr);

	/**
	 * Transition completion delegate for this widget.
	 * Will not fire if OnTransitionCompleteOverride is bound, only firing for the stock behavior in OnTransitionOutComplete.
	 *
	 * @param Widget Widget that just finished it's transition out.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FTransitionCompletionDelegate, UILLUserWidget* /*Widget*/);
	FTransitionCompletionDelegate OnTransitionComplete;

	/**
	 * Transition completion override delegate for this widget.
	 * Can be used to procedurally latch on and override the cleanup behavior of a widget.
	 *
	 * @param Widget Widget that just finished it's transition out.
	 */
	DECLARE_DELEGATE_OneParam(FTransitionCompletionOverrideDelegate, UILLUserWidget* /*Widget*/);
	FTransitionCompletionOverrideDelegate OnTransitionCompleteOverride;

	/**
	 * Delegate fired when this widget has RemoveFromParent called.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FRemovedFromParentDelegate, UILLUserWidget* /*Widget*/);
	FRemovedFromParentDelegate OnRemovedFromParent;

	/** @return true if we are transitioning out */
	bool IsTransitioningOut() const { return bTransitioningOut; }

protected:
	/**
	 * Called by PlayTransitionOut to play a transition-out animation then call to OnTransitionOutComplete.
	 * Default native is to trigger the OnTransitionOutComplete, so do not call this if you want to postpone that.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Transition", meta=(BlueprintProtected))
	void OnPlayTransitionOut(TSubclassOf<UILLUserWidget> TransitioningTo);

	/** Broadcasted to all child ILLUserWidgets when PlayTransitionOut is called. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface", meta=(BlueprintProtected))
	void OnParentTransitioningOut();

	/** Calls OnRemovedFromParentBroadcast on FromWidget then on any UILLUserWidget in it's WidgetTree, recursively. */
	void RecursiveBroadcastTransitionOut(UILLUserWidget* FromWidget);

	/** Call to this from the widget when the PlayTransitionOut animation completes. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Transition", meta=(BlueprintProtected))
	void OnTransitionOutComplete();

	// Should ALL Slate input be suppressed until OnTransitionOutComplete is hit?
	UPROPERTY(EditDefaultsOnly, Category="Transition")
	bool bBlockInputForTransitionOut;

	// Slate input preprocessor for BlockInputForTransitionOut
	TSharedPtr<class FILLNULLSlateViewportInputProcessor> SilentSlatePreprocessor;

	// Transition-out target
	TSubclassOf<UILLUserWidget> TransitionTarget;

	// Are we transitioning out?
	bool bTransitioningOut;

	//////////////////////////////////////////////////
	// Menu support
public:
	/** @return true if popping this menu should be disallowed. */
	UFUNCTION(BlueprintImplementableEvent, Category="Menu")
	bool ShouldDisallowMenuPop() const;

	/** @return true if the axis input event was handled. */
	virtual bool MenuInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad);

	/** @return true if the key input event was handled. */
	virtual bool MenuInputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad);

	/**
	 * Looks for then navigates to WidgetName.
	 * @return true if WidgetName was found and navigated to.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	virtual bool NavigateToNamedWidget(FName WidgetName);

	// When navigating, give focus to all users? Otherwise only the "keyboard index" gets focus. Useful for title screen or anywhere that any gamepad can be used.
	UPROPERTY(EditAnywhere, Category="Menu")
	bool bAllUserFocus;

	// Z-Order to use when this is spawned in the menu stack
	UPROPERTY(EditDefaultsOnly, Category="Menu|Stack Component")
	int32 MenuZOrder;

	// Allow this same menu class to stack on top of other instances of itself?
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Menu|Stack Component")
	bool bAllowMenuStacking;

	// When opened through ILLMenuStackHUDComponent, should this menu replace any other opened menus? Turn this off to act as a popup, overlaying previous menus
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Menu|Stack Component")
	bool bTopMostMenu;

	// Enable navigation updates? Flag this true on bIsNavigable menus
	UPROPERTY(Transient)
	bool bEnableNavigation;

	// Used by ILLMenuStackHUDComponent to hook in our desired initial navigation
	FName RequestedDefaultNavigation;

protected:
	/**
	 * @struct SNavigableWidgetItem
	 */
	struct SNavigableWidgetItem
	{
		// UMG widget that contains the focusable SlateWidget
		UWidget* UMGWidget;

		// Keyboard-focusable Slate widget within an ILLUserWidget
		TSharedPtr<SWidget> SlateWidget;

		// What scrollbox this is in
		UScrollBox* WithinScrollBox;

		// Geometry of the Slate widget
		FGeometry Geometry;

		// Cached Slate rect for Geometry
		FSlateRect Rect;

		// Allowed for gamepad navigation?
		bool bAllowedForGamepad;

		SNavigableWidgetItem()
		: UMGWidget(nullptr)
		, WithinScrollBox(nullptr) {}
	};

	typedef TArray<SNavigableWidgetItem> TNavigableWidgetList;
	
	/** Calls to AttemptDefaultNavigation when we have no keyboard focused widget. */
	virtual void ConditionalAttemptDefaultNavigation(const bool bGamepadCheck);

	/** Attempt the default fallback navigation, from the top-left corner of the screen. */
	virtual void AttemptDefaultNavigation();

	/** Attempt to navigate in the NavigationType direction. */
	virtual void AttemptNavigation(const EUINavigation NavigationType, const bool bForGamepad, const bool bForgeCursorMove = false);
	virtual void AttemptNavigation(const EUINavigation NavigationType, FSlateRect SourceRect, bool bForGamepad, const bool bForgeCursorMove = false);

	/** Updates NavigableWidgetCache when bNavigationCacheInvalidated. */
	virtual void ConditionalUpdateNavigationCache();

	/** @return Visible navigable widget that has keyboard focus. */
	SNavigableWidgetItem FindKeyboardFocusedNavigableWidget();

	/** Performs actual pseudo-navigation: hides the mouse cursor and contrives a cursor move over the WidgetToFocus, as well as assigning keyboard focus to it. */
	virtual void NavigateToWidget(const SNavigableWidgetItem& WidgetToFocus, bool bForGamepad, const bool bForgeCursorMove);

	/** Callback for ILLPlayerInput::OnUsingGamepadChanged. */
	UFUNCTION()
	virtual void OnUsingGamepadChanged(bool bUsingGamepad);

	/** Collects a list of visible navigable widgets and puts them into OutNavigableWidgets. */
	virtual void InternalRecursiveCollectNavigableWidgets(UILLUserWidget* FromWidget, TNavigableWidgetList& OutNavigableWidgets, FName WithinWidgetNamed, UScrollBox* LastScrollBox);
	virtual void RecursiveCollectNavigableWidgets(UILLUserWidget* FromWidget, TNavigableWidgetList& OutNavigableWidgets, FName WithinWidgetNamed = FName());

	/** Forces keyboard focus to the widget under the cursor. */
	virtual void SyncKeyboardWithMouseFocus();

	// Process navigation when this menu receives input?
	UPROPERTY(EditDefaultsOnly, Category="Menu")
	bool bIsNavigable;

	// Allow the gamepad to navigate to this widget?
	UPROPERTY(EditDefaultsOnly, Category="Menu")
	bool bAllowGamepadNavigation;

	// Name of the default widget to focus when the menu first opens and the mouse cursor is not over anything focusable, or keyboard focus is lost or set to something invalid
	UPROPERTY(EditDefaultsOnly, Category="Menu")
	FName DefaultFocusWidgetName;

	// How long to defer navigation for after this widget becomes navigable and in the foreground etc
	UPROPERTY(EditDefaultsOnly, Category="Menu")
	float DeferNavigationTime;

	// Last NativeTick FSlateRect for MyGeometry
	FSlateRect CachedWidgetRect;

	// Navigable widget cache, updated when bNavigationCacheInvalidated is flagged
	TNavigableWidgetList NavigableWidgetCache;

	// Input axis deltas from the last MenuInputAxis call
	FVector2D LastAxisDeltas;

	// Local foreground frame time
	float ForegroundFrameTime;

	// Update NavigableWidgetCache in the next ConditionalUpdateNavigationCache hit?
	bool bNavigationCacheInvalidated;

private:
	/**
	 * Helper specifically for AttemptNavigation. Widget distance evaluator for navigation.
	 * Iterates over the NavigableWidgetCache, calling DistanceFunc on each entry to calculate the widget distance.
	 * When two widgets have the same distance, TieBreakFunc is called to see if the new best widget should be taken over the current.
	 *
	 * @param bForGamepad Is this for gamepad navigation?
	 * @param MinimumDistance 
	 * @return Closest widget found in the list.
	 */
	template<typename TDistanceFunc, typename TTieBreakFunc>
	SNavigableWidgetItem ClosestWidgetEvaluator(const bool bForGamepad, const float MinimumDistance, TDistanceFunc DistanceFunc, TTieBreakFunc TieBreakFunc);

	//////////////////////////////////////////////////
	// Custom button tooltip support
public:
	/** Broadcasts OnButtonTipEmitted locally and on all ILLUserWidget children. */
	UFUNCTION(BlueprintCallable, Category="ButtonTip")
	void EmitButtonTip(const FText& TipText);

	/** Broadcasts OnButtonTipEmitted locally and on all ILLUserWidget children. */
	UFUNCTION(BlueprintCallable, Category = "ButtonTip")
	void EmitButtonTipLeft(const FText& TipText);

protected:
	/** Event handler for a custom tip change. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="ButtonTip", meta=(BlueprintProtected))
	void OnButtonTipEmitted(const FText& TipText);

	/** Event handler for a custom tip change. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "ButtonTip", meta = (BlueprintProtected))
	void OnButtonTipLeftEmitted(const FText& TipText);

	/** Helper function which calls OnButtonTipEmitted on FromWidget and all of it's ILLUserWidget children with TipText. */
	void RecursiveBroadcastButtonTip(UILLUserWidget* FromWidget, const FText& TipText);

	/** Helper function which calls OnButtonTipEmitted on FromWidget and all of it's ILLUserWidget children with TipText. */
	void RecursiveBroadcastButtonTipLeft(UILLUserWidget* FromWidget, const FText& TipText);

	//////////////////////////////////////////////////
	// Input Axis Listening
	virtual void OnInputAxis(float AxisValue, FILLOnInputAxis Callback);

	UFUNCTION(BlueprintCallable, Category="Input", meta=(BlueprintProtected="true"))
	void ListenForInputAxis(FName ActionName, bool bConsume, FILLOnInputAxis Callback);

public:
	UFUNCTION(BlueprintCallable, Category = "Additions")
	UObject* DoStaticLoadObj(TSoftObjectPtr<UObject> Asset);
};
