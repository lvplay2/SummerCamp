// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLSessionBeaconState.h"
#include "ILLPartyBeaconState.generated.h"

struct FILLBackendPlayerIdentifier;

namespace ILLPartyLeaderState
{
	extern ILLGAMEFRAMEWORK_API FName Idle;
	extern ILLGAMEFRAMEWORK_API FName Matchmaking;
	extern ILLGAMEFRAMEWORK_API FName CreatingPrivateMatch;
	extern ILLGAMEFRAMEWORK_API FName CreatingPublicMatch;
	extern ILLGAMEFRAMEWORK_API FName JoiningGame;
	extern ILLGAMEFRAMEWORK_API FName InMatch;
};

/**
 * @class AILLPartyBeaconState
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API AILLPartyBeaconState
: public AILLSessionBeaconState
{
	GENERATED_UCLASS_BODY()

	/** @return List of AccountIDs from this party. */
	TArray<FILLBackendPlayerIdentifier> GenerateAccountIdList() const;

	/** @return Party leader state name. */
	FORCEINLINE FName GetLeaderStateName() const { return LeaderStateName; }

	/** @return Localized party leader state description. */
	virtual FText GetLeaderStateDescription() const;

	/** Changes the LeaderStateName and simulates OnRep_LeaderStateName. */
	virtual void SetLeaderState(FName NewState)
	{
		if (LeaderStateName != NewState)
		{
			LeaderStateName = NewState;
			OnRep_LeaderStateName();
			ForceNetUpdate();
		}
	}

protected:
	/** Replication handler for LeaderStateName. */
	UFUNCTION()
	virtual void OnRep_LeaderStateName();

	// Current "state" of what our party leader is doing
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_LeaderStateName)
	FName LeaderStateName;
};
