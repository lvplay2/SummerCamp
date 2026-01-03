// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameInstance.h"

#include "Base64.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendKeySession.h"
#include "ILLBackendRequestManager.h"
#include "ILLBackendServer.h"
#include "ILLLiveRequestManager.h"
#include "ILLPSNRequestManager.h"
#include "ILLBuildDefines.h"
#include "ILLGameEngine.h"
#include "ILLGameMode.h"
#include "ILLGame_Lobby.h"
#include "ILLGameSession.h"
#include "ILLGameState.h"
#include "ILLLobbyBeaconHost.h"
#include "ILLLobbyBeaconHostObject.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"
#include "ILLOnlineSubsystem.h"
#include "ILLPartyBeaconClient.h"
#include "ILLPartyBeaconHost.h"
#include "ILLPartyBeaconState.h"
#include "ILLProjectSettings.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLGameInstance"

UILLGameInstance::UILLGameInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MinPlayerRatioToLive = .3f;
	MaxInstanceCycles = 3;

	BackendRequestManagerClass = UILLBackendRequestManager::StaticClass();
	LiveRequestManagerClass = UILLLiveRequestManager::StaticClass();
	PSNRequestManagerClass = UILLPSNRequestManager::StaticClass();
}

void UILLGameInstance::Init()
{
	Super::Init();

	if (IsRunningCommandlet())
	{
		return;
	}

#if PLATFORM_DESKTOP
	// Cache the resolution list
	RHIGetAvailableResolutions(ResolutionList, true);
#endif

	// Spawn the online matchmaking instance
	OnlineMatchmaking = NewObject<UILLOnlineMatchmakingClient>(this, GetOnlineMatchmakingClass());
	if (OnlineMatchmaking)
	{
		OnlineMatchmaking->RegisterOnlineDelegates();
	}

	// Spawn HTTP request manager instances
	if (BackendRequestManagerClass)
	{
		BackendRequestManager = NewObject<UILLBackendRequestManager>(this, BackendRequestManagerClass);
		BackendRequestManager->Init();
	}
	if (IsRunningDedicatedServer())
	{
		if (LiveRequestManagerClass)
		{
			LiveRequestManager = NewObject<UILLLiveRequestManager>(this, LiveRequestManagerClass);
			LiveRequestManager->Init();
		}
		if (PSNRequestManagerClass)
		{
			PSNRequestManager = NewObject<UILLPSNRequestManager>(this, PSNRequestManagerClass);
			PSNRequestManager->Init();
		}
	}

	// Listen for engine failure events
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::OnNetworkFailure);
		GEngine->OnNetworkLagStateChanged().AddUObject(this, &ThisClass::OnNetworkLagStateChanged);
		GEngine->OnTravelFailure().AddUObject(this, &ThisClass::OnTravelFailure);
	}

	// Network encryption
	EncryptionContext = IPlatformCrypto::Get().CreateContext();
	FNetDelegates::OnRequireNetworkEncryption.BindUObject(this, &ThisClass::RequireNetworkEncryption);
}

void UILLGameInstance::Shutdown()
{
	Super::Shutdown();

	// Cleanup online matchmaking
	if (OnlineMatchmaking)
	{
		OnlineMatchmaking->ClearOnlineDelegates();
		OnlineMatchmaking = nullptr;
	}

	// Tear down HTTP request manager instances
	if (BackendRequestManager)
	{
		BackendRequestManager->Shutdown();
		BackendRequestManager = nullptr;
	}
	if (LiveRequestManager)
	{
		LiveRequestManager->Shutdown();
		LiveRequestManager = nullptr;
	}
	if (PSNRequestManager)
	{
		PSNRequestManager->Shutdown();
		PSNRequestManager = nullptr;
	}

	// Network encryption
	FNetDelegates::OnRequireNetworkEncryption.Unbind();
}

void UILLGameInstance::StartGameInstance()
{
	Super::StartGameInstance();

	UE_LOG(LogIGF, Log, TEXT("UILLGameInstance initialization"));
	UE_LOG(LogIGF, Log, TEXT("... Build: %i"), ILLBUILD_BUILD_NUMBER);
	UE_LOG(LogIGF, Log, TEXT("... GameLift: %s"), (ALLOW_GAMELIFT_PLUGIN != 0) ? TEXT("true") : TEXT("false"));

	FILLOnlineSubsystem* ILLSubsystem = static_cast<FILLOnlineSubsystem*>(IOnlineSubsystem::Get(ILL_SUBSYSTEM));
	if (!ILLSubsystem)
		UE_LOG(LogIGF, Fatal, TEXT("ILLOnlineSubsystem missing!"));

#if PLATFORM_DESKTOP
	// Verify the presence of the Steam OSS
	IOnlineSubsystem* SteamSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	if (!SteamSubsystem)
		UE_LOG(LogIGF, Fatal, TEXT("Online session interface missing, please make sure Steam is running!"));
#endif
}

bool UILLGameInstance::JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	check(0); // FIXME: pjackson: Is this even used..?!
	return Super::JoinSession(LocalPlayer, SessionIndexInSearchResults);
}

bool UILLGameInstance::JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult)
{
	if (UILLOnlineSessionClient* SessionClient = Cast<UILLOnlineSessionClient>(OnlineSession))
	{
		return SessionClient->JoinSession(NAME_GameSession, SearchResult, nullptr);
	}

	return false;
}

TSubclassOf<UOnlineSession> UILLGameInstance::GetOnlineSessionClass()
{
	return UILLOnlineSessionClient::StaticClass();
}

void UILLGameInstance::ReceivedNetworkEncryptionToken(UObject* RecipientObject, const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate)
{
	check(RecipientObject);
	if (RecipientObject->IsA<AILLPartyBeaconHost>())
	{
		BuildPartyNetworkEncryptionResponse(EncryptionToken, Delegate);
	}
	else if (RecipientObject->IsA<AILLLobbyBeaconHost>())
	{
		// Make a server key request
		FString KeySessionId;
		if (EncryptionTokenToKeySession(EncryptionToken, KeySessionId))
		{
			FOnILLBackendKeySessionDelegate Callback = FOnILLBackendKeySessionDelegate::CreateUObject(this, &ThisClass::RetrieveKeySessionResponse);
			if (!GetBackendRequestManager()->SendKeySessionRetrieveRequest(KeySessionId, Callback, Delegate))
			{
				UE_LOG(LogIGF, Warning, TEXT("ReceivedNetworkEncryptionToken: Failed to send key request for EncryptionToken: %s"), *EncryptionToken);
				FEncryptionKeyResponse Response;
				Response.Response = EEncryptionResponse::Failure;
				Delegate.ExecuteIfBound(Response);
			}
		}
		else
		{
			UE_LOG(LogIGF, Warning, TEXT("ReceivedNetworkEncryptionToken: Received invalid EncryptionToken: %s"), *EncryptionToken);
			FEncryptionKeyResponse Response;
			Response.Response = EEncryptionResponse::InvalidToken;
			Delegate.ExecuteIfBound(Response);
		}
	}
	else
	{
		// This is for the game connection, we should have a lobby beacon connection established at this point
		check(RecipientObject == GetWorld());
		UILLOnlineSessionClient* SessionClient = Cast<UILLOnlineSessionClient>(OnlineSession);
		AILLLobbyBeaconClient* CorrespondingBeaconClient = SessionClient->GetLobbyHostBeaconObject() ? Cast<AILLLobbyBeaconClient>(SessionClient->GetLobbyHostBeaconObject()->FindClientByEncryptionToken(EncryptionToken)) : nullptr;
		UNetConnection* CorrespondingBeaconConnection = CorrespondingBeaconClient ? CorrespondingBeaconClient->GetNetConnection() : nullptr;

		// Grab the keys from the beacon connection
		FEncryptionKeyResponse Response;
		if (CorrespondingBeaconConnection)
		{
			Response.Response = EEncryptionResponse::Success;
			CorrespondingBeaconConnection->GetEncryptionKeys(Response.EncryptionKey, Response.InitializationVector, Response.HashKey);
		}
		else
		{
			UE_LOG(LogIGF, Warning, TEXT("ReceivedNetworkEncryptionToken: Did not find a corresponding beacon connection for EncryptionToken: %s"), *EncryptionToken);
			Response.Response = EEncryptionResponse::NoKey;
		}
		Delegate.ExecuteIfBound(Response);
	}
}

void UILLGameInstance::ReceivedNetworkEncryptionAck(UObject* RecipientObject, const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate)
{
	check(RecipientObject);
	if (RecipientObject->IsA<AILLPartyBeaconClient>())
	{
		BuildPartyNetworkEncryptionResponse(EncryptionToken, Delegate);
	}
	else
	{
		// Grab the key for our client from the lobby beacon client
		FEncryptionKeyResponse Response;
		UILLOnlineSessionClient* SessionClient = Cast<UILLOnlineSessionClient>(OnlineSession);
		if (SessionClient->GetLobbyClientBeacon())
		{
			const FILLBackendKeySession& KeySession = SessionClient->GetLobbyClientBeacon()->GetSessionKeySet();
			if (KeySession.EncryptionToken.IsEmpty())
			{
				UE_LOG(LogIGF, Warning, TEXT("ReceivedNetworkEncryptionAck: Lobby client beacon EncryptionToken empty!"));
				Response.Response = EEncryptionResponse::NoKey;
			}
			else
			{
				check(KeySession.EncryptionToken == EncryptionToken);
				Response.Response = EEncryptionResponse::Success;
				Response.EncryptionKey = KeySession.EncryptionKey;
				Response.InitializationVector = KeySession.InitializationVector;
				Response.HashKey = KeySession.HashKey;
			}
		}
		else
		{
			UE_LOG(LogIGF, Warning, TEXT("ReceivedNetworkEncryptionAck: No lobby client beacon!"));
			Response.Response = EEncryptionResponse::NoKey;
		}

		Delegate.ExecuteIfBound(Response);
	}
}

bool UILLGameInstance::RequireNetworkEncryption(UObject* RecipientObject)
{
	check(RecipientObject);

#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		if (World->IsPlayInEditor())
			return false;
	}
#endif

#if PLATFORM_DESKTOP && ILLNETENCRYPTION_DESKTOP_REQUIRED
	return (BackendRequestManager->KeyDistributionService_Create && BackendRequestManager->KeyDistributionService_Retrieve);
#else
	return false;
#endif
}

bool UILLGameInstance::EncryptionTokenToKeySession(const FString& EncryptionToken, FString& OutKeySessionId) const
{
#if 0
	OutKeySessionId = EncryptionToken;
	return true;
#else
	uint32 TokenAESKey[] = ILLBUILD_ENCRYPTIONTOKEN_AESKEY;
	static_assert(sizeof(TokenAESKey) == ILLNETENCRYPTION_AESKEY_BYTES, "ILLBUILD_ENCRYPTIONTOKEN_AESKEY is not the right size");
	TArrayView<uint8> AESKeyView((uint8*)TokenAESKey, sizeof(TokenAESKey));

	uint32 TokenAESInitVector[] = ILLBUILD_ENCRYPTIONTOKEN_AESIV;
	static_assert(sizeof(TokenAESInitVector) == ILLNETENCRYPTION_AESIV_BYTES, "ILLBUILD_ENCRYPTIONTOKEN_AESIV is not the right size");
	TArrayView<uint8> AESInitVectorView((uint8*)TokenAESInitVector, sizeof(TokenAESInitVector));

	// Decode from Base64
	TArray<uint8> Ciphertext;
	FString CorrectedToken = EncryptionToken;
	CorrectedToken.ReplaceInline(TEXT(":"), TEXT("="));
	CorrectedToken.ReplaceInline(TEXT("_"), TEXT("/"));
	if (!ensureAlways(FBase64::Decode(CorrectedToken, Ciphertext)))
	{
		return false;
	}

	// Decrypt the contents
	EPlatformCryptoResult DecryptResult = EPlatformCryptoResult::Failure;
	TArray<uint8> Plaintext = EncryptionContext->Decrypt_AES_256_CBC(Ciphertext, AESKeyView, AESInitVectorView, DecryptResult);
	if (!ensureAlways(DecryptResult == EPlatformCryptoResult::Success))
	{
		return false;
	}

	// Convert from bytes to string
	OutKeySessionId = ANSI_TO_TCHAR((ANSICHAR*)Plaintext.GetData());
	return true;
#endif
}

bool UILLGameInstance::KeySessionToEncryptionToken(const FString& KeySessionId, FString& OutEncryptionToken) const
{
#if 0
	OutEncryptionToken = KeySessionId;
	return true;
#else
	uint32 TokenAESKey[] = ILLBUILD_ENCRYPTIONTOKEN_AESKEY;
	TArrayView<uint8> AESKeyView((uint8*)TokenAESKey, sizeof(TokenAESKey));
	uint32 TokenAESInitVector[] = ILLBUILD_ENCRYPTIONTOKEN_AESIV;
	TArrayView<uint8> AESInitVectorView((uint8*)TokenAESInitVector, sizeof(TokenAESInitVector));

	// Encrypt the KeySessionId into Ciphertext
	EPlatformCryptoResult EncryptResult = EPlatformCryptoResult::Failure;
	TArray<uint8> Ciphertext = EncryptionContext->Encrypt_AES_256_CBC(TArrayView<uint8>((uint8*)TCHAR_TO_ANSI(*KeySessionId), KeySessionId.Len()+1), AESKeyView, AESInitVectorView, EncryptResult);
	if (!ensureAlways(EncryptResult == EPlatformCryptoResult::Success))
	{
		return false;
	}

	// Encode in Base64 for transmission as a string
	OutEncryptionToken = FBase64::Encode(Ciphertext);
	OutEncryptionToken.ReplaceInline(TEXT("="), TEXT(":"));
	OutEncryptionToken.ReplaceInline(TEXT("/"), TEXT("_"));
	return true;
#endif
}

void UILLGameInstance::RetrieveKeySessionResponse(int32 ResponseCode, const FString& ResponseContent, const FOnEncryptionKeyResponse& Delegate)
{
	FEncryptionKeyResponse Response;

	FILLBackendKeySession SessionKeySet;
	if (FILLBackendKeySession::ParseFromJSON(this, ResponseCode, ResponseContent, SessionKeySet))
	{
		Response.Response = EEncryptionResponse::Success;
		Response.EncryptionKey = SessionKeySet.EncryptionKey;
		Response.InitializationVector = SessionKeySet.InitializationVector;
		Response.HashKey = SessionKeySet.HashKey;
	}
	else
	{
		Response.Response = EEncryptionResponse::NoKey;
	}

	Delegate.ExecuteIfBound(Response);
}

void UILLGameInstance::BuildPartyNetworkEncryptionResponse(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) const
{
	FEncryptionKeyResponse Response;

	FString KeySessionId;
	if (EncryptionTokenToKeySession(EncryptionToken, KeySessionId))
	{
		if (KeySessionId == TEXT("Party"))
		{
			uint32 PartyAESKey[] = ILLBUILD_NETENCRYPTION_PARTY_AESKEY;
			static_assert(sizeof(PartyAESKey) == ILLNETENCRYPTION_AESKEY_BYTES, "ILLBUILD_NETENCRYPTION_PARTY_AESKEY is not the right size");
			Response.EncryptionKey.SetNumUninitialized(sizeof(PartyAESKey));
			memcpy(Response.EncryptionKey.GetData(), PartyAESKey, sizeof(PartyAESKey));

			uint32 PartyAESInitVector[] = ILLBUILD_NETENCRYPTION_PARTY_AESIV;
			static_assert(sizeof(PartyAESInitVector) == ILLNETENCRYPTION_AESIV_BYTES, "ILLBUILD_NETENCRYPTION_PARTY_AESIV is not the right size");
			Response.InitializationVector.SetNumUninitialized(sizeof(PartyAESInitVector));
			memcpy(Response.InitializationVector.GetData(), PartyAESInitVector, sizeof(PartyAESInitVector));

			uint32 PartyHashKey[] = ILLBUILD_NETENCRYPTION_PARTY_HASHKEY;
			static_assert(sizeof(PartyHashKey) == ILLNETENCRYPTION_HMACKEY_BYTES, "ILLBUILD_NETENCRYPTION_PARTY_HASHKEY is not the right size");
			Response.HashKey.SetNumUninitialized(sizeof(PartyHashKey));
			memcpy(Response.HashKey.GetData(), PartyHashKey, sizeof(PartyHashKey));

			Response.Response = EEncryptionResponse::Success;
		}
		else
		{
			Response.Response = EEncryptionResponse::SessionIdMismatch;
		}
	}
	else
	{
		Response.Response = EEncryptionResponse::InvalidToken;
	}

	Delegate.ExecuteIfBound(Response);
}

bool UILLGameInstance::CheckInstanceRecycle(const bool bForce/* = false*/)
{
	if (bPendingRecycle)
		return true;

	// Only do this to dedicated servers
	if (!SupportsInstanceRecycling())
	{
		return false;
	}

	UWorld* World = GetWorld();
	AILLGameMode* GameMode = World->GetAuthGameMode<AILLGameMode>();
	const bool bIsInLobby = GameMode->IsA(AILLGame_Lobby::StaticClass());

	// Lobby modes will only recycle when empty
	// Other modes check the ratio of players against MinPlayerRatioToLive
	AILLGameSession* GameSession = Cast<AILLGameSession>(GameMode->GameSession);
	const int32 RequiredNumPlayers = bIsInLobby ? 1 : FMath::TruncToInt(float(GameSession->GetMaxPlayers()) * MinPlayerRatioToLive);

	if (bForce)
	{
		UE_LOG(LogIGF, Log, TEXT("Instance pending recycling by force"));
		bPendingRecycle = true;
	}
	else if (GameMode->GetNumPlayers() < RequiredNumPlayers)
	{
		UE_LOG(LogIGF, Log, TEXT("Instance pending recycling due population: %i players, %i required"), GameMode->GetNumPlayers(), RequiredNumPlayers);
		bPendingRecycle = true;
	}
	else if (!bIsInLobby)
	{
		// Only increment NumInstanceCycles for non-Lobby modes, so we don't recycle after getting a lobby together
		++NumInstanceCycles;
		if (MaxInstanceCycles > 0 && NumInstanceCycles >= MaxInstanceCycles)
		{
			UE_LOG(LogIGF, Log, TEXT("Instance pending recycling due cycle limit of %i"), MaxInstanceCycles);
			bPendingRecycle = true;
		}
	}

	// Notify systems of pending recycle
	if (bPendingRecycle)
	{
		if (AILLGameState* GameState = World->GetGameState<AILLGameState>())
		{
			GameState->HandlePendingInstanceRecycle();
		}
	}

	return bPendingRecycle;
}

void UILLGameInstance::OnGameRegisteredStandalone()
{
	// We have probably just recycled, so reset the cycle counter
	NumInstanceCycles = 0;
}

bool UILLGameInstance::SupportsInstanceRecycling() const
{
	if (IsRunningDedicatedServer())
	{
		// Only allow instance recycling for deferred dedicated servers
		UILLGameEngine* GameEngine = CastChecked<UILLGameEngine>(GEngine);
		if (GameEngine && GameEngine->UseDeferredDedicatedStart())
			return true;
	}

	return false;
}

void UILLGameInstance::PlayOnlineRequiredPatchAvailable(const FText& ErrorHeading/* = FText()*/)
{
	HandleLocalizedError(ErrorHeading, LOCTEXT("RequiredPatchAvailableMessage", "Game update required to use online features."), false);
}

void UILLGameInstance::PlayOnlineRequiredSystemUpdate(const FText& ErrorHeading/* = FText()*/)
{
	HandleLocalizedError(ErrorHeading, LOCTEXT("RequiredSystemUpdateMessage", "System update required to use online features."), false);
}

void UILLGameInstance::PlayOnlineAgeRestrictionFailure(const FText& ErrorHeading/* = FText()*/)
{
	HandleLocalizedError(ErrorHeading, LOCTEXT("AgeRestrictionFailureMessage", "Online features blocked due to age restrictions."), false);
}

void UILLGameInstance::OnNetworkLagStateChanged(UWorld* World, UNetDriver* NetDriver, ENetworkLagState::Type LagType)
{
	// Discard results from other worlds (like the beacon world)
	if (World != GetWorld())
		return;

	for (FLocalPlayerIterator It(GEngine, World); It; ++It)
	{
		if (AILLPlayerController* PC = Cast<AILLPlayerController>(It->PlayerController))
		{
			PC->bLagging = (LagType == ENetworkLagState::Lagging);
		}
	}
}

void UILLGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GetOnlineSession()))
	{
		// Beacon failures are handled on their own, try to keep this limited to game network failures
		if (World && (OSC->GetLobbyBeaconWorld() == World || OSC->GetPartyBeaconWorld() == World))
			return;
	}

	// Localize the failure
	FText FailureText;
	switch (FailureType)
	{
	case ENetworkFailure::NetDriverAlreadyExists: FailureText = LOCTEXT("NetworkFailure_NetDriverAlreadyExists", "NetDriver already exists!"); break;
	case ENetworkFailure::NetDriverCreateFailure: FailureText = LOCTEXT("NetworkFailure_NetDriverCreateFailure", "NetDriver creation failed!"); break;
	case ENetworkFailure::NetDriverListenFailure: FailureText = LOCTEXT("NetworkFailure_NetDriverListenFailure", "NetDriver listen failed!"); break;
	case ENetworkFailure::ConnectionLost: FailureText = LOCTEXT("NetworkFailure_ConnectionLost", "Connection to host lost!"); break;
	case ENetworkFailure::ConnectionTimeout: FailureText = LOCTEXT("NetworkFailure_ConnectionTimeout", "Connection timed out!"); break;
	case ENetworkFailure::FailureReceived: FailureText = FText::Format(LOCTEXT("NetworkFailure_FailureReceived", "Network failure received: {0}"), FText::FromString(ErrorString)); break;
	case ENetworkFailure::OutdatedClient: FailureText = LOCTEXT("NetworkFailure_OutdatedClient", "Outdated client! You must update before you can join."); break;
	case ENetworkFailure::OutdatedServer: FailureText = LOCTEXT("NetworkFailure_OutdatedServer", "Outdated host! They must update before you can join."); break;
	case ENetworkFailure::PendingConnectionFailure: FailureText = FText::Format(LOCTEXT("NetworkFailure_PendingConnectionFailure", "Pending connection failure: {0}"), FText::FromString(ErrorString)); break;
	case ENetworkFailure::NetGuidMismatch: FailureText = LOCTEXT("NetworkFailure_NetGuidMismatch", "Network GUID mismatch!"); break;
	case ENetworkFailure::NetChecksumMismatch: FailureText = LOCTEXT("NetworkFailure_NetChecksumMismatch", "Network checksum mismatch!"); break;
	default: check(0); break;
	}

	// Determine if this failure will cause travel
	bool bWillTravel = true;
	switch (FailureType)
	{
	case ENetworkFailure::ConnectionLost:
	case ENetworkFailure::ConnectionTimeout:
		// Verify this is for our world and not the beacon world or something
		bWillTravel = (World && World == GetWorld()) && (NetDriver->GetNetMode() == NM_Client);
		break;
	}

	OnNetworkFailure(World, NetDriver, FailureType, FailureText, bWillTravel);
}

void UILLGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	FText FailureText;
	switch (FailureType)
	{
	case ETravelFailure::NoLevel: FailureText = LOCTEXT("TravelFailure_NoLevel", "No level found in the loaded package!"); break;
	case ETravelFailure::LoadMapFailure: FailureText = FText::Format(LOCTEXT("TravelFailure_LoadMapFailure", "LoadMap failed! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::InvalidURL: FailureText = FText::Format(LOCTEXT("TravelFailure_InvalidURL", "Invalid URL! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::PackageMissing: FailureText = FText::Format(LOCTEXT("TravelFailure_PackageMissing", "Package missing on client! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::PackageVersion: FailureText = FText::Format(LOCTEXT("TravelFailure_PackageVersion", "Package version mismatch! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::NoDownload: FailureText = FText::Format(LOCTEXT("TravelFailure_NoDownload", "Package missing and can not be downloaded! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::TravelFailure: FailureText = FText::Format(LOCTEXT("TravelFailure_TravelFailure", "Travel failure! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::CheatCommands:  FailureText = LOCTEXT("TravelFailure_CheatCommands", "Cheat commands are blocking travel!"); break;
	case ETravelFailure::PendingNetGameCreateFailure: FailureText = FText::Format(LOCTEXT("TravelFailure_PendingNetGameCreateFailure", "Pending net-game creation failed! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::CloudSaveFailure: FailureText = FText::Format(LOCTEXT("TravelFailure_CloudSaveFailure", "Cloud save failure! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::ServerTravelFailure: FailureText = FText::Format(LOCTEXT("TravelFailure_ServerTravelFailure", "Server travel failure! {0}"), FText::FromString(ErrorString)); break;
	case ETravelFailure::ClientTravelFailure: FailureText = FText::Format(LOCTEXT("TravelFailure_ClientTravelFailure", "Client travel failure! {0}"), FText::FromString(ErrorString)); break;
	}
	OnTravelFailure(World, FailureType, FailureText);
}

void UILLGameInstance::ShowLoadingScreen(const FURL& NextURL)
{
	if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GetOnlineSession()))
	{
		// Attempt to update party leader status
		if (OSC->IsInPartyAsLeader())
		{
			AILLPartyBeaconState* PBS = OSC->GetPartyBeaconState();
			if (NextURL.HasOption(TEXT("listen")))
			{
				if (NextURL.HasOption(TEXT("bIsPrivateMatch")))
				{
					PBS->SetLeaderState(ILLPartyLeaderState::CreatingPrivateMatch);
				}
				else
				{
					PBS->SetLeaderState(ILLPartyLeaderState::CreatingPublicMatch);
				}
			}
			else if (NextURL.HasOption(TEXT("closed")))
			{
				// Cleanup a previous join
				PBS->SetLeaderState(ILLPartyLeaderState::Idle);
			}
		}
	}
}

void UILLGameInstance::OnControllerPairingLost(const int32 LocalUserNum, FText UserErrorText)
{
	UnPairedControllers.AddUnique(LocalUserNum);
}

void UILLGameInstance::OnControllerPairingRegained(const int32 LocalUserNum)
{
	UnPairedControllers.Remove(LocalUserNum);
}

void UILLGameInstance::CheckControllerPairings()
{
	for (int32 i = 0; i < UnPairedControllers.Num(); ++i)
	{
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			if ((*It)->GetControllerId() == UnPairedControllers[i])
			{
				if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(*It))
				{
					if (LocalPlayer->HasLostControllerPairing() && LocalPlayer->HasPlatformLogin())
					{
						LocalPlayer->ShowControllerDisconnected();
					}
					else
					{
						// No longer using a gamepad, remove from the list
						UnPairedControllers.RemoveAt(i);
						--i;
					}

					break;
				}
			}
		}
	}
}

TSubclassOf<UILLOnlineMatchmakingClient> UILLGameInstance::GetOnlineMatchmakingClass() const
{
	return UILLOnlineMatchmakingClient::StaticClass();
}

FString UILLGameInstance::PickRandomQuickPlayMap() const
{
	return UGameMapsSettings::GetServerDefaultMap();
}

#undef LOCTEXT_NAMESPACE
