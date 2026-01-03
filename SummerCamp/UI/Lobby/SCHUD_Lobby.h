// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLHUD_Lobby.h"
#include "SCPlayerState.h"
#include "ILLMenuStackHUDComponent.h"
#include "SCHUD_Lobby.generated.h"

class USCLobbyWidget;

/**
 * @class ASCHUD_Lobby
 */
UCLASS()
class SUMMERCAMP_API ASCHUD_Lobby
: public AILLHUD_Lobby
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	// Begin AILLHUD_Lobby interface
	virtual void HideLobbyUI() override;
	// End AILLHUD_Lobby interface

protected:
	// Begin AILLHUD_Lobby interface
	virtual void SpawnLobbyWidget() override;
	virtual UILLLobbyWidget* GetLobbyWidget() const override;
	// End AILLHUD_Lobby interface

public:
	/** Used to force the Lobby widget to the front when the countdown timer is low. */
	void ForceLobbyUI();

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

	/** Wraps to ILLMenuStackHUDComponent::GetOpenMenu. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	FORCEINLINE UILLUserWidget* GetOpenMenuByClass(TSubclassOf<UILLUserWidget> MenuClass) const { return MenuStackComponent->GetOpenMenuByClass(MenuClass); }

protected:
	// Soft class reference to the lobby widget
	UPROPERTY()
	TSoftClassPtr<USCLobbyWidget> SoftLobbyWidgetClass;

	// Class of the lobby widget once loaded
	UPROPERTY(Transient)
	TSubclassOf<USCLobbyWidget> HardLobbyWidgetClass;
};
