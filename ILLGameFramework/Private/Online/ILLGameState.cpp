// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLGameState.h"

#include "Online.h"

#include "ILLCharacter.h"
#include "ILLGameInstance.h"
#include "ILLGameStateLocalMessage.h"
#include "ILLPlayerState.h"

AILLGameState::AILLGameState(const FObjectInitializer& ObjectInitializer) 
: Super(ObjectInitializer)
{
	NetPriority = 1.25f;

	bDeferBeginPlay = true;
}

void AILLGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AILLGameState, bPendingInstanceRecycle);
}

void AILLGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UWorld* World = GetWorld())
	{
		// Seed the RNG so we get better random
		FMath::RandInit(FPlatformTime::Cycles());
		FMath::SRandInit(FPlatformTime::Cycles());

		// Broadcast events for AILLCharacters that may have replicated before we did
		for (TActorIterator<AILLCharacter> It(World); It; ++It)
		{
			CharacterInitialized(*It);
		}
	}
}

void AILLGameState::HandleMatchIsWaitingToStart()
{
	if (bDeferBeginPlay)
	{
		// Do NOT call the Super function. We do not want BeginPlay firing immediately. @see AILLGameMode::HandleMatchIsWaitingToStart.
	}
	else
	{
		Super::HandleMatchIsWaitingToStart();
	}
}

void AILLGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (Role != ROLE_Authority && bDeferBeginPlay)
	{
		// Server handles this in AGameMode::HandleMatchHasStarted, and because of bDeferBeginPlay we need to do it here! @see AILLGameMode::HandleMatchIsWaitingToStart.
		GetWorldSettings()->NotifyBeginPlay();
	}
}

void AILLGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	if (Role == ROLE_Authority)
	{
		// Force a replication update immediately
		ForceNetUpdate();
	}

	// Display a notification if desired
	UWorld* World = GetWorld();
	if (GameStateMessageClass && World)
	{
		const UILLGameStateLocalMessage* const DefaultMessage = GameStateMessageClass->GetDefaultObject<UILLGameStateLocalMessage>();
		for (FLocalPlayerIterator It(GEngine, World); It; ++It)
		{
			if (APlayerController* PC = It->PlayerController)
			{
				PC->ClientReceiveLocalizedMessage(GameStateMessageClass, DefaultMessage->SwitchForMatchState(MatchState), nullptr, nullptr, this);
			}
		}
	}
}

void AILLGameState::HandlePendingInstanceRecycle()
{
	check(Role == ROLE_Authority);
	if (!bPendingInstanceRecycle)
	{
		bPendingInstanceRecycle = true;
		OnRep_bPendingInstanceRecycle();
	}
}

void AILLGameState::OnRep_bPendingInstanceRecycle()
{
}

bool AILLGameState::IsMatchWaitingToStart() const
{
	return (GetMatchState() == MatchState::WaitingToStart);
}

bool AILLGameState::IsMatchWaitingPostMatch() const
{
	return (GetMatchState() == MatchState::WaitingPostMatch);
}

bool AILLGameState::IsUsingMultiplayerFeatures() const
{
	return (HasMatchStarted() && !HasMatchEnded());
}

AILLPlayerState* AILLGameState::GetCurrentHost() const
{
	for (APlayerState* Player : PlayerArray)
	{
		if (AILLPlayerState* PS = Cast<AILLPlayerState>(Player))
		{
			if (PS->IsHost())
			{
				return PS;
			}
		}
	}

	return nullptr;
}

void AILLGameState::CharacterInitialized(AILLCharacter* Character)
{
	OnCharacterInitialized.Broadcast(Character);
}

void AILLGameState::CharacterDestroyed(AILLCharacter* Character)
{
	OnCharacterDestroyed.Broadcast(Character);
}
