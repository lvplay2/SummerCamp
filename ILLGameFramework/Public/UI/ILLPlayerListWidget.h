// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLUserWidget.h"
#include "ILLPlayerListWidget.generated.h"

class AILLLobbyBeaconMemberState;
class AILLPlayerState;
class UILLPlayerListItemWidget;

/**
 * @class UILLPlayerListWidget
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLPlayerListWidget
: public UILLUserWidget
{
	GENERATED_UCLASS_BODY()

	// Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void SetPlayerContext(const FLocalPlayerContext& InPlayerContext) override;
	// End UUserWidget interface

protected:
	/** UMG notification for when a PlayerWidget was added. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="ILLPlayerList", meta=(BlueprintProtected))
	void AddPlayerWidget(UILLPlayerListItemWidget* PlayerWidget);

	/** UMG notification for when a PlayerWidget was removed. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="ILLPlayerList", meta=(BlueprintProtected))
	void RemovePlayerWidget(UILLPlayerListItemWidget* PlayerWidget);

	/** Exchanges each element of PlayerList with NewPlayerList before taking it as PlayerList. */
	UFUNCTION(BlueprintCallable, Category="ILLPlayerListItemWidget")
	void ExchangePlayerList(const TArray<UILLPlayerListItemWidget*>& NewPlayerList);

	/** @return Widget instance for MemberState. */
	UILLPlayerListItemWidget* FindWidgetForMember(AILLLobbyBeaconMemberState* MemberState) const;

	/** @return Widget instance for PlayerState. */
	UILLPlayerListItemWidget* FindWidgetForPlayer(AILLPlayerState* PlayerState) const;

	/** Notification for when a lobby beacon member state registers. */
	virtual void LobbyBeaconMemberStateAdded(AILLLobbyBeaconMemberState* MemberState);

	/** Notification for when a lobby beacon member state unregisters. */
	virtual void LobbyBeaconMemberStateRemoved(AILLLobbyBeaconMemberState* MemberState);

	/** Notification for when a player state registers with the game session. */
	virtual void PlayerStateRegistered(AILLPlayerState* PlayerState);

	/** Notification for when a player state unregisters with the game session. */
	virtual void PlayerStateUnregistered(AILLPlayerState* PlayerState);

	/** AActor::OnDestroyed callback. */
	UFUNCTION()
	virtual void OnLobbyMemberDestroyed(AActor* DestroyedMember);

	/** AActor::OnDestroyed callback. */
	UFUNCTION()
	virtual void OnPlayerStateDestroyed(AActor* DestroyedPlayer);

	/** Called to update our lobby and player state lists. */
	virtual void PollForPlayers();

	/** Calls AttempToSynchronize on all PlayerList entries. */
	virtual void SynchronizePlayers();

	// List of player widget entries
	UPROPERTY(BlueprintReadOnly, Category="ILLPlayerList", Transient)
	TArray<UILLPlayerListItemWidget*> PlayerList;

	// Class to use for spawned player entries
	UPROPERTY(EditDefaultsOnly, Category="ILLPlayerList")
	TSubclassOf<UILLPlayerListItemWidget> PlayerWidgetClass;

	// Defer registering other player states until a local player registers?
	UPROPERTY(EditDefaultsOnly, Category="ILLPlayerList")
	bool bDeferPlayerStatesForLocal;

	// Registered lobby beacon members
	UPROPERTY(Transient)
	TArray<AILLLobbyBeaconMemberState*> RegisteredLobbyMembers;

	// Registered game player states
	UPROPERTY(Transient)
	TArray<AILLPlayerState*> RegisteredPlayerStates;
};
