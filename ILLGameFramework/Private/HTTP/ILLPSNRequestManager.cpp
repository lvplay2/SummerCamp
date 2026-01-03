// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPSNRequestManager.h"

UILLPSNRequestManager::UILLPSNRequestManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AccessTokenExpirationMultiplier = .75f;
	BaseUrlExpirationMultiplier = .75f;
	BaseUrlRefreshMultiplier = .5f;
}

void UILLPSNRequestManager::Shutdown()
{
	// Fire off a request to destroy the game session, before the Super call tears down the thread
	DestroyGameSession();
	TaskThread->TriggerWorkEvent(true);

	// Now we can tear down the thread
	Super::Shutdown();
}

UILLPSNHTTPRequestTask* UILLPSNRequestManager::CreatePSNRequest(UClass* RequestClass)
{
	checkSlow(IsInGameThread());

	UILLPSNHTTPRequestTask* RequestTask = NewObject<UILLPSNHTTPRequestTask>(this, RequestClass);

	// Attempt to assign the OnlineEnvironment
	for (const FAccessToken& CachedToken : CachedAccessTokens)
	{
		RequestTask->OnlineEnvironment = CachedToken.OnlineEnvironment;
		break;
	}

	if (PrepareRequest(RequestTask))
	{
		UE_LOG(LogILLHTTP, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());
		return RequestTask;
	}

	return nullptr;
}

bool UILLPSNRequestManager::StartAuthorizationSequence(const FString& ClientAppId, const FString& ClientAuthCode, const EOnlineEnvironment::Type OnlineEnvironment, FOnILLPSNClientAuthorzationCompleteDelegate CompletionDelegate)
{
	// Forbid OnlineEnvironments from mixing
	for (const FAccessToken& CachedToken : CachedAccessTokens)
	{
		if (CachedToken.OnlineEnvironment != OnlineEnvironment)
		{
			UE_LOG(LogILLHTTP, Warning, TEXT("%s: Forbidding online environment %s from authorizing, already using %s!"), ANSI_TO_TCHAR(__FUNCTION__), ToString(OnlineEnvironment), ToString(CachedToken.OnlineEnvironment));
			return false;
		}
		break;
	}

	// Build and queue a request
	if (UILLPSNGetAccessTokenRequestTask* RequestTask = CreatePSNRequest<UILLPSNGetAccessTokenRequestTask>())
	{
		RequestTask->ClientAppId = ClientAppId;
		RequestTask->ClientAuthCode = ClientAuthCode;
		RequestTask->OnlineEnvironment = OnlineEnvironment;
		RequestTask->CompletionDelegate = CompletionDelegate;
		RequestTask->QueueRequest();
		return true;
	}

	return false;
}

void UILLPSNRequestManager::RegisterAccessToken(const FString& ClientAppId, const FString& AccessToken, const FString& CountryCode, const EOnlineEnvironment::Type OnlineEnvironment, const int32 ExpiresInSeconds)
{
	// Store the entry
	FAccessToken NewEntry;
	NewEntry.Counter = AccessTokenCounter++;
	NewEntry.AccessToken = AccessToken;
	NewEntry.ClientAppId = ClientAppId;
	NewEntry.CountryCode = CountryCode;
	NewEntry.OnlineEnvironment = OnlineEnvironment;
	const int32 Index = CachedAccessTokens.Add(NewEntry);

	// Set a timer to automatically remove it
	GetTimerManager().SetTimer(CachedAccessTokens[Index].TimerHandle_Expiration, FTimerDelegate::CreateUObject(this, &ThisClass::ExpireAccessToken, NewEntry.Counter), ExpiresInSeconds*AccessTokenExpirationMultiplier, false);
}

void UILLPSNRequestManager::ExpireAccessToken(uint32 Counter)
{
	for (int32 Index = 0; Index < CachedAccessTokens.Num(); ++Index)
	{
		if (CachedAccessTokens[Index].Counter == Counter)
		{
			CachedAccessTokens.RemoveAt(Index);
			break;
		}
	}
}

FString UILLPSNRequestManager::FindBaseURL(const FString& ApiGroup)
{
	if (const FBaseUrl* const BaseUrl = CachedBaseUrls.FindByPredicate([&](const FBaseUrl& ExistingUrl) -> bool { return (ExistingUrl.ApiGroup == ApiGroup); }))
	{
		return BaseUrl->URL;
	}

	return FString();
}

bool UILLPSNRequestManager::LookupBaseUrls(const FString& ApiGroup)
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ApiGroup);

	if (!CachedAccessTokens.Num())
	{
		UE_LOG(LogILLHTTP, Error, TEXT("%s: No CachedAccessTokens to perform lookup with!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	// Collect the online environment we need base URLs for
	// We do not allow mixing of online environments, so the first one will do
	EOnlineEnvironment::Type OnlineEnvironment;
	for (const FAccessToken& CachedToken : CachedAccessTokens)
	{
		OnlineEnvironment = CachedToken.OnlineEnvironment;
		break;
	}

	// Build a list of access tokens to drive the ApiGroup fetching, by CountryCode
	// We do this in reverse order to use the newest access tokens first
	TArray<FAccessToken*> QueryTokens;
	for (int32 TokenIndex = CachedAccessTokens.Num()-1; TokenIndex >= 0; --TokenIndex)
	{
		// When not already cached
		if (!CachedBaseUrls.FindByPredicate([&](const FBaseUrl& ExistingUrl) -> bool
			{ return (ExistingUrl.ApiGroup == ApiGroup && ExistingUrl.CountryCode == CachedAccessTokens[TokenIndex].CountryCode && ExistingUrl.OnlineEnvironment == OnlineEnvironment && ExistingUrl.TimerHandle_Refresh.IsValid()); }))
		{
			QueryTokens.Add(&CachedAccessTokens[TokenIndex]);
		}
	}
	if (!QueryTokens.Num())
	{
		return false;
	}

	// Check if we already have requests in-flight
	for (const UILLPSNBaseUrlRequestTask* const ExistingTask : BaseUrlRequests)
	{
		for (int32 TokenIndex = 0; TokenIndex < QueryTokens.Num(); )
		{
			if (ExistingTask->ApiGroup == ApiGroup
			 && ExistingTask->CountryCode == QueryTokens[TokenIndex]->CountryCode
			 && ExistingTask->OnlineEnvironment == QueryTokens[TokenIndex]->OnlineEnvironment)
			{
				QueryTokens.RemoveAt(TokenIndex);
			}
			else
			{
				++TokenIndex;
			}
		}
	}
	if (!QueryTokens.Num())
	{
		// If we are empty at this point, it's because we are technically making requests -- return true so the caller knows to listen for completion
		return true;
	}

	// Now request for each relevant access token
	FOnILLPSNBaseUrlRequestCompleteDelegate LookupDelegate = FOnILLPSNBaseUrlRequestCompleteDelegate::CreateUObject(this, &ThisClass::OnLookupBaseUrlCompleted);
	for (const FAccessToken* const CachedToken : QueryTokens)
	{
		UILLPSNBaseUrlRequestTask* RequestTask = CreatePSNRequest<UILLPSNBaseUrlRequestTask>();
		check(RequestTask);
		BaseUrlRequests.Add(RequestTask);

		RequestTask->AccessToken = CachedToken->AccessToken;
		RequestTask->ApiGroup = ApiGroup;
		RequestTask->CountryCode = CachedToken->CountryCode;
		RequestTask->OnlineEnvironment = CachedToken->OnlineEnvironment;
		RequestTask->CompletionDelegate = LookupDelegate;
		RequestTask->QueueRequest();
	}

	return true;
}

void UILLPSNRequestManager::OnLookupBaseUrlCompleted(UILLPSNBaseUrlRequestTask* RequestTask)
{
	if (BaseUrlRequests.RemoveSingle(RequestTask) > 0)
	{
		if (!RequestTask->ReceivedUrl.IsEmpty() && RequestTask->ExpiresInSeconds > 0)
		{
			// Remove the previous older entry
			for (int32 Index = 0; Index < CachedBaseUrls.Num(); ++Index)
			{
				if (CachedBaseUrls[Index].ApiGroup == RequestTask->ApiGroup
				 && CachedBaseUrls[Index].CountryCode == RequestTask->CountryCode
				 && CachedBaseUrls[Index].OnlineEnvironment == RequestTask->OnlineEnvironment)
				{
					// Clear the timers too!
					GetTimerManager().ClearTimer(CachedBaseUrls[Index].TimerHandle_Refresh);
					GetTimerManager().ClearTimer(CachedBaseUrls[Index].TimerHandle_Expiration);
					CachedBaseUrls.RemoveAt(Index);
					break;
				}
			}

			// Store the entry
			FBaseUrl NewEntry;
			NewEntry.Counter = BaseUrlCounter++;
			NewEntry.ApiGroup = RequestTask->ApiGroup;
			NewEntry.CountryCode = RequestTask->CountryCode;
			NewEntry.OnlineEnvironment = RequestTask->OnlineEnvironment;
			NewEntry.URL = RequestTask->ReceivedUrl;
			const int32 Index = CachedBaseUrls.Add(NewEntry);

			// Set timers to automatically refresh and expire it
			GetTimerManager().SetTimer(CachedBaseUrls[Index].TimerHandle_Refresh, FTimerDelegate::CreateUObject(this, &ThisClass::RefreshBaseUrl, NewEntry.Counter), RequestTask->ExpiresInSeconds*BaseUrlRefreshMultiplier, false);
			GetTimerManager().SetTimer(CachedBaseUrls[Index].TimerHandle_Expiration, FTimerDelegate::CreateUObject(this, &ThisClass::ExpireBaseUrl, NewEntry.Counter), RequestTask->ExpiresInSeconds*BaseUrlExpirationMultiplier, false);

			// Check if we are done
			if (BaseUrlRequests.Num() == 0)
			{
				OnPSNBaseUrlLookupCompleteDelegate.Broadcast();
				OnPSNBaseUrlLookupCompleteDelegate.Clear();
			}
		}
		else
		{
			UE_LOG(LogILLHTTP, Error, TEXT("%s: BaseUrl lookup failed for %s!"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());
		}
	}
	else
	{
		UE_LOG(LogILLHTTP, Error, TEXT("%s: Did not find %s in BaseUrlRequests!"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());
	}
}

void UILLPSNRequestManager::RefreshBaseUrl(uint32 Counter)
{
	for (int32 Index = 0; Index < CachedBaseUrls.Num(); ++Index)
	{
		if (CachedBaseUrls[Index].Counter == Counter)
		{
			// Invalidating the timer will make it appear invalid in the URL cache
			CachedBaseUrls[Index].TimerHandle_Refresh.Invalidate();
			LookupBaseUrls(CachedBaseUrls[Index].ApiGroup);
			break;
		}
	}
}

void UILLPSNRequestManager::ExpireBaseUrl(uint32 Counter)
{
	for (int32 Index = 0; Index < CachedBaseUrls.Num(); ++Index)
	{
		if (CachedBaseUrls[Index].Counter == Counter)
		{
			CachedBaseUrls.RemoveAt(Index);
			break;
		}
	}
}

void UILLPSNRequestManager::CreateGameSession(FOnILLPSNCreateSessionCompleteDelegate CompletionDelegate)
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	check(!bCreatingGameSession);
	bCreatingGameSession = true;

	const TCHAR* BaseURL = TEXT("sessionInvitation");
	if (LookupBaseUrls(BaseURL))
	{
		// Listen for completion
		OnPSNBaseUrlLookupCompleteDelegate.AddUObject(this, &ThisClass::OnLookupBaseUrlForCreateGameSessionCompleted, CompletionDelegate);
	}
	else if (UILLPSNPostSessionRequestTask* RequestTask = CreatePSNRequest<UILLPSNPostSessionRequestTask>())
	{
		RequestTask->AccessToken = CachedAccessTokens.Last().AccessToken;
		RequestTask->BaseURL = FindBaseURL(BaseURL);
		RequestTask->ClientAppId = CachedAccessTokens.Last().ClientAppId;
		RequestTask->CompletionDelegate = CompletionDelegate;
		RequestTask->QueueRequest();
	}
	else
	{
		CompletionDelegate.ExecuteIfBound(FString());
		bCreatingGameSession = false;
	}
}

void UILLPSNRequestManager::DestroyGameSession()
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (GameSessionId.IsEmpty())
	{
		UE_LOG(LogILLHTTP, Verbose, TEXT("%s: No GameSessionId to destroy"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	const FString BaseURL = FindBaseURL(TEXT("sessionInvitation"));
	if (BaseURL.IsEmpty())
	{
		UE_LOG(LogILLHTTP, Error, TEXT("%s: No FindBaseURL to destroy GameSessionId %s!"), ANSI_TO_TCHAR(__FUNCTION__), *GameSessionId);
		return;
	}

	if (UILLPSNDeleteSessionRequestTask* RequestTask = CreatePSNRequest<UILLPSNDeleteSessionRequestTask>())
	{
		GameSessionId.Empty();
		RequestTask->AccessToken = CachedAccessTokens.Last().AccessToken;
		RequestTask->BaseURL = BaseURL;
		RequestTask->SessionId = GameSessionId;
		RequestTask->QueueRequest();
	}
}

void UILLPSNRequestManager::OnLookupBaseUrlForCreateGameSessionCompleted(FOnILLPSNCreateSessionCompleteDelegate CompletionDelegate)
{
	// LookupBaseUrls should return false now that we have finished this
	bCreatingGameSession = false;
	CreateGameSession(CompletionDelegate);
}
