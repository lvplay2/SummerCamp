// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLFindSessionsExtendedCallbackProxy.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLGameSession.h"
#include "ILLOnlineSessionSearch.h"

UILLFindSessionsExtendedCallbackProxy::UILLFindSessionsExtendedCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, Delegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnCompleted))
{
}

UILLFindSessionsExtendedCallbackProxy* UILLFindSessionsExtendedCallbackProxy::FindSessionsExtended(UObject* WorldContextObject, class APlayerController* PlayerController, const int MaxResults, const bool bLanQuery, const bool bDedicatedServersOnly, const bool bNonEmptyOnly)
{
	UILLFindSessionsExtendedCallbackProxy* Proxy = NewObject<UILLFindSessionsExtendedCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->MaxResults = MaxResults;
	Proxy->bIsLanQuery = bLanQuery;
	Proxy->bDedicatedOnly = bDedicatedServersOnly;
	Proxy->bNonEmptySearch = bNonEmptyOnly;
	return Proxy;
}

void UILLFindSessionsExtendedCallbackProxy::Activate()
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(World))
	{
		if (APlayerState* PlayerState = PlayerControllerWeakPtr.IsValid() ? PlayerControllerWeakPtr.Get()->PlayerState : nullptr)
		{
			TSharedPtr<const FUniqueNetId> UserID = PlayerState->UniqueId.GetUniqueNetId();
			if (UserID.IsValid())
			{
				auto Sessions = OnlineSub->GetSessionInterface();
				if (Sessions.IsValid())
				{
					DelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(Delegate);

					SearchObject = MakeShareable(new FILLOnlineSessionSearch);
					SearchObject->MaxSearchResults = MaxResults;
					SearchObject->bIsLanQuery = bIsLanQuery;
					SearchObject->QuerySettings.Set(SEARCH_PRESENCE, bIsLanQuery, EOnlineComparisonOp::Equals);
					SearchObject->QuerySettings.Set(SEARCH_DEDICATED_ONLY, bDedicatedOnly ? 1 : 0, EOnlineComparisonOp::Equals);
					SearchObject->QuerySettings.Set(SEARCH_NONEMPTY_SERVERS_ONLY, bNonEmptySearch ? 1 : 0, EOnlineComparisonOp::Equals);

					Sessions->FindSessions(*UserID, SearchObject.ToSharedRef());
					return;
				}
				else
				{
					FFrame::KismetExecutionMessage(TEXT("UILLFindSessionsExtendedCallbackProxy - Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
				}
			}
			else
			{
				FFrame::KismetExecutionMessage(*FString::Printf(TEXT("UILLFindSessionsExtendedCallbackProxy - Cannot map local player to unique net ID")), ELogVerbosity::Warning);
			}
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("UILLFindSessionsExtendedCallbackProxy - Invalid or uninitialized OnlineSubsystem")), ELogVerbosity::Warning);
	}

	// Fail immediately
	TArray<FBlueprintSessionResult> Results;
	OnFailure.Broadcast(Results);
}

void UILLFindSessionsExtendedCallbackProxy::OnCompleted(bool bSuccess)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(World);
	if (OnlineSub == nullptr)
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("UILLFindSessionsExtendedCallbackProxy - Invalid or uninitialized OnlineSubsystem")), ELogVerbosity::Warning);
	}
	else
	{
		auto Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(DelegateHandle);
		}
	}

	TArray<FBlueprintSessionResult> Results;

	if (bSuccess && OnlineSub && SearchObject.IsValid())
	{
		for (auto& Result : SearchObject->SearchResults)
		{
			FBlueprintSessionResult BPResult;
			BPResult.OnlineResult = Result;
			Results.Add(BPResult);
		}

		OnSuccess.Broadcast(Results);
	}
	else
	{
		OnFailure.Broadcast(Results);
	}
}
