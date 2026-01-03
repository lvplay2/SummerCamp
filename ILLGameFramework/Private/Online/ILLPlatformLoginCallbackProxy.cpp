// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlatformLoginCallbackProxy.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLPlayerController.h"

UILLPlatformLoginCallbackProxy::UILLPlatformLoginCallbackProxy(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, WorldContextObject(nullptr)
, LoginUIClosedDelegate(FOnLoginUIClosedDelegate::CreateUObject(this, &ThisClass::OnShowLoginUICompleted))
, AutoLoginDelegate(FOnlineAutoLoginComplete::CreateUObject(this, &ThisClass::OnAutoLoginComplete))
, PrivilegesCollectedDelegate(FILLOnPrivilegesCollectedDelegate::CreateUObject(this, &ThisClass::OnPrivilegeCheckCompleted))
{
}

UILLPlatformLoginCallbackProxy* UILLPlatformLoginCallbackProxy::ShowPlatformLoginCheckEntitlements(UObject* WorldContextObject, AILLPlayerController* PlayerController, const bool bForceLoginUI/* = false*/)
{
	UILLPlatformLoginCallbackProxy* Proxy = NewObject<UILLPlatformLoginCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->bForceLoginUI = bForceLoginUI;
	return Proxy;
}

void UILLPlatformLoginCallbackProxy::Activate()
{
	if (!PlayerControllerWeakPtr.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("A player controller must be provided in order to show the external login UI."), ELogVerbosity::Warning);
		OnLoginFailed.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(World);
	if (!OnlineSub)
	{
		FFrame::KismetExecutionMessage(TEXT("External UI not supported by the current online subsystem"), ELogVerbosity::Warning);
		OnLoginFailed.Broadcast(PlayerControllerWeakPtr.Get());
		return;
	}

	AILLPlayerController* PlayerController = PlayerControllerWeakPtr.Get();
	UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->Player);
	if (LocalPlayer == nullptr)
	{
		FFrame::KismetExecutionMessage(TEXT("Can only show login UI for local players"), ELogVerbosity::Warning);
		OnLoginFailed.Broadcast(PlayerController);
		return;
	}

	// Notify the local player what we're doing
	LocalPlayer->BeginPlatformLogin();

	// Not already logged in, prompt the user
	bool bWaitForDelegate = false;
	if (!PlayerController->GetWorld()->IsPlayInEditor() && (bForceLoginUI || !UKismetSystemLibrary::IsLoggedIn(PlayerController)))
	{
		bWaitForDelegate = OnlineSub->GetExternalUIInterface()->ShowLoginUI(LocalPlayer->GetControllerId(), false, false, LoginUIClosedDelegate);
	}
	AddToRoot();
	if (!bWaitForDelegate)
	{
		// Fake a completion, this platform does not support ShowLoginUI and so we need to skip the privilege checking
		OnShowLoginUICompleted(LocalPlayer->GetPreferredUniqueNetId(), LocalPlayer->GetControllerId());
	}
}

void UILLPlatformLoginCallbackProxy::OnShowLoginUICompleted(TSharedPtr<const FUniqueNetId> UniqueId, int LocalPlayerNum)
{
	PendingUniqueId = UniqueId;

	AILLPlayerController* PlayerController = PlayerControllerWeakPtr.Get();
	if (!UOnlineEngineInterface::Get()->AutoLogin(PlayerController->GetWorld(), LocalPlayerNum, AutoLoginDelegate))
	{
		RemoveFromRoot();
		OnLoginFailed.Broadcast(PlayerControllerWeakPtr.Get());
	}
}

void UILLPlatformLoginCallbackProxy::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error)
{
	AILLPlayerController* PlayerController = PlayerControllerWeakPtr.Get();

	if (!PendingUniqueId.IsValid() && PlayerController)
	{
		// User didn't select a different account, so log in as them
		UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer());
		PendingUniqueId = LocalPlayer->GetPreferredUniqueNetId();
	}

	if (PendingUniqueId.IsValid() && PlayerController && UOnlineEngineInterface::Get()->IsLoggedIn(PlayerController->GetWorld(), LocalUserNum))
	{
		UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer());
		LocalPlayer->CollectUserPrivileges(PendingUniqueId, PrivilegesCollectedDelegate);
	}
	else
	{
		RemoveFromRoot();
		OnLoginFailed.Broadcast(PlayerControllerWeakPtr.Get());
	}
}

void UILLPlatformLoginCallbackProxy::OnPrivilegeCheckCompleted(UILLLocalPlayer* LocalPlayer)
{
	RemoveFromRoot();

	if (LocalPlayer->HasCanPlayPrivilege())
	{
		LocalPlayer->OnPlatformLoginComplete(PendingUniqueId);
		OnSuccess.Broadcast(PlayerControllerWeakPtr.Get());
	}
	else
	{
		OnPrivilegesFailed.Broadcast(PlayerControllerWeakPtr.Get());
	}
}
