// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameOnlineBlueprintLibrary.h"

#include "OnlineExternalUIInterface.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLBackendBlueprintLibrary.h"
#include "ILLGameInstance.h"
#include "ILLGameMode.h"
#include "ILLGameSession.h"
#include "ILLGameViewportClient.h"
#include "ILLLocalPlayer.h"
#include "ILLMatchmakingBlueprintLibrary.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBlueprintLibrary.h"
#include "ILLPlayerController.h"
#include "ILLPlayerState.h"

bool GAntiCheatBlockingOnlineMatches = false;

FORCEINLINE int32 _GetSessionNumPlayers(const FOnlineSession& Session)
{
	int32 PlayerCount = 0;
#ifdef SETTING_PLAYERCOUNT
	Session.SessionSettings.Get(SETTING_PLAYERCOUNT, PlayerCount);
#endif
	return FMath::Max(PlayerCount, Session.SessionSettings.NumPublicConnections - Session.NumOpenPublicConnections);
}
FORCEINLINE int32 _GetSessionNumPlayers(const FNamedOnlineSession* NamedSession)
{
	return NamedSession->RegisteredPlayers.Num();
}

FORCEINLINE int32 _GetSessionMaxPlayers(const FOnlineSession& Session)
{
	return Session.SessionSettings.NumPublicConnections;
}
FORCEINLINE int32 _GetSessionMaxPlayers(const FNamedOnlineSession* NamedSession)
{
	return _GetSessionMaxPlayers(*NamedSession);
}

bool UILLGameOnlineBlueprintLibrary::HasPlatformLogin(AILLPlayerController* PlayerController)
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
	{
		if (LocalPlayer->HasPlatformLogin() && UKismetSystemLibrary::IsLoggedIn(PlayerController))
			return true;
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::HasPlayOnlinePrivilege(AILLPlayerController* PlayerController)
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
	{
		return LocalPlayer->HasCanPlayOnlinePrivilege();
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::HasPlayOnlineFailureFlags(AILLPlayerController* PlayerController, IOnlineIdentity::EPrivilegeResults PrivilegeFlags)
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->GetLocalPlayer()))
	{
		if (static_cast<uint32>(LocalPlayer->GetPlayOnlinePrivileges()) & static_cast<uint32>(PrivilegeFlags))
		{
			return true;
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShouldEnableOfflineMatches(AILLPlayerController* PlayerController)
{
	if (UILLMatchmakingBlueprintLibrary::IsMatchmaking(PlayerController))
		return false;

	if (UILLPartyBlueprintLibrary::IsInParty(PlayerController))
		return false;

	return true;
}

bool UILLGameOnlineBlueprintLibrary::ShouldEnableOnlineMatches(AILLPlayerController* PlayerController)
{
	if (GAntiCheatBlockingOnlineMatches)
		return false;

	if (!UILLMatchmakingBlueprintLibrary::IsMatchmaking(PlayerController))
	{
		// Disallow creating/joining another game session while we still have one
		// PSN takes ages to cleanup a session so if you close then create a private match too quickly it can cause a broken game
		// NOTE: Loading screen may cover up this entirely now that it stays up until ILLGameSession allows BeginPlay
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (!SessionInt.IsValid() || SessionInt->GetNamedSession(NAME_GameSession))
			return false;

#if PLATFORM_PS4 || PLATFORM_XBOXONE
		// Allow online match buttons on PS4/XB1 when the only reason they are disabled is due to missing PSPlus
		// This allows us to show the "buy PSPlus!" or "buy XboxLive Gold!" nag via ConditionalShowAccountUpgradeUI
		if (HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::AccountTypeFailure))
		{
			return true;
		}
#endif

		// Allow online features to function when a patch or system update is required, so that using them will show a prompt via ConditionalShowAccountUpgradeUI
		if(HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable)
		|| HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate)
		|| HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure))
		{
			return true;
		}

		return HasPlayOnlinePrivilege(PlayerController);
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShouldEnableLANMatch(AILLPlayerController* PlayerController)
{
	if (ShouldEnableOnlineMatches(PlayerController)
	&& !UILLPartyBlueprintLibrary::IsPartyBlockingLANSessions(PlayerController))
	{
		return true;
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShouldEnablePrivateMatch(AILLPlayerController* PlayerController)
{
	if (ShouldEnableOnlineMatches(PlayerController)
	&& !UILLPartyBlueprintLibrary::IsPartyBlockingOnlineSessions(PlayerController)
	&& !UILLBackendBlueprintLibrary::IsInOfflineMode(PlayerController)
	&& !UILLPartyBlueprintLibrary::IsInParty(PlayerController)) // Disallow parties to enter private matches 'cause PS4
	{
		return true;
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShouldEnableQuickPlay(AILLPlayerController* PlayerController)
{
	if (ShouldEnableOnlineMatches(PlayerController)
	&& !UILLPartyBlueprintLibrary::IsPartyBlockingOnlineSessions(PlayerController)
	&& !UILLBackendBlueprintLibrary::IsInOfflineMode(PlayerController)
	&& !UILLMatchmakingBlueprintLibrary::HasMatchmakingBan(PlayerController))
	{
		return true;
	}

	return false;
}

FString UILLGameOnlineBlueprintLibrary::GetLastInputNickname(AILLPlayerController* PlayerController)
{
	FString PlatformNickname;

	if (UWorld* World = PlayerController->GetWorld())
	{
		if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->Player))
		{
			int32 ControllerId = LocalPlayer->GetControllerId();
			if (UILLGameViewportClient* VPC = Cast<UILLGameViewportClient>(PlayerController->GetGameInstance()->GetGameViewportClient()))
			{
				ControllerId = VPC->LastKeyDownEvent.GetUserIndex();
			}

			if (!UOnlineEngineInterface::Get()->GetPlayerPlatformNickname(World, ControllerId, PlatformNickname))
			{
				// Fallback to the unique ID when platform nickname lookup fails
				TSharedPtr<const FUniqueNetId> UniqueId = LocalPlayer->GetPreferredUniqueNetId();
				if (UniqueId.IsValid())
				{
					return UOnlineEngineInterface::Get()->GetPlayerNickname(World, *UniqueId);
				}
			}
		}
	}

	return PlatformNickname;
}

void UILLGameOnlineBlueprintLibrary::SignOutAndDisconnect(AILLPlayerController* PlayerController, FText FailureText)
{
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(PlayerController->Player))
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(PlayerController->GetGameInstance()->GetOnlineSession()))
		{
			OSC->PerformSignoutAndDisconnect(LocalPlayer, FailureText);
		}
	}
}

bool UILLGameOnlineBlueprintLibrary::ShowAccountUpgradeUI(AILLPlayerController* PlayerController)
{
	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	if (OnlineExternalUI.IsValid())
	{
		return OnlineExternalUI->ShowAccountUpgradeUI(*PlayerController->GetLocalPlayer()->GetPreferredUniqueNetId());
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ConditionalShowAccountUpgradeUI(AILLPlayerController* PlayerController)
{
#if PLATFORM_PS4 || PLATFORM_XBOXONE
	if (HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::AccountTypeFailure))
	{
# if PLATFORM_PS4
		// Call ShowAccountUpgradeUI to show the "buy PSPlus!" nag
		ShowAccountUpgradeUI(PlayerController);
# elif PLATFORM_XBOXONE
		// Refresh the PlayOnline privilege, which displays the "buy XboxLive Gold!" nag
		UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(PlayerController->Player);
		LocalPlayer->RefreshPlayOnlinePrivilege();
# endif
		return true;
	}
	else
#endif
	if (UILLGameInstance* GameInstance = PlayerController ? CastChecked<UILLGameInstance>(PlayerController->GetGameInstance()) : nullptr)
	{
		if (HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable))
		{
			GameInstance->PlayOnlineRequiredPatchAvailable();
			return true;
		}
		else if (HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate))
		{
			GameInstance->PlayOnlineRequiredSystemUpdate();
			return true;
		}
		else if (HasPlayOnlineFailureFlags(PlayerController, IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure))
		{
			GameInstance->PlayOnlineAgeRestrictionFailure();
			return true;
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShowExternalGameInviteUI(AILLPlayerController* PlayerController)
{
	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	if (PlayerController->GetLocalPlayer() && OnlineExternalUI.IsValid())
	{
		return OnlineExternalUI->ShowInviteUI(PlayerController->GetLocalPlayer()->GetControllerId(), NAME_GameSession);
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShowExternalInviteUI(AILLPlayerController* PlayerController)
{
	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	IOnlineSessionPtr OnlineSessions = Online::GetSessionInterface();
	if (PlayerController->GetLocalPlayer() && OnlineExternalUI.IsValid() && OnlineSessions.IsValid())
	{
		FNamedOnlineSession* PartySession = OnlineSessions->GetNamedSession(NAME_PartySession);
		FName InviteSessionName = PartySession ? NAME_PartySession : NAME_GameSession;
		return OnlineExternalUI->ShowInviteUI(PlayerController->GetLocalPlayer()->GetControllerId(), InviteSessionName);
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShowExternalWebUI(const FString& WebURL)
{
	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	if (OnlineExternalUI.IsValid())
	{
		return OnlineExternalUI->ShowWebURL(WebURL, FShowWebUrlParams(), FOnShowWebUrlClosedDelegate());
	}

	return false;
}

static bool _ShowExternalProfileUI(const FUniqueNetIdRepl& Requestor, const FUniqueNetIdRepl& Requestee)
{
	if (!Requestor.IsValid())
	{
		UE_LOG(LogIGF, Warning, TEXT("ShowExternalProfileUI: Invalid Requestor!"));
		return false;
	}
	if (!Requestee.IsValid())
	{
		UE_LOG(LogIGF, Warning, TEXT("ShowExternalProfileUI: Invalid Requestee!"));
		return false;
	}

	IOnlineExternalUIPtr OnlineExternalUI = Online::GetExternalUIInterface();
	if (OnlineExternalUI.IsValid())
	{
		return OnlineExternalUI->ShowProfileUI(*Requestor, *Requestee);
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::ShowExternalProfileUI(AILLPlayerState* Requestor, AILLPlayerState* Requestee)
{
	return _ShowExternalProfileUI(Requestor ? Requestor->UniqueId : FUniqueNetIdRepl(), Requestee ? Requestee->UniqueId : FUniqueNetIdRepl());
}

bool UILLGameOnlineBlueprintLibrary::ShowPartyMemberExternalProfileUI(AILLPlayerState* Requestor, AILLPartyBeaconMemberState* Requestee)
{
	return _ShowExternalProfileUI(Requestor ? Requestor->UniqueId : FUniqueNetIdRepl(), Requestee ? Requestee->GetMemberUniqueId() : FUniqueNetIdRepl());
}

bool UILLGameOnlineBlueprintLibrary::CanInviteToCurrentGameSession(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			if (Session->SessionSettings.bIsLANMatch)
			{
				return false;
			}

			return (Session->SessionSettings.bAllowInvites && !IsCurrentGameSessionFull(WorldContextObject));
		}
	}

	return false;
}

FString UILLGameOnlineBlueprintLibrary::GetCurrentGameSessionName(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface();
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Identity.IsValid() && Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			// Display the owning username, will be the dedicated server name on those
			if (!Session->OwningUserName.IsEmpty())
				return Session->OwningUserName;

			// Fallback to the host's nickname for non-dedicated sessions
			if (Session->OwningUserId.IsValid() && !Session->SessionSettings.bIsDedicated)
				return Identity->GetPlayerNickname(*Session->OwningUserId);
		}
	}

	return FString();
}

int32 UILLGameOnlineBlueprintLibrary::GetCurrentGameSessionPlayerCount(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface();
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Identity.IsValid() && Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			return _GetSessionNumPlayers(Session);
		}
	}

	return -1;
}

int32 UILLGameOnlineBlueprintLibrary::GetCurrentGameSessionPlayerMax(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface();
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Identity.IsValid() && Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			return _GetSessionMaxPlayers(Session);
		}
	}

	return -1;
}

bool UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionAntiCheatProtected(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			return Session->SessionSettings.bAntiCheatProtected;
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionFull(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			const int32 NumPlayers = _GetSessionNumPlayers(Session);
			const int32 MaxPlayers = _GetSessionMaxPlayers(Session);
			return (MaxPlayers > 0 && NumPlayers >= MaxPlayers);
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionDedicated(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			return Session->SessionSettings.bIsDedicated;
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionLAN(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			return Session->SessionSettings.bIsLANMatch;
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionStandalone(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* GameSession  = Sessions->GetNamedSession(NAME_GameSession))
		{
			return false;
		}
	}

	return World ? World->GetNetMode() == NM_Standalone : false;
}

bool UILLGameOnlineBlueprintLibrary::IsCurrentGameSessionPrivate(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* GameSession  = Sessions->GetNamedSession(NAME_GameSession))
		{
			return !GameSession ->SessionSettings.bShouldAdvertise;
		}
	}

	return false;
}

bool UILLGameOnlineBlueprintLibrary::IsLocalPlayerGameSessionServer(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* GameSession = Sessions->GetNamedSession(NAME_GameSession))
		{
			if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(World->GetFirstLocalPlayerFromController()))
			{
				if(!LocalPlayer->GetPreferredUniqueNetId().IsValid()
				|| !GameSession->OwningUserId.IsValid())
					return false;

				if (*LocalPlayer->GetPreferredUniqueNetId() == *GameSession->OwningUserId)
					return true;
			}
		}
	}

	return false;
}

FString UILLGameOnlineBlueprintLibrary::GetGameModeSetting(const FBlueprintSessionResult& Result)
{
	FString Setting(TEXT(""));
	Result.OnlineResult.Session.SessionSettings.Get(SETTING_GAMEMODE, Setting);
	return Setting;
}

FString UILLGameOnlineBlueprintLibrary::GetMapNameSetting(const FBlueprintSessionResult& Result)
{
	FString Setting(TEXT(""));
	Result.OnlineResult.Session.SessionSettings.Get(SETTING_MAPNAME, Setting);
	return Setting;
}

int32 UILLGameOnlineBlueprintLibrary::GetMaxPlayers(const FBlueprintSessionResult& Result)
{
	return _GetSessionMaxPlayers(Result.OnlineResult.Session);
}

int32 UILLGameOnlineBlueprintLibrary::GetNumPlayers(const FBlueprintSessionResult& Result)
{
	return _GetSessionNumPlayers(Result.OnlineResult.Session);
}

bool UILLGameOnlineBlueprintLibrary::IsSessionAntiCheatProtected(const FBlueprintSessionResult& Result)
{
	return Result.OnlineResult.Session.SessionSettings.bAntiCheatProtected;
}

bool UILLGameOnlineBlueprintLibrary::IsSessionPrivate(const FBlueprintSessionResult& Result)
{
	return !Result.OnlineResult.Session.SessionSettings.bShouldAdvertise;
}

bool UILLGameOnlineBlueprintLibrary::IsSessionDedicated(const FBlueprintSessionResult& Result)
{
	return Result.OnlineResult.Session.SessionSettings.bIsDedicated;
}

bool UILLGameOnlineBlueprintLibrary::IsSessionPassworded(const FBlueprintSessionResult& Result)
{
	return Result.OnlineResult.Session.SessionSettings.bIsPassworded;
}

TArray<FBlueprintSessionResult> UILLGameOnlineBlueprintLibrary::GetNonFullSessions(const TArray<FBlueprintSessionResult>& SessionList)
{
	TArray<FBlueprintSessionResult> Results;
	Results.Empty(SessionList.Num());
	for (auto Session : SessionList)
	{
		if (GetNumPlayers(Session) < GetMaxPlayers(Session))
		{
			Results.Add(Session);
		}
	}

	return Results;
}

TArray<FBlueprintSessionResult> UILLGameOnlineBlueprintLibrary::GetNonEmptySessions(const TArray<FBlueprintSessionResult>& SessionList)
{
	TArray<FBlueprintSessionResult> Results;
	Results.Empty(SessionList.Num());
	for (auto Session : SessionList)
	{
		if (GetNumPlayers(Session) > 0)
		{
			Results.Add(Session);
		}
	}

	return Results;
}

TArray<FBlueprintSessionResult> UILLGameOnlineBlueprintLibrary::SortSessionList(const TArray<FBlueprintSessionResult>& SessionList, const ESessionSortColumn Column, const bool bAscending)
{
	struct FCompareSessionBase
	{
		FCompareSessionBase(const bool bInAscending)
		: bSortAscending(bInAscending) {}

		bool bSortAscending;
	};

	struct FCompareSessionByName
	: public FCompareSessionBase
	{
		FCompareSessionByName(const bool bInAscending)
		: FCompareSessionBase(bInAscending) {}

		FORCEINLINE bool operator()(const FBlueprintSessionResult& A, const FBlueprintSessionResult& B) const
		{
			const auto AValue = A.OnlineResult.Session.OwningUserName;
			const auto BValue = B.OnlineResult.Session.OwningUserName;
			if (AValue == BValue)
			{
				// Tie-break by ping
				return A.OnlineResult.PingInMs < B.OnlineResult.PingInMs;
			}
			return bSortAscending ? AValue < BValue : AValue > BValue;
		}
	};

	struct FCompareSessionByMap
	: public FCompareSessionBase
	{
		FCompareSessionByMap(const bool bInAscending)
		: FCompareSessionBase(bInAscending) {}

		FORCEINLINE bool operator()(const FBlueprintSessionResult& A, const FBlueprintSessionResult& B) const
		{
			const auto AValue = GetMapNameSetting(A);
			const auto BValue = GetMapNameSetting(B);
			if (AValue == BValue)
			{
				// Tie-break by ping
				return A.OnlineResult.PingInMs < B.OnlineResult.PingInMs;
			}
			return bSortAscending ? AValue < BValue : AValue > BValue;
		}
	};

	struct FCompareSessionByMode
	: public FCompareSessionBase
	{
		FCompareSessionByMode(const bool bInAscending)
		: FCompareSessionBase(bInAscending) {}

		FORCEINLINE bool operator()(const FBlueprintSessionResult& A, const FBlueprintSessionResult& B) const
		{
			const auto AValue = GetGameModeSetting(A);
			const auto BValue = GetGameModeSetting(B);
			if (AValue == BValue)
			{
				// Tie-break by ping
				return A.OnlineResult.PingInMs < B.OnlineResult.PingInMs;
			}
			return bSortAscending ? AValue < BValue : AValue > BValue;
		}
	};

	struct FCompareSessionByPlayers
	: public FCompareSessionBase
	{
		FCompareSessionByPlayers(const bool bInAscending)
		: FCompareSessionBase(bInAscending) {}

		FORCEINLINE bool operator()(const FBlueprintSessionResult& A, const FBlueprintSessionResult& B) const
		{
			int32 AValue = UILLGameOnlineBlueprintLibrary::GetNumPlayers(A);
			int32 BValue = UILLGameOnlineBlueprintLibrary::GetNumPlayers(B);
			if (AValue == BValue)
			{
				// Tie-break by ping
				return A.OnlineResult.PingInMs < B.OnlineResult.PingInMs;
			}
			return bSortAscending ? AValue < BValue : AValue > BValue;
		}
	};

	struct FCompareSessionByPing
	: public FCompareSessionBase
	{
		FCompareSessionByPing(const bool bInAscending)
		: FCompareSessionBase(bInAscending) {}

		FORCEINLINE bool operator()(const FBlueprintSessionResult& A, const FBlueprintSessionResult& B) const
		{
			const auto AValue = A.OnlineResult.PingInMs;
			const auto BValue = B.OnlineResult.PingInMs;
			return bSortAscending ? AValue > BValue : AValue < BValue;
		}
	};

	TArray<FBlueprintSessionResult> Results = SessionList;
	switch (Column)
	{
	case ESessionSortColumn::Name: Results.Sort(FCompareSessionByName(bAscending)); break;
	case ESessionSortColumn::Map: Results.Sort(FCompareSessionByMap(bAscending)); break;
	case ESessionSortColumn::Mode: Results.Sort(FCompareSessionByMode(bAscending)); break;
	case ESessionSortColumn::Players: Results.Sort(FCompareSessionByPlayers(bAscending)); break;
	case ESessionSortColumn::Ping: Results.Sort(FCompareSessionByPing(bAscending)); break;
	default:
		check(0);
		break;
	}

	return Results;
}

bool UILLGameOnlineBlueprintLibrary::CanKickPlayer(AILLPlayerController* Requestor, AILLPlayerState* PlayerState)
{
	if (Requestor && PlayerState)
	{
		return Requestor->CanKickPlayer(PlayerState);
	}

	return false;
}

void UILLGameOnlineBlueprintLibrary::KickPlayer(AILLPlayerController* Requestor, AILLPlayerState* PlayerState)
{
	if (Requestor && PlayerState)
	{
		Requestor->ServerRequestKickPlayer(PlayerState);
	}
}
