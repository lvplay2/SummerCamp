// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendRequestManager.h"

#include "Json.h"
#include "JsonUtilities.h"
#include "Online.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLBackendPlayer.h"
#include "ILLBackendServer.h"
#include "ILLGameInstance.h"
#include "ILLLocalPlayer.h"

DEFINE_LOG_CATEGORY(LogILLBackend);

UILLBackendRequestManager::UILLBackendRequestManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, BackendServer(nullptr)
{
	PlayerClass = UILLBackendPlayer::StaticClass();
	ServerClass = UILLBackendServer::StaticClass();
}

void UILLBackendRequestManager::BeginDestroy()
{
	Super::BeginDestroy();

	PlayerList.Empty();
}

void UILLBackendRequestManager::Init()
{
	Super::Init();

	if (IsRunningDedicatedServer())
	{
		// Create a BackendServer instance if needed
		BackendServer = NewObject<UILLBackendServer>(this, ServerClass);
	}
}

bool UILLBackendRequestManager::CanSendAnyRequests() const
{
	if (BackendServer)
	{
		return true;
	}
	else
	{
		if (!HasAnyLoggedInPlayers())
			return false;
	}

	return true;
}

bool UILLBackendRequestManager::HasAnyLoggedInPlayers() const
{
	for (UILLBackendPlayer* Player : PlayerList)
	{
		if (Player->IsLoggedIn())
		{
			check(!GILLUserRequestedBackendOfflineMode);
			return true;
		}
	}

	return false;
}

UILLBackendHTTPRequestTask* UILLBackendRequestManager::CreateBackendRequest(UClass* RequestClass, const FILLBackendEndpointHandle& Endpoint, UILLBackendUser* User/* = nullptr*/)
{
	check(IsInGameThread());
	check(Endpoint);
	check(EndpointList.IsValidIndex(Endpoint.EndpointIndex));

	// Verify we can send this request at all
	if (EndpointList[Endpoint.EndpointIndex].bRequireUser && !CanSendAnyRequests())
		return nullptr;

	// Default the User param to the first available when not supplied
	if (!User)
	{
		if (GetBackendServer())
			User = GetBackendServer();
		else if (PlayerList.Num() > 0 && PlayerList[0]->IsLoggedIn())
			User = PlayerList[0];
	}

	// Spawn the request task
	UILLBackendHTTPRequestTask* RequestTask = NewObject<UILLBackendHTTPRequestTask>(this, RequestClass);
	RequestTask->Endpoint = Endpoint;
	RequestTask->User = User;

	if (PrepareRequest(RequestTask))
	{
		UE_LOG(LogILLBackend, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());
		return RequestTask;
	}

	return nullptr;
}

UILLBackendSimpleHTTPRequestTask* UILLBackendRequestManager::CreateSimpleRequest(const FILLBackendEndpointHandle& Endpoint, FOnILLBackendSimpleRequestDelegate Callback, const FString& Params/* = FString()*/, UILLBackendUser* User/* = nullptr*/, const uint32 RetryCount/* = 0*/)
{
	if (UILLBackendSimpleHTTPRequestTask* RequestTask = CreateBackendRequest<UILLBackendSimpleHTTPRequestTask>(Endpoint, User))
	{
		RequestTask->RetryCount = RetryCount;
		RequestTask->URLParameters = Params;
		RequestTask->CompletionDelegate = Callback;
		return RequestTask;
	}

	return nullptr;
}

bool UILLBackendRequestManager::StartAuthRequestSequence(UILLLocalPlayer* LocalPlayer, FOnILLBackendAuthCompleteDelegate Callback)
{
	// Must have a platform login first
	if (LocalPlayer && UKismetSystemLibrary::IsLoggedIn(LocalPlayer->PlayerController))
	{
		IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
		if (OnlineIdentity.IsValid())
		{
			TSharedPtr<const FUniqueNetId> UniqueId = OnlineIdentity->GetUniquePlayerId(LocalPlayer->GetControllerId());
			if (UniqueId.IsValid())
			{
				// Fill out a request task and queue it
				if (UILLBackendAuthHTTPRequestTask* RequestTask = CreateBackendRequest<UILLBackendAuthHTTPRequestTask>(AuthService_Login))
				{
					RequestTask->Nickname = OnlineIdentity->GetPlayerNickname(*UniqueId);
					RequestTask->PlatformId = UniqueId->ToString();
					RequestTask->LocalPlayer = LocalPlayer;
					RequestTask->CompletionDelegate = Callback;
					RequestTask->QueueRequest();
					return true;
				}
			}
		}
	}

	Callback.Execute(EILLBackendAuthResult::NoPlatformLogin);
	return false;
}

void UILLBackendRequestManager::SendArbiterHeartbeatRequest(UILLBackendPlayer* Player, FOnILLBackendArbiterHeartbeatCompleteDelegate Callback)
{
	if (UILLBackendArbiterHeartbeatHTTPRequestTask* RequestTask = CreateBackendRequest<UILLBackendArbiterHeartbeatHTTPRequestTask>(ArbiterService_Heartbeat, Player))
	{
		RequestTask->User = Player;
		if (UWorld* World = GetWorld())
		{
			RequestTask->MapName = World->GetMapName();
		}
		RequestTask->CompletionDelegate = Callback;
		RequestTask->QueueRequest();
	}
}

EILLBackendLogoutResult UILLBackendRequestManager::LogoutPlayer(UILLBackendPlayer* Player)
{
	UE_LOG(LogILLBackend, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	if (Player)
	{
		if (PlayerList.Contains(Player))
		{
			// Log the existing user out
			Player->NotifyLoggedOut();
			PlayerList.Remove(Player);
			Player->MarkPendingKill();
			Player->GetOuterUILLLocalPlayer()->SetCachedBackendPlayer(nullptr);
			return EILLBackendLogoutResult::Success;
		}
		else
		{
			return EILLBackendLogoutResult::UserAlreadyLoggedOut;
		}
	}

	return EILLBackendLogoutResult::NoUserProvided;
}

bool UILLBackendRequestManager::RequestMatchmakingStatus(UILLBackendUser* User, const TArray<FILLBackendPlayerIdentifier> AccountIdList, FOnILLBackendMatchmakingStatusDelegate Callback)
{
	if (AccountIdList.Num() <= 0)
		return false;

	// Fake completion when not configured
	if (!ProfileAuthority_MatchmakingStatus)
	{
		Callback.ExecuteIfBound(TArray<FILLBackendMatchmakingStatus>());
		return true;
	}

	if (UILLBackendMatchmakingStatusHTTPRequestTask* RequestTask = CreateBackendRequest<UILLBackendMatchmakingStatusHTTPRequestTask>(ProfileAuthority_MatchmakingStatus, User))
	{
		RequestTask->AccountIdList = AccountIdList;
		RequestTask->CompletionDelegate = Callback;
		RequestTask->QueueRequest();
		return true;
	}

	return false;
}

bool UILLBackendRequestManager::SendKeySessionRetrieveRequest(const FString& KeySessionId, FOnILLBackendKeySessionDelegate& Callback, const FOnEncryptionKeyResponse& Delegate)
{
	if (UILLBackendKeySessionRetrieveHTTPRequestTask* RequestTask = CreateBackendRequest<UILLBackendKeySessionRetrieveHTTPRequestTask>(KeyDistributionService_Retrieve))
	{
		RequestTask->KeySessionId = KeySessionId;
		RequestTask->EncryptionDelegate = Delegate;
		RequestTask->CompletionDelegate = Callback;
		RequestTask->QueueRequest();
		return true;
	}

	return false;
}

bool UILLBackendRequestManager::SendServerReport(const FString& ReportBuffer, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (UILLBackendSimpleHTTPRequestTask* RequestTask = CreateSimpleRequest(InstanceAuthority_Report, Callback))
	{
		RequestTask->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		RequestTask->SetContentAsString(ReportBuffer);
		RequestTask->QueueRequest();
		return true;
	}

	return false;
}

bool UILLBackendRequestManager::SendServerHeartbeat(const FString& HeartbeatBuffer, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (UILLBackendSimpleHTTPRequestTask* RequestTask = CreateSimpleRequest(InstanceAuthority_Heartbeat, Callback))
	{
		RequestTask->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		RequestTask->SetContentAsString(HeartbeatBuffer);
		RequestTask->QueueRequest();
		return true;
	}

	return false;
}

void UILLBackendRequestManager::ClearActiveXBLSessionUri()
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Clear Pending too, in case UILLBackendPutXBLSessionHTTPRequestTask is still processing
	PendingXBLSessionName.Empty();
	ActiveXBLSessionUri.Empty();
}

bool UILLBackendRequestManager::SendXBLServerSessionRequest(const FString& InitiatorXUID, const FString& SandboxId, FOnILLPutXBLSessionCompleteDelegate CompletionDelegate)
{
	if (!ensureAlways(PendingXBLSessionName.IsEmpty()) || !ensureAlways(ActiveXBLSessionUri.IsEmpty()))
		return false;

	if (UILLBackendPutXBLSessionHTTPRequestTask* RequestTask = CreateBackendRequest<UILLBackendPutXBLSessionHTTPRequestTask>(ArbiterService_XboxServerSession))
	{
		PendingXBLSessionName = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);

		RequestTask->CompletionDelegate = CompletionDelegate;
		RequestTask->InitiatorXUID = InitiatorXUID;
		RequestTask->SandboxId = SandboxId;
		RequestTask->SessionName = PendingXBLSessionName;
		RequestTask->QueueRequest();
		return true;
	}

	return false;
}
