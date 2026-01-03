// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "OnlineEngineInterface.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#include "ILLBackendPlayerIdentifier.h"
#include "ILLMenuStackHUDComponent.h"
#include "ILLPlayerController.generated.h"

class AILLLobbyBeaconClient;
class AILLPartyBeaconMemberState;
class AILLPlayerState;
class UILLBackendPlayer;
class USoundCue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLOnPlayerHUDCreated, AILLHUD*, HUD);

/**
 * @class UILLPlayerNotificationMessage
 */
UCLASS(BlueprintType, Transient)
class ILLGAMEFRAMEWORK_API UILLPlayerNotificationMessage
: public UObject
{
	GENERATED_UCLASS_BODY()

	// Localized notification text to display
	UPROPERTY(BlueprintReadOnly)
	FText NotificationText;

	UILLPlayerNotificationMessage() {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FILLOnPlayerReceivedNotification, UILLPlayerNotificationMessage*, Notification);

/**
 * @class UILLPlayerStateChatMessage
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPlayerStateChatMessage
: public UILLPlayerNotificationMessage
{
	GENERATED_UCLASS_BODY()

	// Player state this message relates to
	UPROPERTY(BlueprintReadOnly)
	AILLPlayerState* PlayerState;

	// Type of chat message
	UPROPERTY(BlueprintReadOnly)
	FName Type;

	// Time the chat message should stay up, or <=0 for default
	UPROPERTY(BlueprintReadOnly)
	float Lifetime;
};

/**
 * @class AILLPlayerController
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API AILLPlayerController
: public APlayerController
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual void BeginDestroy() override;
	// End UObject interface

	// Begin AActor interface
	virtual void Destroyed() override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual bool UseShortConnectTimeout() const override;
	virtual void PostNetReceive() override;
	// End AActor interface

	// Begin AController interface
	virtual void SetupInputComponent() override;
	virtual void InitPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual bool IsMoveInputIgnored() const override;
	virtual bool IsLookInputIgnored() const override;
	// End AController interface

	// Begin APlayerController interface
	virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<class AActor*>& ActorList) override;
	virtual void SeamlessTravelFrom(class APlayerController* OldPC) override;
	virtual void UpdatePing(float InPing);
	virtual void ClientEnableNetworkVoice_Implementation(bool bEnable) override;
	virtual void ClientSetHUD_Implementation(TSubclassOf<class AHUD> NewHUDClass) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void InitInputSystem() override;
	virtual void ClientStartOnlineSession_Implementation() override;
	virtual void ClientPlayForceFeedback_Implementation(UForceFeedbackEffect* ForceFeedbackEffect, bool bLooping, bool bIgnoreTimeDilation, FName Tag) override;
	virtual void ReceivedPlayer() override;
	virtual void SetCameraMode(FName NewCamMode) override;
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
	virtual bool InputKey(FKey Key, int32 ControllerId, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
	virtual bool InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;
	virtual void ClientTeamMessage_Implementation(APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime) override;
	virtual void ClientReturnToMainMenu_Implementation(const FText& ReturnReason) override;
	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;
	virtual void StartTalking() override;
	virtual void StopTalking() override;
	virtual void ToggleSpeaking(bool bInSpeaking) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	// End APlayerController interface

	/** @return Time remaining for TimerHandle_UnFreeze or -1.f if not set. */
	UFUNCTION(BlueprintPure, Category="Controller")
	float GetTimeUntilUnFreeze() const { return GetWorldTimerManager().GetTimerRemaining(TimerHandle_UnFreeze); }

	// Delegate fired when ClientSetHUD completes
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="HUD")
	FILLOnPlayerHUDCreated OnPlayerHUDCreated;

protected:
	// XboxOne: Take Kinect GPU reserve when this controller is locally controlled?
	// Set this to true for "in-game" player controllers to unlock the 7th core on XboxOne
	bool bTakeKinectCPUReserve;
	
	//////////////////////////////////////////////////////////////////////////
	// Online
public:
	/** @return true if we are using multiplayer features. */
	virtual bool IsUsingMultiplayerFeatures() const;

	/** @return true if this local player is lagging out. */
	UFUNCTION(BlueprintPure, Category="Online")
	bool IsLaggingOut() const;

	// Lobby client actor linked during login, only valid on authority
	TWeakObjectPtr<AILLLobbyBeaconClient> LinkedLobbyClient;

	// Is this player lagging out?
	bool bLagging;

protected:
	// Last state of IsUsingMultiplayerFeatures polled in PlayerTick
	bool bUsedMultiplayerFeatures;

	// Have we sent our initial SetUsingMultiplayerFeatures status?
	bool bUpdatedMultiplayerFeatures;

	//////////////////////////////////////////////////////////////////////////
	// Travel
public:
	/** Return the client to the main menu and start matchmaking automatically. */
	UFUNCTION(Reliable, Client)
	virtual void ClientReturnToMatchMaking();

	/** Server telling this client to show their loading screen for travel. */
	UFUNCTION(Client, Reliable)
	virtual void ClientShowLoadingScreen(const FURL& NextURL);

	/**
	 * @param bCanUpdate Allow this call to update the cached bHasFullyTraveled status if not already true.
	 * @return true if this player has fully traveled and synchronized.
	 */
	UFUNCTION(BlueprintPure, Category="Travel")
	virtual bool HasFullyTraveled(const bool bCanUpdate = false) const;

	/** @return true if we can kick PlayerState. */
	virtual bool CanKickPlayer(AILLPlayerState* KickPlayerState) const;

	/** Client is requesting to kick a player. Check if they are host, and if so allow it. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRequestKickPlayer(AILLPlayerState* KickPlayerState);

	/** Cache the current menu stack before traveling. */
	UFUNCTION(BlueprintCallable, Category = "Travel")
	virtual void CacheMenuStack();

	/** 
	 * Load and reset our cached menu stack. 
	 * @return True if a cached menu stack was restored.
	 */
	UFUNCTION(BlueprintCallable, Category = "Travel")
	virtual bool RestoreMenuStack();

protected:
	/** Helper function for attempting to start the game session. Registers voice. */
	virtual void AttemptOrRetryStartGameSession();

	// Should MyHUD persist across levels when using seamless travel?
	UPROPERTY(EditDefaultsOnly, Category="Travel")
	bool bSeamlessTravelHUD;

	// Has this player state fully traveled into the current match?
	mutable bool bHasFullyTraveled;

	// Timer handle for AttemptOrRetryStartGameSession
	FTimerHandle TimerHandle_AttemptOrRetryStartGameSession;

	//////////////////////////////////////////////////////////////////////////
	// Invite handling
public:
	/** Call this before showing your main menu to process any pending deferred invites. */
	UFUNCTION(BlueprintCallable, Category="Invite")
	bool FlushPendingInvite();

	//////////////////////////////////////////////////////////////////////////
	// Player initialization
public:
	/** @return true if this player has acked their UPlayer. */
	FORCEINLINE bool HasReceivedPlayer() const { return bHasReceivedPlayer; }

	// Last PlayerState the client acknowledged
	UPROPERTY()
	APlayerState* AcknowledgedPlayerState;

protected:
	/** Calls ServerAcknowledgePlayerState for local clients. */
	virtual void AcknowledgePlayerState(APlayerState* PS);

	/** Server notification for the PlayerState last received. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerAcknowledgePlayerState(APlayerState* PS);

	/** Called from OnPostLoadMap to notify the server that this player has finished loading. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerNotifyReceivedPlayer();

	// Flagged locally in ReceivedPlayer and in ServerNotifyReceivedPlayer so the server knows
	UPROPERTY(Transient)
	bool bHasReceivedPlayer;

	//////////////////////////////////////////////////////////////////////////
	// Message handling
public:
	/** @return true if AILLGameMode::ConditionalBroadcast should call ClientTeamMessage on this. */
	virtual bool CanReceiveTeamMessage(AILLPlayerState* Sender, FName Type) const;

	/** @return List of received notifications. */
	FORCEINLINE const TArray<UILLPlayerNotificationMessage*>& GetNotificationMessages() const { return NotificationMessages; }

	/** Localized notification handler for party member states. */
	virtual void HandleLocalizedNotification(UILLPlayerNotificationMessage* Notification);

	/** Broadcast a "say" message to all other players. */
	UFUNCTION(BlueprintCallable, Exec, Category="Messages")
	virtual void Say(const FString& Message);

	/** Sends the server your "say" message to send to all other players. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSay(const FString& Message);

	// Tag name for the broadcast from ServerSay
	static FName NAME_Say;

	// Delegate fired when HandleLocalizedNotification is called
	UPROPERTY(BlueprintAssignable, Category="Messages")
	FILLOnPlayerReceivedNotification OnPlayerReceivedNotification;

	// Message length limit for Say
	UPROPERTY(Config)
	int32 SayLengthLimit;

protected:
	// List of notification messages received
	UPROPERTY(Transient)
	TArray<UILLPlayerNotificationMessage*> NotificationMessages;

	// Maximum number of notification messages to buffer in NotificationMessages
	UPROPERTY(Config)
	int32 MaxNotificationMessages;

	//////////////////////////////////////////////////////////////////////////
	// Backend
public:
	/** @return Backend user handle for the local player. */
	UILLBackendPlayer* GetLocalBackendPlayer() const;

	/** @return Casted Backend player handle. */
	template<class T>
	FORCEINLINE T* GetLocalBackendPlayer(bool bCastChecked = true) const
	{
		if (bCastChecked)
		{
			return CastChecked<T>(GetLocalBackendPlayer(), ECastCheckedType::NullAllowed);
		}
		else
		{
			return Cast<T>(GetLocalBackendPlayer());
		}
	}

	/**
	 * Called on the client and server to cache off the backend account identifier for this player.
	 * Since servers do not have the UILLLocalPlayer that houses the UILLBackendPlayer, this allows them to identify this player.
	 * It is only simulated for the local player for simplicity.
	 */
	virtual void SetBackendAccountId(const FILLBackendPlayerIdentifier& InBackendAccountId);

	/** @return ILLAccountID for this player. */
	FORCEINLINE const FILLBackendPlayerIdentifier& GetBackendAccountId() const { return BackendAccountId; }

private:
	// AccountID for this player
	FILLBackendPlayerIdentifier BackendAccountId;

	//////////////////////////////////////////////////////////////////////////
	// Input
public:
	/** @return First FKey found for ActionName, or EKeys::AnyKey if not found. */
	UFUNCTION(BlueprintCallable, Category="Input")
	FKey FirstKeyForAction(const FName ActionName) const; // FIXME: pjackson: Move to a BP library?

	/**
	 * Blueprint helper for telling this client to return to the main menu.
	 *
	 * @param ReturnReason Why we are returning to the main menu.
	 */
	UFUNCTION(BlueprintCallable, Category="Input", meta=(DeprecatedFunction, DeprecationMessage="Use ShowLoadingAndReturnToMainMenu instead"))
	void ReturnToMainMenu(const FText& ReturnReason);

	/** @return true if game play related actions (movement, weapon usage, etc) are allowed right now. Affects IsMoveInputIgnored and IsLookInputIgnored. */
	virtual bool IsGameInputAllowed() const;

	/** Inverts the controller or the mouse look. */
	void SetInvertedLook(bool bInInvertedYAxis, const FKey KeyToInvert);

	/** Sets mouse sensitivity value */
	void SetMouseSensitivity(float Value);

	/** Sets horizontal controller sensitivity value */
	void SetControllerHorizontalSensitivity(float Sensitivity);

	/** Sets vertical controller sensitivity value */
	void SetControllerVerticalSensitivity(float Sensitivity);
	
	// Should crouching be a toggle?
	bool bCrouchToggle;

protected:
	/** Callback for ILLPlayerInput::OnUsingGamepadChanged. */
	UFUNCTION()
	virtual void OnUsingGamepadChanged(bool bUsingGamepad);

	// Player input class to use
	UPROPERTY()
	TSubclassOf<UPlayerInput> PlayerInputClass;

	// When bCinematicMode should IsGameInputAllowed still return true?
	bool bAllowGameInputInCinematicMode;

	//////////////////////////////////////////////////////////////////////////
	// Cheat commands
private:
	/**
	 * Sends a cheat command to the server when a client. When the server, it is executed locally.
	 *
	 * @param CheatCommand Cheat console command to send to the server.
	 */
	UFUNCTION(Exec)
	void Cheat(const FString& CheatCommand);

	/**
	 * Receives cheat command requests from players to be processed on the server.
	 *
	 * @param CheatCommand Cheat console command received from the client.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCheat(const FString& CheatCommand);

	//////////////////////////////////////////////////////////////////////////
	// Voice
public:
	/** @return true if this local player is using push to talk. */
	virtual bool IsLocalPlayerUsingPushToTalk() const;

	/** Handles push to talk being off and gamepad related state changes with speaking. */
	virtual void UpdatePushToTalkTransmit();

	// Sound to play when talking starts
	UPROPERTY(EditDefaultsOnly, Category="Voice")
	USoundCue* TalkingStartSound;

	// Sound to play when talking starts
	UPROPERTY(EditDefaultsOnly, Category="Voice")
	USoundCue* TalkingStopSound;

	// Does this session require push to talk?
	bool bSessionRequiresPushToTalk;

	// Last value called to ToggleSpeaking
	bool bIsSpeaking;

	//////////////////////////////////////////////////////////////////////////
	// Entitlements
public:
	/** @return true if this player owns EntitlementId. */
	UFUNCTION(BlueprintCallable, Category="Entitlements") // CLEANUP: dgarcia: BlueprintPure and const...
	bool DoesUserOwnEntitlement(const FString& EntitlementId) const;

	//////////////////////////////////////////////////////////////////////////
	// Persistent Input Binding
public:
	/**
	 * Binds a delegate function to an Action defined in the project settings.
	 * Returned reference is only guaranteed to be valid until another action is bound.
	 */
	template<class UserClass>
	FInputActionBinding& BindPersistentAction(const FName ActionName, const EInputEvent KeyEvent, UserClass* Object, typename FInputActionHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputActionBinding AB(ActionName, KeyEvent);
		AB.ActionDelegate.BindDelegate(Object, Func);
		return AddActionBinding(AB);
	}

	/**
	 * Binds a delegate function an Axis defined in the project settings.
	 * Returned reference is only guaranteed to be valid until another axis is bound.
	 */
	template<class UserClass>
	FInputAxisBinding& BindPersistentAxis(const FName AxisName, UserClass* Object, typename FInputAxisHandlerSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr Func)
	{
		FInputAxisBinding AB(AxisName);
		AB.AxisDelegate.BindDelegate(Object, Func);
		AxisBindings.Add(AB);
		return AxisBindings.Last();
	}

	/** Clear all persistent action bindings */
	void ClearPersistentActions()
	{
		ActionBindings.Reset();
	}

	/** Clear all persistent axis bindings */
	void ClearPersistentAxes()
	{
		AxisBindings.Reset();
	}

	/** Clear all persistent bindings */
	void ClearPersistentBindings()
	{
		ActionBindings.Reset();
		AxisBindings.Reset();
	}

protected:
	/** Verifies an action binding and adds it to our list of persistent actions */
	FInputActionBinding& AddActionBinding(FInputActionBinding& InBinding);

	/** Removes an action binding from our persistent actions */
	void RemoveActionBinding(const int32 BindingIndex);

	/** Iterate through our input action lists and update the input states and call corresponding delegates if necessary */
	void ProcessActions(UPlayerInput* Input);

	/** Iterate through our input axes list and update the input state and call corresponding delegates if necessary */
	void ProcessAxes(UPlayerInput* Input, bool bGamePaused);

	// The collection of axis key bindings
	TArray<FInputActionBinding> ActionBindings;

	// The collection of axis key bindings.
	TArray<FInputAxisBinding> AxisBindings;
};
