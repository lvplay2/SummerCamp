// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCreatePartyCallbackProxy.h"

#include "ILLGameInstance.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerController.h"

UILLCreatePartyCallbackProxy::UILLCreatePartyCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLCreatePartyCallbackProxy::Activate()
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
					SessionClient->OnCreatePartyComplete().BindDynamic(this, &ThisClass::OnPartyCreationComplete);
					if (!SessionClient->CreateParty())
					{
						// Do not trigger failure from here, OnPartyCreationComplete will have already
					}
					return;
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("ILLCreatePartyCallbackProxy - Invalid session client"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("ILLCreatePartyCallbackProxy - Invalid game instance"), ELogVerbosity::Warning);
			}
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("ILLCreatePartyCallbackProxy - Invalid player controller"), ELogVerbosity::Warning);
	}

	FFrame::KismetExecutionMessage(TEXT("ILLCreatePartyCallbackProxy - Failed to issue create party request"), ELogVerbosity::Warning);
	OnFailure.Broadcast();
}

UILLCreatePartyCallbackProxy* UILLCreatePartyCallbackProxy::CreateParty(class AILLPlayerController* PlayerController)
{
	UILLCreatePartyCallbackProxy* Proxy = NewObject<UILLCreatePartyCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	return Proxy;
}

void UILLCreatePartyCallbackProxy::OnPartyCreationComplete(bool bWasSuccessful)
{
	RemoveFromRoot();

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
}
