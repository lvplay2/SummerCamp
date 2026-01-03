// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLHUD.h"

#include "UObjectToken.h"
#include "DisplayDebugHelpers.h"

#include "ILLGameInstance.h"
#include "ILLGameState.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLMinimapIconComponent.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerState.h"

FName AILLHUD::MenuStackCompName(TEXT("MenuStackComponent"));

AILLHUD::AILLHUD(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AILLHUD::OverrideMenuStackComp(const TArray<FILLMenuStackElement>& InNewMenuStack)
{
	// Destroy the current stack.
	MenuStackComponent->ChangeRootMenu(nullptr, false);

	// Update with the new stack.
	MenuStackComponent->OverrideStackElements(InNewMenuStack);

	// Respawn our new menus.
	MenuStackComponent->ReSpawnMenuStack();
}

bool AILLHUD::HUDInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	// Allow the first menu stack component to handle it
	if (UILLMenuStackHUDComponent* StackComp = GetMenuStackComp())
	{
		if (StackComp->MenuStackInputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad))
		{
			return true;
		}
	}

	return false;
}

bool AILLHUD::HUDInputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	// Allow the first menu stack component to handle it
	if (UILLMenuStackHUDComponent* StackComp = GetMenuStackComp())
	{
		if (StackComp->MenuStackInputKey(Key, EventType, AmountDepressed, bGamepad))
		{
			return true;
		}
	}

	return false;
}

bool AILLHUD::RegisterMinimapIcon(UILLMinimapIconComponent* IconComponent)
{
	// Fix your warnings before you ship!
#if WITH_EDITOR
	if (MinimapIcons.Contains(IconComponent))
	{
		FMessageLog("PIE").Warning(FText::FromString(TEXT("Trying to add duplicate ")))
				->AddToken(FUObjectToken::Create(IconComponent))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" to HUD "))))
				->AddToken(FUObjectToken::Create(this));
		return false;
	}
#endif

	MinimapIcons.Add(IconComponent);
	OnMinimapIconRegistered.Broadcast(IconComponent);

	return true;
}

void AILLHUD::UnregisterMinimapIcon(UILLMinimapIconComponent* IconComponent)
{
	// Fix your warnings before you ship!
#if WITH_EDITOR
	if (MinimapIcons.Contains(IconComponent) == false)
	{
		FMessageLog("PIE").Warning(FText::FromString(TEXT("Trying to remove ")))
				->AddToken(FUObjectToken::Create(IconComponent))
				->AddToken(FTextToken::Create(FText::FromString(TEXT(" that doesn't exist in HUD "))))
				->AddToken(FUObjectToken::Create(this));
	}
	else
#endif
	{
		MinimapIcons.Remove(IconComponent);
		OnMinimapIconUnregistered.Broadcast(IconComponent);
	}
}

TSubclassOf<UILLMinimapWidget> AILLHUD::GetMinimapIcon(const UILLMinimapIconComponent* IconComponent) const
{
	return IconComponent->DefaultIcon;
}
