// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameInstance.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLGameEngine.h"
#include "ILLGameOnlineBlueprintLibrary.h"

#if WITH_EAC
# include "IEasyAntiCheatClient.h"
# include "IEasyAntiCheatServer.h"
#endif

#include "SCBackendRequestManager.h"
#include "SCLiveRequestManager.h"
#include "SCPSNRequestManager.h"
#include "SCCounselorCharacter.h"
#include "SCCounselorWardrobe.h"
#include "SCKillerCharacter.h"
#include "SCGameViewportClient.h"
#include "SCGameSession.h"
#include "SCHUD.h"
#include "SCHUD_Lobby.h"
#include "SCLoadingScreenModule.h"
#include "SCLocalPlayer.h"
#include "SCMapRegistry.h"
#include "SCOnlineMatchmakingClient.h"
#include "SCOnlineSessionClient.h"
#include "SCPlayerController_Lobby.h"
#include "SCPlayerState.h"
#include "SCPlayerState_Lobby.h"
#include "SCWidgetBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCGameInstance"

static FName LoadingScreenModuleName(TEXT("SCLoadingScreen"));

TSubclassOf<USCMapDefinition> GLongLoadMapDefinition;

ISCLoadingScreenModule* const GetLoadingScreenModule()
{
	return FModuleManager::LoadModulePtr<ISCLoadingScreenModule>(LoadingScreenModuleName);
}

USCGameInstance::USCGameInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// HACK: Make sure this blueprint content loads earlier and doesn't throw random ass asserts
	USCWidgetBlueprintLibrary::StaticClass()->GetDefaultObject();

	BackendRequestManagerClass = USCBackendRequestManager::StaticClass();
	LiveRequestManagerClass = USCLiveRequestManager::StaticClass();
	PSNRequestManagerClass = USCPSNRequestManager::StaticClass();

	ConnectingLoadScreenClass = StaticLoadClass(UILLUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Loading/ConnectingLoadScreenWidget.ConnectingLoadScreenWidget_C"));
	LongLoadScreenClass = StaticLoadClass(UILLUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Loading/LoadingScreenWidget.LoadingScreenWidget_C"));
	PartyConnectingLoadScreenClass = StaticLoadClass(UILLUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Loading/ConnectingToPartyLoadScreenWidget.ConnectingToPartyLoadScreenWidget_C"));
	PartyFindingLeaderGameLoadScreenClass = ConnectingLoadScreenClass;
	PartyReservingLoadScreenClass = StaticLoadClass(UILLUserWidget::StaticClass(), nullptr, TEXT("/Game/UI/Loading/JoiningPartyLoadScreenWidget.JoiningPartyLoadScreenWidget_C"));

	static ConstructorHelpers::FClassFinder<USCMapRegistry> MapRegistry(TEXT("/Game/Blueprints/MapRegistry/MapRegistry.MapRegistry_C"));
	MapRegistryClass = MapRegistry.Class;
}

void USCGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMap);

	ErrorDialogDelegate.BindDynamic(this, &ThisClass::OnAcceptError);
	ControllerPairingErrorDialogDelegate.BindDynamic(this, &ThisClass::OnAcceptControllerPairingError);

#if WITH_EAC
	if (IEasyAntiCheatClient::IsAvailable())
	{
		IEasyAntiCheatClient::Get().ErrorDelegate.BindUObject(this, &ThisClass::OnEACError);
	}
	// @see ASCGameMode + ASCGame_Lobby functions: GenericPlayerInitialization, SwapPlayerControllers and Logout
	if (IEasyAntiCheatServer::IsAvailable())
	{
		FEasyAntiCheatServerOptions Options;
		Options.GameCallsClientConnect = true;
		Options.GameCallsClientDisconnect = true;
		IEasyAntiCheatServer::Get().SetOptions(Options);
	}
#endif

	CounselorWardrobe = NewObject<USCCounselorWardrobe>(this, USCCounselorWardrobe::StaticClass());
	CounselorWardrobe->Init(this);
}

TSubclassOf<UOnlineSession> USCGameInstance::GetOnlineSessionClass()
{
	return USCOnlineSessionClient::StaticClass();
}

void USCGameInstance::HandleLocalizedError(const FText& ErrorHeading, const FText& ErrorMessage, bool bWillTravel)
{
	SetFailureText(ErrorMessage, ErrorHeading);

	if (!bWillTravel)
		QueueFailureErrorCheck();
}

void USCGameInstance::OnConnectionFailurePreDisconnect(const FText& FailureText)
{
	if (PendingFailureText.IsEmpty())
		SetFailureText(FailureText);
}

void USCGameInstance::OnInviteJoinSessionFailure(FName SessionName, EOnJoinSessionCompleteResult::Type Result, const bool bWillTravel)
{
	if (PendingFailureText.IsEmpty())
		SetFailureText(EOnJoinSessionCompleteResult::GetLocalizedDescription(SessionName, Result));

	if (!bWillTravel)
	{
		// Hide any previous connecting screen
		HideLoadingScreen();

		// Display this immediately because we are not traveling immediately after this fires
		QueueFailureErrorCheck();
	}
}

void USCGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FText& FailureText, const bool bWillTravel)
{
	// Report crappy hosts
	if (LastP2PQuickPlayHostAccountId.IsValid())
	{
		switch (FailureType)
		{
		case ENetworkFailure::ConnectionLost:
		case ENetworkFailure::ConnectionTimeout:
		case ENetworkFailure::FailureReceived:
			GetBackendRequestManager<USCBackendRequestManager>()->ReportInfraction(LastP2PQuickPlayHostAccountId, InfractionTypes::HostAbandonedMatch);
			LastP2PQuickPlayHostAccountId.Reset();
			break;
		}
	}

	if (bWillTravel)
	{
		// Only override the travel failure string if it's empty
		// This is because typically the first error message is the most accurate, for example there can be several 'Your connection to the host has been lost.' to follow 'Server is full.'
		if (PendingFailureText.IsEmpty())
			SetFailureText(FailureText, LOCTEXT("NetworkErrorHeader", "Network Error!"));
	}
}

void USCGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FText& FailureText)
{
	if (PendingFailureText.IsEmpty())
		SetFailureText(FailureText, LOCTEXT("TravelErrorHeader", "Travel Error!"));
}

void USCGameInstance::SetLastPlayerKickReason(const FText& KickText, const bool bWillTravel/* = true*/)
{
	// Reset our last host so we do not flag them for a kick
	LastP2PQuickPlayHostAccountId.Reset();

	SetFailureText(KickText);

	if (!bWillTravel)
	{
		QueueFailureErrorCheck();
	}
}

void USCGameInstance::ShowConnectingLoadScreen(FName SessionName, const FOnlineSessionSearchResult& JoiningSession)
{
	// Intentionally skip the Super call, it doesn't do what we wants

	if (SessionName == NAME_GameSession)
	{
		// When not matchmaking...
		if (!GetOnlineMatchmaking()->IsMatchmaking())
		{
			// Display the connecting loading screen until OnPreLoadMap hits
			ShowLoadingWidget(ConnectingLoadScreenClass);
		}
	}
	else if (SessionName == NAME_PartySession)
	{
		// Display the party connecting loading screen
		ShowLoadingWidget(PartyConnectingLoadScreenClass);
	}
}

void USCGameInstance::OnPreLoadMap(const FString& MapName)
{
	// Skip showing a loading screen when in PIE so the listen server host's doesn't get stuck on the screen
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsPlayInEditor())
#endif
	{
		// Try to display the map-specific loading screen
		// Only do this on worlds that have begun play, to avoid this firing on startup
		if (GetWorld() && GetWorld()->HasBegunPlay())
		{
			ShowLoadingScreen(FPackageName::GetLongPackageAssetName(MapName));
		}
	}
}

void USCGameInstance::OnPostLoadMap(UWorld* World)
{
	QueueFailureErrorCheck();
}

void USCGameInstance::ShowTravelingScreen(const FOnlineSessionSearchResult& JoiningSession)
{
	// Display the connecting loading screen until OnPreLoadMap hits
	ShowLoadingWidget(ConnectingLoadScreenClass);
}

void USCGameInstance::ShowLoadingScreen(const FURL& NextURL)
{
	Super::ShowLoadingScreen(NextURL);

	// Raw travel, try to display the map-specific loading screen
	ShowLoadingScreen(FPackageName::GetLongPackageAssetName(NextURL.Map));
}

void USCGameInstance::ShowLoadingScreen(const FString& MapAssetName)
{
	if (!GetGameViewportClient() || IsDedicatedServerInstance())
	{
		return;
	}

	ISCLoadingScreenModule* const LoadingScreenModule = GetLoadingScreenModule();
	if (LoadingScreenModule != nullptr)
	{
		// Search for a map definition matching the pending map
		TSubclassOf<USCMapDefinition> MapDefinition = USCMapRegistry::FindMapByAssetName(MapRegistryClass, MapAssetName);

		// Display a short loading screen when no map definition is found
		GLongLoadMapDefinition = MapDefinition;
		LoadingScreenModule->DisplayLoadingScreen(GetGameViewportClient(), (MapDefinition == nullptr) ? nullptr : LongLoadScreenClass);

		// Take focus, cleanup other UIs
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			if (APlayerController* PC = (*It)->PlayerController)
			{
				// Update focus
				FInputModeGameOnly InputMode;
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = false;

				if (ASCPlayerController_Lobby* LobbyPC = Cast<ASCPlayerController_Lobby>(PC))
				{
					// Kill the Lobby UI
					if (ASCHUD_Lobby* LobbyHUD = Cast<ASCHUD_Lobby>(LobbyPC->GetHUD()))
					{
						LobbyHUD->HideLobbyUI();
					}
				}
				else if (ASCHUD* HUD = Cast<ASCHUD>(PC->GetHUD()))
				{
					// Kill any menus
					HUD->GetMenuStackComp()->ChangeRootMenu(nullptr, true);
				}
			}
		}
	}
}

void USCGameInstance::ShowLoadingWidget(TSubclassOf<UILLUserWidget> LoadingScreenWidget)
{
	if (!GetGameViewportClient() || IsDedicatedServerInstance())
	{
		return;
	}

	ISCLoadingScreenModule* const LoadingScreenModule = GetLoadingScreenModule();
	if (LoadingScreenModule != nullptr)
	{
		LoadingScreenModule->DisplayLoadingScreen(GetGameViewportClient(), LoadingScreenWidget);
	}
}

void USCGameInstance::HideLoadingScreen()
{
	if (!GetGameViewportClient() || IsDedicatedServerInstance())
	{
		return;
	}

	ISCLoadingScreenModule* const LoadingScreenModule = GetLoadingScreenModule();
	if (LoadingScreenModule != nullptr)
	{
		LoadingScreenModule->RemoveLoadingScreen(GetGameViewportClient());
	}

	Super::HideLoadingScreen();
}

void USCGameInstance::ShowReservingScreen(FName SessionName, const FOnlineSessionSearchResult& JoiningSession)
{
	if (SessionName == NAME_GameSession)
	{
		// When not matchmaking...
		if (!GetOnlineMatchmaking()->IsMatchmaking())
		{
			// Show the connecting loading screen
			ShowLoadingWidget(ConnectingLoadScreenClass);
		}
	}
	else if (SessionName == NAME_PartySession)
	{
		// Show party requesting reservation loading screen
		ShowLoadingWidget(PartyReservingLoadScreenClass);
	}
}

void USCGameInstance::OnGameSessionCreateFailurePreTravel(const FText& FailureReasonText)
{
	SetFailureText(FailureReasonText);
}

void USCGameInstance::OnSessionReservationFailed(FName SessionName, const EILLSessionBeaconReservationResult::Type Response)
{
	// Hide any previous loading screen
	HideLoadingScreen();

	if (SessionName == NAME_PartySession || !GetOnlineMatchmaking()->IsMatchmaking())
	{
		// The user will travel back to the main menu and display an error prompt after this
		SetFailureText(EILLSessionBeaconReservationResult::GetLocalizedFailureText(Response));
		QueueFailureErrorCheck();
	}
}

void USCGameInstance::OnSessionJoinFailure(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Hide the loading screen
	Super::OnSessionJoinFailure(SessionName, Result);

	if (SessionName == NAME_GameSession)
	{
		if (!GetOnlineMatchmaking()->IsMatchmaking())
			SetFailureText(EOnJoinSessionCompleteResult::GetLocalizedDescription(SessionName, Result));
	}
	else if (SessionName == NAME_PartySession)
	{
		SetFailureText(EOnJoinSessionCompleteResult::GetLocalizedDescription(SessionName, Result));
		QueueFailureErrorCheck();
	}
}

void USCGameInstance::OnPartyFindingLeaderGameSession()
{
	ShowLoadingWidget(PartyFindingLeaderGameLoadScreenClass);
}

void USCGameInstance::OnPartyClientHostShutdown(const FText& ClosureReasonText)
{
	// Display the failure immediately
	SetFailureText(ClosureReasonText);
	QueueFailureErrorCheck();
}

void USCGameInstance::OnPartyNetworkFailurePreTravel(const FText& FailureText)
{
	if (!FailureText.IsEmpty())
	{
		SetFailureText(FailureText);
	}
}

void USCGameInstance::OnControllerPairingLost(const int32 LocalUserNum, FText UserErrorText)
{
	Super::OnControllerPairingLost(LocalUserNum, UserErrorText);

	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		// We only care about pairings lost for our player
		if ((*It)->GetControllerId() == LocalUserNum)
		{
			if (APlayerController* PC = (*It)->PlayerController)
			{
				if (AILLHUD* HUD = Cast<AILLHUD>(PC->GetHUD()))
				{
					// Make sure the error dialog isn't on screen before trying to push a new one.
					if (!IsValid(ControllerPairingLostDialog) || HUD->GetMenuStackComp()->GetOpenMenuByClass(USCErrorDialogWidget::StaticClass()) == nullptr)
						ControllerPairingLostDialog = USCWidgetBlueprintLibrary::ShowErrorDialog(HUD, ControllerPairingErrorDialogDelegate, UserErrorText, FText());

					if (ControllerPairingLostDialog)
						ControllerPairingLostDialog->bAllUserFocus = true;
				}
			}
			break;
		}
	}
}

void USCGameInstance::OnControllerPairingRegained(const int32 LocalUserNum)
{
	Super::OnControllerPairingRegained(LocalUserNum);

	if (ControllerPairingLostDialog)
	{
		ControllerPairingLostDialog->PlayTransitionOut();
		ControllerPairingLostDialog = nullptr;
	}
}

TSubclassOf<UILLOnlineMatchmakingClient> USCGameInstance::GetOnlineMatchmakingClass() const
{
	return USCOnlineMatchmakingClient::StaticClass();
}

FString USCGameInstance::PickRandomQuickPlayMap() const
{
#if 0 // TODO: pjackson: Enable this to have QuickPlay skip the Lobby!
	// Pick a new random map
	// Do not pick the current map again
	UWorld* World = GetWorld();
	UILLGameEngine* GameEngine = Cast<UILLGameEngine>(GEngine);
	TSubclassOf<USCMapDefinition> CurrentMap = GameEngine->bHasStarted ? USCMapRegistry::FindMapByAssetName(MapRegistryClass, World->GetMapName()) : nullptr;
	TSubclassOf<USCMapDefinition> NextMap = USCMapRegistry::PickRandomMap(MapRegistryClass, CurrentMap);
	TSubclassOf<USCModeDefinition> NextMode = MapRegistryClass->GetDefaultObject<USCMapRegistry>()->QuickPlayGameMode;
	if (NextMap && ensure(NextMode))
	{
		FString Result = NextMap->GetDefaultObject<USCMapDefinition>()->Path;
		Result += TEXT("?game=");
		Result += NextMode->GetDefaultObject<USCModeDefinition>()->Alias;
		return Result;
	}
#endif

	return Super::PickRandomQuickPlayMap();
}

void USCGameInstance::ShowFirstPlayerConfirmationDialog(FSCOnConfirmedDialogDelegate Delegate, FText UserConfirmationText)
{
	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		if (APlayerController* PC = (*It)->PlayerController)
		{
			if (AILLHUD* HUD = Cast<AILLHUD>(PC->GetHUD()))
			{
				USCWidgetBlueprintLibrary::ShowConfirmationDialog(HUD, Delegate, UserConfirmationText, FText());
				break;
			}
		}
	}
}

bool USCGameInstance::ShowFirstPlayerErrorDialog(const FText& UserErrorText, const FText& HeaderText/* = FText()*/)
{
	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		if (APlayerController* PC = (*It)->PlayerController)
		{
			if (AILLHUD* HUD = Cast<AILLHUD>(PC->GetHUD()))
			{
				if (USCErrorDialogWidget* ErrorDialog = USCWidgetBlueprintLibrary::ShowErrorDialog(HUD, ErrorDialogDelegate, UserErrorText, HeaderText))
				{
					// HACK: lpederson - This is so the sign out dialog can always have focus.
					//  When you sign out your controller id seems to change after this dialog is created,
					//  need to track down where and update slate to reflect the new id.
					ErrorDialog->bAllUserFocus = true;
					return true;
				}
			}
		}
	}

	return false;
}

void USCGameInstance::OnAcceptError()
{
	QueueFailureErrorCheck();
}

void USCGameInstance::OnAcceptControllerPairingError()
{
	// Who pressed that button?
	const USCGameViewportClient* VPC = Cast<USCGameViewportClient>(GetGameViewportClient());
	const int32 LastPressUserIndex = VPC->LastKeyDownEvent.GetUserIndex();

	// Remove them from the unpaired controller list since they gave input to the error dialog.
	UnPairedControllers.Remove(LastPressUserIndex);

	// Who was controlling our game before?
	USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
	const int32 PrimaryUserIndex = LocalPlayer->GetControllerId();

	// Did they change?
	if (LastPressUserIndex != PrimaryUserIndex)
	{
		// If this is a blank controller, let them pick what account they want to be
		TSharedPtr<const FUniqueNetId> NewUserId = UOnlineEngineInterface::Get()->GetUniquePlayerId(GetWorld(), LastPressUserIndex);
		if (!NewUserId.IsValid())
		{
			IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
			const bool bLoginDisplayed = OnlineSub->GetExternalUIInterface()->ShowLoginUI(LastPressUserIndex, false, false, FOnLoginUIClosedDelegate::CreateUObject(this, &ThisClass::OnShowLoginUICompleted));
			if (!bLoginDisplayed)
			{
				// :(
				CastChecked<USCOnlineSessionClient>(GetOnlineSession())->PerformSignoutAndDisconnect(LocalPlayer, FText());
			}
		}
		// Different user! Kick back to title screen
		else
		{
			CastChecked<USCOnlineSessionClient>(GetOnlineSession())->PerformSignoutAndDisconnect(LocalPlayer, FText());
		}
	}
	else
	{
		QueueFailureErrorCheck();
	}
}

void USCGameInstance::OnShowLoginUICompleted(TSharedPtr<const FUniqueNetId> UniqueId, int LocalPlayerNum)
{
	// If our controller picked the account they were already logged into on another controller (batteries died?)
	// then let them keep playing
	USCLocalPlayer* LocalPlayer = CastChecked<USCLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
	if (UniqueId.IsValid() && *UniqueId == *LocalPlayer->GetCachedUniqueNetId())
	{
		FSlateApplication::Get().KeyboardIndexOverride = LocalPlayerNum;
		LocalPlayer->SetControllerId(LocalPlayerNum);
	}
	// They picked a different user, kick back to the title screen
	else
	{
		CastChecked<USCOnlineSessionClient>(GetOnlineSession())->PerformSignoutAndDisconnect(LocalPlayer, FText());
	}
}

#if WITH_EAC
void USCGameInstance::OnEACError(const FString& Message)
{
	// Display EAC error
	ShowFirstPlayerErrorDialog(FText::FromString(Message), LOCTEXT("EACErrorHeader", "EasyAntiCheat Error!"));

	// Block online play due to error
	GAntiCheatBlockingOnlineMatches = true;
}
#endif

void USCGameInstance::FlushFailureText()
{
	if (!PendingFailureText.IsEmpty())
	{
		// Ensure we aren't in the middle of loading
		USCOnlineSessionClient* OSC = CastChecked<USCOnlineSessionClient>(GetOnlineSession());
		ISCLoadingScreenModule* const LoadingScreenModule = GetLoadingScreenModule();
		bool bCanBeDisplayed = true;
		if (!GetWorld() || GetWorld()->IsInSeamlessTravel() || OSC->bLoadingMap)
		{
			bCanBeDisplayed = false;
		}
		else if (LoadingScreenModule && LoadingScreenModule->IsDisplayingLoadingScreen())
		{
			bCanBeDisplayed = false;
		}
		else
		{
			// Wait for a menu to be active
			APlayerController* PC = GetFirstLocalPlayerController();
			AILLHUD* HUD = PC ? Cast<AILLHUD>(PC->GetHUD()) : nullptr;
			if (!HUD || !HUD->GetMenuStackComp() || !HUD->GetMenuStackComp()->GetCurrentMenu())
				bCanBeDisplayed = false;
		}

		// Now attempt to display it
		if (bCanBeDisplayed && ShowFirstPlayerErrorDialog(PendingFailureText, PendingFailureHeader))
		{
			// Only flush the failure text when it was displayed to the user
			PendingFailureHeader = FText();
			PendingFailureText = FText();
		}
		else
		{
			// Otherwise try again next frame
			QueueFailureErrorCheck();
		}
	}
}

void USCGameInstance::QueueFailureErrorCheck()
{
	GetTimerManager().SetTimerForNextTick(this, &ThisClass::FlushFailureText);
}

void USCGameInstance::SetFailureText(const FText& NewFailureText, const FText& NewFailureHeader/* = FText()*/)
{
	if (!NewFailureText.IsEmpty())
	{
		PendingFailureHeader = NewFailureHeader;
		PendingFailureText = NewFailureText;
	}
}

bool USCGameInstance::IsPickedKillerPlayer(ASCPlayerState* PlayerState) const
{
	return (PickedKillerAccountId.IsValid() && PickedKillerAccountId == PlayerState->GetAccountId());
}

void USCGameInstance::StoreAllPersistentPlayerData()
{
	// Clear the last picked killer too
	ClearPickedKillerPlayer();

	// Now store persistent data to the game instance for the match (picked counselor character/perks, picked killer class, picked killer)
	UWorld* World = GetWorld();
	for (APlayerState* Player : World->GetGameState()->PlayerArray)
	{
		if (ASCPlayerState* PS = Cast<ASCPlayerState>(Player))
		{
			StorePersistentPlayerData(PS);
		}
	}
}

void USCGameInstance::StorePersistentPlayerData(ASCPlayerState* PlayerState)
{
	// Check if they are the picked killer
	if (PlayerState->IsPickedKiller())
	{
		PickedKillerAccountId = PlayerState->GetAccountId();
	}
}

void USCGameInstance::RevertCustomMatchSettings() {

}

void USCGameInstance::InitCustomMatchSettings() {

}

bool USCGameInstance::CustomMatchSettingsChanged() {
	return false;
}

void USCGameInstance::BackupCustomMatchSettings() {

}

#undef LOCTEXT_NAMESPACE
