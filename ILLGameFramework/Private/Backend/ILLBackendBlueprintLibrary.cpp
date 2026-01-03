// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendBlueprintLibrary.h"

#include "ILLBackendRequestManager.h"
#include "ILLBackendPlayer.h"
#include "ILLLocalPlayer.h"
#include "ILLPlayerController.h"

bool GILLUserRequestedBackendOfflineMode = false;

bool UILLBackendBlueprintLibrary::IsInOfflineMode(AILLPlayerController* PlayerController)
{
	// Check for user-requested offline mode
	if (HasUserRequestedOfflineMode())
		return true;

	// Can't be online until we have this
	if (!PlayerController)
		return true;

	// Platform authentication typically happens before backend authentication can start, so check that first
	if (!HasBackendLogin(PlayerController))
		return true;

	// Check platform login
	if (!UKismetSystemLibrary::IsLoggedIn(PlayerController))
		return true;

	return false;
}

bool UILLBackendBlueprintLibrary::HasBackendLogin(AILLPlayerController* PlayerController)
{
	if (PlayerController)
	{
		if (UILLLocalPlayer* LP = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			// Skip requiring authentication for PIE
			if (PlayerController->GetWorld()->IsPlayInEditor())
				return true;

			if (UILLBackendPlayer* Player = LP->GetBackendPlayer())
			{
				return Player->IsLoggedIn();
			}
		}
	}

	return false;
}

bool UILLBackendBlueprintLibrary::NeedsAuthRequest(AILLPlayerController* PlayerController)
{
	return !HasBackendLogin(PlayerController);
}

bool UILLBackendBlueprintLibrary::CanProceedIntoOfflineMode(AILLPlayerController* PlayerController)
{
	if (PlayerController)
	{
#if PLATFORM_DESKTOP
		// Steam will not report as logged in for the UILLPlatformLoginCallbackProxy, where XB1 does
		// So just always allow them to proceed into offline mode
		return true;
#else
		if (UILLLocalPlayer* LP = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			// We must have gone through platform login first!
			return LP->HasPlatformLogin();
		}
#endif
	}

	return false;
}

void UILLBackendBlueprintLibrary::RequestOfflineMode(AILLPlayerController* PlayerController)
{
	UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(PlayerController->GetLocalPlayer());

	// Track that this was requested
	GILLUserRequestedBackendOfflineMode = true;

	// Ensure account privileges are reset to offline defaults
	LocalPlayer->ResetPrivileges();

	// Force platform login completion that would normally occur after privilege collection
	LocalPlayer->OnPlatformLoginComplete(LocalPlayer->GetPreferredUniqueNetId());

	// Notify offline mode completion
	LocalPlayer->NotifyOfflineMode();
}
