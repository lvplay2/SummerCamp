// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "IHttpRequest.h"
#include "OnlineIdentityInterface.h"

#include "SCGameMode_Online.h"
#include "SCTypes.h"
#include "SCGameMode_Hunt.generated.h"

class UILLBackendSimpleHTTPRequestTask;

class ASCCharacter;
class ASCCounselorCharacter;
class ASCKillerCharacter;
class ASCPlayerController;
class ASCWeapon;

extern TAutoConsoleVariable<int32> CVarDebugSpectating;

/**
 * @class ASCGameMode_Hunt
 */
UCLASS(Config = Game)
class SUMMERCAMP_API ASCGameMode_Hunt
: public ASCGameMode_Online
{
	GENERATED_UCLASS_BODY()

protected:
	// Begin AGameMode interface
	virtual void HandleMatchHasStarted() override;
	virtual void HandleLeavingMap() override;
	// End AGameMode interface

	// Begin AILLGameMode interface
	virtual void GameSecondTimer() override;
	// End AILLGameMode interface

public:
	// Begin ASCGameMode interface
	virtual void CharacterKilled(AController* Killer, AController* KilledPlayer, ASCCharacter* KilledCharacter, const UDamageType* const DamageType) override;
	virtual bool CharacterEscaped(AController* CounselorController, ASCCounselorCharacter* CounselorCharacter) override;
	virtual bool CanTeamKill() const override;
	// End ASCGameMode interface

protected:
	// Begin ASCGameMode interface
	virtual void HandleWaitingPreMatch() override;
	virtual void TickWaitingPreMatch() override;
	virtual bool CanCompleteWaitingPreMatch() const override;
	virtual void HandlePreMatchIntro() override;
	virtual void HandleWaitingPostMatchOutro() override;
	// End ASCGameMode interface

	/** @return Current game state instance. */
	UFUNCTION(BlueprintCallable, Category=GameMode)
	ASCGameState_Hunt* GetSCGameState(); // CLEANUP: pjackson: Bad name

	//////////////////////////////////////////////////
	// Backend Jason selection
protected:
	/** Response handler for JasonSelectRequest. */
	virtual void RequestJasonSelectionCallback(int32 ResponseCode, const FString& ResponseContent);

	// Player that was picked as the killer by the backend
	UPROPERTY(Transient)
	ASCPlayerState* JasonSelectPlayer;

	// Backend request handle for selecting Jason
	TWeakObjectPtr<UILLBackendSimpleHTTPRequestTask> JasonSelectRequest;

	// Time that JasonSelectRequest was created
	float JasonSelectRealTimeSeconds;

	// Have we received our JasonSelectRequest response from the backend?
	bool bReceivedJasonSelectResponse;
};
