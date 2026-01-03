// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPartyBeaconMemberState.h"
#include "SCPartyBeaconMemberState.generated.h"

class ASCPlayerState;

/**
 * @class ASCPartyBeaconMemberState
 */
UCLASS(Config=Engine)
class SUMMERCAMP_API ASCPartyBeaconMemberState
: public AILLPartyBeaconMemberState
{
	GENERATED_UCLASS_BODY()
	
	//////////////////////////////////////////////////
	// Avatar
public:
	/** @return Avatar texture asset. */
	UFUNCTION(BlueprintCallable, Category = "Avatar")
	UTexture2D* GetAvatar();

private:
	// Cached avatar reference
	UPROPERTY()
	UTexture2D* CachedAvatar;
	
	//////////////////////////////////////////////////
	// Profile
public:
	/** Notification from the SCPlayerState that they have updated their backend profile info, and we should copy from it to update. */
	void UpdateBackendProfileFrom(ASCPlayerState* PlayerState);

	/** Request the backend profile information for this user. */
	void RequestBackendProfileUpdate();

	/** @return Ratio into the current level. */
	UFUNCTION(BlueprintPure, Category="Player Profile")
	float GetCurrentLevelPercentage() const;

protected:
	// Begin AILLSessionBeaconMemberState interface
	virtual void OnRep_AccountId() override;
	// End AILLSessionBeaconMemberState interface

	/** Callback for when the RequestBackendProfileUpdate request completes. */
	UFUNCTION()
	void OnRetrieveBackendProfile(int32 ResponseCode, const FString& ResponseContents);

	// Experience from profile
	UPROPERTY(Replicated, Transient)
	int64 TotalExperience;

	// Level from profile
	UPROPERTY(BlueprintReadOnly, Category="Player Profile", Replicated, Transient)
	int32 PlayerLevel;

	// Amount of XP needed to reach our current level
	UPROPERTY(Replicated, Transient)
	int64 XPAmountToCurrentLevel;

	// Amount of XP until the next Level from profile
	UPROPERTY(Replicated, Transient)
	int64 XPAmountToNextLevel;
};
