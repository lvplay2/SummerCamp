// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerState_Online.h"
#include "SCPlayerState_Hunt.generated.h"

/**
 * @enum ESCCharacterPreference
 */
UENUM()
enum class ESCCharacterPreference : uint8
{
	NoPreference,
	Counselor,
	Killer
};

/**
 * @class ASCPlayerState_Hunt
 */
UCLASS()
class ASCPlayerState_Hunt
: public ASCPlayerState_Online
{
	GENERATED_UCLASS_BODY()

	// Begin APlayerState interface
	virtual void SetUniqueId(const FUniqueNetIdRepl& InUniqueId) override;
	// Begin APlayerState interface

	//////////////////////////////////////////////////
	// User preferences
public:
	/** Updates this player's preferences. */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetSpawnPreference(ESCCharacterPreference InSpawnPreference);

	/** @return Last reported spawn preference to the server. */
	FORCEINLINE ESCCharacterPreference GetSpawnPreference() const { return SpawnPreference; }

protected:
	// Last recorded user spawn preference
	ESCCharacterPreference SpawnPreference;

public:
	virtual void OnRevivePlayer() override;
};
