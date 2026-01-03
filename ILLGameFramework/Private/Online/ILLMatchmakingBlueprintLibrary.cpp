// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLMatchmakingBlueprintLibrary.h"

#include "ILLGameBlueprintLibrary.h"
#include "ILLGameInstance.h"
#include "ILLOnlineMatchmakingClient.h"
#include "ILLOnlineSessionClient.h"

bool UILLMatchmakingBlueprintLibrary::BeginMatchmaking(UObject* WorldContextObject, const bool bForLAN)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		return OMC->BeginMatchmaking(bForLAN);
	}

	return false;
}

FString UILLMatchmakingBlueprintLibrary::GetMatchmakingRunningTimeString(UObject* WorldContextObject, const bool bAlwaysShowMinute/* = true*/)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();

		const EILLMatchmakingState State = OMC->GetMatchmakingState();
		if(State != EILLMatchmakingState::NotRunning
		&& State != EILLMatchmakingState::Canceling
		&& State != EILLMatchmakingState::Canceled)
		{
			const float StartTime = OMC->GetMatchmakingStartRealTime();
			if (StartTime >= 0.f)
			{
				const float RealTime = World->GetRealTimeSeconds();
				const int32 Duration = FMath::RoundToInt(RealTime - StartTime);
				return UILLGameBlueprintLibrary::TimeSecondsToTimeString(Duration, bAlwaysShowMinute, false);
			}
		}
	}

	return FString();
}

FText UILLMatchmakingBlueprintLibrary::GetMatchmakingStatusText(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		const EILLMatchmakingState State = OMC->GetMatchmakingState();

		// Show nothing when not matchmaking
		if (State == EILLMatchmakingState::NotRunning)
			return FText();

		// If we are in a game session, then we are performing QoS
		// Override the display to Joining so the user is made aware immediately
		IOnlineSessionPtr Sessions = OMC->GetSessionInt();
		if (FNamedOnlineSession* GameSession = Sessions->GetNamedSession(NAME_GameSession))
		{
			if (GameSession->SessionState != EOnlineSessionState::Destroying)
				return GetLocalizedDescription(EILLMatchmakingState::Joining);
		}

		// Hold "Canceling" instead of showing "Canceled"
		if (State == EILLMatchmakingState::Canceled)
			return GetLocalizedDescription(EILLMatchmakingState::Canceling);

		// Return the matchmaking status
		return GetLocalizedDescription(State);
	}

	return FText();
}

int32 UILLMatchmakingBlueprintLibrary::GetMatchmakingBanRemainingSeconds(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		if (OMC->GetLastAggregateMatchmakingStatus() == EILLBackendMatchmakingStatusType::Banned)
		{
			const float RemainingSeconds = OMC->GetLastAggregateMatchmakingUnbanRealTime() - World->GetRealTimeSeconds();
			return FMath::Max((int32)RemainingSeconds, 0);
		}
	}

	return 0;
}

bool UILLMatchmakingBlueprintLibrary::HasMatchmakingBan(UObject* WorldContextObject)
{
	return (GetMatchmakingBanRemainingSeconds(WorldContextObject) > 0);
}

bool UILLMatchmakingBlueprintLibrary::HasLowPriorityMatchmaking(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		return (OMC->GetLastAggregateMatchmakingStatus() == EILLBackendMatchmakingStatusType::LowPriority);
	}

	return false;
}

bool UILLMatchmakingBlueprintLibrary::IsLeadingMatchmaking(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession());
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		return (OMC->IsMatchmaking() && (OSC->IsInPartyAsLeader() || (!OSC->IsInPartyAsMember() && !OSC->IsJoiningParty())));
	}

	return false;
}

bool UILLMatchmakingBlueprintLibrary::IsMatchmaking(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		return OMC->IsMatchmaking();
	}

	return false;
}

bool UILLMatchmakingBlueprintLibrary::IsRegionalizedMatchmakingSupported(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		UILLOnlineMatchmakingClient* OMC = GI->GetOnlineMatchmaking();
		return OMC->SupportsRegionalizedMatchmaking();
	}

	return false;
}
