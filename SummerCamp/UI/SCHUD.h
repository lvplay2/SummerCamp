// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLHUD.h"
#include "ILLMenuStackHUDComponent.h"
#include "SCHUD.generated.h"

class USCMenuWidget;

/**
 * @class ASCHUD
 * @brief Base class for F13 specific HUD actors (excluding the Lobby which uses IGF). If you're confused open SCInGameHUD.h.
 */
UCLASS(Abstract)
class SUMMERCAMP_API ASCHUD
: public AILLHUD
{
	GENERATED_UCLASS_BODY()

	// Begin AILLHUD interface
	virtual TSubclassOf<UILLMinimapWidget> GetMinimapIcon(const UILLMinimapIconComponent* IconComponent) const override;
	// End AILLHUD interface

	/** Wraps to ILLMenuStackHUDComponent::ChangeRootMenu. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	FORCEINLINE UILLUserWidget* ChangeRootMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bForce = false)
	{
		return MenuStackComponent->ChangeRootMenu(MenuClass, bForce);
	}

	/** Wraps to ILLMenuStackHUDComponent::PushMenu. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta=(AdvancedDisplay="bSkipTransitionOut"))
	FORCEINLINE UILLUserWidget* PushMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bSkipTransitionOut = false)
	{
		return MenuStackComponent->PushMenu(MenuClass, bSkipTransitionOut);
	}

	/** Wraps to ILLMenuStackHUDComponent::PopMenu. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta=(AdvancedDisplay="bForce"))
	FORCEINLINE bool PopMenu(FName WidgetToFocus = NAME_None, const bool bForce = false)
	{
		return MenuStackComponent->PopMenu(WidgetToFocus, bForce);
	}

	/** Wraps to ILLMenuStackHUDComponent::RemoveMenuByClass. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (AdvancedDisplay = "bForce"))
	FORCEINLINE bool RemoveMenuByClass(TSubclassOf<UILLUserWidget> MenuClass)
	{
		return MenuStackComponent->RemoveMenuByClass(MenuClass);
	}
	
	/** Wraps to ILLMenuStackHUDComponent::GetCurrentMenu. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	FORCEINLINE UILLUserWidget* GetCurrentMenu() const { return MenuStackComponent->GetCurrentMenu(); }

	/** Wraps to ILLMenuStackHUDComponent::GetOpenMenuByClass. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	FORCEINLINE UILLUserWidget* GetOpenMenuByClass(TSubclassOf<UILLUserWidget> MenuClass) const { return MenuStackComponent->GetOpenMenuByClass(MenuClass); }

	/** Checks to see if there is an error dialog in the menu stack */
	UFUNCTION(BlueprintPure, Category = "Menu")
	bool IsErrorDialogShown() const;

protected:
	// If true, the minimap will use the Killer Icons rather than the Default Icons 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SCHUD")
	bool bIsKiller;
};
