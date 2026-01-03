// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Engine/GameEngine.h"

#ifndef USE_GAMELIFT_SERVER_SDK
	#define USE_GAMELIST_SERVER_SDK 0
#endif

#if USE_GAMELIFT_SERVER_SDK
# include "GameLiftServerSDK.h"
#endif

#include "ILLGameSession.h"
#include "ILLProjectSettings.h"
#include "ILLGameEngine.generated.h"

/**
 * @class UILLGameEngine
 */
UCLASS(Config=Engine)
class ILLGAMEFRAMEWORK_API UILLGameEngine
: public UGameEngine
{
	GENERATED_UCLASS_BODY()

	// Begin UEngine interface
	virtual void Init(class IEngineLoop* InEngineLoop) override;
	virtual void Start() override;
	virtual void PreExit() override;
	virtual void Tick(float DeltaSeconds, bool bIdleMode) override;
	// End UEngine interface

	//////////////////////////////////////////////////
	// Deferred dedicated server handling
public:
	/** Calls directly to Super::Start. */
	virtual void ForceStart();

	/** @return true if we want to defer dedicated server map loading. Reduces CPU usage and ensures seamless travel due to not ticking world time. */
	virtual bool UseDeferredDedicatedStart() const;

	// Have we started the game engine?
	bool bHasStarted;

protected:
	// Game session class to use for a deferred-dedicated server, MaxPlayers is pulled from here
	// Setting this enables deferred dedicated server use
	UPROPERTY()
	TSubclassOf<AILLGameSession> DeferredDedicatedGameSessionClass;

	// Time between game session updates for deferred dedicated state
	float DeferredDedicatedSessionUpdateInterval;

	// Time until we handshake with the backend and OSS
	float TimeUntilDeferredDedicatedSessionUpdate;

#if USE_GAMELIFT_SERVER_SDK
	//////////////////////////////////////////////////
	// GameLift integration
public:
	/** @return GameLift module instance. */
	FGameLiftServerSDKModule& GetGameLiftModule() const;

	/** @return GameSessionId found for this game session after it started. */
	FORCEINLINE const FString& GetGameLiftGameSessionId() const { return GameSessionId; }

protected:
	/** GameLift notification for when we need to start the game session. Used to ForceStart. */
	virtual void GameLift_OnStartGameSession(Aws::GameLift::Server::Model::GameSession GameSession);

	/** GameLift notification for when this instance is about to be shutdown. */
	virtual void GameLift_OnTerminate();

	// Has GameLift requested a game session start?
	FThreadSafeBool bGameLiftStartRequested;

	// Game session information cached in GameLift_OnStartGameSession
	FString GameSessionId;

	// Has GameLift requested a termination?
	FThreadSafeBool bGameLifeTerminateRequested;
#endif // USE_GAMELIFT_SERVER_SDK
};
