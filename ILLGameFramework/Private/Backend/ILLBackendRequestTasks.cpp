// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendRequestTasks.h"

#include "Online.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Resources/Version.h"

#include "ILLBackendRequestManager.h"
#include "ILLBackendPlayer.h"
#include "ILLBackendServer.h"
#include "ILLBuildDefines.h"
#include "ILLGameInstance.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"

bool UILLBackendHTTPRequestTask::PrepareRequest()
{
	if (!Super::PrepareRequest())
		return false;

	const UILLBackendRequestManager* const RequestManager = GetOuterUILLBackendRequestManager();
	const FILLBackendEndpointResource& Resource = RequestManager->EndpointList[Endpoint.EndpointIndex];

	// Setup our debug name
	int32 ColonIndex = INDEX_NONE;
	if (Resource.URL.FindLastChar(TEXT(':'), ColonIndex))
	{
		DebugName = FString::Printf(TEXT("[%s %s %s]"), *GetName(), ToString(Resource.Verb), *Resource.URL.RightChop(ColonIndex));
	}

	// Append user-specific headers
	// Done on the game thread because some online subsystems will crash if you don't, for some of the calls within this function
	if (User)
	{
		User->SetupRequest(HttpRequest);
	}

	return true;
}

void UILLBackendHTTPRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	const UILLBackendRequestManager* const RequestManager = GetOuterUILLBackendRequestManager();
	const FILLBackendEndpointResource& Resource = RequestManager->EndpointList[Endpoint.EndpointIndex];

	// Now fill out the request
	HttpRequest->SetURL(Resource.URL + AsyncGetURLParameters());
	HttpRequest->SetVerb(ToString(Resource.Verb));

	// Append build headers
#if PLATFORM_WINDOWS
	HttpRequest->SetHeader(TEXT("Build-Platform"), TEXT("Windows"));
#elif PLATFORM_LINUX
	HttpRequest->SetHeader(TEXT("Build-Platform"), TEXT("Linux"));
#elif PLATFORM_PS4
	HttpRequest->SetHeader(TEXT("Build-Platform"), TEXT("PS4"));
#elif PLATFORM_XBOXONE
	HttpRequest->SetHeader(TEXT("Build-Platform"), TEXT("XboxOne"));
#else
# error
#endif
	HttpRequest->SetHeader(TEXT("Build-Demo"), FString::FromInt(ILLBUILD_DEMO));
	HttpRequest->SetHeader(TEXT("Build-Number"), FString::FromInt(ILLBUILD_BUILD_NUMBER));
	HttpRequest->SetHeader(TEXT("Build-Changelist"), FString::FromInt(BUILT_FROM_CHANGELIST));

	// When using cURL, turn peer verification on
#if PLATFORM_DESKTOP && UE_BUILD_SHIPPING
	// 02/03/2021 -> disabled to use new backend
	/*HttpRequest->SetHeader(TEXT("Verify-Peer"), TEXT("1"));
	HttpRequest->SetHeader(TEXT("Verify-Host"), TEXT("1"));*/
#endif

	// Append the client key
	if (RequestManager->ClientKeyList.IsValidIndex(Resource.ClientKeyIndex))
	{
		const FGuid& ClientKey = RequestManager->ClientKeyList[Resource.ClientKeyIndex];
		check(ClientKey.IsValid());
		HttpRequest->SetHeader(TEXT("Client-Key"), ClientKey.ToString(EGuidFormats::DigitsWithHyphens).ToLower());
	}
}

void UILLBackendSimpleHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	// See if we should try again
	// Do not bother for 404s
	const int32 ResponseCode = HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0;
	if (RetryCount > 0 && (ResponseCode < 200 || ResponseCode > 299) && ResponseCode != 404)
	{
		UE_LOG(LogILLBackend, Verbose, TEXT("%s: Response code: %i, trying again..."), ANSI_TO_TCHAR(__FUNCTION__), ResponseCode);
		--RetryCount;
		QueueRequest();
		return;
	}

	// Fire our delegate
	if (HttpResponse.IsValid())
	{
		CompletionDelegate.ExecuteIfBound(ResponseCode, HttpResponse->GetContentAsString());
	}
	else
	{
		CompletionDelegate.ExecuteIfBound(0, FString());
	}
}

void UILLBackendAuthHTTPRequestTask::QueueRequest()
{
#if PLATFORM_XBOXONE
	// Assigning this header tells Microsoft's servers to send the Authorization token to the backend
	IOnlineIdentityPtr OnlineIdentity = Online::GetIdentityInterface();
	if (OnlineIdentity.IsValid())
	{
		HttpRequest->SetHeader(TEXT("xbl-authz-actor-10"), OnlineIdentity->GetUserHashToken(LocalPlayer->GetControllerId()));
	}
#endif

	Super::QueueRequest();
}

void UILLBackendAuthHTTPRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	// Tack on the Gamer-Tag header manually, since there is no UILLBackendPlayer yet for SetupUser to do it
	HttpRequest->SetHeader(TEXT("Gamer-Tag"), Nickname);
}

void UILLBackendAuthHTTPRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	ErrorCode = EILLBackendAuthResult::UnknownError;
	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: Failed! Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ToDebugString(HttpResponse));

		ErrorCode = HttpResponse.IsValid() ? EILLBackendAuthResult::InvalidAuthResponse : EILLBackendAuthResult::NoAuthResponse;

		if (HttpResponse.IsValid() && HttpResponse->GetResponseCode() == 401)
		{
			// Parse the Json response to check for a ban
			TSharedPtr<FJsonObject> JsonParsed;
			TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
			if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
			{
				FString ErrorString;
				if (JsonParsed->TryGetStringField(TEXT("Error"), ErrorString))
				{
					if (ErrorString.Contains(TEXT("account is locked"))) // FIXME: pjackson: Ew...
						ErrorCode = EILLBackendAuthResult::AccountBanned;
				}
			}
		}
	}
	else if (HttpResponse->GetContentType() != TEXT("application/json"))
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: Invalid content type \"%s\"!"), ANSI_TO_TCHAR(__FUNCTION__), *HttpResponse->GetContentType());
		ErrorCode = EILLBackendAuthResult::InvalidAuthResponse;
	}
	else
	{
		// Parse the Json response
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
		if (!FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			ErrorCode = EILLBackendAuthResult::InvalidAuthResponse;
		}
		else
		{
			FString AccountId;
			if(!JsonParsed->TryGetStringField(TEXT("UserId"), AccountId)
			|| !JsonParsed->TryGetStringField(TEXT("SessionToken"), SessionToken))
			{
				ErrorCode = EILLBackendAuthResult::IncompleteAuthResponse;
			}
			else
			{
				// Re-use their existing backend user instance if possible
				if (LocalPlayer->GetBackendPlayer())
				{
					User = LocalPlayer->GetBackendPlayer();
				}
				else
				{
					const UILLBackendRequestManager* const RequestManager = GetOuterUILLBackendRequestManager();
					User = NewObject<UILLBackendPlayer>(LocalPlayer, RequestManager->PlayerClass);
				}

				// Cache results to prepare to send the arbiter validate request from the game thread below
				BackendId = FILLBackendPlayerIdentifier(AccountId);
				ErrorCode = EILLBackendAuthResult::Success;
			}
		}
	}
}

void UILLBackendAuthHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	if (ErrorCode == EILLBackendAuthResult::Success)
	{
		// Configure our backend player
		UILLBackendPlayer* PendingBackendPlayer = CastChecked<UILLBackendPlayer>(User);
		PendingBackendPlayer->NotifyAuthenticated(BackendId, SessionToken);
		LocalPlayer->SetCachedBackendPlayer(PendingBackendPlayer);

		// Fill out a request task and queue it
		UILLBackendRequestManager* RequestManager = GetOuterUILLBackendRequestManager();
		UILLBackendArbiterValidateHTTPRequestTask* RequestTask = RequestManager->CreateBackendRequest<UILLBackendArbiterValidateHTTPRequestTask>(RequestManager->ArbiterService_Validate, User);
		RequestTask->LocalPlayer = LocalPlayer;
		RequestTask->CompletionDelegate = CompletionDelegate;
		RequestTask->QueueRequest();
	}
	else
	{
		// Only notify of failure, because we still need the arbiter validate response to complete this process
		CompletionDelegate.ExecuteIfBound(ErrorCode);
	}
}

FString UILLBackendAuthHTTPRequestTask::AsyncGetURLParameters() const
{
#if PLATFORM_DESKTOP
	return FString::Printf(TEXT("?SteamID=%s"), *PlatformId);
#elif PLATFORM_XBOXONE
	return FString();
#elif PLATFORM_PS4
	return FString::Printf(TEXT("?PlaystationID=%s&PlaystationNumericalID=%s"), *Nickname, *PlatformId);
#endif
}

void UILLBackendArbiterHTTPRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	UILLBackendPlayer* BackendPlayer = CastChecked<UILLBackendPlayer>(User);

	// Build our Json body
	// CLEANUP: pjackson: All of these values are in the header, to be corrected on the next project
	FString Payload;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
	JsonWriter->WriteObjectStart();
	JsonWriter->WriteValue(TEXT("SessionToken"), BackendPlayer->GetSessionToken());
	JsonWriter->WriteValue(TEXT("AccountId"), BackendPlayer->GetAccountId().GetIdentifier());
	WriteRequestBody(JsonWriter);
	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	// Set the contents
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Payload);
}

void UILLBackendArbiterValidateHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	// Determine if there was an error
	EILLBackendAuthResult ErrorCode = EILLBackendAuthResult::UnknownError;
	if (HttpResponse.IsValid() && HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ToDebugString(HttpResponse));
		ErrorCode = EILLBackendAuthResult::InvalidArbiterResponseCode;
	}
	else if (!HttpResponse.IsValid())
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ToDebugString(HttpResponse));
		ErrorCode = EILLBackendAuthResult::NoArbiterResponse;
	}
	else
	{
		ErrorCode = EILLBackendAuthResult::Success;
	}

	// Notify the backend player
	UILLBackendPlayer* PendingBackendPlayer = CastChecked<UILLBackendPlayer>(User);
	if (ErrorCode == EILLBackendAuthResult::Success)
	{
		// Store a reference to the new backend user
		UILLBackendRequestManager* RequestManager = GetOuterUILLBackendRequestManager();
		RequestManager->PlayerList.AddUnique(PendingBackendPlayer);

		// Notify the backend user they have been arbitrated
		PendingBackendPlayer->NotifyArbitrated();
	}
	else
	{
		PendingBackendPlayer->NotifyArbitrationFailed();
	}

	// Broadcast the event
	CompletionDelegate.ExecuteIfBound(ErrorCode);
}

void UILLBackendArbiterHeartbeatHTTPRequestTask::WriteRequestBody(TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>>& JsonWriter) const
{
	Super::WriteRequestBody(JsonWriter);

	// Inform the backend of our current map
	JsonWriter->WriteValue(TEXT("MapName"), MapName);

	// Inform the backend of our last known matchmaking status
	UILLBackendPlayer* BackendPlayer = CastChecked<UILLBackendPlayer>(User);
	JsonWriter->WriteValue(TEXT("MatchmakingStatus"), ToString(BackendPlayer->LastMatchmakingStatus));
}

void UILLBackendArbiterHeartbeatHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	EILLBackendArbiterHeartbeatResult ErrorCode = EILLBackendArbiterHeartbeatResult::UnknownError;
	if (HttpResponse.IsValid() && HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ToDebugString(HttpResponse));
		ErrorCode = (HttpResponse->GetResponseCode() == 400) ? EILLBackendArbiterHeartbeatResult::HeartbeatDenied : EILLBackendArbiterHeartbeatResult::InvalidArbiterResponseCode;
	}
	else if (!HttpResponse.IsValid())
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *ToDebugString(HttpResponse));
		ErrorCode = EILLBackendArbiterHeartbeatResult::NoArbiterResponse;
	}
	else
	{
		ErrorCode = EILLBackendArbiterHeartbeatResult::Success;
	}

	CompletionDelegate.ExecuteIfBound(ErrorCode);
}

void UILLBackendKeySessionRetrieveHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	if (HttpResponse.IsValid())
	{
		if (HttpResponse->GetResponseCode() != 200)
		{
			UE_LOG(LogILLBackend, Warning, TEXT("%s: Response code: %i"), ANSI_TO_TCHAR(__FUNCTION__), HttpResponse->GetResponseCode());
		}
		CompletionDelegate.ExecuteIfBound(HttpResponse->GetResponseCode(), HttpResponse->GetContentAsString(), EncryptionDelegate);
	}
	else
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: No or invalid response!"), ANSI_TO_TCHAR(__FUNCTION__));
		CompletionDelegate.ExecuteIfBound(0, FString(), EncryptionDelegate);
	}
}

void UILLBackendMatchmakingStatusHTTPRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (HttpResponse.IsValid())
	{
		if (HttpResponse->GetResponseCode() != 200)
		{
			UE_LOG(LogILLBackend, Warning, TEXT("%s: Response code: %i"), ANSI_TO_TCHAR(__FUNCTION__), HttpResponse->GetResponseCode());
		}
		else
		{
			bool bParseError = true;
			TSharedPtr<FJsonObject> JsonParsed;
			TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
			if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
			{
				const TArray<TSharedPtr<FJsonValue>>* ParsedValues = nullptr;
				if (JsonParsed->TryGetArrayField(TEXT("Statuses"), ParsedValues))
				{
					for (TSharedPtr<FJsonValue> ParsedStatusValue : *ParsedValues)
					{
						const TSharedPtr<FJsonObject>* StatusObject = nullptr;
						if (ParsedStatusValue->TryGetObject(StatusObject))
						{
							FString AccountIdString;
							FString MatchmakingStatusString;
							double MatchmakingBanSeconds = 0.0;
							if((*StatusObject)->TryGetStringField(TEXT("AccountID"), AccountIdString)
							&& (*StatusObject)->TryGetStringField(TEXT("MatchmakingStatus"), MatchmakingStatusString)
							&& (*StatusObject)->TryGetNumberField(TEXT("MatchmakingBanSeconds"), MatchmakingBanSeconds))
							{
								FILLBackendMatchmakingStatus Entry;
								Entry.AccountId = FILLBackendPlayerIdentifier(AccountIdString);
								TSharedPtr<FJsonValue> ErrorField = (*StatusObject)->TryGetField(TEXT("Error"));
								if (ErrorField.IsValid() && !ErrorField->IsNull())
								{
									UE_LOG(LogILLBackend, Warning, TEXT("%s: Received error '%s' for %s"), ANSI_TO_TCHAR(__FUNCTION__), *ErrorField->AsString(), *AccountIdString);
									Entry.MatchmakingStatus = EILLBackendMatchmakingStatusType::Error;
								}
								else
								{
									if (MatchmakingStatusString == TEXT("OK"))
									{
										Entry.MatchmakingStatus = EILLBackendMatchmakingStatusType::Ok;
									}
									else if (MatchmakingStatusString == TEXT("Banned") && ensureAlways(MatchmakingBanSeconds > 0.0))
									{
										Entry.MatchmakingStatus = EILLBackendMatchmakingStatusType::Banned;
										Entry.MatchmakingBanSeconds = (float)MatchmakingBanSeconds;
									}
									else
									{
										check(MatchmakingStatusString == TEXT("LowPriority"));
										Entry.MatchmakingStatus = EILLBackendMatchmakingStatusType::LowPriority;
									}
								}
								Statuses.Add(Entry);
								bParseError = false;
							}
							else
							{
								bParseError = true;
								break;
							}
						}
					}
				}
			}
			if (bParseError)
				Statuses.Empty();
		}
	}
	else
	{
		UE_LOG(LogILLBackend, Warning, TEXT("%s: No or invalid response!"), ANSI_TO_TCHAR(__FUNCTION__));
	}
}

void UILLBackendMatchmakingStatusHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	// Cache matchmaking status for local player(s)
	for (const FILLBackendMatchmakingStatus& StatusEntry : Statuses)
	{
		for (UILLBackendPlayer* BackendPlayer : GetOuterUILLBackendRequestManager()->PlayerList)
		{
			if (StatusEntry.AccountId == BackendPlayer->GetAccountId())
			{
				BackendPlayer->LastMatchmakingStatus = StatusEntry.MatchmakingStatus;
			}
		}
	}

	// Broadcast completion
	CompletionDelegate.ExecuteIfBound(Statuses);
}

FString UILLBackendMatchmakingStatusHTTPRequestTask::AsyncGetURLParameters() const
{
	FString Params = TEXT("?AccountID=");
	Params += AccountIdList[0].GetIdentifier();
	for (int32 Index = 1; Index < AccountIdList.Num(); ++Index)
	{
		Params += TEXT("&AccountID=");
		Params += AccountIdList[Index].GetIdentifier();
	}

	return Params;
}

bool UILLBackendPutXBLSessionHTTPRequestTask::PrepareRequest()
{
	if (!Super::PrepareRequest())
		return false;

	// Grab the session settings for our Steam game session
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	if (!Session || Session->SessionState != EOnlineSessionState::InProgress)
	{
		UE_LOG(LogILLHTTP, Error, TEXT("%s: Failed! Game session invalid or not InProgress!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	SessionSettings = Session->SessionSettings;
	UILLBackendRequestManager* RequestManager = GetOuterUILLBackendRequestManager();
	UILLGameInstance* GameInstance = RequestManager->GetOuterUILLGameInstance();
	UILLOnlineSessionClient* SessionClient = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());
	SessionClient->UpdateGameSessionSettings(SessionSettings);

	return true;
}

void UILLBackendPutXBLSessionHTTPRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	UILLBackendRequestManager* RequestManager = GetOuterUILLBackendRequestManager();
	UILLGameInstance* GameInstance = RequestManager->GetOuterUILLGameInstance();
	UILLOnlineSessionClient* SessionClient = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());

#if PLATFORM_DESKTOP && UE_BUILD_SHIPPING
	HttpRequest->SetHeader(TEXT("Verify-Peer"), TEXT("0")); // FIXME: pjackson: Hack because the cert is not working on this endpoint
	HttpRequest->SetHeader(TEXT("Verify-Host"), TEXT("0"));
#endif

	// Generate our JSON payload
	FString Payload;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload);
	JsonWriter->WriteObjectStart();

	JsonWriter->WriteObjectStart(TEXT("constants"));
		JsonWriter->WriteObjectStart(TEXT("system"));
			JsonWriter->WriteValue(TEXT("visibility"), TEXT("open"));
		JsonWriter->WriteObjectEnd();
	JsonWriter->WriteObjectEnd();

	JsonWriter->WriteObjectStart(TEXT("members"));
		JsonWriter->WriteObjectStart(TEXT("reserve_0"));
			JsonWriter->WriteObjectStart(TEXT("constants"));
			//	JsonWriter->WriteObjectStart(TEXT("custom"));
			//	JsonWriter->WriteObjectEnd();
				JsonWriter->WriteObjectStart(TEXT("system"));
					JsonWriter->WriteValue(TEXT("xuid"), InitiatorXUID);
				JsonWriter->WriteObjectEnd();
			JsonWriter->WriteObjectEnd();
		JsonWriter->WriteObjectEnd();
	JsonWriter->WriteObjectEnd();

	// Build the session settings flags
	int32 BitShift = 0;
	int32 SessionSettingsFlags = 0;
	SessionSettingsFlags |= ((int32)SessionSettings.bShouldAdvertise) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bAllowJoinInProgress) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bIsLANMatch) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bIsDedicated) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bUsesStats) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bAllowInvites) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bUsesPresence) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bAllowJoinViaPresence) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bAllowJoinViaPresenceFriendsOnly) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bAntiCheatProtected) << BitShift++;
	SessionSettingsFlags |= ((int32)SessionSettings.bIsPassworded) << BitShift++;

	// Fill out properties object
	JsonWriter->WriteObjectStart(TEXT("properties"));
		JsonWriter->WriteObjectStart(TEXT("custom"));
			JsonWriter->WriteValue(TEXT("_flags"), FString::Printf(TEXT("%d"), SessionSettingsFlags));
			JsonWriter->WriteValue(SETTING_BEACONPORT.ToString(), SessionClient->GetLobbyBeaconHostListenPort());
		JsonWriter->WriteObjectEnd();
	JsonWriter->WriteObjectEnd();

	// Build the host address
	FString HostAddr = GameInstance->GetBackendRequestManager()->GetBackendServer()->GenerateHostName();
	HostAddr += TEXT(":");
	UWorld* World = GameInstance->GetWorld();
	HostAddr += FString::FromInt(World->URL.Port);

	// Grab the server name
	FString ServerName;
	FParse::Value(FCommandLine::Get(), TEXT("ServerName="), ServerName);

	// Fill out servers object
	JsonWriter->WriteObjectStart(TEXT("servers"));
		JsonWriter->WriteObjectStart(TEXT("Game"));
			JsonWriter->WriteObjectStart(TEXT("constants"));
				JsonWriter->WriteObjectStart(TEXT("custom"));
					JsonWriter->WriteValue(TEXT("HostAddr"), HostAddr);
					JsonWriter->WriteValue(TEXT("HostName"), ServerName);
					JsonWriter->WriteValue(TEXT("InitiatorXUID"), InitiatorXUID);
				JsonWriter->WriteObjectEnd();
			JsonWriter->WriteObjectEnd();
		JsonWriter->WriteObjectEnd();
	JsonWriter->WriteObjectEnd();

	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();

	// Set the contents
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Payload);
}

void UILLBackendPutXBLSessionHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	UILLBackendRequestManager* RequestManager = GetOuterUILLBackendRequestManager();

	const bool bSuccessful = (HttpResponse.IsValid() && HttpResponse->GetResponseCode() >= 200 && HttpResponse->GetResponseCode() <= 299 && !RequestManager->PendingXBLSessionName.IsEmpty());
	RequestManager->ActiveXBLSessionName.Empty();
	RequestManager->ActiveXBLSessionUri.Empty();
	RequestManager->PendingXBLSessionName.Empty();
	if (bSuccessful)
	{
		FString ServiceConfigId;
		verify(GConfig->GetString(TEXT("/Script/XboxOnePlatformEditor.XboxOneTargetSettings"), TEXT("ServiceConfigId"), ServiceConfigId, GEngineIni));

		RequestManager->ActiveXBLSessionName = SessionName;
		RequestManager->ActiveXBLSessionUri = FString::Printf(TEXT("/serviceconfigs/%s/sessiontemplates/%s/sessions/%s"), *ServiceConfigId, *XBOXONE_QUICKPLAY_SESSIONTEMPLATE, *SessionName);
	}

	CompletionDelegate.ExecuteIfBound(RequestManager->ActiveXBLSessionName, RequestManager->ActiveXBLSessionUri);
}

FString UILLBackendPutXBLSessionHTTPRequestTask::AsyncGetURLParameters() const
{
	FString ServiceConfigId;
	verify(GConfig->GetString(TEXT("/Script/XboxOnePlatformEditor.XboxOneTargetSettings"), TEXT("ServiceConfigId"), ServiceConfigId, GEngineIni));

	FString TitleId;
	verify(GConfig->GetString(TEXT("/Script/XboxOnePlatformEditor.XboxOneTargetSettings"), TEXT("TitleId"), TitleId, GEngineIni));

	return FString::Printf(TEXT("?SandboxID=%s&TitleID=%s&SCID=%s&SessionTemplateName=%s&SessionName=%s"), *SandboxId, *TitleId, *ServiceConfigId, *XBOXONE_QUICKPLAY_SESSIONTEMPLATE, *SessionName);
}
