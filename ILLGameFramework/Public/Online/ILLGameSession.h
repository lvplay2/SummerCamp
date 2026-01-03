// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/GameSession.h"
#include "ILLGameSession.generated.h"

class FILLGameSessionSettings;

/**
 * @struct FILLPlayerSessionRegistration
 */
USTRUCT()
struct FILLPlayerSessionRegistration
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	APlayerController* Player;

	FUniqueNetIdRepl UniqueId;

	bool bFromInvite;
};

/**
 * @class AILLGameSession
 * @brief This is a decently modified version of AShooterGameSession.
 */
UCLASS(Config=Game)
class ILLGAMEFRAMEWORK_API AILLGameSession
: public AGameSession
{
	GENERATED_UCLASS_BODY()
		
	// Begin AGameSession interface
	virtual void InitOptions(const FString& Options) override;
	virtual FString ApproveLogin(const FString& Options) override;
	virtual void RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite) override;
	virtual void UnregisterPlayer(FName InSessionName, const FUniqueNetIdRepl& UniqueId) override;
	virtual bool AtCapacity(bool bSpectator) override;
	virtual bool KickPlayer(APlayerController* KickedPlayer, const FText& KickReason) override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	// End AGameSession interface

	//////////////////////////////////////////////////
	// Game session
public:
	/** Calls RegisterPlayer on PlayersPendingRegistration, then flushes it. */
	virtual void FlushPlayersPendingRegistration();

	/** @return String of additional travel options for server map changes. */
	virtual FString GetPersistentTravelOptions() const;

	/** @return true if the game session is ready for the match to start too. */
	virtual bool IsReadyToStartMatch();

	/** Called from AILLGameMode::HandleLeavingMap. */
	virtual void HandleLeavingMap();

	/** @return Game session host settings. */
	virtual TSharedPtr<FILLGameSessionSettings> GenerateHostSettings() const;

	/** @return Maximum number of players allowed in this game session. */
	FORCEINLINE int32 GetMaxPlayers() const { return (UpperPlayerLimit > 0) ? FMath::Min(MaxPlayers, UpperPlayerLimit) : MaxPlayers; }

	/** @return true if this is a LAN match. */
	FORCEINLINE bool IsLANMatch() const { return (bIsLanMatch || SafeHasWorldOption(TEXT("bIsLanMatch"))); }

	/** @return true if this is a listen session. */
	FORCEINLINE bool IsListenSession() const { return ((GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer) || SafeHasWorldOption(TEXT("listen"))); }

	/** @return If game server is passworded */
	FORCEINLINE bool IsPassworded() const { return !ServerPassword.IsEmpty(); }

	/** @return true if this is a private match. */
	FORCEINLINE bool IsPrivateMatch() const { return (bIsPrivateMatch || SafeHasWorldOption(TEXT("bIsPrivateMatch"))); }

	/** @return true if this is a QuickPlay match. */
	FORCEINLINE bool IsQuickPlayMatch() const { return (IsListenSession() || IsRunningDedicatedServer()) && !IsPrivateMatch() && !IsPassworded(); }

protected:
	/** Helper function for IsLANMatch etc. */
	FORCEINLINE bool SafeHasWorldOption(const TCHAR* Option) const
	{
		if (UWorld* World = GetWorld())
		{
			return World->URL.HasOption(Option);
		}

		return false;
	}

	// Players pending game session registration
	UPROPERTY()
	TArray<FILLPlayerSessionRegistration> PlayersPendingRegistration;

	// Password required to connect to this server
	UPROPERTY(Config)
	FString ServerPassword;

	// Upper-most player count limit to throttle Super::MaxPlayers
	int32 UpperPlayerLimit;

	// Was this session initialized for LAN use?
	bool bIsLanMatch;

	// Was this session not meant to advertise?
	bool bIsPrivateMatch;
};
