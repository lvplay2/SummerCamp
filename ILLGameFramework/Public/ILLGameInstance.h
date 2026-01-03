// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "IPlatformCrypto.h"
#include "OnlineSessionInterface.h"
#include "RHI.h"

#include "ILLSessionBeaconClient.h" // For EILLSessionBeaconReservationResult mostly
#include "ILLGameInstance.generated.h"

class AILLPlayerController;
class UILLBackendRequestManager;
class UILLLiveRequestManager;
class UILLPSNRequestManager;
class UILLOnlineMatchmakingClient;
class UILLUserWidget;

/**
 * @class UILLGameInstance
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API UILLGameInstance
: public UGameInstance
{
	GENERATED_UCLASS_BODY()

	// Begin UGameInstance interface
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void StartGameInstance() override;
	virtual bool JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults) override;
	virtual bool JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult) override;
	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
	virtual void ReceivedNetworkEncryptionToken(UObject* RecipientObject, const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) override;
	virtual void ReceivedNetworkEncryptionAck(UObject* RecipientObject, const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) override;
	// End UGameInstance interface

	//////////////////////////////////////////////////
	// Network encryption
public:
	/**
	 * @param EncryptionToken Encryption token received.
	 * @param OutKeySessionId Found by decrypting and verifying EncryptionToken.
	 * @return true if decryption was successful.
	 */
	virtual bool EncryptionTokenToKeySession(const FString& EncryptionToken, FString& OutKeySessionId) const;

	/**
	 * @param KeySessionId Key session identifier received from the backend.
	 * @param OutEncryptionToken Created by encrypting and hashing the KeySessionId.
	 * @return true if encryption was successful.
	 */
	virtual bool KeySessionToEncryptionToken(const FString& KeySessionId, FString& OutEncryptionToken) const;

protected:
	/**
	 * Delegate for requiring network encryption server-side.
	 *
	 * @param RecipientObject Network object that received the connection request.
	 * @return true if network encryption is required.
	 */
	virtual bool RequireNetworkEncryption(UObject* RecipientObject);

	/** Response handler for requesting client key sessions. */
	virtual void RetrieveKeySessionResponse(int32 ResponseCode, const FString& ResponseContent, const FOnEncryptionKeyResponse& Delegate);

	/**
	 * Sets up network encryption keys for party communication, triggering Delegate with the response.
	 *
	 * @param EncryptionToken Token received/acked.
	 * @param Delegate Encryption response delegate to execute with a response.
	 */
	virtual void BuildPartyNetworkEncryptionResponse(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) const;

	// Local encryption context handle
	TUniquePtr<FEncryptionContext> EncryptionContext;

	//////////////////////////////////////////////////
	// Rendering
public:
#if PLATFORM_DESKTOP
	// Cached resolution options
	FScreenResolutionArray ResolutionList;
#endif

	//////////////////////////////////////////////////
	// Instance management
public:
	/** @return true if this instance is pending recycling. */
	FORCEINLINE bool IsPendingRecycling() const { return bPendingRecycle; }

	/**
	 * Notifies us that the game mode match has ended and is about to change maps. Perform recycle checks.
	 * Will flag this instance as bPendingRecycle when the MinPlayerRatioToLive requirement is not met.
	 *
	 * @param bForce Force instance recycling to occur?
	 * @return true if the instance is now pending recycling.
	 */
	virtual bool CheckInstanceRecycle(const bool bForce = false);

	/** Called when the game has registered as standalone. */
	virtual void OnGameRegisteredStandalone();

	/** @return true if this instance supports being recycled. */
	virtual bool SupportsInstanceRecycling() const;

protected:
	// Required player to MaxPlayers ratio to not recycle
	UPROPERTY(Config)
	float MinPlayerRatioToLive;

	// Maximum number of times to cycle back to the lobby before the instance should recycle
	UPROPERTY(Config)
	int32 MaxInstanceCycles;

	// Number of times this instance has cycled back to the lobby
	int32 NumInstanceCycles;

	// Is this instance pending a recycle?
	bool bPendingRecycle;

	//////////////////////////////////////////////////
	// Error notifications
public:
	/**
	 * Generic localized error message handler.
	 *
	 * @param ErrorHeading Optional error heading.
	 * @param ErrorMessage Message to display to the user.
	 * @param bWillTravel Will the user travel immediately after this is called? If so the error message will be deferred until after travel, otherwise it should show immediately.
	 */
	virtual void HandleLocalizedError(const FText& ErrorHeading, const FText& ErrorMessage, bool bWillTravel) {}

	//////////////////////////////////////////////////
	// Player account privilege error handling
public:
	/** Notification that a local player has the IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable flag in their play online privilege results. */
	virtual void PlayOnlineRequiredPatchAvailable(const FText& ErrorHeading = FText());

	/** Notification that a local player has the IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate flag in their play online privilege results. */
	virtual void PlayOnlineRequiredSystemUpdate(const FText& ErrorHeading = FText());

	/** Notification that a local player has the IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure flag in their play online privilege results. */
	virtual void PlayOnlineAgeRestrictionFailure(const FText& ErrorHeading = FText());

	//////////////////////////////////////////////////
	// Engine network / travel error handling
public:
	/** Called before reloading the main menu after UILLOnlineSessionClient::OnConnectionStatusChanged receives a connection failure. */
	virtual void OnConnectionFailurePreDisconnect(const FText& FailureText) {}

	/** Called from UILLOnlineSessionClient when joining a session invite fails. */
	virtual void OnInviteJoinSessionFailure(FName SessionName, EOnJoinSessionCompleteResult::Type Result, const bool bWillTravel) {}

	/** UEngine::OnNetworkLagStateChanged callback. */
	virtual void OnNetworkLagStateChanged(UWorld* World, UNetDriver* NetDriver, ENetworkLagState::Type LagType);
	
	/** Called from AILLPlayerController::ClientWasKicked_Implementation and used by beacon kicks. */
	virtual void SetLastPlayerKickReason(const FText& KickText, const bool bWillTravel = true) {}

protected:
	/** Localized overrideable UEngine::OnNetworkFailure callback. */
	virtual void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FText& FailureText, const bool bWillTravel) {}

	/** Localized overrideable UEngine::OnTravelFailure callback. */
	virtual void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FText& FailureText) {}

public:
	/** UEngine::OnNetworkFailure callback. */
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

private:
	/** UEngine::OnTravelFailure callback. */
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	//////////////////////////////////////////////////
	// Loading screen
public:
	/**
	 * Show a connecting screen for JoiningSession, while your previous session is cleaned up, your backend connection is established, or the OSS is hit to query information about the session.
	 * Default behavior is just to call ShowTravelingScreen when SessionName is NAME_GameSession.
	 * NOTE: May be called several times before ShowTravelingScreen gets hit.
	 *
	 * @param SessionName Name of the session. Typically NAME_GameSession or NAME_PartySession.
	 * @param JoiningSession Session we are joining.
	 */
	virtual void ShowConnectingLoadScreen(FName SessionName, const FOnlineSessionSearchResult& JoiningSession)
	{
		if (SessionName == NAME_GameSession)
		{
			ShowTravelingScreen(JoiningSession);
		}
	}

	/** Called to show the loading screen before joining JoiningSession. */
	virtual void ShowTravelingScreen(const FOnlineSessionSearchResult& JoiningSession) {}

	/** Called to show the loading screen before traveling to a raw URL. */
	virtual void ShowLoadingScreen(const FURL& NextURL);

	/** Called to display a specific loading-screen style widget. */
	virtual void ShowLoadingWidget(TSubclassOf<UILLUserWidget> LoadingScreenWidget) {}

	/**
	 * Called after ShowConnectingLoadScreen when we have finished joining the SessionName.
	 * Calls HideLoadingScreen by default, assuming you are doing that.
	 */
	virtual void HandleFinishedJoinInvite(FName SessionName, bool bWasSuccessful)
	{
		if (SessionName == NAME_PartySession)
		{
			// Hide the loading screen now that we have joined the party, since we may not be traveling immediately after
			HideLoadingScreen();
		}
	}

	/** Called to close a previously-requested connecting or loading screen. */
	virtual void HideLoadingScreen() 
	{
		// Check controller pairings in case one was disconnected during loading.
		CheckControllerPairings();
	}

	//////////////////////////////////////////////////
	// Session notifications
public:
	/** Called after ShowConnectingLoadScreen (when OnSessionJoinFailure is not hit) and the reservation request was sent off to the beacon. */
	virtual void ShowReservingScreen(FName SessionName, const FOnlineSessionSearchResult& JoiningSession) {}

	/** Called after attempting to create/host a game session has failed, and we are about to return to the main menu. */
	virtual void OnGameSessionCreateFailurePreTravel(const FText& FailureReasonText) {}

	/** Called when party session and beacon reservation has completed, when OnSessionJoinFailure is not hit. */
	virtual void OnSessionReservationFailed(FName SessionName, const EILLSessionBeaconReservationResult::Type Response) {}

	/** Notification for when a session join fails to start or complete. */
	virtual void OnSessionJoinFailure(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
	{
		HideLoadingScreen();
	}

	//////////////////////////////////////////////////
	// Party notifications
public:
	/**
	 * Party client notification for when we are searching for our leader's game session.
	 * May fire several times as it sometimes takes a couple retries to find it.
	 */
	virtual void OnPartyFindingLeaderGameSession() {}

	/** Party host destroyed party notification. */
	virtual void OnPartyClientHostShutdown(const FText& ClosureReasonText) {}

	/** Called whenever there is a party network failure and we are about to travel (to the main menu) because of it. */
	virtual void OnPartyNetworkFailurePreTravel(const FText& FailureText) {}

	//////////////////////////////////////////////////
	// Controller notifications
public:
	/** Notification for when the local player loses their controller pairing. */
	virtual void OnControllerPairingLost(const int32 LocalUserNum, FText UserErrorText);

	/** Notification for when the local player regains their controller pairing. */
	virtual void OnControllerPairingRegained(const int32 LocalUserNum);

	/** Checks the pairing status of controllers for all local players. */
	virtual void CheckControllerPairings();

protected:
	// List of unpaired controllers
	UPROPERTY(Transient)
	TArray<int32> UnPairedControllers;
	
	//////////////////////////////////////////////////
	// Matchmaking
public:
	/** @return Online matchmaking client instance. */
	virtual UILLOnlineMatchmakingClient* GetOnlineMatchmaking() const { return OnlineMatchmaking; }

	/** @return Online matchmaking client instance. */
	virtual TSubclassOf<UILLOnlineMatchmakingClient> GetOnlineMatchmakingClass() const;

	/** Notification for when matchmaking cancellation completes. */
	virtual void OnMatchmakingCanceledComplete() { HideLoadingScreen(); }

	/** @return Randomly-selected map for QuickPlay. Try to avoid the current map! */
	virtual FString PickRandomQuickPlayMap() const;

protected:
	// Online matchmaking instance
	UPROPERTY()
	UILLOnlineMatchmakingClient* OnlineMatchmaking;

	//////////////////////////////////////////////////
	// Assets
public:
	// Streamable manager instance
	FStreamableManager StreamableManager;

	//////////////////////////////////////////////////
	// HTTP request managers
public:
	/** @return Backend request manager instance. */
	template<class T = UILLBackendRequestManager>
	FORCEINLINE T* GetBackendRequestManager() { return Cast<T>(BackendRequestManager); }

	/** @return Live request manager instance. */
	template<class T = UILLLiveRequestManager>
	FORCEINLINE T* GetLiveRequestManager() { return Cast<T>(LiveRequestManager); }

	/** @return PSN request manager instance. */
	template<class T = UILLPSNRequestManager>
	FORCEINLINE T* GetPSNRequestManager() { return Cast<T>(PSNRequestManager); }

protected:
	// Class to use for managing requests with the backend
	UPROPERTY()
	TSubclassOf<UILLBackendRequestManager> BackendRequestManagerClass;

	// Instance of BackendRequestManagerClass
	UPROPERTY(Transient)
	UILLBackendRequestManager* BackendRequestManager;

	// Class to use for managing requests with Live
	UPROPERTY()
	TSubclassOf<UILLLiveRequestManager> LiveRequestManagerClass;

	// Instance of LiveRequestManagerClass
	UPROPERTY(Transient)
	UILLLiveRequestManager* LiveRequestManager;

	// Class to use for managing requests with PSN
	UPROPERTY()
	TSubclassOf<UILLPSNRequestManager> PSNRequestManagerClass;

	// Instance of PSNRequestManagerClass
	UPROPERTY(Transient)
	UILLPSNRequestManager* PSNRequestManager;
};
