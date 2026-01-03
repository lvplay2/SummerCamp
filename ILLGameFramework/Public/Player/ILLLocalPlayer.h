// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Engine/LocalPlayer.h"
#include "OnlineEntitlementsInterface.h"
#include "OnlineIdentityInterface.h"

#include "ILLMenuStackHUDComponent.h"
#include "ILLLocalPlayer.generated.h"

class AILLPlayerState;
class FILLPlayerSettingsSaveGameRunnable;
class FILLPlayerLoadSaveGameRunnable;
class FILLPlayerWriteSaveGameRunnable;
class UILLBackendPlayer;
class UILLLocalPlayer;
class UILLPlayerSettingsSaveGame;

enum class EILLBackendAuthResult : uint8;
enum class EILLPlayerSettingsSaveType : uint8;

DECLARE_DELEGATE_OneParam(FILLOnPrivilegesCollectedDelegate, UILLLocalPlayer* /*LocalPlayer*/);

/**
 * @class UILLLocalPlayer
 */
UCLASS(Config=Engine, Transient)
class ILLGAMEFRAMEWORK_API UILLLocalPlayer
: public ULocalPlayer
{
	GENERATED_UCLASS_BODY()

	// Begin ULocalPlayer interface
	virtual void PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID) override;
	virtual void PlayerRemoved() override;
	virtual FString GetGameLoginOptions() const override;
	virtual void SetControllerId(int32 NewControllerId) override;
	// End ULocalPlayer interface

protected:
	/** Called before a map change while logged in. */
	virtual void OnPreLoadMap(const FString& MapName);

	/** Called after a map change while logged in. */
	virtual void OnPostLoadMap(UWorld* World);

	//////////////////////////////////////////////////////////////////////////
	// Player settings
public:
	/** @return true if there is a loading save game task active or pending. */
	bool IsLoadingSettingsSaveGame() const;

	/** @return true if a save game thread is active. */
	FORCEINLINE bool IsSettingsSaveGameThreadActive() const { return SettingsSaveGameTick.IsTickFunctionEnabled(); }

	/**
	 * Creates a new SaveGameClass instance at default values.
	 *
	 * @param SaveGameClass Class of the save game to load/create.
	 * @return New player save instance.
	 */
	UILLPlayerSettingsSaveGame* CreateSettingsSave(TSubclassOf<UILLPlayerSettingsSaveGame> SaveGameClass);
	
	/** @return Loaded and cached instance of SaveGameClass. */
	UILLPlayerSettingsSaveGame* GetLoadedSettingsSave(TSubclassOf<UILLPlayerSettingsSaveGame> SaveGameClass);

	/** Templated version of GetLoadedSettingsSave. */
	template<class T>
	FORCEINLINE T* GetLoadedSettingsSave(bool bCastChecked = false)
	{
		if (bCastChecked)
		{ 
			return CastChecked<T>(GetLoadedSettingsSave(T::StaticClass()), ECastCheckedType::NullAllowed);
		}
		else
		{
			return Cast<T>(GetLoadedSettingsSave(T::StaticClass()));
		}
	}

	/**
	 * Called to queue a player settings save write.
	 * Determines the SaveType from SaveGame and calls the override below.
	 *
	 * @param SaveGame Settings save instance to write.
	 */
	void QueuePlayerSettingsSaveGameWrite(UILLPlayerSettingsSaveGame* SaveGame);

	/**
	 * Called to queue a player settings save write.
	 *
	 * @param SaveType Settings type to write.
	 */
	void QueuePlayerSettingsSaveGameWrite(const EILLPlayerSettingsSaveType SaveType);

protected:
	/**
	 * @struct FILLSaveGameTickFunction
	 */
	struct FILLSaveGameTickFunction
	: public FTickFunction
	{
		// Begin FTickFunction interface
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		// End FTickFunction interface

		UILLLocalPlayer* Target;
	};

	/** Registers the custom check task tick function with the level tick. */
	virtual void RegisterSettingsSaveGameTickFunction();

	/** Unregisters the custom check task tick function with the level tick. */
	virtual void UnRegisterSettingsSaveGameTickFunction();

	/** Called from OnPlatformLoginComplete to load our UILLPlayerSettingsSaveGame classes. */
	virtual void LoadAndApplySaveGameSettings();

	/** Called on a timer when RunningSettingsSaveGameTask is occupied. */
	virtual void CheckSettingsSaveGameTask();

	// Classes to load for settings save game classes
	TArray<TSubclassOf<UILLPlayerSettingsSaveGame>> SettingsSaveGameClasses;

	// Loaded player saves
	UPROPERTY(Transient)
	TArray<UILLPlayerSettingsSaveGame*> SettingsSaveGameCache;

	// Pending save game thread jobs
	TArray<FILLPlayerSettingsSaveGameRunnable*> PendingSettingsSaveGameTasks;

	// Currently running save game job
	FILLPlayerSettingsSaveGameRunnable* RunningSettingsSaveGameTask;

	// Counter for unique thread names
	int32 SettingsSaveGameCounter;

	// Custom tick function to cover the use case of queuing a save while the game is paused
	FILLSaveGameTickFunction SettingsSaveGameTick;

	// Necessary friends
	friend FILLPlayerSettingsSaveGameRunnable;
	friend FILLPlayerLoadSaveGameRunnable;
	friend FILLPlayerWriteSaveGameRunnable;

	//////////////////////////////////////////////////////////////////////////
	// Platform privileges
public:
	/** Kicks off requests to collect all user platform privilege information. */
	virtual void CollectUserPrivileges(TSharedPtr<const FUniqueNetId> PendingUniqueId, const FILLOnPrivilegesCollectedDelegate& Delegate = FILLOnPrivilegesCollectedDelegate());

	/** Helper function to update the EUserPrivileges::CanPlayOnline specifically. */
	virtual void RefreshPlayOnlinePrivilege();

	/** @return Play online privilege result. */
	FORCEINLINE IOnlineIdentity::EPrivilegeResults GetPlayOnlinePrivileges() const { return PlayOnlinePrivilegeResult; }

	/** @return true if this player has privileges to play this game. */
	FORCEINLINE bool HasCanPlayPrivilege() const { return bHasCanPlayPrivilege; }

	/** @return true if this player has privileges to play this game online. */
	FORCEINLINE bool HasCanPlayOnlinePrivilege() const { return bHasCanPlayOnlinePrivilege; }

	/** @return true if this player has privileges to communicate with other players online. */
	FORCEINLINE bool HasCanCommunicateOnlinePrivilege() const { return bHasCanCommunicateOnlinePrivilege; }

	/** @return true if this player has privileges to use user-generated content. */
	FORCEINLINE bool HasCanUseUserGeneratedContentPrivilege() const { return bHasCanUseUserGeneratedContentPrivilege; }

	/** @return true if we are still checking this user's privilege. */
	FORCEINLINE bool IsCheckingPrivileges() const { return (PendingPrivilegeChecks > 0 || bIsCheckingPrivilege); }

	/** @return true if this player has completed the platform login and is not pending a sign out. */
	FORCEINLINE bool HasPlatformLogin() const { return (bCompletedPlatformLogin && GetCachedUniqueNetId().IsValid() && !bPendingSignout); }

	/** Reset cached preference settings. */
	virtual void ResetPrivileges();

protected:
	/** Callback for privilege checks fired off after logging in. */
	virtual void OnPrivilegeCheckComplete(const FUniqueNetId& UniqueId, EUserPrivileges::Type Privilege, uint32 PrivilegeResult);

	// Privilege check callback delegate
	FILLOnPrivilegesCollectedDelegate OnPrivilegesCollectedDelegate;
	int32 PendingPrivilegeChecks;

	// Are we performing a privilege check right now?
	bool bIsCheckingPrivilege;

	// Cached privilege results
	IOnlineIdentity::EPrivilegeResults PlayOnlinePrivilegeResult;
	bool bHasCanPlayPrivilege;
	bool bHasCanPlayOnlinePrivilege;
	bool bHasCanCommunicateOnlinePrivilege;
	bool bHasCanUseUserGeneratedContentPrivilege;

	//////////////////////////////////////////////////
	// Application status handling
protected:
	/** Application will be suspended notification. */
	virtual void OnApplicationWillDeactivate();

	/** Application will be resumed from suspended notification. */
	virtual void OnApplicationHasReactivated();

	/** Standalone client automatic database login result. */
	virtual void OnReLogAuthComplete(EILLBackendAuthResult AuthResult);

	/** Application is going into the background. */
	virtual void OnApplicationWillEnterBackground();

	/** Called from OnApplicationWillEnterBackground. Will queue a timer for the next tick to call itself again if map loading is in-progress. */
	virtual void ProcessApplicationEnteredBackground();

	/** Application is coming into the foreground. */
	virtual void OnApplicationHasEnteredForeground();

	/** Called from OnApplicationHasEnteredForeground. Will queue a timer for the next tick to call itself again if map loading is in-progress. */
	virtual void ProcessApplicationEnteredForeground();

	/**
	 * Called from a timer at a delay after ProcessApplicationEnteredForeground processes for standalone users.
	 * The arbitrary delay waits for any other connection status or controller pairing errors to fire, checking if we are still authenticated before processing it.
	 */
	virtual void BeginStandaloneReLogAuth();

	/** Platform external UI open/closed delegate. Only hit when logged in. */
	virtual void OnExternalUIChanged(bool bIsOpening);

	// Delegate handle for OnExternalUIChanged
	FDelegateHandle OnExternalUIChangeDelegateHandle;

	// Timer handle for BeginStandaloneReLogAuth
	FTimerHandle TimerHandle_BeginStandaloneReLogAuth;

	// Amount of time to wait before performing re-log authentication
	float ReLogAuthenticationDelay;

	//////////////////////////////////////////////////
	// Connection status handling
protected:
	/** Delegate fired for connection status changes. */
	virtual void OnConnectionStatusChanged(EOnlineServerConnectionStatus::Type LastConnectionState, EOnlineServerConnectionStatus::Type ConnectionState);

	// Delegate handle for OnConnectionStatusChanged
	FDelegateHandle OnConnectionStatusChangedDelegateHandle;

	//////////////////////////////////////////////////
	// Login status handling
public:
	/**
	 * Called when platform login is starting. The external login UI may or may not display after this.
	 * Update the ControllerId from the last input. CollectUserPrivileges should begin after the platform login finishes.
	 */
	virtual void BeginPlatformLogin();

	/** Called from UILLPlatformLoginCallbackProxy when platform login and privilege checks complete successfully. */
	virtual void OnPlatformLoginComplete(TSharedPtr<const FUniqueNetId> NewUniqueId);

	/** Called from UILLOnlineSessionClient::PerformSignoutAndDisconnect when a signout is pending (deferred). */
	virtual void PendingSignout() { bPendingSignout = true; }

	/** Called from UILLOnlineSessionClient::PerformSignoutAndDisconnect to sign this user out. */
	virtual void PerformSignout();

	/** @return Online environment we are using. */
	FORCEINLINE EOnlineEnvironment::Type GetOnlineEnvironment() const { return OnlineEnvironment; }

protected:
	/** Stops listening for login status changes for this user. */
	virtual void StopListeningForUserChanges();

	/** Delegate fired for login status changes. */
	virtual void OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId);

	// Delegate handle for OnLoginStatusChanged
	FDelegateHandle OnLoginStatusChangedDelegateHandle;

	// Online environment we are using
	EOnlineEnvironment::Type OnlineEnvironment;

	// Have we completed platform login?
	bool bCompletedPlatformLogin;

	// Are we pending a sign out?
	bool bPendingSignout;

	//////////////////////////////////////////////////
	// Controller status handling
protected:
	/** Delegate fired for controller connection changes. */
	virtual void OnControllerConnectionChanged(bool bConnected, int32 UserId, int32 LocalUserNum);

	/** Delegate fired for controller pairing status changes (XBL). */
	virtual void OnControllerPairingChanged(int32 LocalUserNum, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser);

	/** Helper function for the above functions. */
	virtual void HandleControllerDisconnected();

	/** Helper function for the above functions. */
	virtual void HandleControllerConnected();

	// Delegate handle for OnControllerConnectionChanged
	FDelegateHandle OnControllerConnectionChangedDelegateHandle;

	// Delegate handle for OnControllerPairingChanged
	FDelegateHandle OnControllerPairingChangedDelegateHandle;

	// Did we lose controller pairing?
	bool bLostControllerPairing;

public:
	/** @return true if this player has lost controller pairing. */
	bool HasLostControllerPairing() const { return bLostControllerPairing; }

	/** Calls to HandleControllerDisconnected to show the controller disconnected dialog. */
	void ShowControllerDisconnected() { HandleControllerDisconnected(); }

	//////////////////////////////////////////////////////////////////////////
	// Backend
public:
	/** Called from our UILLBackendPlayer when it's created. */
	virtual void SetCachedBackendPlayer(UILLBackendPlayer* NewPlayer);

	/** @return Backend player instance, if available. */
	FORCEINLINE UILLBackendPlayer* GetBackendPlayer() const { return BackendPlayer; }

	/** @return Casted Backend player handle. */
	template<class T>
	FORCEINLINE T* GetBackendPlayer(bool bCastChecked = true) const
	{
		if (bCastChecked)
		{
			return CastChecked<T>(GetBackendPlayer(), ECastCheckedType::NullAllowed);
		}
		else
		{
			return Cast<T>(GetBackendPlayer());
		}
	}

	/** Called when our BackendPlayer has arbitrated. */
	virtual void NotifyBackendPlayerArbitrated();

	/** Called when the user selects to enter offline mode. */
	virtual void NotifyOfflineMode();

protected:
	/**
	 * Calls to UILLPlayerSettingsSaveGame::VerifyBackendSettingsOwnership on all SettingsSaveGameCache once requirements are met:
	 * 1) Save game loading has completed.
	 * 2) Backend authentication completes, or the user has entered offline mode.
	 */
	virtual void ConditionalVerifyBackendSettingsOwnership();

	// Backend player instance
	UPROPERTY(Transient)
	UILLBackendPlayer* BackendPlayer;

	//////////////////////////////////////////////////////////////////////////
	// Mute list
public:
	/** @return true if UniqueId is muted for this player. */
	FORCEINLINE bool HasMuted(FUniqueNetIdRepl UniqueId) const { return MutedPlayerList.Contains(UniqueId); }
	virtual bool HasMuted(const AILLPlayerState* PlayerState) const;

	/** Mutes PlayerState. */
	virtual void MutePlayer(const AILLPlayerState* PlayerState);

	/** Un-mutes PlayerState. */
	virtual void UnmutePlayer(const AILLPlayerState* PlayerState);

	/**
	 * Check if PlayerState is muted, send an RPC to the server if they are.
	 *
	 * @param PlayerState Player to replicate the muted state for.
	 */
	virtual void ReplicateMutedState(const AILLPlayerState* PlayerState);

protected:
	// List of players we have muted
	UPROPERTY()
	TArray<FUniqueNetIdRepl> MutedPlayerList;

	//////////////////////////////////////////////////
	// Entitlements
public:
	/** @return true if this player owns EntitlementId. */
	virtual bool DoesUserOwnEntitlement(FUniqueEntitlementId EntitlementId) const;

	/** @return true if we have received entitlements from the platform. */
	FORCEINLINE bool HasReceivedEntitlements() const { return bReceivedEntitlements; }

	/** Requests an entitlements update from the OSS. */
	virtual void QueryEntitlements();

protected:
	/** Delegate fired when DLC query completes. */
	virtual void OnQueryEntitlementsComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/** Called from OnQueryEntitlementsComplete when we have successfully retrieved our entitlements. */
	virtual void OnReceivedEntitlements();

	/**
	 * Calls to UILLPlayerSettingsSaveGame::VerifyEntitlementSettingOwnership on all SettingsSaveGameCache once requirements are met:
	 * 1) Save game loading has completed.
	 * 2) Entitlement fetching has completed via OnReceivedEntitlements.
	 *
	 * May fire more than once since QueryEntitlements can be fired arbitrarily after the player logs in.
	 */
	virtual void ConditionalVerifyEntitlementSettingsOwnership();

	// Cache of owned entitlements for this user
	TArray<FUniqueEntitlementId> OwnedEntitlements;

	// Delegate handle for OnQueryEntitlementsComplete
	FDelegateHandle OnQueryEntitlementsCompleteDelegateHandle;

	// Have we ever received entitlements from the platform?
	bool bReceivedEntitlements;

	//////////////////////////////////////////////////////
	// Menu navigation
public:
	// Cached menu stack so that we can re-open the old menu stack when returning to Main Menu.
	UPROPERTY()
	TArray<FILLMenuStackElement> CachedMenuStack;
};
