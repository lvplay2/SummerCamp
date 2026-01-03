// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendPlayer.h"

#include "OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLBackendRequestManager.h"
#include "ILLGameInstance.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerController.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLBackendPlayer"

UILLBackendPlayer::UILLBackendPlayer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	UserArbiterHeartbeatInterval = 120.f;
	MaxArbiterRetries = 10;
	ArbiterRetryDelay = 5.f;
}

void UILLBackendPlayer::BeginDestroy()
{
	Super::BeginDestroy();

	SetLoggedIn(false);
}

UWorld* UILLBackendPlayer::GetWorld() const
{
	return GetOuterUILLLocalPlayer() ? GetOuterUILLLocalPlayer()->GetWorld() : nullptr;
}

void UILLBackendPlayer::SetupRequest(FHttpRequestPtr HttpRequest)
{
	Super::SetupRequest(HttpRequest);

	// Send backend identification
	HttpRequest->SetHeader(TEXT("Account-Id"), GetAccountId().GetIdentifier());
	HttpRequest->SetHeader(TEXT("Session-Token"), GetSessionToken());

	// Send player name
	UILLLocalPlayer* LocalPlayer = GetOuterUILLLocalPlayer();
	HttpRequest->SetHeader(TEXT("Gamer-Tag"), LocalPlayer->GetNickname());

#if PLATFORM_XBOXONE
	// Assigning this header tells Microsoft's servers to send the Authorization token to the backend
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (OnlineIdentity.IsValid())
	{
		HttpRequest->SetHeader(TEXT("xbl-authz-actor-10"), OnlineIdentity->GetUserHashToken(LocalPlayer->GetControllerId()));
	}
#endif
}

UILLGameInstance* UILLBackendPlayer::GetGameInstance() const
{
	return CastChecked<UILLGameInstance>(GetOuterUILLLocalPlayer()->GetGameInstance());
}

void UILLBackendPlayer::NotifyAuthenticated(const FILLBackendPlayerIdentifier& InAccountId, const FString& InSessionToken)
{
	UE_LOG(LogILLBackend, Verbose, TEXT("%s::NotifyAuthenticated"), *GetName());

	AccountId = InAccountId;
	SessionToken = InSessionToken;
}

void UILLBackendPlayer::NotifyArbitrated()
{
	UE_LOG(LogILLBackend, Verbose, TEXT("%s::NotifyArbitrated"), *GetName());

	GILLUserRequestedBackendOfflineMode = false; // Break out of user-requested offline mode

	// Flag as logged in
	SetLoggedIn(true);

	// Start a heartbeat timer
	UILLGameInstance* GI = GetGameInstance();
	GI->GetTimerManager().SetTimer(TimerHandle_ArbiterHeartbeat, this, &ThisClass::SendArbiterHeartbeat, UserArbiterHeartbeatInterval, true);

	// Simulate login
	if (AILLPlayerController* OwningPC = Cast<AILLPlayerController>(GetOuterUILLLocalPlayer()->GetPlayerController(GetWorld())))
	{
		OwningPC->SetBackendAccountId(AccountId);
	}

	// Query baseline matchmaking status
	UILLGameInstance* GameInstance = CastChecked<UILLGameInstance>(GetWorld()->GetGameInstance());
	UILLOnlineMatchmakingClient* OMC = CastChecked<UILLOnlineMatchmakingClient>(GameInstance->GetOnlineMatchmaking());
	OMC->RequestPartyMatchmakingStatus();

	// Listen for map loads
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMap);

	// Notify our outer
	GetOuterUILLLocalPlayer()->NotifyBackendPlayerArbitrated();
}

void UILLBackendPlayer::NotifyArbitrationFailed()
{
	UE_LOG(LogILLBackend, Warning, TEXT("%s::NotifyArbitrationFailed"), *GetName());

	UILLGameInstance* GI = GetGameInstance();
	GI->GetTimerManager().ClearTimer(TimerHandle_ArbiterHeartbeat);
}

void UILLBackendPlayer::NotifyLoggedOut()
{
	UE_LOG(LogILLBackend, Warning, TEXT("%s::NotifyLoggedOut"), *GetName());

	UILLGameInstance* GI = GetGameInstance();
	GI->GetTimerManager().ClearTimer(TimerHandle_ArbiterHeartbeat);
	SetLoggedIn(false);
}

void UILLBackendPlayer::SendArbiterHeartbeat()
{
	UE_LOG(LogILLBackend, Verbose, TEXT("%s::SendArbiterHeartbeat"), *GetName());

	// Only send a heartbeat when we aren't already waiting on a response for one
	if (!bWaitingOnHeartbeat)
	{
		UILLGameInstance* GI = GetGameInstance();
		GI->GetBackendRequestManager()->SendArbiterHeartbeatRequest(this, FOnILLBackendArbiterHeartbeatCompleteDelegate::CreateUObject(this, &ThisClass::ArbiterHeartbeatCompleted));
		bWaitingOnHeartbeat = true;

		// Reset the timer to send a heartbeat in case this was called directly
		GI->GetTimerManager().SetTimer(TimerHandle_ArbiterHeartbeat, this, &ThisClass::SendArbiterHeartbeat, UserArbiterHeartbeatInterval, true);
	}
}

void UILLBackendPlayer::ArbiterHeartbeatCompleted(EILLBackendArbiterHeartbeatResult HeartbeatResult)
{
	bWaitingOnHeartbeat = false;

	if (HeartbeatResult == EILLBackendArbiterHeartbeatResult::Success)
	{
		UE_LOG(LogILLBackend, Verbose, TEXT("%s::ArbiterHeartbeatCompleted"), *GetName());

		ArbiterRetries = 0;
		bRetryingArbiter = false;

		// Notify listeners
		BackendPlayerArbiterHeartbeatComplete.Broadcast(this, HeartbeatResult);
	}
	else
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s::ArbiterHeartbeatCompleted: HeartbeatResult: %i bLoggedIn: %s"), *GetName(), static_cast<uint8>(HeartbeatResult), bLoggedIn ? TEXT("true") : TEXT("false"));

		if (bLoggedIn)
		{
			// Check if we can perform a retry
			// Backend request timeouts can happen rarely, so it's best to try again before we kick someone out of a game
			UILLGameInstance* GI = GetGameInstance();
			if (HeartbeatResult != EILLBackendArbiterHeartbeatResult::HeartbeatDenied && ArbiterRetries < MaxArbiterRetries)
			{
				// Stay logged in while we retry to authorize with the arbiter
				GI->GetTimerManager().SetTimer(TimerHandle_ArbiterHeartbeat, this, &ThisClass::SendArbiterHeartbeat, ArbiterRetryDelay);
				++ArbiterRetries;
				bRetryingArbiter = true;
			}
			else
			{
				ArbiterRetries = 0;
				bRetryingArbiter = false;

				// Notify listeners
				BackendPlayerArbiterHeartbeatComplete.Broadcast(this, HeartbeatResult);

				// Disconnect and sign out
				UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GI->GetOnlineSession());
				OSC->PerformSignoutAndDisconnect(GetOuterUILLLocalPlayer(), FText(LOCTEXT("BackendArbiterHeartbeatFailure", "Session heartbeat failure! Verify internet connectivity.")));
			}
		}
	}
}

void UILLBackendPlayer::OnPostLoadMap(UWorld* World)
{
	if (bLoggedIn)
	{
		// Send a heartbeat to compensate for potentially long load times, avoiding a backend timeout
		SendArbiterHeartbeat();
	}
}

void UILLBackendPlayer::SetLoggedIn(bool bIsLoggedIn)
{
	if (bLoggedIn != bIsLoggedIn)
	{
		bLoggedIn = bIsLoggedIn;
	}
}

#undef LOCTEXT_NAMESPACE
