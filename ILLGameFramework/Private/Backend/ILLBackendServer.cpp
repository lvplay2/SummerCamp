// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendServer.h"

#include "JsonUtilities.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Resources/Version.h"

#if USE_GAMELIFT_SERVER_SDK
# include "GameLiftServerSDK.h"
#endif

#include "ILLBackendBlueprintLibrary.h"
#include "ILLBackendRequestManager.h"
#include "ILLBuildDefines.h"
#include "ILLGameEngine.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLGameSession.h"
#include "ILLOnlineSessionClient.h"

UILLBackendServer::UILLBackendServer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DedicatedArbiterHeartbeatInterval = 30.f;
}

UWorld* UILLBackendServer::GetWorld() const
{
	return GetOuterUILLBackendRequestManager()->GetWorld();
}

UILLGameInstance* UILLBackendServer::GetGameInstance() const
{
	return GetOuterUILLBackendRequestManager()->GetOuterUILLGameInstance();
}

FTimerManager& UILLBackendServer::GetTimerManager() const
{
	return GetOuterUILLBackendRequestManager()->GetTimerManager();
}

FString UILLBackendServer::GenerateHostName() const
{
	FString Hostname;

	// Take the parameter hostname if specified
	if (FParse::Value(FCommandLine::Get(), TEXT("Hostname="), Hostname))
		return Hostname;

#ifdef STEAMKEY_HOSTIP
	// Grab the key from Steam
	if (IsRunningDedicatedServer())
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface();
		if (FNamedOnlineSession* GameSession = Sessions->GetNamedSession(NAME_GameSession))
		{
			GameSession->SessionSettings.Get(STEAMKEY_HOSTIP, Hostname);
		}
	}
#else
	check(0);
#endif

	return Hostname;
}

void UILLBackendServer::GenerateServerReport(TSharedPtr<FJsonObject> JsonObject)
{ 
	UILLGameInstance* GameInstance = GetGameInstance();
	UILLOnlineSessionClient* SessionClient = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());
	UWorld* World = GetWorld();

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
	if (SessionInt.IsValid())
	{
		FNamedOnlineSession* Session = SessionInt->GetNamedSession(NAME_GameSession);
		if (Session && Session->SessionState == EOnlineSessionState::InProgress)
		{
			// Basic platform information
			JsonObject->SetNumberField(TEXT("build_number"), ILLBUILD_BUILD_NUMBER);
			JsonObject->SetNumberField(TEXT("changelist"), BUILT_FROM_CHANGELIST);
			JsonObject->SetStringField(TEXT("owning_userid"), Session->OwningUserId->ToString());

			// Figure out the port and map
			int32 GameServerGamePort = 7777;
			FString MapName;
			if (World)
			{
				GameServerGamePort = World->URL.Port;
				MapName = World->GetMapName();
			}
			else
			{
				if (FParse::Value(FCommandLine::Get(), TEXT("Port="), GameServerGamePort) == false)
				{
					GConfig->GetInt(TEXT("URL"), TEXT("Port"), GameServerGamePort, GEngineIni);
				}
				MapName = UGameMapsSettings::GetServerDefaultMap();
			}
			JsonObject->SetNumberField(TEXT("port"), GameServerGamePort);

			// Command-line parameters to send
			JsonObject->SetStringField(TEXT("hostname"), GenerateHostName());

			FString ServerName;
			FParse::Value(FCommandLine::Get(), TEXT("ServerName="), ServerName);
			JsonObject->SetStringField(TEXT("server_name"), ServerName);

			FString ServerRegion;
			FParse::Value(FCommandLine::Get(), TEXT("ServerRegion="), ServerRegion);
			JsonObject->SetStringField(TEXT("server_region"), ServerRegion);

			AILLGameMode* GameMode = World ? World->GetAuthGameMode<AILLGameMode>() : nullptr;
			JsonObject->SetNumberField(TEXT("num_players"), FMath::Max(GameMode ? GameMode->GetNumPlayers() : 0, SessionClient->GetLobbyMemberList().Num()));
			if (GameMode)
			{
				// Get Game mode
				JsonObject->SetStringField(TEXT("game_mode"), GameMode->GetClass()->GetName());
				if (AILLGameSession* GameSession = Cast<AILLGameSession>(GameMode->GameSession))
				{
					JsonObject->SetNumberField(TEXT("max_players"), GameSession->GetMaxPlayers());
					JsonObject->SetBoolField(TEXT("password"), GameSession->IsPassworded());
				}
			}
		}
	}
}

FString UILLBackendServer::GenerateHeartbeat()
{
	FString Payload;

	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
	JsonWriter->WriteObjectStart();

	int32 GameServerGamePort = 7777;
	if (UWorld* World = GetWorld())
	{
		GameServerGamePort = World->URL.Port;
	}
	else if (FParse::Value(FCommandLine::Get(), TEXT("Port="), GameServerGamePort) == false)
	{
		GConfig->GetInt(TEXT("URL"), TEXT("Port"), GameServerGamePort, GEngineIni);
	}
	JsonWriter->WriteValue(TEXT("port"), GameServerGamePort);

	JsonWriter->WriteValue(TEXT("hostname"), GenerateHostName());

	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	return Payload;
}

void UILLBackendServer::SendReport()
{
	UE_LOG(LogILLBackend, Verbose, TEXT("%s::SendReport"), *GetName());

	if (bPendingReportResponse)
	{
		// Queue another report update after we get a response for our last
		// This prevents us from hammering the backend with reports when this is called too many times
		bReportQueuedAfterResponse = true;
		return;
	}

	// Cancel the heartbeat timer, it will be resumed after the response comes back
	if (TimerHandle_ArbiterHeartbeat.IsValid())
	{
		GetTimerManager().ClearTimer(TimerHandle_ArbiterHeartbeat);
		TimerHandle_ArbiterHeartbeat.Invalidate();
	}

	if (UWorld* World = GetWorld())
	{
		// Generate Report
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		GenerateServerReport(JsonObject);

		FString Report;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Report);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

		// Send Server Report
		bPendingReportResponse = GetOuterUILLBackendRequestManager()->SendServerReport(Report, FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnServerReport));
	}
}

void UILLBackendServer::OnServerReport(int32 ResponseCode, const FString& ResponseContents)
{
	bPendingReportResponse = false;

	if (ResponseCode == 200)
	{
		UE_LOG(LogILLBackend, Verbose, TEXT("%s::OnServerReport: ResponseCode 200"), *GetName());

		// Break out of user-requested offline mode
		GILLUserRequestedBackendOfflineMode = false;
	}
	else
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s::OnServerReport: ResponseCode %i"), *GetName(), ResponseCode);
	}

	// Was SendReport called before this response came? Fire it off again
	if (bReportQueuedAfterResponse)
	{
		SendReport();
		bReportQueuedAfterResponse = false;
	}
	else
	{
		// Cancel the existing heartbeat timer
		if (TimerHandle_ArbiterHeartbeat.IsValid())
		{
			GetTimerManager().ClearTimer(TimerHandle_ArbiterHeartbeat);
			TimerHandle_ArbiterHeartbeat.Invalidate();
		}

		// Set a timer to send heartbeats
		GetTimerManager().SetTimer(
			TimerHandle_ArbiterHeartbeat,
			this,
			&ThisClass::SendBackendServerHeartbeat,
			DedicatedArbiterHeartbeatInterval,
			true);
	}
}

void UILLBackendServer::SendBackendServerHeartbeat()
{
	if (bPendingReportResponse)
		return;

	UE_LOG(LogILLBackend, Verbose, TEXT("%s::SendBackendServerHeartbeat"), *GetName());

	// Generate Report
	GetOuterUILLBackendRequestManager()->SendServerHeartbeat(GenerateHeartbeat(), FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnServerHeartbeatResponse));
}

void UILLBackendServer::OnServerHeartbeatResponse(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode == 200)
	{
		UE_LOG(LogILLBackend, Verbose, TEXT("%s::OnServerHeartbeatResponse: ResponseCode 200"), *GetName());
	}
	else
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s::OnServerHeartbeatResponse: ResponseCode %i"), *GetName(), ResponseCode);

		if (ResponseContents.Contains(TEXT("report")))
		{
			// Try to re-establish the connection by sending a report
			SendReport();
		}
		else
		{
			// Otherwise just keep sending heartbeats...
		}
	}
}
