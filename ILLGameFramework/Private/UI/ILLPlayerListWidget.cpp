// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerListWidget.h"

#include "ILLGameInstance.h"
#include "ILLGameState.h"
#include "ILLLobbyBeaconMemberState.h"
#include "ILLLobbyBeaconState.h"
#include "ILLOnlineSessionClient.h"
#include "ILLPlayerState.h"
#include "ILLPlayerListItemWidget.h"

UILLPlayerListWidget::UILLPlayerListWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UILLPlayerListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PollForPlayers();
}

void UILLPlayerListWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UILLPlayerListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	PollForPlayers();
}

void UILLPlayerListWidget::SetPlayerContext(const FLocalPlayerContext& InPlayerContext)
{
	Super::SetPlayerContext(InPlayerContext);

	PollForPlayers();
}

void UILLPlayerListWidget::ExchangePlayerList(const TArray<UILLPlayerListItemWidget*>& NewPlayerList)
{
	// Exchange existing entries
	for (int32 Index = 0; Index < NewPlayerList.Num(); ++Index)
	{
		PlayerList[Index]->ExchangeWith(NewPlayerList[Index]);
	}

	// Now take the entire list
	PlayerList = NewPlayerList;
}

UILLPlayerListItemWidget* UILLPlayerListWidget::FindWidgetForMember(AILLLobbyBeaconMemberState* MemberState) const
{
	if (MemberState)
	{
		for (UILLPlayerListItemWidget* PlayerWidget : PlayerList)
		{
			if (IsValid(PlayerWidget))
			{
				if (PlayerWidget->GetLobbyMemberState() == MemberState)
					return PlayerWidget;

				const FILLBackendPlayerIdentifier& WidgetAccountId = PlayerWidget->GetPlayerAccountId();
				if (WidgetAccountId.IsValid() && MemberState->GetAccountId().IsValid())
				{
					if (WidgetAccountId == MemberState->GetAccountId())
						return PlayerWidget;
				}
				else
				{
					FUniqueNetIdRepl WidgetUniqueId = PlayerWidget->GetPlayerUniqueId();
					if (WidgetUniqueId.IsValid() && MemberState->GetMemberUniqueId().IsValid() && *WidgetUniqueId == *MemberState->GetMemberUniqueId())
						return PlayerWidget;
				}
			}
		}
	}

	return nullptr;
}

UILLPlayerListItemWidget* UILLPlayerListWidget::FindWidgetForPlayer(AILLPlayerState* PlayerState) const
{
	if (PlayerState)
	{
		for (UILLPlayerListItemWidget* PlayerWidget : PlayerList)
		{
			if (IsValid(PlayerWidget))
			{
				if (PlayerWidget->GetPlayerState() == PlayerState)
					return PlayerWidget;

				const FILLBackendPlayerIdentifier& WidgetAccountId = PlayerWidget->GetPlayerAccountId();
				if (WidgetAccountId.IsValid() && PlayerState->GetAccountId().IsValid())
				{
					if (WidgetAccountId == PlayerState->GetAccountId())
						return PlayerWidget;
				}
				else
				{
					FUniqueNetIdRepl WidgetUniqueId = PlayerWidget->GetPlayerUniqueId();
					if (WidgetUniqueId.IsValid() && PlayerState->UniqueId.IsValid() && *WidgetUniqueId == *PlayerState->UniqueId)
						return PlayerWidget;
				}
			}
		}
	}

	return nullptr;
}

void UILLPlayerListWidget::LobbyBeaconMemberStateAdded(AILLLobbyBeaconMemberState* MemberState)
{
	check(IsValid(MemberState));
	if (!RegisteredLobbyMembers.Contains(MemberState))
	{
		// Store and register bindings
		RegisteredLobbyMembers.Add(MemberState);
		MemberState->OnDestroyed.AddDynamic(this, &ThisClass::OnLobbyMemberDestroyed);

		// Attempt to update existing widgets first
		SynchronizePlayers();

		// Create a new widget if none is found
		if (!FindWidgetForMember(MemberState))
		{
			UE_LOG(LogILLWidget, Verbose, TEXT("ILLPlayerListWidget::LobbyBeaconMemberStateAdded: Creating widget for %s"), *MemberState->GetName());

			// Spawn a new widget
			UILLPlayerListItemWidget* NewPlayer = NewObject<UILLPlayerListItemWidget>(this, PlayerWidgetClass);
			if (GetPlayerContext().IsValid())
				NewPlayer->SetPlayerContext(GetOwningPlayer());
			NewPlayer->Initialize();
			NewPlayer->AssignLobbyMemberState(MemberState);
			PlayerList.Add(NewPlayer);

			// Notify Blueprint
			AddPlayerWidget(NewPlayer);
		}
	}
}

void UILLPlayerListWidget::LobbyBeaconMemberStateRemoved(AILLLobbyBeaconMemberState* MemberState)
{
	check(MemberState);
	MemberState->OnDestroyed.RemoveAll(this);

	const int32 NumRemoved = RegisteredLobbyMembers.Remove(MemberState);
	if (NumRemoved > 0)
	{
		if (UILLPlayerListItemWidget* PlayerWidget = FindWidgetForMember(MemberState))
		{
			UE_LOG(LogILLWidget, Verbose, TEXT("ILLPlayerListWidget::LobbyBeaconMemberStateRemoved: Removing widget for %s"), *MemberState->GetName());

			RemovePlayerWidget(PlayerWidget);
			PlayerList.Remove(PlayerWidget);
		}

		SynchronizePlayers();
	}
}

void UILLPlayerListWidget::PlayerStateRegistered(AILLPlayerState* PlayerState)
{
	check(IsValid(PlayerState));
	if (!RegisteredPlayerStates.Contains(PlayerState))
	{
		// Store and register bindings
		RegisteredPlayerStates.Add(PlayerState);
		PlayerState->OnDestroyed.AddDynamic(this, &ThisClass::OnPlayerStateDestroyed);

		// Attempt to update existing widgets first
		SynchronizePlayers();

		// Create a new widget if none is found
		if (!FindWidgetForPlayer(PlayerState))
		{
			UE_LOG(LogILLWidget, Verbose, TEXT("ILLPlayerListWidget::PlayerStateRegistered: Creating widget for %s"), *PlayerState->GetName());

			// Spawn a new widget
			UILLPlayerListItemWidget* NewPlayer = NewObject<UILLPlayerListItemWidget>(this, PlayerWidgetClass);
			if (GetPlayerContext().IsValid())
				NewPlayer->SetPlayerContext(GetOwningPlayer());
			NewPlayer->Initialize();
			NewPlayer->AssignPlayerState(PlayerState);
			PlayerList.Add(NewPlayer);

			// Notify Blueprint
			AddPlayerWidget(NewPlayer);
		}
	}
}

void UILLPlayerListWidget::PlayerStateUnregistered(AILLPlayerState* PlayerState)
{
	check(PlayerState);
	PlayerState->OnDestroyed.RemoveAll(this);

	const int32 NumRemoved = RegisteredPlayerStates.Remove(PlayerState);
	if (NumRemoved > 0)
	{
		if (UILLPlayerListItemWidget* PlayerWidget = FindWidgetForPlayer(PlayerState))
		{
			if (PlayerWidget->GetLobbyMemberState())
			{
				// Still associated with the game, keep the widget around
				UE_LOG(LogILLWidget, Verbose, TEXT("ILLPlayerListWidget::PlayerStateUnregistered: Keeping widget for %s"), *PlayerState->GetName());
			}
			else
			{
				// Not associated with the game anymore
				UE_LOG(LogILLWidget, Verbose, TEXT("ILLPlayerListWidget::PlayerStateUnregistered: Removing widget for %s"), *PlayerState->GetName());

				RemovePlayerWidget(PlayerWidget);
				PlayerList.Remove(PlayerWidget);
			}
		}
		else
		{
			UE_LOG(LogILLWidget, Warning, TEXT("ILLPlayerListWidget::PlayerStateUnregistered: Failed to find widget for %s"), *PlayerState->GetName());
		}

		SynchronizePlayers();
	}
}

void UILLPlayerListWidget::OnLobbyMemberDestroyed(AActor* DestroyedMember)
{
	if (AILLLobbyBeaconMemberState* Member = Cast<AILLLobbyBeaconMemberState>(DestroyedMember))
	{
		LobbyBeaconMemberStateRemoved(Member);
	}
}

void UILLPlayerListWidget::OnPlayerStateDestroyed(AActor* DestroyedPlayer)
{
	if (AILLPlayerState* PlayerState = Cast<AILLPlayerState>(DestroyedPlayer))
	{
		PlayerStateUnregistered(PlayerState);
	}
}

void UILLPlayerListWidget::PollForPlayers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		// FIXME: pjackson: Doing this allows all kinds of dumb crashes to occur
	/*	if (UILLGameInstance* GI = Cast<UILLGameInstance>(GetOuter()))
			World = GI->GetWorld();*/
	}
	if (!World)
		return;

	auto IsPlayerStateLocal = [](AILLPlayerState* PS) -> bool
	{
		if (APlayerController* PC = Cast<APlayerController>(PS->GetOwner()))
		{
			return PC->IsLocalController();
		}

		return false;
	};
	auto IsLocalPlayerStateRegistered = [&]() -> bool
	{
		for (AILLPlayerState* PS : RegisteredPlayerStates)
		{
			if (IsPlayerStateLocal(PS))
				return true;
		}
		return false;
	};

	if (UILLGameInstance* GI = Cast<UILLGameInstance>(World->GetGameInstance()))
	{
		if (UILLOnlineSessionClient* OSC = Cast<UILLOnlineSessionClient>(GI->GetOnlineSession()))
		{
			if (OSC->GetLobbyBeaconState() && (!bDeferPlayerStatesForLocal || IsLocalPlayerStateRegistered()))
			{
				for (int32 MemberIndex = 0; MemberIndex < OSC->GetLobbyBeaconState()->GetNumMembers(); ++MemberIndex)
				{
					AILLLobbyBeaconMemberState* MS = Cast<AILLLobbyBeaconMemberState>(OSC->GetLobbyBeaconState()->GetMemberAtIndex(MemberIndex));
					if (IsValid(MS) && !MS->IsPendingKillPending() && MS->bHasSynchronized && !RegisteredLobbyMembers.Contains(MS))
					{
						LobbyBeaconMemberStateAdded(MS);
					}
				}
			}
		}
	}

	if (AILLGameState* GameState = World->GetGameState<AILLGameState>())
	{
		auto CanRegisterPlayerState = [&](AILLPlayerState* PS) -> bool
		{
			if (IsValid(PS) && PS->HasFullyTraveled())
			{
				return !bDeferPlayerStatesForLocal || IsLocalPlayerStateRegistered() || IsPlayerStateLocal(PS);
			}

			return false;
		};

		// Check for any new player states to register
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (AILLPlayerState* PS = Cast<AILLPlayerState>(PlayerState))
			{
				if (!RegisteredPlayerStates.Contains(PS) && CanRegisterPlayerState(PS))
					PlayerStateRegistered(PS);
			}
		}

		// Check if previously-registered player states are valid
		for (int32 Index = 0; Index < RegisteredPlayerStates.Num(); )
		{
			AILLPlayerState* PS = RegisteredPlayerStates[Index];
			if (CanRegisterPlayerState(PS))
			{
				++Index;
				continue;
			}

			if (PS)
			{
				PlayerStateUnregistered(PS);
			}
			else
			{
				++Index;
			}
		}
	}
	else
	{
		// No world or game state, flush the list
		for (int32 Index = 0; Index < RegisteredPlayerStates.Num(); )
		{
			AILLPlayerState* PS = RegisteredPlayerStates[Index];
			if (PS)
			{
				PlayerStateUnregistered(PS);
			}
			else
			{
				++Index;
			}
		}
		RegisteredPlayerStates.Empty();
	}
}

void UILLPlayerListWidget::SynchronizePlayers()
{
	// I don't think widgets can be taken out from under us, but just in case...
	for (int32 Index = 0; Index < PlayerList.Num(); )
	{
		UILLPlayerListItemWidget* PlayerWidget = PlayerList[Index];
		if (IsValid(PlayerWidget))
		{
			PlayerWidget->AttemptToSynchronize();
			++Index;
		}
		else
		{
			PlayerList.RemoveAtSwap(Index);
		}
	}
}
