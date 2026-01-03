// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCOnlineSessionClient.h"

#if WITH_EAC
# include "IEasyAntiCheatServer.h"
#endif

#include "ILLLocalPlayer.h"

#include "SCGameInstance.h"
#include "SCLobbyBeaconClient.h"
#include "SCLobbyBeaconHost.h"
#include "SCLobbyBeaconHostObject.h"
#include "SCPartyBeaconClient.h"
#include "SCPartyBeaconHost.h"
#include "SCPartyBeaconHostObject.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCOnlineSessionClient"

USCOnlineSessionClient::USCOnlineSessionClient(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	LobbyClientClass = ASCLobbyBeaconClient::StaticClass();
	LobbyHostObjectClass = ASCLobbyBeaconHostObject::StaticClass();
	LobbyBeaconHostClass = ASCLobbyBeaconHost::StaticClass();

	PartyClientClass = ASCPartyBeaconClient::StaticClass();
	PartyHostObjectClass = ASCPartyBeaconHostObject::StaticClass();
	PartyBeaconHostClass = ASCPartyBeaconHost::StaticClass();
}

bool USCOnlineSessionClient::HostGameSession(TSharedPtr<FILLGameSessionSettings> SessionSettings)
{
#if WITH_EAC
	if (!IEasyAntiCheatServer::IsAvailable())
	{
		HandleGameSessionCreationFailure(LOCTEXT("GameSessionCreationError_AntiCheatNotAvailable", "Anti-cheat is not available, preventing game session creation!"));
		return false;
	}
#endif

	return Super::HostGameSession(SessionSettings);
}

void USCOnlineSessionClient::PerformSignoutAndDisconnect(UILLLocalPlayer* Player, FText FailureText)
{
	if (Player)
	{
		// Clear our menu stack so we go back to entry
		Player->CachedMenuStack.Empty();
	}

	Super::PerformSignoutAndDisconnect(Player, FailureText);
}

#undef LOCTEXT_NAMESPACE
