// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPSNRequestTasks.h"

#include "Base64.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendRequestManager.h"
#include "ILLBackendServer.h"
#include "ILLGameInstance.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPSNRequestManager.h"

void UILLPSNHTTPRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	if (OnlineEnvironment == EOnlineEnvironment::Development || OnlineEnvironment == EOnlineEnvironment::Certification)
	{
		UILLPSNRequestManager* RequestManager = GetOuterUILLPSNRequestManager();
		HttpRequest->SetHeader(TEXT("Proxy-Address"), RequestManager->DevelopmentProxyAddress);
		HttpRequest->SetHeader(TEXT("Proxy-UserPassword"), RequestManager->DevelopmentProxyUserPass);
	}
}

void UILLPSNAuthorizationRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	HttpRequest->SetHeader(TEXT("Content-type"), TEXT("application/x-www-form-urlencoded"));

	UILLPSNRequestManager* RequestManager = GetOuterUILLPSNRequestManager();
	const FGuid& ClientId = RequestManager->ClientId;
	FString ClientSecretString;
	ClientSecretString.Empty(RequestManager->ClientSecret.Num()+1);
	for (uint8 SecretByte : RequestManager->ClientSecret)
	{
		ClientSecretString.AppendChar((TCHAR)SecretByte);
	}
	const FString Authorization(ClientId.ToString(EGuidFormats::DigitsWithHyphens).ToLower() + TEXT(":") + ClientSecretString);
	HttpRequest->SetHeader(TEXT("Authorization"), FString(TEXT("Basic ")) + FBase64::Encode(Authorization));
}

FString UILLPSNAuthorizationRequestTask::GenerateOAuthURL() const
{
	FString RequestURL(TEXT("https://auth.api."));
	switch (OnlineEnvironment)
	{
	case EOnlineEnvironment::Development: RequestURL += TEXT("sp-int."); break;
	case EOnlineEnvironment::Certification: RequestURL += TEXT("prod-qa."); break;
	}
	RequestURL += TEXT("sonyentertainmentnetwork.com/2.0/oauth/token");
	return RequestURL;
}

void UILLPSNGetAccessTokenRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	FString RequestBody = TEXT("grant_type=authorization_code&redirect_uri=orbis%3A%2F%2Fgames&code=");
	RequestBody += ClientAuthCode;
	HttpRequest->SetContentAsString(RequestBody);
	HttpRequest->SetURL(GenerateOAuthURL());
	HttpRequest->SetVerb(TEXT("POST"));
}

void UILLPSNGetAccessTokenRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *HttpRequest->GetURL(), *ToDebugString(HttpResponse));
		return;
	}

	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Parse the JSON
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		JsonParsed->TryGetStringField(TEXT("access_token"), AccessToken);
		JsonParsed->TryGetNumberField(TEXT("expires_in"), ExpiresInSeconds);
	}
}

void UILLPSNGetAccessTokenRequestTask::HandleResponse()
{
	Super::HandleResponse();

	if (!AccessToken.IsEmpty())
	{
		// Now we need to validate
		UILLPSNValidateAccessTokenRequestTask* RequestTask = GetOuterUILLPSNRequestManager()->CreatePSNRequest<UILLPSNValidateAccessTokenRequestTask>();
		RequestTask->AccessToken = AccessToken;
		RequestTask->ClientAppId = ClientAppId;
		RequestTask->ExpiresInSeconds = ExpiresInSeconds - FMath::RoundToInt(ProcessingTime);
		RequestTask->OnlineEnvironment = OnlineEnvironment;
		RequestTask->CompletionDelegate = CompletionDelegate;
		RequestTask->QueueRequest();
	}
	else
	{
		CompletionDelegate.ExecuteIfBound(false);
	}
}

void UILLPSNValidateAccessTokenRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	FString RequestURL = GenerateOAuthURL();
	RequestURL.AppendChar(TEXT('/'));
	RequestURL += AccessToken;
	HttpRequest->SetURL(RequestURL);
	HttpRequest->SetVerb(TEXT("GET"));
}

void UILLPSNValidateAccessTokenRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *HttpRequest->GetURL(), *ToDebugString(HttpResponse));
		return;
	}

	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Parse the JSON
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		JsonParsed->TryGetStringField(TEXT("country_code"), CountryCode);
	}
}

void UILLPSNValidateAccessTokenRequestTask::HandleResponse()
{
	Super::HandleResponse();

	// Check for failure
	if (AccessToken.IsEmpty() || CountryCode.IsEmpty())
	{
		CompletionDelegate.ExecuteIfBound(false);
		return;
	}

	// Adjust ExpiresInSeconds with the time spent on this request
	ExpiresInSeconds -= GetTotalRequestTime();

	// Store the access token in the manager's cache
	GetOuterUILLPSNRequestManager()->RegisterAccessToken(ClientAppId, AccessToken, CountryCode, OnlineEnvironment, ExpiresInSeconds);

	// Broadcast success
	CompletionDelegate.ExecuteIfBound(true);
}

void UILLPSNBaseUrlRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	FString RequestURL(TEXT("https://asm."));
	switch (OnlineEnvironment)
	{
	case EOnlineEnvironment::Development: RequestURL += TEXT("sp-int."); break;
	case EOnlineEnvironment::Certification: RequestURL += TEXT("prod-qa."); break;
	case EOnlineEnvironment::Production: RequestURL += TEXT("np."); break;
	}
	RequestURL += TEXT("community.playstation.net/asm/v1/apps/me/baseUrls/");
	RequestURL += ApiGroup;
	HttpRequest->SetURL(RequestURL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
}

void UILLPSNBaseUrlRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *HttpRequest->GetURL(), *ToDebugString(HttpResponse));
		return;
	}

	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Parse the age from the Cache-Control header
	const FString CacheControlHeader = HttpResponse->GetHeader(TEXT("Cache-Control"));
	TArray<FString> CacheControlSettings;
	CacheControlHeader.ParseIntoArray(CacheControlSettings, TEXT(", "), true);
	for (FString& CacheSetting : CacheControlSettings)
	{
		if (CacheSetting.RemoveFromStart(TEXT("max-age=")))
		{
			ExpiresInSeconds = FCString::Atoi(*CacheSetting);
		}
	}

	// Parse the JSON
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		JsonParsed->TryGetStringField(TEXT("url"), ReceivedUrl);
	}
}

void UILLPSNBaseUrlRequestTask::HandleResponse()
{
	Super::HandleResponse();

	// Broadcast completion
	CompletionDelegate.ExecuteIfBound(this);
}

void UILLPSNDeleteSessionRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	HttpRequest->SetVerb(TEXT("DELETE"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
	HttpRequest->SetURL(FString::Printf(TEXT("%s/v1/sessions/%s"), *BaseURL, *SessionId));
}

void UILLPSNDeleteSessionRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 204)
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *HttpRequest->GetURL(), *ToDebugString(HttpResponse));
		return;
	}
}

void UILLPSNPostSessionRequestTask::AsyncPrepareRequest()
{
	Super::AsyncPrepareRequest();

	// This is more or less duplicate code, but using standardized HTTPS requests
	// @see OnlineSessionInterfacePS4.cpp
	UILLPSNRequestManager* RequestManager = GetOuterUILLPSNRequestManager();

	// Load the InviteIcon
	// @see FOnlineAsyncTaskPS4SessionCreate::DoWork
	TArray<uint8> ImageData;
	{
		IFileHandle* ImageFileHandle = nullptr;
		FString ImageFullPath = FPaths::EngineDir() + TEXT("Build/PS4/InviteIcon.jpg"); // TODO: pjackson: This will need to package with non-PS4 builds too now...
		ImageFileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*ImageFullPath);
		if (ImageFileHandle)
		{
			ImageData.AddZeroed(ImageFileHandle->Size());
			if (!ImageFileHandle->Read(ImageData.GetData(), ImageData.Num()))
			{
				ImageData.Empty();
			}
			delete ImageFileHandle;
		}
	}
	if (ImageData.Num() == 0)
	{
		UE_LOG(LogILLHTTP, Error, TEXT("%s: Failed! Session image is empty or not found!"), ANSI_TO_TCHAR(__FUNCTION__));
		RequestManager->CancelRequest(this);
		return;
	}

	// Collect the current platform's game session settings
	IOnlineSessionPtr Sessions = Online::GetSessionInterface();
	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	if (!Session || Session->SessionState != EOnlineSessionState::InProgress)
	{
		UE_LOG(LogILLHTTP, Error, TEXT("%s: Failed! Game session invalid or not InProgress!"), ANSI_TO_TCHAR(__FUNCTION__));
		RequestManager->CancelRequest(this);
		return;
	}
	FOnlineSessionSettings SessionSettings = Session->SessionSettings;
	UILLGameInstance* GameInstance = RequestManager->GetOuterUILLGameInstance();
	UILLOnlineSessionClient* SessionClient = CastChecked<UILLOnlineSessionClient>(GameInstance->GetOnlineSession());
	SessionClient->UpdateGameSessionSettings(SessionSettings);

	// Build the session request JSON part
	// @see FOnlineAsyncTaskPS4SessionCreate::DoWork
	FString SessionRequestJsonStr;
	SessionRequestJsonStr += TEXT("{");
	SessionRequestJsonStr += FString::Printf(TEXT("\"npTitleId\" : \"%s\", "), *ClientAppId); // This parameter is specific to application servers
	SessionRequestJsonStr += FString::Printf(TEXT("\"sessionPrivacy\" : \"%s\", "), SessionSettings.bShouldAdvertise ? TEXT("public") : TEXT("private"));
	SessionRequestJsonStr += FString::Printf(TEXT("\"sessionMaxUser\" : %d, "), SessionSettings.NumPrivateConnections + SessionSettings.NumPublicConnections);
	SessionRequestJsonStr += FString::Printf(TEXT("\"sessionName\" : \"%s\", "), *Session->SessionName.ToString());
	SessionRequestJsonStr += TEXT("\"index\" : 0, ");
	SessionRequestJsonStr += TEXT("\"priority\" : 1, ");
	bool bUseHostMigration = false;
	SessionSettings.Get(SETTING_HOST_MIGRATION, bUseHostMigration);
	SessionRequestJsonStr += TEXT("\"sessionType\" : ");
	SessionRequestJsonStr += bUseHostMigration ? TEXT("\"owner-migration\", ") : TEXT("\"owner-bind\", ");
	SessionRequestJsonStr += TEXT("\"availablePlatforms\" : [ \"PS4\" ]");
	SessionRequestJsonStr += TEXT("}");

	// Build the session data part
	// @see CopyCustomSettingDataToBuffer
	FString SessionData(TEXT("DedicatedServer"));
	check(SessionData.Len() <= 1024);

	// Build the changeable session data part
	// @see FOnlineAsyncTaskPS4SessionCreate::DoWork
	UWorld* World = GameInstance->GetWorld();
	FString ServerName;
	FParse::Value(FCommandLine::Get(), TEXT("ServerName="), ServerName);
	FString HostURL = GameInstance->GetBackendRequestManager()->GetBackendServer()->GenerateHostName();
	HostURL += TEXT(":");
	HostURL += FString::FromInt(World->URL.Port);
	HostURL += TEXT("?bIsDedicated");
	if (SessionSettings.bShouldAdvertise)
		HostURL += TEXT("?bShouldAdvertise");
	if (SessionSettings.bAllowJoinInProgress)
		HostURL += TEXT("?bAllowJoinInProgress");
	if (SessionSettings.bAllowInvites)
		HostURL += TEXT("?bAllowInvites");
	HostURL += TEXT("?ServerName=");
	HostURL += ServerName;
	HostURL += TEXT("?BeaconPort=");
	HostURL += FString::FromInt(SessionClient->GetLobbyBeaconHostListenPort());
	FString ChangeableSessionData = HostURL;
	check(ChangeableSessionData.Len() <= 1024);

	// Build the PreImageContents and PostImageContents
	FString PreImageContents;
	PreImageContents += TEXT("--boundary_parameter\r\n");
	PreImageContents += TEXT("Content-Type:application/json; charset=utf-8\r\n");
	PreImageContents += TEXT("Content-Description:session-request\r\n");
	PreImageContents += FString::Printf(TEXT("Content-Length:%i\r\n\r\n"), SessionRequestJsonStr.Len());
	PreImageContents += SessionRequestJsonStr;
	PreImageContents += TEXT("\r\n");
	PreImageContents += TEXT("--boundary_parameter\r\n");
	PreImageContents += TEXT("Content-Type:image/jpeg\r\n");
	PreImageContents += TEXT("Content-Description:session-image\r\n");
	PreImageContents += TEXT("Content-Disposition:attachment\r\n");
	PreImageContents += FString::Printf(TEXT("Content-Length:%i\r\n\r\n"), ImageData.Num());
	FString PostImageContents;
	PostImageContents += TEXT("\r\n");
	PostImageContents += TEXT("--boundary_parameter\r\n");
	PostImageContents += TEXT("Content-Type:application/octet-stream\r\n");
	PostImageContents += TEXT("Content-Description:session-data\r\n");
	PostImageContents += TEXT("Content-Disposition:attachment\r\n");
	PostImageContents += FString::Printf(TEXT("Content-Length:%i\r\n\r\n"), SessionData.Len());
	PostImageContents += SessionData;
	PostImageContents += TEXT("\r\n");
	PostImageContents += TEXT("--boundary_parameter\r\n");
	PostImageContents += TEXT("Content-Type:application/octet-stream\r\n");
	PostImageContents += TEXT("Content-Description:changeable-session-data\r\n");
	PostImageContents += TEXT("Content-Disposition:attachment\r\n");
	PostImageContents += FString::Printf(TEXT("Content-Length:%i\r\n\r\n"), ChangeableSessionData.Len());
	PostImageContents += ChangeableSessionData;
	PostImageContents += TEXT("\r\n");
	PostImageContents += TEXT("--boundary_parameter--\r\n");

	// Build the ContentPayload
	TArray<uint8> ContentPayload;
	FTCHARToUTF8 PreImageConverter(*PreImageContents);
	FTCHARToUTF8 PostImageConverter(*PostImageContents);
	ContentPayload.SetNum(PreImageConverter.Length()+ImageData.Num()+PostImageConverter.Length());
	FMemory::Memcpy(ContentPayload.GetData(), (uint8*)(ANSICHAR*)PreImageConverter.Get(), PreImageConverter.Length());
	FMemory::Memcpy(ContentPayload.GetData()+PreImageConverter.Length(), ImageData.GetData(), ImageData.Num());
	FMemory::Memcpy(ContentPayload.GetData()+PreImageConverter.Length()+ImageData.Num(), (uint8*)(ANSICHAR*)PostImageConverter.Get(), PostImageConverter.Length());

	// Form this into a request
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("multipart/mixed; boundary=boundary_parameter"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
	HttpRequest->SetURL(FString::Printf(TEXT("%s/v1/sessions"), *BaseURL));
	HttpRequest->SetContent(ContentPayload);
}

void UILLPSNPostSessionRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s Response: %s"), ANSI_TO_TCHAR(__FUNCTION__), *HttpRequest->GetURL(), *ToDebugString(HttpResponse));
		return;
	}

	// Parse the JSON
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		JsonParsed->TryGetStringField(TEXT("sessionId"), SessionId);
	}
}

void UILLPSNPostSessionRequestTask::HandleResponse()
{
	Super::HandleResponse();

	UILLPSNRequestManager* RequestManager = GetOuterUILLPSNRequestManager();

	RequestManager->GameSessionId = SessionId;
	RequestManager->bCreatingGameSession = false;
	CompletionDelegate.ExecuteIfBound(SessionId);
}
