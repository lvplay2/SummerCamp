// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPartyBlueprintLibrary.h"

#include "OnlineStats.h"
#include "OnlineExternalUIInterface.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ILLGameInstance.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconClient.h"
#include "ILLPartyBeaconHostObject.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"

bool UILLPartyBlueprintLibrary::CanCreateParty(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			return OSC->CanCreateParty();
		}
	}

	return false;
}

bool UILLPartyBlueprintLibrary::CanInviteToParty(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			return OSC->CanInviteToParty();
		}
	}

	return false;
}

int32 UILLPartyBlueprintLibrary::GetNumPartyMembers(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_PartySession))
		{
			return Session->RegisteredPlayers.Num();
		}
	}

	return -1;
}

int32 UILLPartyBlueprintLibrary::GetMaxPartyMembers(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
	if (Sessions.IsValid())
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_PartySession))
		{
			return Session->SessionSettings.NumPublicConnections;
		}
	}

	return -1;
}

FText UILLPartyBlueprintLibrary::GetPartyLeaderStatusDescription(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			if (OSC->IsInPartyAsLeader() || OSC->IsInPartyAsMember())
			{
				return OSC->GetPartyBeaconState()->GetLeaderStateDescription();
			}
		}
	}

	return FText();
}

bool UILLPartyBlueprintLibrary::IsInParty(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			return (OSC->IsCreatingParty() || OSC->IsJoiningParty() || OSC->IsInPartyAsMember() || OSC->IsInPartyAsLeader());
		}
	}

	return false;
}

bool UILLPartyBlueprintLibrary::IsInPartyAsLeader(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			return OSC->IsInPartyAsLeader();
		}
	}

	return false;
}

bool UILLPartyBlueprintLibrary::IsInPartyAsMember(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			return OSC->IsInPartyAsMember();
		}
	}

	return false;
}

bool UILLPartyBlueprintLibrary::IsJoiningParty(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			return OSC->IsJoiningParty();
		}
	}

	return false;
}

bool UILLPartyBlueprintLibrary::IsPartyBlockingLANSessions(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			// Parties are not allowed anywhere near LAN
			if (OSC->IsInPartyAsLeader() || OSC->IsInPartyAsMember())
				return true;

			// Also disallow it while creating/joining a party
			if (OSC->IsCreatingParty() || OSC->IsJoiningParty())
				return true;

			// Disallow it while joining a game session
			if (OSC->IsJoiningGameSession())
				return true;

			// Do nothing while pending invite
			if (OSC->HasDeferredInvite())
				return true;

			return false;
		}
	}

	return true;
}

bool UILLPartyBlueprintLibrary::IsPartyBlockingOnlineSessions(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			// Party members must follow their leader
			if (OSC->IsInPartyAsMember())
				return true;

			// Wait for pending party members to reestablish their connections
			if (OSC->HasPendingPartyMembers())
				return true;

			// Do not allow this while already joining a party or game session
			if (OSC->IsJoiningParty() || OSC->IsJoiningGameSession())
				return true;

			// Do nothing while pending invite
			if (OSC->HasDeferredInvite())
				return true;

			return false;
		}
	}

	return true;
}

bool UILLPartyBlueprintLibrary::IsPartyLeader(AILLPartyBeaconMemberState* MemberState)
{
	if (MemberState && MemberState->bArbitrationMember)
	{
		return true;
	}
	return false;
}

void UILLPartyBlueprintLibrary::InviteFriendToParty(UObject* WorldContextObject)
{
	if (!IsInParty(WorldContextObject))
	{
		return;
	}

	if (IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get())
	{
		if (OnlineSub->GetExternalUIInterface().IsValid())
		{
			OnlineSub->GetExternalUIInterface()->ShowInviteUI(0, NAME_PartySession);
		}
	}
}

void UILLPartyBlueprintLibrary::KickPartyMember(AILLPartyBeaconMemberState* MemberState)
{
	UWorld* World = (MemberState && MemberState->GetBeaconClientActor()) ? MemberState->GetWorld() : nullptr;
	if (UILLGameInstance* GI = World ? Cast<UILLGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			if (OSC->IsInPartyAsLeader() && OSC->GetPartyHostBeaconObject())
			{
				if (AILLPartyBeaconClient* PartyMember = CastChecked<AILLPartyBeaconClient>(MemberState->GetBeaconClientActor()))
					OSC->GetPartyHostBeaconObject()->KickPlayer(PartyMember, FText());
			}
		}
	}
}
