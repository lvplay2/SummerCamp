// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/HUD.h"
#include "ILLMenuStackHUDComponent.h"
#include "ILLHUD.generated.h"

class AILLLobbyBeaconMemberState;
class AILLPlayerState;

class UILLMenuStackHUDComponent;
class UILLMinimapIconComponent;
class UILLMinimapWidget;

/**
 * Delegate triggered for HUD lobby beacon member state registration/un-registration.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLHUDLobbyBeaconMemberUpdate, AILLLobbyBeaconMemberState*, MemberState);

/**
 * Delegate triggered for HUD player state registration/un-registration.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLHUDPlayerStateUpdate, AILLPlayerState*, PlayerState);

/**
 * Delegate triggered for minimap icon component registration.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLHUDMinimapIconComponentRegistered, UILLMinimapIconComponent*, IconComponent);

/**
 * @class AILLHUD
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLHUD
: public AHUD
{
	GENERATED_UCLASS_BODY()

	//////////////////////////////////////////////////
	// Menu stack
public:
	/** First menu stack component on this HUD. */
	UFUNCTION(BlueprintPure, Category="Components")
	UILLMenuStackHUDComponent* GetMenuStackComp() const { return MenuStackComponent; }

	/** Force a new Menu stack component and load the top most widget */
	virtual void OverrideMenuStackComp(const TArray<FILLMenuStackElement>& InNewMenuStack);

protected:
	// Name for MenuStackComponent
	static FName MenuStackCompName;

	// Menu stack component instance
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Components")
	UILLMenuStackHUDComponent* MenuStackComponent;

	//////////////////////////////////////////////////
	// Input
public:
	/**
	 * Player analog input handler.
	 * Called before ILLPlayerController::InputAxis attempts to handle it.
	 *
	 * @return true if the input event was handled.
	 */
	bool HUDInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad);

	/**
	 * Player key input handler.
	 * Called before ILLPlayerController::InputKey attempts to handle it.
	 *
	 * @return true if the input event was handled.
	 */
	bool HUDInputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad);

	//////////////////////////////////////////////////
	// Minimap
public:
	/** Adds this icon to the list of icons we're tracking for the minimap widget */
	bool RegisterMinimapIcon(UILLMinimapIconComponent* IconComponent);

	/** Removes this icon from the list of icons we're tracking for the minimap widget */
	void UnregisterMinimapIcon(UILLMinimapIconComponent* IconComponent);

	/** Gets the list of minimap icons for display/parsing */
	const TArray<UILLMinimapIconComponent*>& GetMinimapIconComponents() const { return MinimapIcons; }

	/** Returns the icon for a given component, used to override default behavior */
	virtual TSubclassOf<UILLMinimapWidget> GetMinimapIcon(const UILLMinimapIconComponent* IconComponent) const;

	/** Called whenever a new component is registered with the HUD */
	UPROPERTY()
	FILLHUDMinimapIconComponentRegistered OnMinimapIconRegistered;

	/** Called whenever a component is unregistered from the HUD */
	UPROPERTY()
	FILLHUDMinimapIconComponentRegistered OnMinimapIconUnregistered;

private:
	/** List of icons we're tracking for the minimap widget*/
	UPROPERTY()
	TArray<UILLMinimapIconComponent*> MinimapIcons;
};
