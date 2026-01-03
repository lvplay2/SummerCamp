// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/ActorComponent.h"
#include "ILLUserWidget.h"
#include "ILLMenuStackHUDComponent.generated.h"

class AILLPlayerController;

class UInputComponent;

/**
 * @struct FILLMenuStackElement
 */
USTRUCT()
struct FILLMenuStackElement
{
	GENERATED_USTRUCT_BODY()

	// Class to spawn the menu
	UPROPERTY()
	TSubclassOf<UILLUserWidget> Class;

	// Instance of Class
	UPROPERTY()
	UILLUserWidget* Instance;

	// Input component for the menu
	UPROPERTY(Transient)
	UInputComponent* InputComponent;

	// Focus to assign this widget when respawned
	FName PendingFocus;

	FILLMenuStackElement()
	: Class(nullptr)
	, Instance(nullptr)
	, InputComponent(nullptr)
	{
	}

	FILLMenuStackElement(TSubclassOf<UILLUserWidget> InClass, UILLUserWidget* InInstance, UInputComponent* InInputComponent)
	: Class(InClass), Instance(InInstance), InputComponent(InInputComponent)
	{
	}
};

/**
 * @class UILLMenuStackHUDComponent
 * @brief Wraps common menu stack behavior so that SCHUD and SCHUD_Lobby can use the same functionality for this.
 */
UCLASS(Within=ILLHUD, ClassGroup="UI", meta=(BlueprintSpawnableComponent))
class ILLGAMEFRAMEWORK_API UILLMenuStackHUDComponent
: public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** @return Owning player controller. */
	AILLPlayerController* GetPlayerOwner() const;

	//////////////////////////////////////////////////////////////////////////
	// Menu stack
public:
	/**
	 * Flushes the menu stack before calling PushMenu with MenuClass, making it the new root menu. Supports a NULL MenuClass as an easy way to close everything.
	 * @return Menu instance created by PushMenu.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu", meta=(AdvancedDisplay="bForce"))
	virtual UILLUserWidget* ChangeRootMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bForce = false);

	/**
	 * Spawns MenuClass as the new top-most menu.
	 *
	 * @param MenuClass Class of the menu to spawn.
	 * @param bSkipTransitionOut Skip PlayTransitionOut and straight to RemoveFromParent on previous menus?
	 * @return Instance of MenuClass after being pushed onto the stack.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu", meta=(AdvancedDisplay="bSkipTransitionOut"))
	virtual UILLUserWidget* PushMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bSkipTransitionOut = false);

	/**
	 * Closes the last pushed menu, spawning each previous menu until the first bTopMostMenu is hit.
	 *
	 * @param bForce Pop the menu even if ILLUserWidget::ShouldDisallowMenuPop returns true.
	 * @return true if the top-most menu was popped.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu", meta=(AdvancedDisplay="bForce,bSkipTransitionOut"))
	virtual bool PopMenu(FName WidgetToFocus = NAME_None, const bool bForce = false);

	/**
	 * Removes MenuInstance from anywhere in the menu stack.
	 *
	 * @param MenuInstance Menu instance to remove.
	 * @return true if found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	virtual bool RemoveMenu(UILLUserWidget* MenuInstance);

	/**
	 * Removes the first MenuClass found on the menu stack.
	 * Does not care if the menu element instance exists (in other words, does not care if it is open right now).
	 *
	 * @param MenuClass Menu class to remove.
	 * @return true if found and removbed.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	virtual bool RemoveMenuByClass(TSubclassOf<UILLUserWidget> MenuClass);

	/** @return Top-most open menu instance. */
	UFUNCTION(BlueprintCallable, Category="Menu")
	FORCEINLINE UILLUserWidget* GetCurrentMenu() const { return (MenuStackElements.Num() > 0) ? MenuStackElements.Last().Instance : nullptr; }

	/**
	 * Searches from the last menu opened towards the first for a menu that IsA MenuClass.
	 * @param MenuClass Class of the menu to search for.
	 * @return Last open menu instance of class MenuClass.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	UILLUserWidget* GetOpenMenuByClass(TSubclassOf<UILLUserWidget> MenuClass) const;

	/**
	 * Returns the class type of the previous menu in the list.
	 * @Rrturn Class type for the previous menu in the stack, nullptr if there is no previous menu.
	 */
	UFUNCTION(BlueprintCallable, Category="Menu")
	TSubclassOf<UILLUserWidget> GetPreviousMenuClass() const;

	/** Return the current Menu stack elements. */
	FORCEINLINE const TArray<FILLMenuStackElement>& GetFullStack() const { return MenuStackElements; }

	/** Override the menu stack and push our new elements on. */
	FORCEINLINE void OverrideStackElements(const TArray<FILLMenuStackElement>& InOverrideElements) { MenuStackElements = InOverrideElements; }

	/** Destroys the spawned MenuStackElements widget instances, but preserves the MenuStackElements structure for rebuild with ReSpawnMenuStack. */
	void DestroyMenuStack();

	/** Re-spawns missing elements in MenuStackElements, and re-assigns navigation. */
	void ReSpawnMenuStack();

protected:
	/** Helper function to destroy an element instance and remove it's input component. */
	void CloseElement(FILLMenuStackElement& Element, const bool bSkipTransitionOut);

	/** Callback for when CloseElement finishes transitioning out an element. */
	void OnMenuRemovedFromParent(UILLUserWidget* MenuWidget);

	/** Flags bEnableNavigation off on all widgets, then turns it onto the highest z-order and/or menu stack order element. */
	virtual void ReassignNavigation();

	/** Helper function for RemoveMenu and RemoveMenuByClass. */
	bool RemoveElementAt(const int32 Index);

	/** Helper for PushMenu and PopMenu to create and initialize MenuClass. */
	UILLUserWidget* SpawnMenu(TSubclassOf<UILLUserWidget> MenuClass);

	// Opened menu classes
	UPROPERTY(Transient)
	TArray<FILLMenuStackElement> MenuStackElements;

	// Is navigation disabled?
	bool bNavigationDisabled;

	//////////////////////////////////////////////////////////////////////////
	// Input handling
public:
	/**
	 * Sends the axis event along ot the top-most menu stack element.
	 *
	 * @return true if the input event was handled by said menu, or false if there are no menus.
	 */
	virtual bool MenuStackInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad);

	/**
	 * Sends the key event along ot the top-most menu stack element.
	 *
	 * @return true if the input event was handled by said menu, or false if there are no menus.
	 */
	virtual bool MenuStackInputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad);

protected:
	/** @return New input component instance to push before every menu. */
	virtual UInputComponent* CreateMenuInputComponent(const UILLUserWidget* Widget = nullptr);
};
