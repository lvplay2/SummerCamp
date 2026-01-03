// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGame_Lobby.h"
#include "SCGame_Lobby.generated.h"

namespace ILLLobbyAutoStartReasons
{
	// Custom auto-start check reasons
	extern FName PlayerReady;
}

/**
 * @class SCGame_Lobby
 */
UCLASS()
class SUMMERCAMP_API ASCGame_Lobby
: public AILLGame_Lobby
{
	GENERATED_UCLASS_BODY()

	// Begin AGameModeBase interface
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	virtual void SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(class AController* NewPlayer) override;
	// End AGameModeBase interface

public:
	// Begin AILLGameMode interface
	virtual void GameSecondTimer() override;
	// End AILLGameMode interface

	// Begin AILLGame_Lobby interface
	virtual void CheckAutoStart(FName AutoStartReason, AController* RelevantController = nullptr) override;
	// End AILLGame_Lobby interface

protected:
	// Duration of the countdown for an auto-start when all players are ready
	UPROPERTY(Config)
	int32 AutoStartCountdownAllReady;
};
