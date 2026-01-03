// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLHUD.h"
#include "ILLMenuStackHUDComponent.h"

#include "ILLPlayerController.h"

UILLMenuStackHUDComponent::UILLMenuStackHUDComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

AILLPlayerController* UILLMenuStackHUDComponent::GetPlayerOwner() const
{
	if (AILLHUD* HUD = Cast<AILLHUD>(GetOwner()))
	{
		return Cast<AILLPlayerController>(HUD->PlayerOwner);
	}

	return nullptr;
}

UILLUserWidget* UILLMenuStackHUDComponent::ChangeRootMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bForce/* = false*/)
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::ChangeRootMenu: MenuClass: %s, bForce: %s"), *GetName(), MenuClass ? *MenuClass->GetFullName() : TEXT("NULL"), bForce ? TEXT("true") : TEXT("false"));

	DestroyMenuStack();

	// Only push MenuClass if we have one
	if (MenuClass)
	{
		// Now push the new one
		return PushMenu(MenuClass);
	}

	return nullptr;
}

UILLUserWidget* UILLMenuStackHUDComponent::PushMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bSkipTransitionOut/* = false*/)
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::PushMenu: MenuClass: %s, bSkipTransitionOut: %s"), *GetName(), MenuClass ? *MenuClass->GetFullName() : TEXT("NULL"), bSkipTransitionOut ? TEXT("true") : TEXT("false"));

	UILLUserWidget* NewMenuInstance = nullptr;
	if (ensure(MenuClass))
	{
		const UILLUserWidget* const DefaultMenuInstance = MenuClass->GetDefaultObject<UILLUserWidget>();

		// Menu stack performance checking
		if (!DefaultMenuInstance->bAllowMenuStacking)
		{
			auto BuildMenuStackString = [&]() -> FString
			{
				FString Result;
				for (const FILLMenuStackElement& Menu : MenuStackElements)
				{
					if (!Result.IsEmpty())
						Result += TEXT(", ");
					Result += Menu.Class->GetName();
				}
				return Result;
			};
			if (!ensureAlwaysMsgf(MenuStackElements.Num() == 0 || MenuStackElements.Last().Class != MenuClass, TEXT("Attempted to push same menu twice: %s stack [%s]"), *MenuClass->GetName(), *BuildMenuStackString()))
				return nullptr;
		}

		// Before any menu element is pushed, push the MenuInputComponent
		// This serves to prevent input actions from falling through to the menu or game viewport behind
		APlayerController* PlayerOwner = GetPlayerOwner();
		UInputComponent* InputComponent = CreateMenuInputComponent(DefaultMenuInstance);
		if (ensure(PlayerOwner))
		{
			PlayerOwner->PushInputComponent(InputComponent);
		}

		// Spawn MenuClass
		NewMenuInstance = SpawnMenu(MenuClass);

		// Check to see if this new menu should be placed behind a menu with a higher menu z order
		FILLMenuStackElement NewMenu(MenuClass, NewMenuInstance, InputComponent);
		if (MenuStackElements.Num() && DefaultMenuInstance->MenuZOrder < MenuStackElements.Last().Class->GetDefaultObject<UILLUserWidget>()->MenuZOrder)
		{
			int32 InsertIndex = MenuStackElements.Num() - 1;
			for (; InsertIndex > 0; --InsertIndex)
			{
				// Check the menu under our current index to see if we should insert at this location
				if (DefaultMenuInstance->MenuZOrder < MenuStackElements[InsertIndex - 1].Class->GetDefaultObject<UILLUserWidget>()->MenuZOrder)
					continue;

				break;
			}

			// We need to Make sure the menu we're inserting after is closed properly
			if (DefaultMenuInstance->bTopMostMenu && MenuStackElements[InsertIndex - 1].Instance)
			{
				// Skip the transition since we're pushing a new menu on immediately.
				CloseElement(MenuStackElements[InsertIndex - 1], true);
			}

			// Insert the menu at the proper index
			MenuStackElements.Insert(NewMenu, InsertIndex);	
		}
		else
		{
			if (DefaultMenuInstance->bTopMostMenu)
			{
				// Destroy the previous menu instance(s)
				for (FILLMenuStackElement& Menu : MenuStackElements)
				{
					CloseElement(Menu, bSkipTransitionOut);
				}
			}

			// Add to the stack
			MenuStackElements.Push(NewMenu);
		}

		// Figure out navigation
		ReassignNavigation();
	}

	return NewMenuInstance;
}

bool UILLMenuStackHUDComponent::PopMenu(FName WidgetToFocus/* = NAME_None*/, const bool bForce/* = false*/)
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::PopMenu: WidgetToFocus: %s, bForce: %s"), *GetName(), *WidgetToFocus.ToString(), bForce ? TEXT("true") : TEXT("false"));

	if (MenuStackElements.Num() == 0)
	{
		return false;
	}

	// Check if the widget is allowing the menu to be popped right now
	if (!bForce && MenuStackElements.Last().Instance && MenuStackElements.Last().Instance->ShouldDisallowMenuPop())
	{
		return false;
	}

	// Pop the current menu off the stack
	FILLMenuStackElement Element = [&]() -> FILLMenuStackElement
	{
		for (int i = MenuStackElements.Num() - 1; i >= 0; --i)
		{
			// We should close the menu that is transitioning out first, even if it's not on the top of the stack, just in case a menu was pushed while we were transitioning out (e.g. Error dialog).
			if (MenuStackElements[i].Instance && MenuStackElements[i].Instance->IsTransitioningOut())
			{
				FILLMenuStackElement Menu = MenuStackElements[i];
				MenuStackElements.RemoveAt(i);
				return Menu;
			}
		}

		return MenuStackElements.Pop();
	}();

	// Store off the widget to focus on the menu below
	if (MenuStackElements.Num())
	{
		MenuStackElements.Last().PendingFocus = WidgetToFocus;
	}

	if (Element.Instance)
	{
		// Now transition the element out
		Element.Instance->OnRemovedFromParent.AddUObject(this, &ThisClass::OnMenuRemovedFromParent);
		CloseElement(Element, true);
	}
	else
	{
		OnMenuRemovedFromParent(nullptr);
	}

	UE_LOG(LogILLWidget, Verbose, TEXT("... popped %s"), Element.Class ? *Element.Class->GetFullName() : TEXT("NULL"));

	return true;
}

bool UILLMenuStackHUDComponent::RemoveMenu(UILLUserWidget* MenuInstance)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < MenuStackElements.Num(); ++Index)
	{
		if (MenuStackElements[Index].Instance == MenuInstance)
		{
			FoundIndex = Index;
			break;
		}
	}
	return RemoveElementAt(FoundIndex);
}

bool UILLMenuStackHUDComponent::RemoveMenuByClass(TSubclassOf<UILLUserWidget> MenuClass)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 Index = 0; Index < MenuStackElements.Num(); ++Index)
	{
		if (MenuStackElements[Index].Class == MenuClass)
		{
			FoundIndex = Index;
			break;
		}
	}
	return RemoveElementAt(FoundIndex);
}

UILLUserWidget* UILLMenuStackHUDComponent::GetOpenMenuByClass(TSubclassOf<UILLUserWidget> MenuClass) const
{
	UILLUserWidget* Result = nullptr;

	for (int32 Index = MenuStackElements.Num()-1; Index >= 0; --Index)
	{
		const FILLMenuStackElement& MenuElement = MenuStackElements[Index];
		if (MenuElement.Class == MenuClass)
		{
			Result = MenuElement.Instance;
			break;
		}
	}

	return Result;
}

TSubclassOf<UILLUserWidget> UILLMenuStackHUDComponent::GetPreviousMenuClass() const
{
	if (MenuStackElements.Num() > 1)
		return MenuStackElements[MenuStackElements.Num()-2].Class;

	return nullptr;
}

void UILLMenuStackHUDComponent::DestroyMenuStack()
{
	// Tear down the menu stack
	for (FILLMenuStackElement& Menu : MenuStackElements)
	{
		CloseElement(Menu, true);
	}

	MenuStackElements.Empty();
}

void UILLMenuStackHUDComponent::ReSpawnMenuStack()
{
	if (MenuStackElements.Num() > 0)
	{
		APlayerController* PlayerOwner = GetPlayerOwner();
		check(PlayerOwner);

		// Find the index at which we should start spawning menus
		int32 StartIndex = 0;
		for (int32 Index = MenuStackElements.Num()-1; Index >= 0; --Index)
		{
			FILLMenuStackElement& PreviousMenu = MenuStackElements[Index];
			StartIndex = Index;

			const UILLUserWidget* const DefaultInstance = PreviousMenu.Class->GetDefaultObject<UILLUserWidget>();
			if (DefaultInstance->bTopMostMenu)
			{
				break;
			}
		}

		for (int32 Index = StartIndex; Index < MenuStackElements.Num(); ++Index)
		{
			FILLMenuStackElement& PreviousMenu = MenuStackElements[Index];

			if (PreviousMenu.Instance)
			{
				// Re-enable (handle DisableNavigation)
				PreviousMenu.Instance->SetIsEnabled(true);
			}
			else
			{
				// Push the input component first, our input blanket
				PlayerOwner->PushInputComponent(PreviousMenu.InputComponent);

				// Create our instance
				PreviousMenu.Instance = SpawnMenu(PreviousMenu.Class);

				if (!PreviousMenu.PendingFocus.IsNone() && Index == MenuStackElements.Num()-1)
				{
					// Last instance spawned, focus the PendingFocus then "consume" it so it doesn't linger
					PreviousMenu.Instance->RequestedDefaultNavigation = PreviousMenu.PendingFocus;
					PreviousMenu.PendingFocus = NAME_None;
				}
			}
		}
	}

	ReassignNavigation();
}

void UILLMenuStackHUDComponent::CloseElement(FILLMenuStackElement& Element, const bool bSkipTransitionOut)
{
	if (!IsValid(Element.Instance))
	{
		return;
	}

	// Remove navigation and close the menu
	Element.Instance->bEnableNavigation = false;
	if (bSkipTransitionOut)
	{
		Element.Instance->RemoveFromParent();
	}
	else
	{
		Element.Instance->PlayTransitionOut();
	}
	Element.Instance = nullptr;

	if (APlayerController* PlayerOwner = GetPlayerOwner())
	{
		// Cleanup the input component
		PlayerOwner->PopInputComponent(Element.InputComponent);
	}
}

void UILLMenuStackHUDComponent::OnMenuRemovedFromParent(UILLUserWidget* MenuWidget)
{
	// Ensure the menu stack and navigation are correct
	ReSpawnMenuStack();
}

void UILLMenuStackHUDComponent::ReassignNavigation()
{
	// Take navigation away from previous menus
	for (FILLMenuStackElement& Menu : MenuStackElements)
	{
		if (Menu.Instance)
		{
			Menu.Instance->bEnableNavigation = false;
		}
	}

	// Re-evaluate who should have navigation based on z-order and stack order
	int32 BestZOrder = INT_MIN;
	UILLUserWidget* BestInstance = nullptr;
	for (FILLMenuStackElement& Menu : MenuStackElements)
	{
		if (Menu.Instance && Menu.Instance->MenuZOrder >= BestZOrder)
		{
			BestInstance = Menu.Instance;
			BestZOrder = BestInstance->MenuZOrder;
		}
	}

	// Now update navigation and focus based on what was found
	APlayerController* PlayerOwner = GetPlayerOwner();
	if (BestInstance)
	{
		BestInstance->bEnableNavigation = !bNavigationDisabled;

		// Send focus to the new menu
		if (PlayerOwner)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(BestInstance->TakeWidget());
			PlayerOwner->SetInputMode(InputMode);
			PlayerOwner->bShowMouseCursor = true;
		}
	}
	else
	{
		// Return focus to the game
		if (PlayerOwner)
		{
			FInputModeGameOnly InputMode;
			PlayerOwner->SetInputMode(InputMode);
			PlayerOwner->bShowMouseCursor = false;
		}
	}
}

bool UILLMenuStackHUDComponent::RemoveElementAt(const int32 Index)
{
	if (!MenuStackElements.IsValidIndex(Index))
	{
		return false;
	}

	// Handle the last element by simply popping
	if (Index == MenuStackElements.Num()-1)
	{
		return PopMenu();
	}

	// Copy the element off and remove it from the stack before calling CloseElement, to be consistent with PopMenu
	FILLMenuStackElement Element = MenuStackElements[Index];
	MenuStackElements.RemoveAt(Index);

	// Transition the element out
	if (Element.Instance)
		Element.Instance->OnRemovedFromParent.AddUObject(this, &ThisClass::OnMenuRemovedFromParent);
	CloseElement(Element, true);
	return true;
}

UILLUserWidget* UILLMenuStackHUDComponent::SpawnMenu(TSubclassOf<UILLUserWidget> MenuClass)
{
	UE_LOG(LogILLWidget, Verbose, TEXT("%s::SpawnMenu: MenuClass: %s"), *GetName(), MenuClass ? *MenuClass->GetFullName() : TEXT("NULL"));

	UILLUserWidget* NewMenuInstance = NewObject<UILLUserWidget>(this, MenuClass);
	NewMenuInstance->SetPlayerContext(GetPlayerOwner());
	NewMenuInstance->Initialize();
	NewMenuInstance->AddToViewport(NewMenuInstance->MenuZOrder);
	return NewMenuInstance;
}

bool UILLMenuStackHUDComponent::MenuStackInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	for (int32 Index = MenuStackElements.Num()-1; Index >= 0; --Index)
	{
		FILLMenuStackElement& MenuElement = MenuStackElements[Index];
		if (!MenuElement.Instance)
		{
			break;
		}

		// Top-most element always decides if we consume input
		return MenuElement.Instance->MenuInputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
	}

	return false;
}

bool UILLMenuStackHUDComponent::MenuStackInputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	for (int32 Index = MenuStackElements.Num()-1; Index >= 0; --Index)
	{
		FILLMenuStackElement& MenuElement = MenuStackElements[Index];
		if (!MenuElement.Instance)
		{
			break;
		}

		// Top-most element always decides if we consume input
		return MenuElement.Instance->MenuInputKey(Key, EventType, AmountDepressed, bGamepad);
	}

	return false;
}

UInputComponent* UILLMenuStackHUDComponent::CreateMenuInputComponent(const UILLUserWidget* Widget /*= nullptr*/)
{
	UInputComponent* InputComponent = NewObject<UInputComponent>(this, TEXT("ILLMenuStackInput"));
	InputComponent->RegisterComponent();
	InputComponent->bBlockInput = true;
	InputComponent->Priority = Widget ? Widget->Priority : 100;
	return InputComponent;
}
