// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLLocalPlayer.h"

#include "GameFramework/SaveGame.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLBackendRequestManager.h"
#include "ILLBackendPlayer.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLGameOnlineBlueprintLibrary.h"
#include "ILLGameViewportClient.h"
#include "ILLHUD.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerController.h"
#include "ILLPlayerSettingsSaveGame.h"
#include "ILLPlayerState.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLLocalPlayer"

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> CVarEnableAllEntitlements(
	TEXT("ill.EnableAllEntitlements"),
	0,
	TEXT("Forces all entitlements to enabled.\n")
	TEXT(" 0: Regular entitment checking behavior\n")
	TEXT(" 1: Enables all entitlements")
);
#endif

UILLLocalPlayer::UILLLocalPlayer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ReLogAuthenticationDelay = .1f;

	SettingsSaveGameTick.Target = this;
	SettingsSaveGameTick.bTickEvenWhenPaused = true;
	SettingsSaveGameTick.bCanEverTick = true;
	SettingsSaveGameTick.bStartWithTickEnabled = false;
	SettingsSaveGameTick.bRunOnAnyThread = false;
	SettingsSaveGameTick.SetTickFunctionEnable(false);
}

void UILLLocalPlayer::PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID)
{
	Super::PlayerAdded(InViewportClient, InControllerID);

	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (OnlineIdentity.IsValid())
	{
		// Listen for controller connection and pairing changes
		OnControllerConnectionChangedDelegateHandle = FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &ThisClass::OnControllerConnectionChanged);
		OnControllerPairingChangedDelegateHandle = OnlineIdentity->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &ThisClass::OnControllerPairingChanged));
	}
}

void UILLLocalPlayer::PlayerRemoved()
{
	Super::PlayerRemoved();

	// Stop listening for controller connection and pairing changes
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	FCoreDelegates::OnControllerConnectionChange.Remove(OnControllerConnectionChangedDelegateHandle);
	OnControllerConnectionChangedDelegateHandle.Reset();
	if (OnlineIdentity.IsValid())
		OnlineIdentity->ClearOnControllerPairingChangedDelegate_Handle(OnControllerPairingChangedDelegateHandle);
	OnControllerPairingChangedDelegateHandle.Reset();
}

FString UILLLocalPlayer::GetGameLoginOptions() const
{
	FString Result = Super::GetGameLoginOptions();

	if (BackendPlayer)
	{
		// Send our AccountId
		const FILLBackendPlayerIdentifier& AccountId = BackendPlayer->GetAccountId();
		if (AccountId.IsValid())
		{
			if (!Result.IsEmpty())
				Result += TEXT("?");
			Result += TEXT("AccountId=");
			Result += AccountId.GetIdentifier();
		}
	}

	return Result;
}

void UILLLocalPlayer::SetControllerId(int32 NewControllerId)
{
	// Get our controller Id before the super changes it
	const int32 PreviousControllerId = GetControllerId();

	Super::SetControllerId(NewControllerId);

	// See if the super actually changed our controller Id
	const int32 CurrentControllerId = GetControllerId();
	if (PreviousControllerId != CurrentControllerId)
	{
		// Make sure slate is always using our current controller
		if (FSlateApplication::Get().KeyboardIndexOverride == PreviousControllerId)
			FSlateApplication::Get().KeyboardIndexOverride = CurrentControllerId;
	}
}

void UILLLocalPlayer::OnPreLoadMap(const FString& MapName)
{
	// Unregister our SettingsSaveGameTick before changing levels, since it binds to a world
	if (SettingsSaveGameTick.IsTickFunctionEnabled())
	{
		UnRegisterSettingsSaveGameTickFunction();
	}
}

void UILLLocalPlayer::OnPostLoadMap(UWorld* World)
{
	// Register our SettingsSaveGameTick again if needed
	if (RunningSettingsSaveGameTask || PendingSettingsSaveGameTasks.Num() > 0)
	{
		RegisterSettingsSaveGameTickFunction();
	}
}

bool UILLLocalPlayer::IsLoadingSettingsSaveGame() const
{
	// Wait until we are logged in
	if (!HasPlatformLogin())
		return false;

	// When we have nothing loaded at all, assume we are pending save game loads
	if (SettingsSaveGameCache.Num() == 0)
		return true;

	// Check for any load tasks
	if (RunningSettingsSaveGameTask && RunningSettingsSaveGameTask->IsLoadRunnable())
		return true;

	for (FILLPlayerSettingsSaveGameRunnable* PendingTask : PendingSettingsSaveGameTasks)
	{
		if (PendingTask->IsLoadRunnable())
			return true;
	}

	return false;
}

UILLPlayerSettingsSaveGame* UILLLocalPlayer::CreateSettingsSave(TSubclassOf<UILLPlayerSettingsSaveGame> SaveGameClass)
{
	// This is simply calling NewObject so it does not need threading
	UILLPlayerSettingsSaveGame* NewSaveGame = NewObject<UILLPlayerSettingsSaveGame>(this, SaveGameClass);
	NewSaveGame->SaveGameNewlyCreated(CastChecked<AILLPlayerController>(PlayerController));
	return NewSaveGame;
}

UILLPlayerSettingsSaveGame* UILLLocalPlayer::GetLoadedSettingsSave(TSubclassOf<UILLPlayerSettingsSaveGame> SaveGameClass)
{
	if (!SaveGameClass)
		return nullptr;

	// Check for a cached previously loaded or saved SaveGame
	if (UILLPlayerSettingsSaveGame** CachedSave = SettingsSaveGameCache.FindByPredicate([&](UILLPlayerSettingsSaveGame* Entry) { return Entry->IsA(SaveGameClass); }))
	{
		return *CachedSave;
	}

	return nullptr;
}

void UILLLocalPlayer::QueuePlayerSettingsSaveGameWrite(UILLPlayerSettingsSaveGame* SaveGame)
{
	// Take SaveGame into the cache for the slot name
	if (!SettingsSaveGameCache.Contains(SaveGame))
	{
		SettingsSaveGameCache.RemoveAll([&](UILLPlayerSettingsSaveGame* Entry) { return Entry->IsA(SaveGame->GetClass()); });
		SettingsSaveGameCache.Add(SaveGame);
	}

	QueuePlayerSettingsSaveGameWrite(SaveGame->SaveType);
}

void UILLLocalPlayer::QueuePlayerSettingsSaveGameWrite(const EILLPlayerSettingsSaveType SaveType)
{
	// Only queue a new write for this SaveType if there is not one already pending
	const bool bAlreadyPendingWrite = PendingSettingsSaveGameTasks.ContainsByPredicate([&](FILLPlayerSettingsSaveGameRunnable* PendingRunnable) -> bool
		{
			return (!PendingRunnable->IsLoadRunnable() && PendingRunnable->GetSaveGameType() == SaveType);
		}
	);
	if (!bAlreadyPendingWrite)
	{
		FILLPlayerWriteSaveGameRunnable* QueuedRunnable = new FILLPlayerWriteSaveGameRunnable(this, SaveType);
		PendingSettingsSaveGameTasks.Add(QueuedRunnable);
		RegisterSettingsSaveGameTickFunction();
	}
}

void UILLLocalPlayer::RegisterSettingsSaveGameTickFunction()
{
	// TODO: pjackson: Switch to the CoreTicker and duck micro-managing this across map changes
	if (UWorld* World = GetWorld())
	{
		SettingsSaveGameTick.RegisterTickFunction(World->GetCurrentLevel());
		SettingsSaveGameTick.SetTickFunctionEnable(true);
	}
}

void UILLLocalPlayer::UnRegisterSettingsSaveGameTickFunction()
{
	SettingsSaveGameTick.SetTickFunctionEnable(false);
	SettingsSaveGameTick.UnRegisterTickFunction();
}

void UILLLocalPlayer::FILLSaveGameTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (Target)
	{
		Target->CheckSettingsSaveGameTask();
	}
}

void UILLLocalPlayer::LoadAndApplySaveGameSettings()
{
	SettingsSaveGameCache.Empty();

	for (uint8 Index = 0; Index < static_cast<uint8>(EILLPlayerSettingsSaveType::MAX); ++Index)
	{
		// Load from disk
		FILLPlayerLoadSaveGameRunnable* QueuedRunnable = new FILLPlayerLoadSaveGameRunnable(this, static_cast<EILLPlayerSettingsSaveType>(Index));
		PendingSettingsSaveGameTasks.Add(QueuedRunnable);
		RegisterSettingsSaveGameTickFunction();
	}
}

void UILLLocalPlayer::CheckSettingsSaveGameTask()
{
	// Is our RunningSettingsSaveGameTask completed?
	bool bWasLoading = false;
	if (!RunningSettingsSaveGameTask || RunningSettingsSaveGameTask->HasCompletedAsyncWork())
	{
		// Can we complete it yet? May be changing levels
		AILLPlayerController* PC = Cast<AILLPlayerController>(PlayerController);
		if (IsValid(GetWorld()) && !GetWorld()->IsInSeamlessTravel() && IsValid(PC) && PC->HasFullyTraveled())
		{
			if (RunningSettingsSaveGameTask)
			{
				// Cleanup the runnable
				bWasLoading = RunningSettingsSaveGameTask->IsLoadRunnable();
				RunningSettingsSaveGameTask->FinishWork();
				delete RunningSettingsSaveGameTask;
				RunningSettingsSaveGameTask = nullptr;
			}

			if (PendingSettingsSaveGameTasks.Num() > 0)
			{
				// Consume the first entry
				RunningSettingsSaveGameTask = PendingSettingsSaveGameTasks[0];
				PendingSettingsSaveGameTasks.RemoveAt(0);
				RunningSettingsSaveGameTask->StartWork(SettingsSaveGameCounter++);
			}
		}
	}

	// Check for completion
	if (!RunningSettingsSaveGameTask && PendingSettingsSaveGameTasks.Num() == 0)
	{
		// Cleanup our tick function
		UnRegisterSettingsSaveGameTickFunction();

		if (bWasLoading)
		{
			// Attempt to verify entitlement setting ownership
			ConditionalVerifyEntitlementSettingsOwnership();

			// Attempt to verify backend setting ownership
			ConditionalVerifyBackendSettingsOwnership();
		}
	}
}

void UILLLocalPlayer::CollectUserPrivileges(TSharedPtr<const FUniqueNetId> PendingUniqueId, const FILLOnPrivilegesCollectedDelegate& Delegate/* = FILLOnPrivilegesCollectedDelegate()*/)
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::CollectUserPrivileges: %s"), *GetName(), SafeUniqueIdTChar(*PendingUniqueId));

	if (!PendingUniqueId.IsValid())
	{
		Delegate.ExecuteIfBound(this);
		return;
	}

	// Now collect all privileges
	OnPrivilegesCollectedDelegate = Delegate;
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (OnlineIdentity.IsValid())
	{
		PendingPrivilegeChecks = 4; // CLEANUP: pjackson: Ew.
		OnlineIdentity->GetUserPrivilege(*PendingUniqueId, EUserPrivileges::CanPlay, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &ThisClass::OnPrivilegeCheckComplete));
		OnlineIdentity->GetUserPrivilege(*PendingUniqueId, EUserPrivileges::CanPlayOnline, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &ThisClass::OnPrivilegeCheckComplete));
		OnlineIdentity->GetUserPrivilege(*PendingUniqueId, EUserPrivileges::CanCommunicateOnline, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &ThisClass::OnPrivilegeCheckComplete));
		OnlineIdentity->GetUserPrivilege(*PendingUniqueId, EUserPrivileges::CanUseUserGeneratedContent, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &ThisClass::OnPrivilegeCheckComplete));
	}
}

void UILLLocalPlayer::RefreshPlayOnlinePrivilege()
{
	if (bIsCheckingPrivilege || !HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::RefreshPlayOnlinePrivilege"), *GetName());

	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	bIsCheckingPrivilege = true;
	OnlineIdentity->GetUserPrivilege(*GetCachedUniqueNetId(), EUserPrivileges::CanPlayOnline, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &ThisClass::OnPrivilegeCheckComplete));
}

void UILLLocalPlayer::ResetPrivileges()
{
	PlayOnlinePrivilegeResult = IOnlineIdentity::EPrivilegeResults::UserNotLoggedIn;
	bHasCanPlayPrivilege = false;
	bHasCanPlayOnlinePrivilege = false;
	bHasCanCommunicateOnlinePrivilege = false;
	bHasCanUseUserGeneratedContentPrivilege = false;
}

void UILLLocalPlayer::OnPrivilegeCheckComplete(const FUniqueNetId& UniqueId, EUserPrivileges::Type Privilege, uint32 PrivilegeResult)
{
	const bool bHasPrivilege = (PrivilegeResult == static_cast<uint32>(IOnlineIdentity::EPrivilegeResults::NoFailures));
	if (bHasPrivilege)
	{
		// It's just going to say 0. But you asked.
		UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnPrivilegeCheckComplete: Privilege: %i Result %u"), *GetName(), static_cast<uint8>(Privilege), PrivilegeResult);
	}
	else
	{
		// In reality this should only be a warning, but LogPlayerManagement only displays errors by default, and this is super useful for debugging so....
		UE_LOG(LogPlayerManagement, Error, TEXT("%s::OnPrivilegeCheckComplete: Privilege: %i Result %u"), *GetName(), static_cast<uint8>(Privilege), PrivilegeResult);
	}

	switch (Privilege)
	{
	case EUserPrivileges::CanPlay: bHasCanPlayPrivilege = bHasPrivilege; break;
	case EUserPrivileges::CanPlayOnline:
		bHasCanPlayOnlinePrivilege = bHasPrivilege;
		PlayOnlinePrivilegeResult = static_cast<IOnlineIdentity::EPrivilegeResults>(PrivilegeResult);
		break;
	case EUserPrivileges::CanCommunicateOnline: bHasCanCommunicateOnlinePrivilege = bHasPrivilege; break;
	case EUserPrivileges::CanUseUserGeneratedContent: bHasCanUseUserGeneratedContentPrivilege = bHasPrivilege; break;
	}

	if (PendingPrivilegeChecks > 0 && --PendingPrivilegeChecks == 0)
	{
		// All done checking our privilege
		OnPrivilegesCollectedDelegate.ExecuteIfBound(this);
		OnPrivilegesCollectedDelegate = FILLOnPrivilegesCollectedDelegate();
	}
	else if (bIsCheckingPrivilege)
	{
		bIsCheckingPrivilege = false;

		// CLEANUP: pjackson: We only really want this in some cases, not all...
		// Check for any invites pending processing, since CheckDeferredInvite will block until privilege checks complete
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		OSC->QueueFlushDeferredInvite();
	}
}

void UILLLocalPlayer::OnApplicationWillDeactivate()
{
	if (!HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnApplicationWillDeactivate"), *GetName());
}

void UILLLocalPlayer::OnApplicationHasReactivated()
{
	if (!HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnApplicationHasReactivated"), *GetName());

#if PLATFORM_PS4
	if (!bHasCanPlayOnlinePrivilege)
	{
		// Update our CanPlayOnline status in case the user decided to gain access while the game was in the background
		// We only fire this on PS4 because calling it on XB1 will display the Gold nag
		RefreshPlayOnlinePrivilege();
	}
#endif

	// Update entitlements just in case they minimized and bought some stuffs
	QueryEntitlements();
}

void UILLLocalPlayer::OnReLogAuthComplete(EILLBackendAuthResult AuthResult)
{
	// Do nothing if we have signed out or somehow not signed in since this request was sent
	if (!HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnReLogAuthComplete: AuthResult: %i"), *GetName(), static_cast<uint8>(AuthResult));

	if (AuthResult == EILLBackendAuthResult::Success)
	{
		// Update entitlements when we re-login in case the player bought some stuff while the game was suspended
		QueryEntitlements();
	}
	else
	{
		// Disconnect and sign out
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		OSC->PerformSignoutAndDisconnect(this, FText(LOCTEXT("BackendReAuthFailure", "Could not re-establish database connection! Verify internet connectivity.")));
	}
}

void UILLLocalPlayer::OnApplicationWillEnterBackground()
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnApplicationWillEnterBackground"), *GetName());

	ProcessApplicationEnteredBackground();
}

void UILLLocalPlayer::ProcessApplicationEnteredBackground()
{
	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GI->GetOnlineSession());
	UWorld* World = GetWorld();
	if (!World)
	{
		// Keep retrying until loading finally stops
		GI->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ProcessApplicationEnteredBackground);
		return;
	}

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::ProcessApplicationEnteredBackground"), *GetName());

	if (World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer)
	{
		if (AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>())
		{
			// We are hosting a game session
			// Kick all players
			if (GameMode->GameSession)
			{
				for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					GameMode->GameSession->KickPlayer((*Iterator).Get(), FText(LOCTEXT("GameHostDeactivatedKickMessage", "Host suspended their game!")));
				}
			}
		}
	}
	else if (World->GetNetMode() == NM_Client)
	{
		// Client on a server, leave the game immediately
		// Need to do this immediately so that we disconnect and not take up game space
		OSC->PerformSignoutAndDisconnect(this, FText(LOCTEXT("GameClientDeactivatedSignoutMessage", "Game suspended while connected to a server. Please sign in again.")));
	}
	else if (World->GetNetMode() == NM_Standalone)
	{
		// We are at a main menu
		if (GetBackendPlayer() && GetBackendPlayer()->IsLoggedIn())
		{
			// Log us out of the backend
			GI->GetBackendRequestManager()->LogoutPlayer(GetBackendPlayer());
		}
	}
}

void UILLLocalPlayer::OnApplicationHasEnteredForeground()
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnApplicationHasEnteredForeground"), *GetName());

	ProcessApplicationEnteredForeground();
}

void UILLLocalPlayer::ProcessApplicationEnteredForeground()
{
	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());
	UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GI->GetOnlineSession());
	UWorld* World = GetWorld();
	if (!World)
	{
		// Keep retrying until loading finally stops
		GI->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ProcessApplicationEnteredForeground);
		return;
	}

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::ProcessApplicationEnteredForeground"), *GetName());

	if (World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer)
	{
		// Sign out of the backend and return to the title screen with an error message
		OSC->PerformSignoutAndDisconnect(this, FText(LOCTEXT("GameHostReactivatedApp", "Title was suspended, game session ended.")));
	}
	else if (World->GetNetMode() == NM_Standalone)
	{
		if (HasPlatformLogin())
		{
			// Wait a small amount of time before attempting to re-authenticate in case a connection status or controller pairing change comes in that signs us out
			GI->GetTimerManager().SetTimer(TimerHandle_BeginStandaloneReLogAuth, this, &ThisClass::BeginStandaloneReLogAuth, ReLogAuthenticationDelay);
		}
	}
}

void UILLLocalPlayer::BeginStandaloneReLogAuth()
{
	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());

	// Cleanup our timer
	if (TimerHandle_BeginStandaloneReLogAuth.IsValid())
	{
		GI->GetTimerManager().ClearTimer(TimerHandle_BeginStandaloneReLogAuth);
		TimerHandle_BeginStandaloneReLogAuth.Invalidate();
	}

	// Do nothing if we have signed out or somehow not signed in since this timer was queued
	if (!HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::BeginStandaloneReLogAuth"), *GetName());

	// Start a re-auth
	GI->GetBackendRequestManager()->StartAuthRequestSequence(this, FOnILLBackendAuthCompleteDelegate::CreateUObject(this, &ThisClass::OnReLogAuthComplete));
}

void UILLLocalPlayer::OnExternalUIChanged(bool bIsOpening)
{
	if (!HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnExternalUIChanged: %s"), *GetName(), bIsOpening ? TEXT("true") : TEXT("false"));

#if PLATFORM_PS4
	// Update PlayOnline privilege if we can't right now
	// This is entirely PS4 specific -- updating the PSNPlus status for the current user in case they bought it in the external UI
	if (!bIsOpening && !bHasCanPlayOnlinePrivilege)
	{
		RefreshPlayOnlinePrivilege();
	}
#endif
}

void UILLLocalPlayer::OnConnectionStatusChanged(EOnlineServerConnectionStatus::Type LastConnectionState, EOnlineServerConnectionStatus::Type ConnectionState)
{
	if (!HasPlatformLogin())
		return;

	UE_LOG(LogPlayerManagement, Log, TEXT("%s::OnConnectionStatusChanged: LastConnectionState: %s, ConnectionState: %s"), *GetName(), EOnlineServerConnectionStatus::ToString(LastConnectionState), EOnlineServerConnectionStatus::ToString(ConnectionState));

	check(ConnectionState != EOnlineServerConnectionStatus::Normal); // As far as I can tell this is only ever set to LastConnectionState
	if (ConnectionState == EOnlineServerConnectionStatus::Connected)
	{
		if (LastConnectionState != EOnlineServerConnectionStatus::Connected)
		{
			CollectUserPrivileges(GetCachedUniqueNetId());
		}

		return;
	}

	// If we're in a standalone game we don't care about losing internet connectivity
	if (UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionStandalone(this))
	{
		return;
	}

	// Localize the OSS name
	FText OSSNameText;
	if (IOnlineSubsystem* SteamSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM))
	{
		OSSNameText = LOCTEXT("OnlineSubsystemName_Steam", "Steam");
	}
	else if (IOnlineSubsystem* LiveSubsystem = IOnlineSubsystem::Get(LIVE_SUBSYSTEM))
	{
		OSSNameText = LOCTEXT("OnlineSubsystemName_Live", "Xbox Live");
	}
	else if (IOnlineSubsystem* PSNSubsystem = IOnlineSubsystem::Get(PS4_SUBSYSTEM))
	{
		OSSNameText = LOCTEXT("OnlineSubsystemName_PS4", "PlayStation(TM)Network");
	}
	else
	{
		check(0);
	}

	// Queue a failure message
	FText FailureText;
	if (!OSSNameText.IsEmpty())
	{
		switch (ConnectionState)
		{
		case EOnlineServerConnectionStatus::NotConnected: FailureText = FText::Format(LOCTEXT("ConnectionFailure_Lost", "Internet connection lost! Reconnect and sign in to {0} to use online features."), OSSNameText); break;
		case EOnlineServerConnectionStatus::ConnectionDropped: FailureText = FText::Format(LOCTEXT("ConnectionFailure_Dropped", "Internet connection dropped! Reconnect and sign in to {0} to use online features."), OSSNameText); break;
		case EOnlineServerConnectionStatus::NoNetworkConnection: FailureText = FText::Format(LOCTEXT("ConnectionFailure_NoNetworkConnection", "No network connection or connection lost! Reconnect and sign in to {0} to use online features."), OSSNameText); break;
		case EOnlineServerConnectionStatus::ServiceUnavailable: FailureText = FText::Format(LOCTEXT("ConnectionFailure_ServiceUnavailable", "{0} connection unavailable or lost! You need to be signed in to {0} to use online features."), OSSNameText); break;
		case EOnlineServerConnectionStatus::UpdateRequired: FailureText = FText::Format(LOCTEXT("ConnectionFailure_UpdateRequired", "{0} or game update needed! Make sure you have the latest updates."), OSSNameText); break;
		case EOnlineServerConnectionStatus::ServersTooBusy: FailureText = FText::Format(LOCTEXT("ConnectionFailure_ServersTooBusy", "{0} servers are too busy!"), OSSNameText); break;
		case EOnlineServerConnectionStatus::DuplicateLoginDetected: FailureText = FText::Format(LOCTEXT("ConnectionFailure_DuplicateLoginDetected", "Duplicate {0} login detected!"), OSSNameText); break;
		case EOnlineServerConnectionStatus::InvalidUser: FailureText = FText::Format(LOCTEXT("ConnectionFailure_InvalidUser", "Invalid or unknown {0} user!"), OSSNameText); break;
		case EOnlineServerConnectionStatus::NotAuthorized: FailureText = FText::Format(LOCTEXT("ConnectionFailure_NotAuthorized", "Not authorized to use {0} services!"), OSSNameText); break;
	//	case EOnlineServerConnectionStatus::InvalidSession: FailureText = FText(LOCTEXT("ConnectionFailure_InvalidSession", "Session has been lost on the backend!")); break;
		default: FailureText = FText::Format(LOCTEXT("ConnectionFailure_Default", "Connection failure: {0}!"), FText::FromString(EOnlineServerConnectionStatus::ToString(ConnectionState))); break;
		}
	}

	UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
	OSC->PerformSignoutAndDisconnect(this, FailureText);
}

void UILLLocalPlayer::BeginPlatformLogin()
{
	// Force an ID refresh in case we're re-using this LocalPlayer
	SetCachedUniqueNetId(nullptr);

	// Reset privileges
	ResetPrivileges();

	// HACK: Assign our KeyboardIndexOverride and ControllerId to the last key down event
	if (UILLGameViewportClient* VPC = Cast<UILLGameViewportClient>(GetGameInstance()->GetGameViewportClient()))
	{
		const int32 NewUserIndex = VPC->LastKeyDownEvent.GetUserIndex();
		FSlateApplication::Get().KeyboardIndexOverride = NewUserIndex;
		SetControllerId(NewUserIndex);
	}
}

void UILLLocalPlayer::OnPlatformLoginComplete(TSharedPtr<const FUniqueNetId> NewUniqueId)
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnPlatformLoginComplete: %s"), *GetName(), SafeUniqueIdTChar(*NewUniqueId));

	bCompletedPlatformLogin = true;
	bPendingSignout = false;

	// Update the cached unique ID
	SetCachedUniqueNetId(NewUniqueId);

	UWorld* World = GetWorld();

	// New id, new controller?
	if (NewUniqueId.IsValid() && !IsCachedUniqueNetIdPairedWithControllerId())
	{
		// TODO: SOMEJERK: Replace magic number 8
		// 8 is the max number of controllers for Xbox/PS4, so let's check them all!
		for (int32 ControllerIndex = 0; ControllerIndex < 8; ++ControllerIndex)
		{
			TSharedPtr<const FUniqueNetId> UniqueControllerId = UOnlineEngineInterface::Get()->GetUniquePlayerId(World, ControllerIndex);
			if (UniqueControllerId.IsValid() && *UniqueControllerId == *NewUniqueId)
			{
				// Found our id on a controller, update our internal cache
				SetControllerId(ControllerIndex);
				break;
			}
		}
	}

	// Ensure things are synced up correctly
	if (!ensureAlways(IsCachedUniqueNetIdPairedWithControllerId()))
	{
		TSharedPtr<const FUniqueNetId> UniqueIdFromController = GetUniqueNetIdFromCachedControllerId();
		UE_LOG(LogPlayerManagement, Warning, TEXT("%s::OnPlatformLoginComplete: Unique ID mismatch, some online interactions may be unavailable -- Unique ID: %s  Controller Unique ID: %s"), *GetName(), SafeUniqueIdTChar(*CachedUniqueNetId), SafeUniqueIdTChar(*UniqueIdFromController));
	}

	// Update our PlayerState
	if (PlayerController && PlayerController->PlayerState)
	{
		PlayerController->PlayerState->SetUniqueId(NewUniqueId);
		PlayerController->PlayerState->SetPlayerName(GetNickname());
	}

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(World);
	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();

	// Listen for connection status changes
	OnConnectionStatusChangedDelegateHandle = OnlineSubsystem->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &ThisClass::OnConnectionStatusChanged));

	// Start listening for login changes
	OnLoginStatusChangedDelegateHandle = OnlineIdentity->AddOnLoginStatusChangedDelegate_Handle(GetControllerId(), FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::OnLoginStatusChanged));

	// Listen for external UI changes
	if (OnlineExternalUI.IsValid())
		OnExternalUIChangeDelegateHandle = OnlineExternalUI->AddOnExternalUIChangeDelegate_Handle(FOnExternalUIChangeDelegate::CreateUObject(this, &ThisClass::OnExternalUIChanged));

	// Listen for suspend
	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &ThisClass::OnApplicationWillDeactivate);
	FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this, &ThisClass::OnApplicationHasReactivated);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &ThisClass::OnApplicationWillEnterBackground);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &ThisClass::OnApplicationHasEnteredForeground);

	// Listen for map changes
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMap);

	// Cache our online environment
	OnlineEnvironment = OnlineSubsystem->GetOnlineEnvironment();
	if (OnlineEnvironment == EOnlineEnvironment::Unknown)
	{
		// Steam does not supply this, so guess based on build configuration
#if UE_BUILD_SHIPPING
		OnlineEnvironment = EOnlineEnvironment::Production;
#else
		OnlineEnvironment = EOnlineEnvironment::Development;
#endif
	}

	// Load user settings from save games
	LoadAndApplySaveGameSettings();

	// Query DLC info
	QueryEntitlements();
}

void UILLLocalPlayer::PerformSignout()
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::PerformSignout"), *GetName());

	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());

	// Sign out of the backend
	if (UILLBackendPlayer* Player = GetBackendPlayer())
	{
		if (UILLBackendRequestManager* BRM = GI->GetBackendRequestManager())
		{
			BRM->LogoutPlayer(Player);
		}
	}

	// Cleanup the platform login
	SetCachedUniqueNetId(nullptr);
	ResetPrivileges();
	bCompletedPlatformLogin = false;
	bPendingSignout = false;

	// Flush save games
	SettingsSaveGameCache.Empty();

	// Cleanup delegates
	StopListeningForUserChanges();

	// Stop any queued attempt at re-authentication
	if (TimerHandle_BeginStandaloneReLogAuth.IsValid())
	{
		GI->GetTimerManager().ClearTimer(TimerHandle_BeginStandaloneReLogAuth);
		TimerHandle_BeginStandaloneReLogAuth.Invalidate();
	}
}

void UILLLocalPlayer::StopListeningForUserChanges()
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::StopListeningForUserChanges"), *GetName());

	UWorld* World = GetWorld();
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(World);
	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();

	// Stop listening for connection status changes
	OnlineSubsystem->ClearOnConnectionStatusChangedDelegate_Handle(OnConnectionStatusChangedDelegateHandle);
	OnConnectionStatusChangedDelegateHandle.Reset();

	// Stop listening for login status changes
	OnlineIdentity->ClearOnLoginStatusChangedDelegate_Handle(GetControllerId(), OnLoginStatusChangedDelegateHandle);
	OnLoginStatusChangedDelegateHandle.Reset();

	// Stop listening for external UI changes
	if (OnlineExternalUI.IsValid())
		OnlineExternalUI->ClearOnExternalUIChangeDelegate_Handle(OnExternalUIChangeDelegateHandle);

	// Stop listening for suspend
	FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
	FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveAll(this);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);

	// Stop listening for map changes
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// Clear this so the title screen shows again
	GILLUserRequestedBackendOfflineMode = false;
}

void UILLLocalPlayer::OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId)
{
	if (GetControllerId() != LocalUserNum || !HasPlatformLogin())
	{
		UE_LOG(LogPlayerManagement, VeryVerbose, TEXT("%s::OnLoginStatusChanged: LocalUserNum: %i, OldStatus: %s, NewStatus: %s"), *GetName(), LocalUserNum, ELoginStatus::ToString(OldStatus), ELoginStatus::ToString(NewStatus));
		return;
	}

	UE_LOG(LogPlayerManagement, Log, TEXT("%s::OnLoginStatusChanged: LocalUserNum: %i, OldStatus: %s, NewStatus: %s"), *GetName(), LocalUserNum, ELoginStatus::ToString(OldStatus), ELoginStatus::ToString(NewStatus));

	if ((OldStatus == ELoginStatus::LoggedIn && ensure(NewStatus != ELoginStatus::LoggedIn))
	|| (GetCachedUniqueNetId().IsValid() && (!NewId.IsValid() || *GetCachedUniqueNetId() != NewId)))
	{
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GetGameInstance()->GetOnlineSession());
		OSC->PerformSignoutAndDisconnect(this, FText(LOCTEXT("LoginChangedError", "You were signed out!")));
	}
}

void UILLLocalPlayer::OnControllerConnectionChanged(bool bConnected, int32 UserId, int32 LocalUserNum)
{
	// Why is connecting controllers so hard?
	const int32 CurrentControllerId = GetControllerId();
	if (bConnected)
	{
		UE_LOG(LogPlayerManagement, Log, TEXT("%s::OnControllerConnectionChanged: Connected? %s UserId: %i, LocalUserNum: %i -> %i"), *GetName(), bConnected ? TEXT("true") : TEXT("false"), UserId, CurrentControllerId, LocalUserNum);

		if (CurrentControllerId != LocalUserNum)
		{
			// Figure out who this new controller is paired to
			TSharedPtr<const FUniqueNetId> NewUserId = UOnlineEngineInterface::Get()->GetUniquePlayerId(GetWorld(), LocalUserNum);
			if (!NewUserId.IsValid() || !GetCachedUniqueNetId().IsValid() || *NewUserId != *GetCachedUniqueNetId())
			{
				UE_LOG(LogPlayerManagement, Log, TEXT("%s::OnControllerConnectionChanged: New user with the different controller id, ignoring"), *GetName());
				return;
			}

			// Make sure our controller id is up to date
			SetControllerId(LocalUserNum);
		}
		// Different user account, same controller, we don't handle that here
		else if (HasPlatformLogin() && (GetUniqueNetIdFromCachedControllerId().IsValid() != GetCachedUniqueNetId().IsValid() || *GetUniqueNetIdFromCachedControllerId() != *GetCachedUniqueNetId()))
		{
			UE_LOG(LogPlayerManagement, Log, TEXT("%s::OnControllerConnectionChanged: New user with the same controller id, ignoring"), *GetName());
			return;
		}

		// Make sure slate is always using our current controller
		if (FSlateApplication::Get().KeyboardIndexOverride == CurrentControllerId)
			FSlateApplication::Get().KeyboardIndexOverride = LocalUserNum;

		HandleControllerConnected();
	}
	// Only disconnect if this was our controller
	else if (LocalUserNum == CurrentControllerId)
	{
		HandleControllerDisconnected();
	}
}

void UILLLocalPlayer::OnControllerPairingChanged(int32 LocalUserNum, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser)
{
	UE_LOG(LogPlayerManagement, Log, TEXT("%s::OnControllerPairingChanged: LocalUserNum: %i -> %i"), *GetName(), GetControllerId(), LocalUserNum);
	if (GetControllerId() != LocalUserNum || !HasPlatformLogin())
		return;

	if (!NewUser.IsValid() || NewUser != *GetCachedUniqueNetId() || (PreviousUser.IsValid() && PreviousUser == *GetCachedUniqueNetId()))
	{
		HandleControllerDisconnected();
	}
	else if (NewUser == *GetCachedUniqueNetId())
	{
		HandleControllerConnected();
	}
}

void UILLLocalPlayer::HandleControllerDisconnected()
{
	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());
	static const FText ControllerDisplayTextPS4(LOCTEXT("ControllerPairingLostErrorPS4", "The DUALSHOCK(R)4 wireless controller has been disconnected. Please reconnect to continue."));
	static const FText ControllerDisplayText(LOCTEXT("ControllerPairingLostError", "Controller pairing lost!"));
#if PLATFORM_PS4
	GI->OnControllerPairingLost(GetControllerId(), ControllerDisplayTextPS4);
#else
	GI->OnControllerPairingLost(GetControllerId(), ControllerDisplayText);
#endif

	bLostControllerPairing = true;
}

void UILLLocalPlayer::HandleControllerConnected()
{
	UILLGameInstance* GI = Cast<UILLGameInstance>(GetGameInstance());
	GI->OnControllerPairingRegained(GetControllerId());

	bLostControllerPairing = false;
}

void UILLLocalPlayer::SetCachedBackendPlayer(UILLBackendPlayer* NewPlayer)
{
	BackendPlayer = NewPlayer;
}

void UILLLocalPlayer::NotifyBackendPlayerArbitrated()
{
	// Attempt to verify backend setting ownership
	ConditionalVerifyBackendSettingsOwnership();
}

void UILLLocalPlayer::NotifyOfflineMode()
{
	// Attempt to verify backend setting ownership
	ConditionalVerifyBackendSettingsOwnership();
}

void UILLLocalPlayer::ConditionalVerifyBackendSettingsOwnership()
{
	// Wait for save game loading to complete
	if (SettingsSaveGameCache.Num() == 0 || IsLoadingSettingsSaveGame())
		return;

	// Wait for offline mode or backend arbitration
	if (!GILLUserRequestedBackendOfflineMode && (!BackendPlayer || !BackendPlayer->IsLoggedIn()))
		return;

	// Broadcast ownership checks
	for (UILLPlayerSettingsSaveGame* SettingsSave : SettingsSaveGameCache)
	{
		SettingsSave->VerifyBackendSettingsOwnership(GILLUserRequestedBackendOfflineMode);
	}
}

bool UILLLocalPlayer::HasMuted(const AILLPlayerState* PlayerState) const
{
	if (!PlayerState || !PlayerState->UniqueId.IsValid())
		return false;

	return HasMuted(PlayerState->UniqueId);
}

void UILLLocalPlayer::MutePlayer(const AILLPlayerState* PlayerState)
{
	if (!PlayerState || !PlayerState->UniqueId.IsValid())
		return;

	// Store off this mute to persist
	if (!MutedPlayerList.Contains(PlayerState->UniqueId))
	{
		MutedPlayerList.Add(PlayerState->UniqueId);
	}

	// Update the in-game mute immediately
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(PlayerController))
	{
		PC->ServerMutePlayer(PlayerState->UniqueId);
	}
}

void UILLLocalPlayer::UnmutePlayer(const AILLPlayerState* PlayerState)
{
	if (!PlayerState || !PlayerState->UniqueId.IsValid())
		return;

	// Remove them from the persistent mute list
	if (MutedPlayerList.Contains(PlayerState->UniqueId))
	{
		MutedPlayerList.Remove(PlayerState->UniqueId);
	}

	// Update the in-game mute immediately
	if (AILLPlayerController* PC = Cast<AILLPlayerController>(PlayerController))
	{
		PC->ServerUnmutePlayer(PlayerState->UniqueId);
	}
}

void UILLLocalPlayer::ReplicateMutedState(const AILLPlayerState* PlayerState)
{
	if (!HasPlatformLogin())
		return;
	if (!PlayerState || !PlayerState->UniqueId.IsValid())
		return;

	TSharedPtr<const FUniqueNetId> CachedUniqueId = GetCachedUniqueNetId();
	if (*PlayerState->UniqueId == *CachedUniqueId)
	{
		// This is us
		// Replicate the mute state for everyone else
		// We use an actor iterator instead of grabbing the GameState because the GameState may not be replicated to clients yet
		UWorld* World = GetWorld();
		for (TActorIterator<AILLPlayerState> It(World); It; ++It)
		{
			if (AILLPlayerState* OtherPlayer = *It)
			{
				// For every game session registered player, replicate their muted state
				if (OtherPlayer && OtherPlayer->UniqueId.IsValid() && *OtherPlayer->UniqueId != *CachedUniqueId)
				{
					ReplicateMutedState(OtherPlayer);
				}
			}
		}
	}
	else if (AILLPlayerController* PC = Cast<AILLPlayerController>(PlayerController))
	{
		// This is someone else
		// Tell the server to mute them if we have not already
		if (HasMuted(PlayerState))
		{
			PC->ServerMutePlayer(PlayerState->UniqueId);
		}
	}
}

bool UILLLocalPlayer::DoesUserOwnEntitlement(FUniqueEntitlementId EntitlementId) const
{
	if (EntitlementId.IsEmpty())
		return true;

#if !UE_BUILD_SHIPPING
	if (CVarEnableAllEntitlements.GetValueOnAnyThread() > 0)
		return true;
#endif
	return bReceivedEntitlements && OwnedEntitlements.Contains(EntitlementId);
}

void UILLLocalPlayer::QueryEntitlements()
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::QueryEntitlements"), *GetName());

	IOnlineEntitlementsPtr Entitlements = Online::GetEntitlementsInterface();
	if (Entitlements.IsValid() && HasPlatformLogin())
	{
		// Request entitlements
		OnQueryEntitlementsCompleteDelegateHandle = Entitlements->AddOnQueryEntitlementsCompleteDelegate_Handle(FOnQueryEntitlementsCompleteDelegate::CreateUObject(this, &ThisClass::OnQueryEntitlementsComplete));
		Entitlements->QueryEntitlements(*GetCachedUniqueNetId());
	}
}

void UILLLocalPlayer::OnQueryEntitlementsComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	UE_LOG(LogPlayerManagement, Verbose, TEXT("%s::OnQueryEntitlementsComplete bWasSuccessful: %s, Error: %s"), *GetName(), bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	IOnlineEntitlementsPtr Entitlements = Online::GetEntitlementsInterface();
	if (bWasSuccessful && Entitlements.IsValid())
	{
		// Cleanup our delegate
		Entitlements->ClearOnQueryEntitlementsCompleteDelegate_Handle(OnQueryEntitlementsCompleteDelegateHandle);

		// Track owned entitlements locally
		TArray<FOnlineEntitlement> EtList;
		Entitlements->GetAllEntitlements(UserId, EtList);
		for (FOnlineEntitlement Entitlement : EtList)
		{
			OwnedEntitlements.Add(Entitlement.Id);
		}

		// Broadcast completion
		bReceivedEntitlements = true;
		OnReceivedEntitlements();
	}
}

void UILLLocalPlayer::OnReceivedEntitlements()
{
	// Attempt to verify entitlement setting ownership
	ConditionalVerifyEntitlementSettingsOwnership();
}

void UILLLocalPlayer::ConditionalVerifyEntitlementSettingsOwnership()
{
	// Wait for save game loading to complete
	if (SettingsSaveGameCache.Num() == 0 || IsLoadingSettingsSaveGame())
		return;

	// Wait for entitlement fetching to complete
	if (!bReceivedEntitlements)
		return;

	// Broadcast ownership checks
	for (UILLPlayerSettingsSaveGame* SettingsSave : SettingsSaveGameCache)
	{
		SettingsSave->VerifyEntitlementSettingOwnership();
	}
}

#undef LOCTEXT_NAMESPACE
