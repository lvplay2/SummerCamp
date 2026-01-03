// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameState_Lobby.h"
#include "SCGameState_Lobby.generated.h"

class USCMapDefinition;
class USCMapRegistry;
class USCModeDefinition;

// NOTE: If you make more settings, move these to their own file, don't need a bunch of custom settings clogging up the game state
/**
 * @enum ESCRainSetting
 */
UENUM(BlueprintType)
enum class ESCRainSetting : uint8
{
	Random,
	On,
	Off
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCRainSettingUpdated, ESCRainSetting, NewRainSetting);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCModeUpdated, TSubclassOf<USCModeDefinition>, NewModeClass);

/**
 * @class ASCGameState_Lobby
 */
UCLASS(Config = Game)
class SUMMERCAMP_API ASCGameState_Lobby
: public AILLGameState_Lobby
{
	GENERATED_UCLASS_BODY()

	// Begin AGameState interface
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	// End AGameState interface
		
protected:
	// Begin AILLGameState_Lobby interface
	virtual FString GetSelectedLobbyMap() const override;
	virtual FString GetSelectedGameMode() const override;
	virtual FString GetLevelLoadOptions() const override;
	virtual void OnRep_TravelCountdown() override;
	virtual void TravelCountdownTimer() override;
	// End AILLGameState_Lobby interface

public:
	/** @return true if we can no longer unready. */
	UFUNCTION(BlueprintPure, Category="Lobby")
	bool IsTravelCountdownLow() const { return (bTravelCountingDown && TravelCountdown < LowTravelCountdown); }

	/** @return true if we can no longer unready. */
	UFUNCTION(BlueprintPure, Category="Lobby")
	bool IsTravelCountdownVeryLow() const { return (bTravelCountingDown && TravelCountdown < VeryLowTravelCountdown); }

	/** Changes CurrentMapClass. */
	void SetCurrentMapClass(TSubclassOf<USCMapDefinition> NewMapClass);

	/** Changes CurrentModeClass. */
	void SetCurrentModeClass(TSubclassOf<USCModeDefinition> NewModeClass);

protected:
	// What is considered a low travel countdown timer
	UPROPERTY(Config)
	int32 LowTravelCountdown;

	// What is considered a VERY low travel countdown timer
	UPROPERTY(Config)
	int32 VeryLowTravelCountdown;

	// Sound to play when the travel timer is low
	UPROPERTY()
	USoundCue* LowTravelTickSound;

	// Map registry
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<USCMapRegistry> MapRegistryClass;

	// Currently selected map for this lobby
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	TSubclassOf<USCMapDefinition> CurrentMapClass;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GameMode, Transient)
	TSubclassOf<USCModeDefinition> CurrentModeClass;

	UFUNCTION()
	void OnRep_GameMode();

	UPROPERTY(BlueprintAssignable, Category = "Custom Settings")
	FSCModeUpdated OnGameModeUpdated;

	//////////////////////////////////////////////////
	// Game 
protected:
	// Can the host pick a killer for the match?
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	bool bHostCanPickKiller;

	//////////////////////////////////////////////////
	// Custom Settings
public:
	/** Setter for RainSetting */
	UFUNCTION(BlueprintCallable, Category="Custom Settings")
	void SetRainSetting(const ESCRainSetting NewSetting);

	// Blueprint handler for when the rain option gets changed
	UPROPERTY(BlueprintAssignable, Category="Custom Settings")
	FSCRainSettingUpdated OnRainSettingUpdated;

protected:
	// Lobby setting for rain, carries through to the map
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RainSetting, Transient)
	ESCRainSetting RainSetting;

	/** Rep handler for rain setting changes, calls to OnRainSettingUpdated */
	UFUNCTION()
	void OnRep_RainSetting();

protected:
	// Game Settings
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iJasonPowers;                                             // 0x0490(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iJasonStartingItems;                                      // 0x0494(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bJasonRageOnStart;                                        // 0x0498(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bJasonDisableStunAnim;                                    // 0x0499(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iJasonThrowingKnifeDamage;                                // 0x049C(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bJasonNoWiggleOnGrab;                                     // 0x04A0(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iJasonAttackDamage;                                       // 0x04A4(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bJasonAlwaysVisibleOnMap;                                 // 0x04A8(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bJasonLoseSenseOnRage;                                    // 0x04A9(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorHudOff;                                         // 0x04AA(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorWeakerPerks;                                    // 0x04AB(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorNoPerks;                                        // 0x04AC(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorNoTommySpawn;                                   // 0x04AD(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorTommyIgnoresRage;                               // 0x04AE(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorFearMaxAtStart;                                 // 0x04AF(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorFriendlyFire;                                   // 0x04B0(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iCounselorHealth;                                         // 0x04B4(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bCounselorMoreFear;                                       // 0x04B8(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bMapRemoveMaps;                                           // 0x04B9(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bMapRemoveHealthSpray;                                    // 0x04BA(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bMapAllPowerOff;                                          // 0x04BB(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bMapNoSweater;                                            // 0x04BC(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iMapHalfVehicleSpawn;                                     // 0x04C0(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bMapVehiclesStartFixed;                                   // 0x04C4(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iMapTimer;                                                // 0x04C8(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bMapPoliceEscapeFromStart;                                // 0x04CC(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		int                                                iMapRarerWeapons;                                         // 0x04D0(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bNoShotgun;                                               // 0x04D4(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UPROPERTY(BlueprintReadOnly, Replicated)
		bool                                               bNoPocketKnives;                                          // 0x04D5(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData)
	UFUNCTION(BlueprintCallable)
		void CopyCustomMatchSettings();
};
