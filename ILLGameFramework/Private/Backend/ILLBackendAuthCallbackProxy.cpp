// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendAuthCallbackProxy.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLGameInstance.h"
#include "ILLLocalPlayer.h"
#include "ILLPlayerController.h"

UILLBackendAuthCallbackProxy::UILLBackendAuthCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLBackendAuthCallbackProxy::Activate()
{
	if (PlayerControllerWeakPtr.Get())
	{
		UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerControllerWeakPtr->Player);
		if (LocalPlayer)
		{
#if WITH_EDITOR
			if (PlayerControllerWeakPtr->GetWorld()->IsPlayInEditor())
			{
				// Special case for the editor, since it does not initialize Steam
				GILLUserRequestedBackendOfflineMode = false; // Break out of user-requested offline mode
				OnSuccess.Broadcast();
				return;
			}
#endif
			UILLGameInstance* GI = Cast<UILLGameInstance>(PlayerControllerWeakPtr->GetGameInstance());
			if (GI->GetBackendRequestManager()->StartAuthRequestSequence(LocalPlayer, FOnILLBackendAuthCompleteDelegate::CreateUObject(this, &UILLBackendAuthCallbackProxy::OnCompleted)))
			{
				AddToRoot();
				return;
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("ILLBackendAuthCallbackProxy - Invalid local player"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("ILLBackendAuthCallbackProxy - Invalid player controller"), ELogVerbosity::Warning);
	}

	FFrame::KismetExecutionMessage(TEXT("ILLBackendAuthCallbackProxy - Failed to send auth request"), ELogVerbosity::Warning);
	OnFailure.Broadcast();
}

UILLBackendAuthCallbackProxy* UILLBackendAuthCallbackProxy::SendAuthRequest(class AILLPlayerController* PlayerController)
{
	UILLBackendAuthCallbackProxy* Proxy = NewObject<UILLBackendAuthCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	return Proxy;
}

UILLBackendAuthCallbackProxy* UILLBackendAuthCallbackProxy::SendSteamAuthRequest(class AILLPlayerController* PlayerController)
{
	UILLBackendAuthCallbackProxy* Proxy = NewObject<UILLBackendAuthCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	return Proxy;
}

void UILLBackendAuthCallbackProxy::OnCompleted(EILLBackendAuthResult AuthResult)
{
	RemoveFromRoot();

	if (AuthResult == EILLBackendAuthResult::Success)
	{
		GILLUserRequestedBackendOfflineMode = false; // Break out of user-requested offline mode
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
	if (AuthResult == EILLBackendAuthResult::AccountBanned)
		OnBannedFailure.Broadcast();
	else
		OnGeneralFailure.Broadcast();
}
