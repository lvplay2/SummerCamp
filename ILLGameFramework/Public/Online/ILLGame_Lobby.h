// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLGameMode.h"
#include "ILLGame_Lobby.generated.h"

namespace ILLLobbyAutoStartReasons
{
	// Auto-start check reasons
	extern ILLGAMEFRAMEWORK_API FName PostBeaconLogin;
	extern ILLGAMEFRAMEWORK_API FName PostLogin;
	extern ILLGAMEFRAMEWORK_API FName Logout;
	extern ILLGAMEFRAMEWORK_API FName PostSeamlessTravel;
}

/**
 * @class AILLGame_Lobby
 */
UCLASS(Abstract)
class ILLGAMEFRAMEWORK_API AILLGame_Lobby
: public AILLGameMode
{
	GENERATED_UCLASS_BODY()

public:
	// Begin AGameMode interface
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void PostSeamlessTravel() override;
	virtual void HandleMatchHasEnded() override;
	// End AGameMode interface

protected:
	// Begin AILLGameMode interface
	virtual void AddLobbyMemberState(AILLLobbyBeaconMemberState* MemberState) override;
	virtual void RemoveLobbyMemberState(AILLLobbyBeaconMemberState* MemberState) override;
	// End AILLGameMode interface

	//////////////////////////////////////////////////
	// Auto-starting
public:
	/** Checks if the AutoStartNumPlayers condition has been met and we should autostart, or if we hit below CancelAutoStartMinPlayers players and need to cancel that. */
	virtual void CheckAutoStart(FName AutoStartReason, AController* RelevantController = nullptr);

protected:
	// Duration of the countdown for an auto-start, for bCanAutoStart
	UPROPERTY(Config)
	int32 AutoStartCountdown;

	// LAN/Private AutoStartCountdown override, when <0 means no timer, 0 means use AutoStartCountdown, >0 is the countdown override
	UPROPERTY(Config)
	int32 AutoStartCountdownLANPrivate;

	// Duration of the countdown for an auto-start when there is only one person, for bCanAutoStart
	// When >0 this will be used instead of AutoStartCountdown when there is only one player
	UPROPERTY(Config)
	int32 AutoStartCountdownSolo;

	// LAN/Private AutoStartCountdownSolo override, when <0 means no timer, 0 means use AutoStartCountdownSolo, >0 is the countdown override
	UPROPERTY(Config)
	int32 AutoStartCountdownLANPrivateSolo;

	// Duration of the countdown for an auto-start when the server fills up, for bCanAutoStart
	// When >0 and the server hits the AGameSession::MaxPlayers, the countdown timer will drop to this
	UPROPERTY(Config)
	int32 AutoStartCountdownFull;

	// LAN/Private AutoStartCountdownFull override, when <0 means no timer, 0 means use AutoStartCountdownFull, >0 is the countdown override
	UPROPERTY(Config)
	int32 AutoStartCountdownLANPrivateFull;

	// Minimum auto-start countdown timer when someone hits the lobby beacon (connects before loading)
	// When >0 will increase a previous auto-start countdown timer when someone else joins the lobby
	UPROPERTY(Config)
	int32 MinAutoStartCountdownOnBeaconJoin;

	// Minimum auto-start countdown timer when someone joins
	// When >0 will increase a previous auto-start countdown timer when someone else joins the lobby
	UPROPERTY(Config)
	int32 MinAutoStartCountdownOnJoin;

	// Minimum number of players in the lobby for an auto-start
	UPROPERTY(Config)
	int32 AutoStartNumPlayers;

	// Minimum number of players in the lobby for an auto-start in LAN/Private, 0 (default) to use the same setting as AutoStartNumPlayers or -1 to disable for LAN
	UPROPERTY(Config)
	int32 AutoStartNumLANPrivatePlayers;

	// Minimum number of players in the lobby otherwise a previously started auto-start will be canceled
	UPROPERTY(Config)
	int32 CancelAutoStartMinPlayers;

	// Automatically start the travel countdown when we hit capacity?
	UPROPERTY(Config)
	bool bCanAutoStart;
};
