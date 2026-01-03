// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCPartyStatusWidget.generated.h"

class AILLSessionBeaconMemberState;

class ASCPartyBeaconMemberState;
class ASCPartyBeaconState;
class ASCPlayerState;

class USCLocalPlayer;
class USCOnlineSessionClient;

/**
 * @class USCPartyStatusWidget
 */
UCLASS()
class SUMMERCAMP_API USCPartyStatusWidget
: public USCUserWidget
{
	GENERATED_BODY()

	// Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	// End UUserWidget interface

protected:
	/** Called to poll for our party beacon state to grab members from. */
	void PollPartyState();

	/** @return Online session client from our world's game instance. Helper function. */
	USCOnlineSessionClient* GetOnlnineSessionClient() const;

	/** @return Party beacon state, if leading or a member of a party. */
	ASCPartyBeaconState* GetPartyBeaconState() const;

	/** @return Number of party members, excluding the local player. */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="PartyStatus")
	int32 GetNumOtherPartyMembers() const { return AddedMembers.Num(); }

	/** Event for when a party member is added. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="PartyStatus", meta=(BlueprintProtected))
	void OnPartyMemberAdded(ASCPartyBeaconMemberState* PartyMemberState);

	/** Event for when a party member is removed. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="PartyStatus", meta=(BlueprintProtected))
	void OnPartyMemberRemoved(ASCPartyBeaconMemberState* PartyMemberState);

	/** Callback for ASCPartyBeaconState::OnBeaconMemberAdded. */
	virtual void OnBeaconMemberAdded(AILLSessionBeaconMemberState* MemberState);

	/** Callback for ASCPartyBeaconState::OnBeaconMemberRemoved. */
	virtual void OnBeaconMemberRemoved(AILLSessionBeaconMemberState* MemberState);

	// List of previously-added members
	UPROPERTY(Transient)
	TArray<AILLSessionBeaconMemberState*> AddedMembers;

	// Local player
	UPROPERTY(Transient)
	USCLocalPlayer* LocalPlayer;

	// Player state for the local player
	UPROPERTY(BlueprintReadOnly, Transient, Category="PartyStatus")
	ASCPlayerState* LocalPlayerState;

	// Do we have our party beacon state?
	bool bHasPartyState;
};
