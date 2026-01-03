// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerState.h"

#include "Net/OnlineEngineInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "ILLGameBlueprintLibrary.h"
#include "ILLGameState_Lobby.h"
#include "ILLHUD_Lobby.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLLocalPlayer.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPartyBeaconMemberState.h"
#include "ILLPartyBeaconState.h"
#include "ILLPlayerController.h"
#include "ILLPlayerStateLocalMessage.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLPlayerState"

AILLPlayerState::AILLPlayerState(const FObjectInitializer& ObjectInitializer) 
: Super(ObjectInitializer)
{
	NetPriority = 1.75f;

	PlayerStateMessageClass = UILLPlayerStateLocalMessage::StaticClass();
}

void AILLPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLPlayerState, AccountId);
	DOREPLIFETIME(AILLPlayerState, bIsHost);
	DOREPLIFETIME(AILLPlayerState, bHasAckedPlayerState);
}

void AILLPlayerState::Destroyed()
{
	UWorld* World = GetWorld();

	// Display a localized notification to the local player(s)
	if (PlayerStateMessageClass)
	{
		for (FLocalPlayerIterator It(GEngine, World); It; ++It)
		{
			if (APlayerController* PC = It->PlayerController)
			{
				if (PC->IsLocalPlayerController())
					PC->ClientReceiveLocalizedMessage(PlayerStateMessageClass, static_cast<uint32>(UILLPlayerStateLocalMessage::EMessage::UnregisteredWithGame), this);
			}
		}
	}

	if (Role == ROLE_Authority && bIsHost)
	{
		// Make this person no longer the host
		DemoteFromHost();

		// Pick a new host
		if (AGameStateBase* GameState = World->GetGameState())
		{
			TArray<APlayerState*> HostOptions = GameState->PlayerArray;
			if (HostOptions.Num() > 0)
			{
				// Pick a new random host
				AILLPlayerState* RandomPlayer = nullptr;
				for (int32 Attempts = 0; Attempts < HostOptions.Num(); ++Attempts)
				{
					const int32 RandomIndex = FMath::RandHelper(HostOptions.Num());
					RandomPlayer = Cast<AILLPlayerState>(HostOptions[RandomIndex]);
					if (!RandomPlayer || RandomPlayer == this || !RandomPlayer->CanBeHost())
					{
						// Try again...
						RandomPlayer = nullptr;
						HostOptions.RemoveAtSwap(RandomIndex);
					}
					else
					{
						break;
					}
				}

				// Promote them
				if (RandomPlayer)
				{
					RandomPlayer->PromoteToHost();
				}
			}
		}
	}

	Super::Destroyed();
}

void AILLPlayerState::OnRep_Owner()
{
	Super::OnRep_Owner();

	// Pulse a lobby update
	PlayerSettingsUpdated();
}

void AILLPlayerState::PostNetReceive()
{
	Super::PostNetReceive();

	// Synchronize beacon actors
	SynchronizeBeaconActors();

	// Update travel status
	HasFullyTraveled(true);
}

void AILLPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	// Attempt to register again once the name is available
	RegisterPlayerWithSession(false);
}

void AILLPlayerState::OnRep_UniqueId()
{
	Super::OnRep_UniqueId();

	OnReceivedUniqueId();
}

void AILLPlayerState::SetUniqueId(const FUniqueNetIdRepl& InUniqueId)
{
	Super::SetUniqueId(InUniqueId);

	OnReceivedUniqueId();
}

void AILLPlayerState::SetPlayerName(const FString& S)
{
	Super::SetPlayerName(S);

	RegisterPlayerWithSession(false);
}

int32 AILLPlayerState::GetRealPing() const
{
	return FMath::Min(int32(Ping)*4, 999);
}

FText AILLPlayerState::GetPingDisplayText() const
{
	if (!HasFullyTraveled())
	{
		return FText();
	}

	const uint32 ShortPing = GetRealPing();
	if (ShortPing == 0)
	{
		return FText();
	}

	return FText::Format(LOCTEXT("PingDisplayText", "{0}ms"), FText::FromString(FString::FromInt(ShortPing)));
}

void AILLPlayerState::OnReceivedUniqueId()
{
	// Synchronize beacon actors
	SynchronizeBeaconActors();
}

void AILLPlayerState::SetAccountId(const FILLBackendPlayerIdentifier& InAccountId)
{
	if (AccountId != InAccountId)
	{
		AccountId = InAccountId;

		// Simulate replication on the host
		OnRep_AccountId();
	}
}

void AILLPlayerState::OnRep_AccountId()
{
	// Synchronize beacon actors
	SynchronizeBeaconActors();
}

void AILLPlayerState::UpdatePing(float InPing)
{
	// Only push APlayerState::PingBucket entries after fully joining
	// This mitigates filling the buckets with high pings while the player is still loading/syncing
	if (HasFullyTraveled())
	{
		Super::UpdatePing(InPing);
	}
}

void AILLPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	if (AILLPlayerState* OtherPS = Cast<AILLPlayerState>(PlayerState))
	{
		SetAccountId(OtherPS->GetAccountId());
		bHasRegisteredWithGameSession = OtherPS->bHasRegisteredWithGameSession;
		bIsHost = OtherPS->bIsHost;
	}

	// Synchronize beacon actors
	SynchronizeBeaconActors();
}

void AILLPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState))
	{
		PS->SetAccountId(GetAccountId());

		// Synchronize beacon actors
		PS->SynchronizeBeaconActors();
	}
}

void AILLPlayerState::SeamlessTravelTo(APlayerState* NewPlayerState)
{
	Super::SeamlessTravelTo(NewPlayerState);
	
	if (AILLPlayerState* PS = Cast<AILLPlayerState>(NewPlayerState))
	{
		// Maintain host
		if (bIsHost)
		{
			// PromoteToHost should make this no longer the host
			PS->PromoteToHost();
		}

		if (!PS->bHasRegisteredWithGameSession)
		{
			// Work-around: register them with the game session again, like clients would
			PS->RegisterPlayerWithSession(false);
		}
	}
}

void AILLPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	// Skip the Super call!

	if (GetNetMode() == NM_Standalone)
		return;

	// Wait for both of these to replicate
	if (!UniqueId.IsValid())
		return;
	if (PlayerName.IsEmpty())
		return;

	// Short-out
	if (bHasRegisteredWithGameSession)
		return;

	UWorld* World = GetWorld();
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	check(SessionInt.IsValid());
	FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
	if (!Session)
		return;

	// Determine if they are in the RegisteredPlayers list
	// NOTE: We can't use IsPlayerInSession because it returns true for the host
	bool bFoundInPlayerList = false;
	for (const TSharedRef<const FUniqueNetId>& Player : Session->RegisteredPlayers)
	{
		if (*Player == *UniqueId)
		{
			bFoundInPlayerList = true;
			break;
		}
	}

	// Add them if not
	if (!bFoundInPlayerList)
	{
		if (SessionInt->RegisterPlayer(SessionName, *UniqueId, bWasFromInvite))
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("%s: (%s) %s %s registered"), ANSI_TO_TCHAR(__FUNCTION__), *GetName(), *PlayerName, SafeUniqueIdTChar(UniqueId));
		}
	}
	bHasRegisteredWithGameSession = true;

	// Synchronize beacon actors
	SynchronizeBeaconActors();

	// Update travel status
	HasFullyTraveled(true);

	// Replicate mute states for this player
	ReplicateMuteStates();
}

void AILLPlayerState::UnregisterPlayerWithSession()
{
	// NOTE: Intentionally does not make the Super call. UE4 does this in a very odd way. @see AILLGameSession::UnregisterPlayer

	// This gets called from APlayerState::Destroyed. Do not unregister players with the session when this is a player state from the previous level.
	// This player may still be in the game with a new player state
	if (bFromPreviousLevel)
	{
		return;
	}

	// Only unregister if they have previously registered with a game session!
	UWorld* World = GetWorld();
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
	if (!UniqueId.IsValid() || !SessionInt.IsValid() || !SessionInt->IsPlayerInSession(SessionName, *UniqueId))
	{
		return;
	}

	// Short-out
	if (!bHasRegisteredWithGameSession)
	{
		return;
	}

	// Unregister with the SessionName
	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: (%s) %s %s unregistered"), ANSI_TO_TCHAR(__FUNCTION__), *GetName(), *PlayerName, SafeUniqueIdTChar(UniqueId));
	SessionInt->UnregisterPlayer(SessionName, *UniqueId);
	bHasRegisteredWithGameSession = false;
}

AILLLobbyBeaconMemberState* AILLPlayerState::GetLobbyMemberState() const
{
	if (!LobbyMemberState && (AccountId.IsValid() || UniqueId.IsValid()))
	{
		// Search the lobby beacon state member list for a matching member
		UWorld* World = GetWorld();
		if (UILLOnlineSessionClient* OSC = (World && World->GetGameInstance()) ? Cast<UILLOnlineSessionClient>(World->GetGameInstance()->GetOnlineSession()) : nullptr)
		{
			if (AILLLobbyBeaconState* LobbyState = OSC->GetLobbyBeaconState())
			{
				LobbyMemberState = Cast<AILLLobbyBeaconMemberState>(LobbyState->FindMember(AccountId));
				if (!LobbyMemberState)
					LobbyMemberState = Cast<AILLLobbyBeaconMemberState>(LobbyState->FindMember(UniqueId));
			}
		}
	}

	return LobbyMemberState;
}

AILLPartyBeaconMemberState* AILLPlayerState::GetPartyMemberState() const
{
	if (!PartyMemberState && (AccountId.IsValid() || UniqueId.IsValid()))
	{
		// Search the party beacon state member list for a matching member
		UWorld* World = GetWorld();
		if (UILLOnlineSessionClient* OSC = (World && World->GetGameInstance()) ? Cast<UILLOnlineSessionClient>(World->GetGameInstance()->GetOnlineSession()) : nullptr)
		{
			if (AILLPartyBeaconState* PartyState = OSC->GetPartyBeaconState())
			{
				PartyMemberState = Cast<AILLPartyBeaconMemberState>(PartyState->FindMember(AccountId));
				if (!PartyMemberState)
					PartyMemberState = Cast<AILLPartyBeaconMemberState>(PartyState->FindMember(UniqueId));
			}
		}
	}

	return PartyMemberState;
}

bool AILLPlayerState::HasFullyTraveled(const bool bCanUpdate/* = false*/) const
{
	if (bIsInactive || bFromPreviousLevel || IsPendingKillPending())
		return false;

	if (!bHasFullyTraveled && bCanUpdate)
	{
		check(IsInGameThread());

		UWorld* World = GetWorld();

		// Wait for travel to complete
		if (!World || World->IsInSeamlessTravel())
			return false;

		// Wait for them to acknowledge their PlayerState
		if (!bHasAckedPlayerState)
			return false;

		// Wait for the name to be set
		if (PlayerName.IsEmpty())
			return false;

		if (!bIsABot && GetNetMode() != NM_Standalone)
		{
			// When a lobby beacon is in use, require the LobbyMemberState to be assigned first
			// We require this outside of PIE, which should be the only place no lobby member state is acceptable
			AILLLobbyBeaconMemberState* LMS = GetLobbyMemberState();
			if (!World->IsPlayInEditor() && !LMS)
				return false;

			// Wait for the UniqueId to be set and session registration to complete
			if (UniqueId.IsValid())
			{
				if (!HasRegisteredWithSession())
					return false;
			}
			else if (!World->IsPlayInEditor())
			{
				// We do not care about UniqueId failures for users on other platforms
				if (LMS->GetOnlinePlatformName().IsValid())
				{
					// When using the same online subsystem, we expect the UniqueId to be valid, if not it probably just has not replicated yet
					if (UOnlineEngineInterface::Get()->GetDefaultOnlineSubsystemName() == LMS->GetOnlinePlatformName())
						return false;
				}
				else
				{
					// Has not replicated yet
					return false;
				}
			}
		}

		// Fully traveled!
		// This is gross!
		const_cast<AILLPlayerState*>(this)->PlayerStateFullyTraveled();
	}

	return bHasFullyTraveled;
}

bool AILLPlayerState::HasRegisteredWithSession() const
{
	UWorld* World = GetWorld();

#if WITH_EDITOR
	if (World && World->IsPlayInEditor())
	{
		// Fake registration in PIE
		return true;
	}
#endif

	if (World && UniqueId.IsValid())
	{
		const APlayerState* Default = GetDefault<APlayerState>();
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface(World);
		if (SessionInt.IsValid())
		{
			if (SessionInt->IsPlayerInSession(Default->SessionName, *UniqueId))
			{
				return true;
			}
		}
	}

	check(!bHasRegisteredWithGameSession);
	return false;
}

void AILLPlayerState::PlayerStateFullyTraveled()
{
	UWorld* World = GetWorld();
	bHasFullyTraveled = true;

	// Replicate mute states for this player
	ReplicateMuteStates();

	if (Role == ROLE_Authority)
	{
		// Now that they are registered, attempt to make them host if there is none currently
		if (AILLGameState* GameState = World->GetGameState<AILLGameState>())
		{
			if (!GameState->GetCurrentHost())
			{
				PromoteToHost();
			}
		}
	}

	// Display a localized notification to the local player(s)
	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	if (!OwningController || !OwningController->IsLocalController())
	{
		const UILLPlayerStateLocalMessage::EMessage Message = UILLPlayerStateLocalMessage::EMessage::RegisteredWithGame;
		if (PlayerStateMessageClass)
		{
			for (FLocalPlayerIterator It(GEngine, World); It; ++It)
			{
				if (APlayerController* PC = It->PlayerController)
				{
					if (PC->IsLocalPlayerController())
						PC->ClientReceiveLocalizedMessage(PlayerStateMessageClass, static_cast<uint32>(Message), this);
				}
			}
		}
	}
}

bool AILLPlayerState::CanBeHost() const
{
	return (!bIsABot && HasFullyTraveled());
}

void AILLPlayerState::PromoteToHost()
{
	check(Role == ROLE_Authority);
	if (bIsHost)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s %s"), ANSI_TO_TCHAR(__FUNCTION__), *PlayerName, SafeUniqueIdTChar(UniqueId));

	UWorld* World = GetWorld();
	if (AILLGameState* GameState = World->GetGameState<AILLGameState>())
	{
		// Demote the current host
		if (AILLPlayerState* CurrentHost = GameState->GetCurrentHost())
		{
			CurrentHost->DemoteFromHost();
		}

		// Take host
		bIsHost = true;
		OnRep_bIsHost();
	}
}

void AILLPlayerState::DemoteFromHost()
{
	check(Role == ROLE_Authority);
	if (!bIsHost)
		return;

	UE_LOG(LogOnlineGame, Verbose, TEXT("%s: %s %s"), ANSI_TO_TCHAR(__FUNCTION__), *PlayerName, SafeUniqueIdTChar(UniqueId));

	bIsHost = false;
	OnRep_bIsHost();
}

void AILLPlayerState::OnRep_bIsHost()
{
}

void AILLPlayerState::PlayerSettingsUpdated()
{
	OnPlayerSettingsUpdated.Broadcast(this);

	if (Role == ROLE_Authority)
	{
		// Force a replication update on the server immediately
		ForceNetUpdate();
	}
}

bool AILLPlayerState::IsPlayerTalking() const
{
	if (AILLLobbyBeaconMemberState* LMS = GetLobbyMemberState())
	{
		if (LMS->IsPlayerTalking())
			return true;
	}
	if (AILLPartyBeaconMemberState* PMS = GetPartyMemberState())
	{
		if (PMS->IsPlayerTalking())
			return true;
	}

	return false;
}

bool AILLPlayerState::HasPlayerTalked() const
{
	if (AILLLobbyBeaconMemberState* LMS = GetLobbyMemberState())
	{
		if (LMS->bHasTalked)
			return true;
	}
	if (AILLPartyBeaconMemberState* PMS = GetPartyMemberState())
	{
		if (PMS->bHasTalked)
			return true;
	}

	return false;
}

void AILLPlayerState::ReplicateMuteStates() const
{
	if (UWorld* World = GetWorld())
	{
		for (FLocalPlayerIterator It(GEngine, World); It; ++It)
		{
			if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(*It))
			{
				LocalPlayer->ReplicateMutedState(this);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
