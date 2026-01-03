// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLDestroyPartyCallbackProxy.h"

#include "ILLGameInstance.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerController.h"

UILLDestroyPartyCallbackProxy::UILLDestroyPartyCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLDestroyPartyCallbackProxy::Activate()
{
	if (PlayerControllerWeakPtr.Get())
	{
		if (UWorld* World = PlayerControllerWeakPtr->GetWorld())
		{
			if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
			{
				if (UILLOnlineSessionClient* SessionClient = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
				{
					AddToRoot();
					SessionClient->OnDestroyPartySessionComplete().BindDynamic(this, &ThisClass::OnPartyDestructionComplete);
					if (!SessionClient->DestroyPartySession())
					{
						RemoveFromRoot();
						OnFailure.Broadcast();
					}
					return;
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("ILLDestroyPartyCallbackProxy - Invalid session client"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("ILLDestroyPartyCallbackProxy - Invalid game instance"), ELogVerbosity::Warning);
			}
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("ILLDestroyPartyCallbackProxy - Invalid player controller"), ELogVerbosity::Warning);
	}

	FFrame::KismetExecutionMessage(TEXT("ILLDestroyPartyCallbackProxy - Failed to issue create party request"), ELogVerbosity::Warning);
	OnFailure.Broadcast();
}

UILLDestroyPartyCallbackProxy* UILLDestroyPartyCallbackProxy::DestroyParty(class AILLPlayerController* PlayerController)
{
	UILLDestroyPartyCallbackProxy* Proxy = NewObject<UILLDestroyPartyCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	return Proxy;
}

void UILLDestroyPartyCallbackProxy::OnPartyDestructionComplete(bool bWasSuccessful)
{
	RemoveFromRoot();

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
}
