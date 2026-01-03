// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLeavePartyCallbackProxy.h"

#include "ILLGameInstance.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerController.h"

UILLLeavePartyCallbackProxy::UILLLeavePartyCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLLeavePartyCallbackProxy::Activate()
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
					SessionClient->OnLeavePartyComplete().BindDynamic(this, &ThisClass::OnPartyDestructionComplete);
					if (!SessionClient->LeavePartySession())
					{
						RemoveFromRoot();
						OnFailure.Broadcast();
					}
					return;
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("ILLLeavePartyCallbackProxy - Invalid session client"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("ILLLeavePartyCallbackProxy - Invalid game instance"), ELogVerbosity::Warning);
			}
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("ILLLeavePartyCallbackProxy - Invalid player controller"), ELogVerbosity::Warning);
	}

	FFrame::KismetExecutionMessage(TEXT("ILLLeavePartyCallbackProxy - Failed to issue create party request"), ELogVerbosity::Warning);
	OnFailure.Broadcast();
}

UILLLeavePartyCallbackProxy* UILLLeavePartyCallbackProxy::LeaveParty(class AILLPlayerController* PlayerController)
{
	UILLLeavePartyCallbackProxy* Proxy = NewObject<UILLLeavePartyCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	return Proxy;
}

void UILLLeavePartyCallbackProxy::OnPartyDestructionComplete(bool bWasSuccessful)
{
	RemoveFromRoot();

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
}
