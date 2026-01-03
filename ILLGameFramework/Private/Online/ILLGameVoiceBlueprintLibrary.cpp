// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameVoiceBlueprintLibrary.h"

#include "ILLLocalPlayer.h"
#include "ILLPlayerController.h"
#include "ILLPlayerState.h"

bool UILLGameVoiceBlueprintLibrary::CanPlayerBeMuted(AILLPlayerController* Listener, AILLPlayerState* PlayerState)
{
	if (Listener && PlayerState && PlayerState->UniqueId.IsValid())
	{
		if (AILLPlayerController* PCOwner = Cast<AILLPlayerController>(PlayerState->GetOwner()))
		{
			if (!PCOwner->IsLocalController())
				return true;
		}
		else
		{
			// PlayerController is unknown, remote
			return true;
		}
	}

	return false;
}

void UILLGameVoiceBlueprintLibrary::MutePlayerState(AILLPlayerController* PlayerController, AILLPlayerState* PlayerToMute)
{
	if (PlayerController && PlayerToMute && PlayerToMute->UniqueId.IsValid())
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			LocalPlayer->MutePlayer(PlayerToMute);
		}
	}
}

void UILLGameVoiceBlueprintLibrary::UnmutePlayerState(AILLPlayerController* PlayerController, AILLPlayerState* PlayerToUnmute)
{
	if (PlayerController && PlayerToUnmute && PlayerToUnmute->UniqueId.IsValid())
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			LocalPlayer->UnmutePlayer(PlayerToUnmute);
		}
	}
}

bool UILLGameVoiceBlueprintLibrary::IsPlayerMuted(AILLPlayerController* Listener, AILLPlayerState* PlayerState)
{
	if (Listener && PlayerState && PlayerState->UniqueId.IsValid())
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(Listener->GetLocalPlayer()))
		{
			return LocalPlayer->HasMuted(PlayerState);
		}
	}

	return false;
}
