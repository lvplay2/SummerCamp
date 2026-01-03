// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCMenuWidget.generated.h"

/**
 * @class USCMenuWidget
 * @brief Base class for frontend and ingame menus.
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCMenuWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Menu stack wrapping
public:
	/** Wraps to ILLMenuStackHUDComponent::ChangeRootMenu through the owning player controller. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	UILLUserWidget* ChangeRootMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bForce = false);

	/** Wraps to ILLMenuStackHUDComponent::PushMenu through the owning player controller. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta=(AdvancedDisplay="bSkipTransitionOut"))
	UILLUserWidget* PushMenu(TSubclassOf<UILLUserWidget> MenuClass, const bool bSkipTransitionOut = false);

	/** Wraps to ILLMenuStackHUDComponent::PopMenu through the owning player controller. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta=(AdvancedDisplay="bForce"))
	bool PopMenu(FName WidgetToFocus = NAME_None, const bool bForce = false);

	/** Wraps to ILLMenuStackHUDComponent::PopMenu through the owning player controller. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	bool RemoveMenuByClass(TSubclassOf<UILLUserWidget> MenuClass);

	//////////////////////////////////////////////////
	// Frontend functions
public:
	/** Used by the frontend menus to change the menu background camera. Could use a better name... */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ChangeCamera(const FString& CameraName, const float TransitionTime);

	//////////////////////////////////////////////////
	// Lobby menu functions
public:
	/** @return true if this menu is being viewed in the lobby (outside of the frontend menus). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Menu|Lobby")
	bool IsViewingMenuInLobby() const;

	//////////////////////////////////////////////////
	// In-game menu functions
public:
	/** @return true if this menu is being viewed in-game (outside of the frontend menus). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Menu|InGame")
	bool IsViewingMenuInGame() const;

	//////////////////////////////////////////////////
	// Visual Functions
public:
	/** @return the current set header name. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Menu")
	const FText& GetHeaderName() { return HeaderName; }

protected:
	/** Variable that holds the header name for this menu if dynamic ones are supported. */
	UPROPERTY(EditAnywhere, Category = "Menu")
	FText HeaderName;
};
