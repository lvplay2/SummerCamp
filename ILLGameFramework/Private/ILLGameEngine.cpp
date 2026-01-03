// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameEngine.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"
#include "Resources/Version.h"

#if USE_GAMELIFT_SERVER_SDK
# include "GameLiftServerSDK.h"
#endif

#include "ILLBuildDefines.h"
#include "ILLOnlineSessionClient.h"

UILLGameEngine::UILLGameEngine(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DeferredDedicatedSessionUpdateInterval = 30.f;
}

void UILLGameEngine::Init(class IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);

	// Host a game session so that we can be found
	if (UseDeferredDedicatedStart())
	{
		// Do not send an update immediately (@see Tick)
		TimeUntilDeferredDedicatedSessionUpdate = 1.f;

		// Host a game session
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());
		const AILLGameSession* const DefaultGameSession = DeferredDedicatedGameSessionClass->GetDefaultObject<AILLGameSession>();
		TSharedPtr<FILLGameSessionSettings> HostSettings = MakeShareable(new FILLGameSessionSettings(false, DefaultGameSession->GetMaxPlayers(), true, false));
		OSC->HostGameSession(HostSettings);
	}

	// Update the viewport window title
	if (GameViewportWindow.IsValid())
	{
		const FString CurrentTitle(GameViewportWindow.Pin()->GetTitle().ToString());
		FString NewTitle(CurrentTitle);

#if ILLBUILD_DEMO
		NewTitle += TEXT(" - Demo");
#endif

		NewTitle += FString::Printf(TEXT(" - B%d"), ILLBUILD_BUILD_NUMBER);
#if !UE_BUILD_SHIPPING
		NewTitle += FString::Printf(TEXT(" %s"), BUILD_VERSION);
#endif

		if (NewTitle != CurrentTitle)
		{
			GameViewportWindow.Pin()->SetTitle(FText::FromString(NewTitle));
		}
	}
}

void UILLGameEngine::PreExit()
{
	Super::PreExit();

#if USE_GAMELIFT_SERVER_SDK
	if (IsRunningDedicatedServer())
	{
		// Notify GameLift we are going to end this session
		FGameLiftServerSDKModule& GameLiftModule = GetGameLiftModule();
		GameLiftModule.TerminateGameSession();
		GameLiftModule.ProcessEnding();
	}
#endif // USE_GAMELIFT_SERVER_SDK
}

void UILLGameEngine::Start()
{
#if USE_GAMELIFT_SERVER_SDK
	// GIsRequestingExit can be flagged true if we fail to host a game session or lobby beacon
	if (IsRunningDedicatedServer() && !GIsRequestingExit)
	{
		FGameLiftServerSDKModule& GameLiftModule = GetGameLiftModule();

		// InitSDK establishes a local connection with GameLift's agent to enable further communication
		// This call blocks until there is a response!
		GameLiftModule.InitSDK();
	}
#endif // USE_GAMELIFT_SERVER_SDK

	// Always start when not running a dedicated server or not set to defer it
	if (!UseDeferredDedicatedStart())
	{
		ForceStart();
	}
	else
	{
		UE_LOG(LogIGF, Verbose, TEXT("Deferring UILLGameInstance start"));
	}

#if USE_GAMELIFT_SERVER_SDK
	// GIsRequestingExit can be flagged true if we fail to host a game session or lobby beacon
	if (IsRunningDedicatedServer() && !GIsRequestingExit)
	{
		FGameLiftServerSDKModule& GameLiftModule = GetGameLiftModule();
		FProcessParameters* GameLiftParams = new FProcessParameters();

		// When a game session is created, GameLift sends an activation request to the game server and passes along the game session object containing game properties and other settings.
		// Here is where a game server should take action based on the game session object.
		// Once the game server is ready to receive incoming player connections, it should invoke GameLiftServerAPI.ActivateGameSession()
		GameLiftParams->OnStartGameSession.BindLambda([=](Aws::GameLift::Server::Model::GameSession gameSession) { GameLift_OnStartGameSession(gameSession); });

		// OnProcessTerminate callback. GameLift invokes this callback before shutting down an instance hosting this game server.
		// It gives this game server a chance to save its state, communicate with services, etc., before being shut down.
		GameLiftParams->OnTerminate.BindLambda([=](){ GameLift_OnTerminate(); });

		// This is the HealthCheck callback.
		// GameLift invokes this callback every 60 seconds or so.
		// Here, a game server might want to check the health of dependencies and such.
		// Simply return true if healthy, false otherwise.
		// The game server has 60 seconds to respond with its health status. GameLift defaults to 'false' if the game server doesn't respond in time.
		// In this case, we're always healthy!
		GameLiftParams->OnHealthCheck.BindLambda([](){ return true; });

		// Report our beacon port as the game port since arbitration is done on that
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());
		check(OSC->IsHostingLobbyBeacon());
		check(OSC->GetLobbyBeaconHostListenPort() != 0);
		GameLiftParams->port = OSC->GetLobbyBeaconHostListenPort();

		// Here, the game server tells GameLift what set of files to upload when the game session ends.
		// GameLift uploads everything specified here for the developers to fetch later.
		TArray<FString> LogPaths;
# if PLATFORM_LINUX
		const TCHAR* BaseGameLiftPath = TEXT("/local/game/");
# elif PLATFORM_WINDOWS
		const TCHAR* BaseGameLiftPath = TEXT("c:/game/");
# endif
		const FString SavedPath = FPaths::Combine(BaseGameLiftPath, FApp::GetProjectName(), TEXT("Saved"));
		LogPaths.Add(FPaths::Combine(*SavedPath, TEXT("Logs/")));
		LogPaths.Add(FPaths::Combine(*SavedPath, TEXT("Crashes/")));
		GameLiftParams->logParameters = LogPaths;

		// Calling ProcessReady tells GameLift this game server is ready to receive incoming game sessions!
		FGameLiftGenericOutcome Outcome = GameLiftModule.ProcessReady(*GameLiftParams);
		check(Outcome.success);
	}
#endif // USE_GAMELIFT_SERVER_SDK
}

void UILLGameEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	Super::Tick(DeltaSeconds, bIdleMode);

#if USE_GAMELIFT_SERVER_SDK
	if (IsRunningDedicatedServer())
	{
		if (bGameLiftStartRequested)
		{
			UE_LOG(LogIGF, Log, TEXT("GameLift start requested"));
			bGameLiftStartRequested = false;

			// Force the server to start (load the default server map)
			if (!bHasStarted)
			{
				ForceStart();
			}

			// Tell GameLift we're ready
			FGameLiftServerSDKModule& GameLiftModule = GetGameLiftModule();
			GameLiftModule.ActivateGameSession();
		}
		else if (bGameLifeTerminateRequested)
		{
			UE_LOG(LogIGF, Log, TEXT("GameLift termination requested"));
			bGameLifeTerminateRequested = false;

			// Request an exit, eventually ProcessEnding hits via PreExit
			GIsRequestingExit = true;
		}
	}
#endif // USE_GAMELIFT_SERVER_SDK

	if (!bHasStarted)
	{
		UILLOnlineSessionClient* OSC = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());
		if (OSC->IsHostingLobbyBeacon())
		{
			IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
			FNamedOnlineSession* Session = SessionInt.IsValid() ? SessionInt->GetNamedSession(NAME_GameSession) : nullptr;
			if (Session && Session->SessionState == EOnlineSessionState::InProgress)
			{
				// Send a game session update if we are due
				TimeUntilDeferredDedicatedSessionUpdate -= DeltaSeconds;
				if (TimeUntilDeferredDedicatedSessionUpdate <= 0.f)
				{
					OSC->SendGameSessionUpdate();

					TimeUntilDeferredDedicatedSessionUpdate = DeferredDedicatedSessionUpdateInterval;
				}
			}
		}

		// Update at only 10FPS when we have not started yet!
		FPlatformProcess::Sleep(.1f);
	}
}

void UILLGameEngine::ForceStart()
{
	bHasStarted = true;

	Super::Start();
}

bool UILLGameEngine::UseDeferredDedicatedStart() const
{
	if (IsRunningDedicatedServer() && DeferredDedicatedGameSessionClass)
	{
		static bool bDeferredDedicated = !FParse::Param(FCommandLine::Get(), TEXT("NoDeferredDedicated"));
		return bDeferredDedicated;
	}

	return false;
}

#if USE_GAMELIFT_SERVER_SDK

FGameLiftServerSDKModule& UILLGameEngine::GetGameLiftModule() const
{
	static FName NAME_GameLiftServerSDK(TEXT("GameLiftServerSDK"));
	return FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(NAME_GameLiftServerSDK);
}

void UILLGameEngine::GameLift_OnStartGameSession(Aws::GameLift::Server::Model::GameSession GameSession)
{
	// Cache information
	GameSessionId = UTF8_TO_TCHAR(GameSession.GetGameSessionId());

	// Trigger start last
	bGameLiftStartRequested = true;
}

void UILLGameEngine::GameLift_OnTerminate()
{
	bGameLifeTerminateRequested = true;
}

#endif // USE_GAMELIFT_SERVER_SDK
