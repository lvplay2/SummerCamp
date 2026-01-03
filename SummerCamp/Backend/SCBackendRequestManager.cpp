// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBackendRequestManager.h"

#include "SCBackendPlayer.h"
#include "SCBackendServer.h"

#define PROFILE_REQUEST_RETRY_COUNT 2
#define PERK_REQUEST_RETRY_COUNT 1

namespace InfractionTypes
{
	const FString LeftDuringIntroSequence(TEXT("INF-6"));
	const FString HostAbandonedMatch(TEXT("INF-5"));
	const FString JasonLeftWhileAlive(TEXT("INF-2"));
	const FString JasonLeftWhileBeingKilled(TEXT("INF-0"));
	const FString CounselorBetrayedCounselor(TEXT("INF-4"));
	const FString CounselorLeftWhileAlive(TEXT("INF-3"));
	const FString CounselorLeftWhileBeingKilled(TEXT("INF-1"));
}

void USCBackendNewsHTTPRequestTask::AsyncPrepareResponse()
{
	Super::AsyncPrepareResponse();

	if (!HttpResponse.IsValid() || HttpResponse->GetResponseCode() != 200)
	{
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Grab the total amount of articles
		ArticleCount = JsonObject->GetIntegerField(TEXT("TotalCount"));

		const TArray<TSharedPtr<FJsonValue>>* JsonItems = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("NewsItems"), JsonItems))
		{
			for (TSharedPtr<FJsonValue> JsonItem : *JsonItems)
			{
				TSharedPtr<FJsonObject> JObject = JsonItem->AsObject();

				FSCNewsEntry NewsEntry;
				NewsEntry.NewsItemId = JObject->GetStringField(TEXT("NewsItemId"));

				// Convert the Epoch (Unix) Time to something more UI friendly
				FDateTime DateTime = FDateTime::FromUnixTimestamp(FCString::Atoi64(*JObject->GetStringField(TEXT("UnixTime"))));
				FString MonthString;
				switch (DateTime.GetMonth())
				{
				case 1: MonthString = TEXT("January"); break;
				case 2: MonthString = TEXT("February"); break;
				case 3: MonthString = TEXT("March"); break;
				case 4: MonthString = TEXT("April"); break;
				case 5: MonthString = TEXT("May"); break;
				case 6: MonthString = TEXT("June"); break;
				case 7: MonthString = TEXT("July"); break;
				case 8: MonthString = TEXT("August"); break;
				case 9: MonthString = TEXT("September"); break;
				case 10: MonthString = TEXT("October"); break;
				case 11: MonthString = TEXT("November"); break;
				case 12: MonthString = TEXT("December"); break;
				}

				NewsEntry.PostedTime = FString::Printf(TEXT("%d/%d/%d"), DateTime.GetDay(), DateTime.GetMonth(), DateTime.GetYear());
				NewsEntry.Headline = JObject->GetStringField(TEXT("Headline"));
				NewsEntry.Contents = JObject->GetStringField(TEXT("Content"));
				NewsEntries.Add(NewsEntry);
			}
		}
	}
}

void USCBackendNewsHTTPRequestTask::HandleResponse()
{
	Super::HandleResponse();

	CompletionDelegate.ExecuteIfBound(NewsEntries, ArticleCount);
}

USCBackendRequestManager::USCBackendRequestManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PlayerClass = USCBackendPlayer::StaticClass();
	ServerClass = USCBackendServer::StaticClass();
}

void USCBackendRequestManager::Init()
{
	Super::Init();

	// Map production and staging base URLs
	const TCHAR* ProductionServicesHost =
#if PLATFORM_XBOXONE
		TEXT("https://xbox.f13-services.illfonic.com");
#elif PLATFORM_PS4
		TEXT("https://ps4.f13-services.illfonic.com");
#elif PLATFORM_DESKTOP
		//TEXT("https://steam.f13-services.illfonic.com");
		TEXT("https://steam.f13.illfonic.com"); //Live Server
#endif
	const TCHAR* StagingServicesHost =
#if PLATFORM_XBOXONE
		//TEXT("https://staging.f13-services.illfonic.com"); // Disabling staging on Xbox until we can update the certificate
		TEXT("https://xbox.f13-services.illfonic.com");
#elif PLATFORM_PS4
		TEXT("https://staging.f13-services.illfonic.com");
#elif PLATFORM_DESKTOP
		//TEXT("https://staging.f13-services.illfonic.com");
		TEXT("https://steam.f13.illfonic.com"); //Live Server
#endif

	// Map production and staging client keys
	const FGuid ProductionClientKey
#if PLATFORM_XBOXONE
		(0xb56b6a4f, 0x986d49ba, 0xac1a0de4, 0x0fadfa9b);
#elif PLATFORM_PS4
		(0x59e9f5ad, 0x3e1f47dc, 0x8fcd6be7, 0x1958c5ca);
#elif PLATFORM_DESKTOP
		//(0x723808ad, 0x43d54022, 0x82ba6983, 0x8a4b28e1);
		(0xf5eca462, 0x428c44e7, 0x8c2c494e, 0xd0b834d2); //Live Key
#endif
	const FGuid StagingClientKey
#if PLATFORM_XBOXONE
		= ProductionClientKey;
#elif PLATFORM_PS4
		= ProductionClientKey;
#elif PLATFORM_DESKTOP
		//(0x3ae8287b, 0x4c1b49f8, 0xa2415b4b, 0x1ad43598);
		(0xf5eca462, 0x428c44e7, 0x8c2c494e, 0xd0b834d2); //Live Key
#endif

	// Pick a services host and client key based on build configuration
	const TCHAR* EnvironmentServicesHost =
#if UE_BUILD_SHIPPING
		ProductionServicesHost;
#else
		StagingServicesHost;
#endif
	const FGuid EnvironmentClientKey =
#if UE_BUILD_SHIPPING
		ProductionClientKey;
#else
		StagingClientKey;
#endif

	// Auth service
#if PLATFORM_XBOXONE
	FEndpointService AuthService(EnvironmentServicesHost, 9131, EnvironmentClientKey);
#else
	FEndpointService AuthService(EnvironmentServicesHost, 9130, EnvironmentClientKey);
#endif
	AuthService_Login = RegisterPostEndpoint(AuthService,
#if PLATFORM_DESKTOP
		TEXT("/v0/login/steam")
#elif PLATFORM_XBOXONE
		TEXT("/v1/login/xboxlive")
#elif PLATFORM_PS4
		TEXT("/v0/login/playstation")
#endif
	, false);
	AuthService_GetAccount = RegisterGetEndpoint(AuthService, TEXT("/v0/account"));

	// Arbiter service
	FEndpointService ArbiterService(EnvironmentServicesHost, 9110, EnvironmentClientKey);
	ArbiterService_Validate = RegisterPostEndpoint(ArbiterService, TEXT("/v0/validate"), false);
	ArbiterService_Heartbeat = RegisterPostEndpoint(ArbiterService, TEXT("/v0/heartbeat"));
	ArbiterService_JasonSelect = RegisterPostEndpoint(ArbiterService, TEXT("/v0/jason/select"));
	FEndpointService XboxArbiterService(TEXT("https://xbox.f13-services.illfonic.com"), 9110, FGuid(0xb56b6a4f, 0x986d49ba, 0xac1a0de4, 0x0fadfa9b)); // CLEANUP: pjackson: Ew. This should use the stuff up top.
	ArbiterService_XboxServerSession = RegisterPutEndpoint(XboxArbiterService, TEXT("/v0/xbox/servers/session"));

	// Instance authority
	FEndpointService InstanceAuthority(EnvironmentServicesHost, 9120, EnvironmentClientKey);
	InstanceAuthority_Report = RegisterPostEndpoint(InstanceAuthority, TEXT("/v0/report"));
	InstanceAuthority_Heartbeat = RegisterPostEndpoint(InstanceAuthority, TEXT("/v0/heartbeat"));

	// Profile authority
	FEndpointService ProfileAuthority(EnvironmentServicesHost, 9140, EnvironmentClientKey);
	// Matchmaking status
	ProfileAuthority_MatchmakingStatus = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/matchmaking/status"));
	// Experience and achievements
	ProfileAuthority_AddExperience = RegisterPostEndpoint(ProfileAuthority, TEXT("/v1/experience"));
	ProfileAuthority_AchievementsUnlock = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/achievements/unlock"));
	// Infractions
	ProfileAuthority_AddInfraction = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/infractions/add"));
	// Single player
	ProfileAuthority_GetChallengeProfile = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/singleplayer/profile"));
	ProfileAuthority_ReportChallengeComplete = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/singleplayer/report"));
	// Purchasing
	ProfileAuthority_PushPurchase = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/inventory/purchase"));
	// Perk roll
	ProfileAuthority_PushPurchasePerkRoll = RegisterPostEndpoint(ProfileAuthority, TEXT("/v1/inventory/perks/purchase"));
	ProfileAuthority_CashInPerk = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/inventory/perks/cashin"));
	// Match stats
	ProfileAuthority_ReportMatchStats = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/stats/matches/report"));
	// Player stats
	ProfileAuthority_GetStats = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/stats"));
	// Player achievements
	ProfileAuthority_GetAchievements = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/achievements"));
	// Pamela tapes
	ProfileAuthority_GetActivePamelaTapes = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/data/pamelatapes/active"));
	ProfileAuthority_AddPamelaTape = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/inventory/pamelatapes/add"));
	// Tommy tapes
	ProfileAuthority_GetActiveTommyTapes = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/data/tommytapes/active"));
	ProfileAuthority_AddTommyTape = RegisterPostEndpoint(ProfileAuthority, TEXT("/v0/inventory/tommytapes/add"));
	// Modifiers
	ProfileAuthority_GetModifiers = RegisterGetEndpoint(ProfileAuthority, TEXT("/v0/data/modifiers"));

	// News service
	FEndpointService NewsService(EnvironmentServicesHost, 9150, EnvironmentClientKey);
	NewsService_GetNews = RegisterGetEndpoint(NewsService, TEXT("/v0/news"));

	// Key distribution service
	// This always uses production, and due to certificate conflicts we have to disable verify peer on desktop for non-Shipping builds
	FEndpointService KeyDistributionService(ProductionServicesHost, 9111, ProductionClientKey);
	KeyDistributionService_Create = RegisterPostEndpoint(KeyDistributionService, TEXT("/v0/aes256/create"));
	KeyDistributionService_Retrieve = RegisterGetEndpoint(KeyDistributionService, TEXT("/v0/aes256/retrieve"));

	// Matchmaking services
	const TCHAR* MatchmakingServicesHost = 
#if PLATFORM_DESKTOP
		TEXT("https://steam.f13.illfonic.com");
#elif PLATFORM_PS4
		TEXT("https://ps4.f13-matchmaking.illfonic.com");
#elif PLATFORM_XBOXONE
		TEXT("https://xbox.f13-matchmaking.illfonic.com");
#endif
	FEndpointService MatchmakingService(MatchmakingServicesHost, 3030, EnvironmentClientKey);
	MatchmakingService_Bypass = RegisterPostEndpoint(MatchmakingService, TEXT("/v1/bypass"));
	MatchmakingService_Cancel = RegisterPostEndpoint(MatchmakingService, TEXT("/v1/cancel"));
	MatchmakingService_Describe = RegisterGetEndpoint(MatchmakingService, TEXT("/v1/describe"));
	MatchmakingService_Regions = RegisterGetEndpoint(MatchmakingService, TEXT("/v1/regions/udp"));
	MatchmakingService_Queue = RegisterPostEndpoint(MatchmakingService, TEXT("/v1/queue"));
}

void USCBackendRequestManager::GetPlayerAccountProfile(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(AuthService_GetAccount, Callback, FString::Printf(TEXT("?AccountID=%s"), *AccountId.GetIdentifier()), nullptr, PROFILE_REQUEST_RETRY_COUNT);
}

void USCBackendRequestManager::AddExperienceToProfile(const FILLBackendPlayerIdentifier& AccountId, const int32 Experience, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_AddExperience, Callback, FString::Printf(TEXT("?AccountID=%s&Exp=%d"), *AccountId.GetIdentifier(), Experience));
}

void USCBackendRequestManager::UnlockAchievement(const FILLBackendPlayerIdentifier& AccountId, const FString& AchievementID, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_AchievementsUnlock, Callback, FString::Printf(TEXT("?UnlockID=%s"), *AchievementID));
}

void USCBackendRequestManager::ReportInfraction(const FILLBackendPlayerIdentifier& AccountId, const FString& InfractionID)
{
	QueueSimpleRequest(ProfileAuthority_AddInfraction, FOnILLBackendSimpleRequestDelegate(), FString::Printf(TEXT("?AccountID=%s&InfractionID=%s"), *AccountId.GetIdentifier(), *InfractionID));
}

void USCBackendRequestManager::GetPlayerChallengeProfile(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_GetChallengeProfile, Callback, FString::Printf(TEXT("?AccountID=%s"), *AccountId.GetIdentifier()), nullptr, PROFILE_REQUEST_RETRY_COUNT);
}

void USCBackendRequestManager::ReportChallengeComplete(const FILLBackendPlayerIdentifier& AccountId, const FString& ChallengeData, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (UILLBackendSimpleHTTPRequestTask* RequestTask = CreateSimpleRequest(ProfileAuthority_ReportChallengeComplete, Callback, FString::Printf(TEXT("?AccountID=%s"), *AccountId.GetIdentifier())))
	{
		RequestTask->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		RequestTask->SetContentAsString(ChallengeData);
		RequestTask->QueueRequest();
	}
}

bool USCBackendRequestManager::PurchaseItem(USCBackendPlayer* Player, const FString& SetNameString, const FString& ItemID, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (!Player)
		return false;

	return QueueSimpleRequest(ProfileAuthority_PushPurchase, Callback, FString::Printf(TEXT("?AccountID=%s&SetName=%s&ItemID=%s"), *Player->GetAccountId().GetIdentifier(), *SetNameString, *ItemID), Player);
}

bool USCBackendRequestManager::PurchasePerkRoll(USCBackendPlayer* Player, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (!Player)
		return false;

	return QueueSimpleRequest(ProfileAuthority_PushPurchasePerkRoll, Callback, FString::Printf(TEXT("?AccountID=%s"), *Player->GetAccountId().GetIdentifier()), Player, PERK_REQUEST_RETRY_COUNT);
}

bool USCBackendRequestManager::SellBackPerk(USCBackendPlayer* Player, const FString& InstanceID, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (!Player || InstanceID.IsEmpty())
		return false;

	return QueueSimpleRequest(ProfileAuthority_CashInPerk, Callback, FString::Printf(TEXT("?AccountID=%s&InstanceID=%s"), *Player->GetAccountId().GetIdentifier(), *InstanceID), Player, PERK_REQUEST_RETRY_COUNT);
}

void USCBackendRequestManager::ReportMatchStats(const FString& MatchStats, FOnILLBackendSimpleRequestDelegate Callback)
{
	if (UILLBackendSimpleHTTPRequestTask* RequestTask = CreateSimpleRequest(ProfileAuthority_ReportMatchStats, Callback))
	{
		RequestTask->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		RequestTask->SetContentAsString(MatchStats);
		RequestTask->QueueRequest();
	}
}

void USCBackendRequestManager::GetPlayerStats(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_GetStats, Callback, FString::Printf(TEXT("?AccountID=%s"), *AccountId.GetIdentifier()), nullptr, PROFILE_REQUEST_RETRY_COUNT);
}

void USCBackendRequestManager::GetPlayerAchievements(const FILLBackendPlayerIdentifier& AccountId, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_GetAchievements, Callback, FString::Printf(TEXT("?AccountID=%s"), *AccountId.GetIdentifier()), nullptr, PROFILE_REQUEST_RETRY_COUNT);
}

void USCBackendRequestManager::GetActivePamelaTapes(FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_GetActivePamelaTapes, Callback);
}

void USCBackendRequestManager::GetActiveTommyTapes(FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_GetActiveTommyTapes, Callback);
}

void USCBackendRequestManager::AddPamelaTapeToInventory(const FILLBackendPlayerIdentifier& AccountId, const FString& PamelaTapeID, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_AddPamelaTape, Callback, FString::Printf(TEXT("?AccountID=%s&PamelaTapeID=%s"), *AccountId.GetIdentifier(), *PamelaTapeID));
}

void USCBackendRequestManager::AddTommyTapeToInventory(const FILLBackendPlayerIdentifier& AccountId, const FString& TommyTapeID, FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_AddTommyTape, Callback, FString::Printf(TEXT("?AccountID=%s&TommyTapeID=%s"), *AccountId.GetIdentifier(), *TommyTapeID));
}

void USCBackendRequestManager::GetModifiers(FOnILLBackendSimpleRequestDelegate Callback)
{
	QueueSimpleRequest(ProfileAuthority_GetModifiers, Callback);
}

void USCBackendRequestManager::GetNews(FSCOnBackendNewsDelegate Callback)
{
#if PLATFORM_XBOXONE
	const TCHAR* NewsCategory = TEXT("xboxone");
#elif PLATFORM_PS4
	const TCHAR* NewsCategory = TEXT("ps4");
#elif PLATFORM_DESKTOP
	const TCHAR* NewsCategory = TEXT("desktop");
#endif

	if (USCBackendNewsHTTPRequestTask* RequestTask = CreateBackendRequest<USCBackendNewsHTTPRequestTask>(NewsService_GetNews))
	{
		RequestTask->URLParameters = FString::Printf(TEXT("?Page=%d&NumPerPage=%d&Category=%s"), 1, 1, NewsCategory);
		RequestTask->CompletionDelegate = Callback;
		RequestTask->QueueRequest();
	}
}
