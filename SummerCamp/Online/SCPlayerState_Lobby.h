// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerState.h"
#include "SCPerkData.h"
#include "SCPlayerState_Lobby.generated.h"

struct FSCCounselorOutfit;
class USCMapDefinition;
class USCModeDefinition;

/**
 * @class ASCPlayerState_Lobby
 */
UCLASS()
class ASCPlayerState_Lobby 
: public ASCPlayerState
{
	GENERATED_UCLASS_BODY()

	// Begin ASCPlayerState interface
	virtual bool CanChangePickedCharacters() const override;
	// End ASCPlayerState interface
	
	//////////////////////////////////////////////////
	// Readying
public:
	/** @return true if we can ready up. */
	UFUNCTION(BlueprintPure, Category = "Lobby|Readying")
	bool CanReady() const;

	/** @return true if we can un-ready. */
	UFUNCTION(BlueprintPure, Category = "Lobby|Readying")
	bool CanUnReady() const;

	/** Forces the player to be readied up. */
	void ForceReady();

	/** Server request to ready this player. */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Lobby|Readying")
	void ServerSetReady(bool bNewReady);

	/** @return true if this player is ready. */
	FORCEINLINE bool IsReady() const { return bIsReady; }

protected:
	/** Replication handler for bIsReady. */
	UFUNCTION()
	void OnRep_bIsReady();

	// Sound to play when a remote player readies
	UPROPERTY()
	USoundCue* RemoteReadiedSound;

	// Sound to play when a remote player unreadies
	UPROPERTY()
	USoundCue* RemoteUnReadiedSound;

	// Determines if player is ready to start match
	UPROPERTY(Transient, ReplicatedUsing=OnRep_bIsReady, BlueprintReadOnly, Category = "Lobby|Readying")
	bool bIsReady;

	//////////////////////////////////////////////////
	// Map selection
public:
	/** Request to change the map class. */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Lobby|MapSelection")
	void ServerRequestMapClass(TSubclassOf<USCMapDefinition> NewMapClass);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Lobby|ModeSelection")
	void ServerRequestModeClass(TSubclassOf<USCModeDefinition> NewModeClass);
};
