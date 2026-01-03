// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameInstance.h"
#include "SCTypes.h"
#include "SCConfirmationDialogWidget.h"
#include "SCErrorDialogWidget.h"
#include "SCPlayerState.h"
#include "SCStatsAndScoringData.h"
#include "SCGameInstance.generated.h"

class ASCCounselorCharacter;
class ASCKillerCharacter;
class ASCPlayerState;
class ASCPlayerState_Lobby;

struct FSCCounselorOutfit;
struct FSCBadgeAwardInfo;

class UUserWidget;
class USCErrorDialogWidget;
class USCMapRegistry;
class USCStatBadge;
class USCCounselorWardrobe;

/**
 * @class USCGameInstance
 */
UCLASS(Config=Game)
class SUMMERCAMP_API USCGameInstance
: public UILLGameInstance
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UGameInstance interface
	virtual void Init() override;
	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
	// End UGameInstance interface

	// Begin UILLGameInstance interface
	virtual void HandleLocalizedError(const FText& ErrorHeading, const FText& ErrorMessage, bool bWillTravel) override;
	virtual void OnConnectionFailurePreDisconnect(const FText& FailureText) override;
	virtual void OnInviteJoinSessionFailure(FName SessionName, EOnJoinSessionCompleteResult::Type Result, const bool bWillTravel) override;
	virtual void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FText& FailureText, const bool bWillTravel) override;
	virtual void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FText& FailureText) override;
	virtual void SetLastPlayerKickReason(const FText& KickText, const bool bWillTravel = true) override;
	virtual void ShowConnectingLoadScreen(FName SessionName, const FOnlineSessionSearchResult& JoiningSession) override;
	virtual void ShowTravelingScreen(const FOnlineSessionSearchResult& JoiningSession) override;
	virtual void ShowLoadingScreen(const FURL& NextURL) override;
	virtual void ShowLoadingWidget(TSubclassOf<UILLUserWidget> LoadingScreenWidget) override;
	virtual void HideLoadingScreen() override;
	virtual void ShowReservingScreen(FName SessionName, const FOnlineSessionSearchResult& JoiningSession) override;
	virtual void OnGameSessionCreateFailurePreTravel(const FText& FailureReasonText) override;
	virtual void OnSessionReservationFailed(FName SessionName, const EILLSessionBeaconReservationResult::Type Response) override;
	virtual void OnSessionJoinFailure(FName SessionName, EOnJoinSessionCompleteResult::Type Result) override;
	virtual void OnPartyFindingLeaderGameSession() override;
	virtual void OnPartyClientHostShutdown(const FText& ClosureReasonText) override;
	virtual void OnPartyNetworkFailurePreTravel(const FText& FailureText) override;
	virtual void OnControllerPairingLost(const int32 LocalUserNum, FText UserErrorText) override;
	virtual void OnControllerPairingRegained(const int32 LocalUserNum) override;
	virtual TSubclassOf<UILLOnlineMatchmakingClient> GetOnlineMatchmakingClass() const override;
	virtual FString PickRandomQuickPlayMap() const override;
	// End UILLGameInstance interface

	// Backend account ID for the last P2P QuickPlay host
	FILLBackendPlayerIdentifier LastP2PQuickPlayHostAccountId;

	//////////////////////////////////////////////////
	// System Managers
public:
	/** Returns our wardrobe for counselors to pick out their clothes */
	FORCEINLINE USCCounselorWardrobe* GetCounselorWardrobe() const { return CounselorWardrobe; }

protected:
	// Clothing manager, for counselors
	UPROPERTY(Transient)
	USCCounselorWardrobe* CounselorWardrobe;

	//////////////////////////////////////////////////
	// Error message handling
public:
	/** Helper function to open a confirmation dialog on the first world player controller's HUD. */
	void ShowFirstPlayerConfirmationDialog(FSCOnConfirmedDialogDelegate Delegate, FText UserConfirmationText);

	/** Helper function to open an error dialog on the first world player controller's HUD. */
	bool ShowFirstPlayerErrorDialog(const FText& UserErrorText, const FText& HeaderText = FText());

protected:
	UFUNCTION(BlueprintCallable)
	void RevertCustomMatchSettings();

	/** Callback for ErrorDialogDelegate. */
	UFUNCTION()
	void OnAcceptError();

	/** Callback for ControllerPairingErrorDialogDelegate. */
	UFUNCTION()
	void OnAcceptControllerPairingError();

	UFUNCTION(BlueprintCallable)
	void InitCustomMatchSettings();

	/** Callback from OnAcceptControllerPairingError when we asked to pick an account to sign into */
	void OnShowLoginUICompleted(TSharedPtr<const FUniqueNetId> UniqueId, int LocalPlayerNum);

#if WITH_EAC
	/** Callback for when there is a EAC client error. */
	void OnEACError(const FString& Message);
#endif

	/** Checks if PendingFailureText is non-empty and pushes and error dialog. */
	UFUNCTION()
	void FlushFailureText();

	UFUNCTION(BlueprintCallable)
	bool CustomMatchSettingsChanged();

	UFUNCTION(BlueprintCallable)
	void BackupCustomMatchSettings();

	/** Sets a timer to flush PendingFailureText. */
	void QueueFailureErrorCheck();

	/** Helper function for changing PendingFailureText. */
	void SetFailureText(const FText& NewFailureText, const FText& NewFailureHeader = FText());

	// Error dialog delegate for ShowFirstPlayerErrorDialog
	UPROPERTY(Transient)
	FSCOnAcceptedErrorDialogDelegate ErrorDialogDelegate;

	// Error dialog delegate for OnControllerPairingLost
	UPROPERTY(Transient)
	FSCOnAcceptedErrorDialogDelegate ControllerPairingErrorDialogDelegate;

	// Error dialog instance for the controller pair lost notification, so it can be closed procedurally
	UPROPERTY(Transient)
	USCErrorDialogWidget* ControllerPairingLostDialog;

	// Travel failure text queued for FlushFailureText
	FText PendingFailureHeader;
	FText PendingFailureText;

	//////////////////////////////////////////////////
	// Loading screen
protected:
	/** FCoreUObjectDelegates::PreLoadMap delegate. */
	void OnPreLoadMap(const FString& MapName);

	/** FCoreUObjectDelegates::PostLoadMap delegate. */
	void OnPostLoadMap(UWorld* World);

	/** Helper function for the UILLGameInstance::ShowLoadingScreen functions. */
	void ShowLoadingScreen(const FString& MapAssetName);

	// Loading screen widget class for connecting
	UPROPERTY()
	TSubclassOf<UILLUserWidget> ConnectingLoadScreenClass;

	// Loading screen widget class for long loads
	UPROPERTY()
	TSubclassOf<UILLUserWidget> LongLoadScreenClass;

	// Loading screen widget class for connecting to a party
	UPROPERTY()
	TSubclassOf<UILLUserWidget> PartyConnectingLoadScreenClass;

	// Loading screen widget class for finding a party leader's game session
	UPROPERTY()
	TSubclassOf<UILLUserWidget> PartyFindingLeaderGameLoadScreenClass;

	// Loading screen widget class for making a reservation with a party
	UPROPERTY()
	TSubclassOf<UILLUserWidget> PartyReservingLoadScreenClass;
	
	//////////////////////////////////////////////////
	// Map registry
public:
	// Map registry class
	UPROPERTY()
	TSubclassOf<USCMapRegistry> MapRegistryClass;

	//////////////////////////////////////////////////
	// Picked killer persistent storage
public:
	/** Clears the currently picked killer player. */
	FORCEINLINE void ClearPickedKillerPlayer() { PickedKillerAccountId.Reset(); }

	/** @return true if we have a picked killer player to use. */
	FORCEINLINE bool HasPickedKillerPlayer() const { return PickedKillerAccountId.IsValid(); }

	/** Changes the PickedKillerAccountId.*/
	FORCEINLINE void SetPickedKillerPlayer(const FILLBackendPlayerIdentifier& InPickedKillerAccountId) { PickedKillerAccountId = InPickedKillerAccountId; }

	/** @return true if PlayerState was the previously picked killer player. */
	bool IsPickedKillerPlayer(ASCPlayerState* PlayerState) const;

	// Backend account identifier of the last player to be a killer in a Hunt match
	UPROPERTY(Transient)
	FILLBackendPlayerIdentifier LastKillerAccountId;

private:
	// Backend account identifier of the player to be made the killer
	UPROPERTY(Transient)
	FILLBackendPlayerIdentifier PickedKillerAccountId;

	//////////////////////////////////////////////////
	// Player persistent storage
public:
	/**
	 * Store persistent travel data for all players.
	 * Clears previous data first so there is no accumulation.
	 */
	void StoreAllPersistentPlayerData();

protected:
	/**
	 * Stores various persistent information for a player:
	 * - Picked counselor/killer classes.
	 * - Picked counselor perks.
	 * - PickedKiller if this PlayerState is it.
	 */
	void StorePersistentPlayerData(ASCPlayerState* PlayerState);
};
