// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameDelegates.h"
#include "GameFramework/SaveGame.h"
#include "ILLPlayerSettingsSaveGame.generated.h"

class AILLPlayerController;
class UILLLocalPlayer;

/**
 * @class EILLPlayerSettingsSaveType
 * @brief User will have the same value as Platform on platforms that do not need the separation.
 */
enum class EILLPlayerSettingsSaveType : uint8
{
	Platform,
#if PLATFORM_DESKTOP
	User,
#else
	User = Platform,
#endif
	MAX
};

/**
 * @struct FILLPlayerSettingsSaveGameEntry
 */
struct FILLPlayerSettingsSaveGameEntry
{
	// Class name of this SaveGame
	FString SaveClassName;

	// Serialized object data
	TArray<uint8> ObjectBytes;

	FILLPlayerSettingsSaveGameEntry() {}

	friend FArchive& operator<<(FArchive& Ar, FILLPlayerSettingsSaveGameEntry& E)
	{
		Ar << E.SaveClassName;
		Ar << E.ObjectBytes;
		return Ar;
	}
};

/**
 * @class FILLPlayerSettingsSaveGameRunnable
 */
class ILLGAMEFRAMEWORK_API FILLPlayerSettingsSaveGameRunnable
: public FRunnable
{
	FILLPlayerSettingsSaveGameRunnable();

public:
	FILLPlayerSettingsSaveGameRunnable(UILLLocalPlayer* InLocalPlayer, EILLPlayerSettingsSaveType InSaveGameType);
	virtual ~FILLPlayerSettingsSaveGameRunnable();

	/** @return Save game type of this read/write runnable. */
	FORCEINLINE EILLPlayerSettingsSaveType GetSaveGameType() const { return SaveGameType; }

	/** @return true if this runnable has completed it's work. */
	FORCEINLINE bool HasCompletedAsyncWork() const { return bAsyncCompleted; }

	/** @return true if this is a runnable for loading a save game. */
	virtual bool IsLoadRunnable() const = 0;

	/** Spawns a thread to run this. */
	virtual void StartWork(const int32 ThreadNumber);

	/** Called on the game thread after bAsyncCompleted turns true. */
	virtual void FinishWork();

protected:
	// Player this is for
	UILLLocalPlayer* LocalPlayer;

	// Save game type this is for
	EILLPlayerSettingsSaveType SaveGameType;

	// Save game thread
	FRunnableThread* SaveGameThread;

	// Save slot name
	FString SaveSlotName;

	// Are we done here?
	bool bAsyncCompleted;
};

/**
 * @class FILLPlayerLoadSaveGameRunnable
 */
class ILLGAMEFRAMEWORK_API FILLPlayerLoadSaveGameRunnable
: public FILLPlayerSettingsSaveGameRunnable
{
public:
	typedef FILLPlayerSettingsSaveGameRunnable Super;

	FILLPlayerLoadSaveGameRunnable(UILLLocalPlayer* InLocalPlayer, EILLPlayerSettingsSaveType InSaveGameType)
	: FILLPlayerSettingsSaveGameRunnable(InLocalPlayer, InSaveGameType)
	, bNeedsReSave(false)
	{
	}

	virtual ~FILLPlayerLoadSaveGameRunnable() {}

	// Begin FRunnable interface
	virtual uint32 Run() override;
	// End FRunnable interface

	// Begin FILLPlayerSettingsSaveGameRunnable interface
	virtual bool IsLoadRunnable() const override { return true; }
	virtual void FinishWork() override;
	// End FILLPlayerSettingsSaveGameRunnable interface

protected:
	// Loaded save games
	TArray<FILLPlayerSettingsSaveGameEntry> LoadedSaveEntries;

	// Should we queue a re-save at the end of FinishWork?
	bool bNeedsReSave;
};

/**
 * @class FILLPlayerWriteSaveGameRunnable
 */
class ILLGAMEFRAMEWORK_API FILLPlayerWriteSaveGameRunnable
: public FILLPlayerSettingsSaveGameRunnable
{
public:
	typedef FILLPlayerSettingsSaveGameRunnable Super;

	FILLPlayerWriteSaveGameRunnable(UILLLocalPlayer* InLocalPlayer, EILLPlayerSettingsSaveType InSaveGameType)
	: FILLPlayerSettingsSaveGameRunnable(InLocalPlayer, InSaveGameType)
	{
	}

	virtual ~FILLPlayerWriteSaveGameRunnable() {}

	// Begin FRunnable interface
	virtual uint32 Run() override;
	// End FRunnable interface

	// Begin FILLPlayerSettingsSaveGameRunnable interface
	virtual bool IsLoadRunnable() const override { return false; }
	virtual void StartWork(const int32 ThreadNumber) override;
	// End FILLPlayerSettingsSaveGameRunnable interface

protected:
	// Save games to be written
	TArray<FILLPlayerSettingsSaveGameEntry> WrittenSaveEntries;
};

/**
 * @class UILLPlayerSettingsSaveGame
 * @brief Save game for player specific settings. Automatically loaded and applied when an ILLLocalPlayer logs into their platform.
 */
UCLASS(Abstract, Within=ILLLocalPlayer)
class ILLGAMEFRAMEWORK_API UILLPlayerSettingsSaveGame
: public USaveGame
{
	GENERATED_UCLASS_BODY()

	/**
	 * Applies the current settings in this save game to the system. Override this in your sub-class!
	 * This should typically not perform any delta checks, applying all settings to the system at once even if they did not change.
	 *
	 * @param PlayerController Player these settings pertain to.
	 * @param bAndSave Save to the save game slot too? Will start or queue an asynchronous save.
	 */
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true); // TODO: pjackson: Remove PlayerController and just go through GetOuterUILLLocalPlayer

	/** @return true if this save game has settings that differ from OtherSave. Override this in your sub-class! */
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const { return false; }

	/** Notification that this save game was just loaded for our player. */
	virtual void SaveGameLoaded(AILLPlayerController* PlayerController); // TODO: pjackson: Remove PlayerController and just go through GetOuterUILLLocalPlayer

	/** Notification that this save game was just newly created for a player. */
	virtual void SaveGameNewlyCreated(AILLPlayerController* PlayerController); // TODO: pjackson: Remove PlayerController and just go through GetOuterUILLLocalPlayer

	/** @return Duplicate of this instance, typically for UI for modification. */
	virtual UILLPlayerSettingsSaveGame* SpawnDuplicate() const;

	/**
	 * Used to verify the local player owns backend-oriented settings within this save. Perform backend queries for ownership from here.
	 * Called after save game loading completes and backend authentication completes, or when the user opted into offline mode.
	 * @see UILLLocalPlayer::ConditionalVerifyBackendSettingsOwnership.
	 *
	 * @param bOfflineMode Is the local player in offline mode?
	 */
	virtual void VerifyBackendSettingsOwnership(const bool bOfflineMode) {}

	/**
	 * Used to verify the local player owns entitlement-oriented settings within this save.
	 * Called after save game loading completes and entitlement fetching has completed.
	 * @see UILLLocalPlayer::ConditionalVerifyEntitlementSettingsOwnership.
	 */
	virtual void VerifyEntitlementSettingOwnership() {}

	// Type of player settings save this is
	EILLPlayerSettingsSaveType SaveType;
};
