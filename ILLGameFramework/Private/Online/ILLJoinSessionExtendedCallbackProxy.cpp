// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLJoinSessionExtendedCallbackProxy.h"

#include "ILLOnlineSessionClient.h"

UILLJoinSessionExtendedCallbackProxy::UILLJoinSessionExtendedCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, Delegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCompleted))
{
}

UILLJoinSessionExtendedCallbackProxy* UILLJoinSessionExtendedCallbackProxy::JoinSessionExtended(UObject* WorldContextObject, class APlayerController* PlayerController, const FBlueprintSessionResult& SearchResult, const FString& Password)
{
	UILLJoinSessionExtendedCallbackProxy* Proxy = NewObject<UILLJoinSessionExtendedCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->OnlineSearchResult = SearchResult.OnlineResult;
	Proxy->JoinPassword = Password;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UILLJoinSessionExtendedCallbackProxy::Activate()
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	if (World->GetGameInstance() && World->GetGameInstance()->GetOnlineSession())
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(World->GetGameInstance()->GetOnlineSession()))
		{
			if (!OSC->JoinSession(NAME_GameSession, OnlineSearchResult, Delegate, JoinPassword))
			{
				OnFailure.Broadcast();
			}
		}
	}
}

void UILLJoinSessionExtendedCallbackProxy::OnCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		OnSuccess.Broadcast();
	}

	OnFailure.Broadcast();
}
