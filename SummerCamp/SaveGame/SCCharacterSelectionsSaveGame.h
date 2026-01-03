// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLPlayerSettingsSaveGame.h"

#include "SCClothingData.h"
#include "SCContextKillActor.h"
#include "SCCounselorCharacter.h"
#include "SCEmoteData.h"
#include "SCJasonSkin.h"
#include "SCKillerCharacter.h"
#include "SCMaterialOverride.h"
#include "SCPerkData.h"
#include "SCWeapon.h"

#include "SCCharacterSelectionsSaveGame.generated.h"

/**
 * @struct FSCCounselorCharacterProfile
 */
USTRUCT(BlueprintType)
struct FSCCounselorCharacterProfile
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSoftClassPtr<ASCCounselorCharacter> CounselorCharacterClass;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSCPerkBackendData> Perks;

	UPROPERTY(BlueprintReadOnly)
	FSCCounselorOutfit Outfit;

	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<USCEmoteData>> Emotes;

	FSCCounselorCharacterProfile()
		: CounselorCharacterClass(nullptr)
		, Perks()
		, Outfit()
	{
	}

	FSCCounselorCharacterProfile(TSoftClassPtr<ASCCounselorCharacter> InCounselorCharacterClass, TArray<FSCPerkBackendData> InPerks, FSCCounselorOutfit InOutfit, TArray<TSubclassOf<USCEmoteData>> InEmotes)
	: CounselorCharacterClass(InCounselorCharacterClass)
	, Perks(InPerks)
	, Outfit(InOutfit)
	, Emotes(InEmotes) {}

	FORCEINLINE bool operator ==(const FSCCounselorCharacterProfile& Other) const
	{
		return (CounselorCharacterClass == Other.CounselorCharacterClass && Perks == Other.Perks && Outfit == Other.Outfit && Emotes == Other.Emotes);
	}
};

/**
 * @struct FSCKillerCharacterProfile
 */
USTRUCT(BlueprintType)
struct FSCKillerCharacterProfile
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass;

	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<ASCContextKillActor>> GrabKills;

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<USCJasonSkin> SelectedSkin;


	// Locally stored array for user's ability-unlocking custom ordering.
	TArray<EKillerAbilities> AbilityUnlockOrder;


	UPROPERTY(BlueprintReadOnly)
	TSoftClassPtr<ASCWeapon> SelectedWeapon;

	FSCKillerCharacterProfile()
	: KillerCharacterClass(nullptr)
	, GrabKills()
	, SelectedWeapon(nullptr)
	{}

	FSCKillerCharacterProfile(TSoftClassPtr<ASCKillerCharacter> InKillerCharacterClass, TArray<TSubclassOf<ASCContextKillActor>> InGrabKills)
	: KillerCharacterClass(InKillerCharacterClass)
	, GrabKills(InGrabKills) {}

	FORCEINLINE bool operator ==(const FSCKillerCharacterProfile& Other) const
	{
		return (KillerCharacterClass == Other.KillerCharacterClass && GrabKills == Other.GrabKills && SelectedSkin == Other.SelectedSkin && SelectedWeapon == Other.SelectedWeapon);
	}
};

/**
 * @class USCCharacterSelectionsSaveGame
 */
UCLASS()
class USCCharacterSelectionsSaveGame
: public UILLPlayerSettingsSaveGame
{
	GENERATED_UCLASS_BODY()

	// Begin UILLPlayerSettingsSaveGame interface
	virtual void ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave = true) override;
	virtual bool HasChangedFrom(UILLPlayerSettingsSaveGame* OtherSave) const override;
	virtual void SaveGameLoaded(AILLPlayerController* PlayerController) override;
	// End UILLPlayerSettingsSaveGame interface

	/** Called to verify that we own our CounselorPick and KillerPick. */
	void VerifySettingOwnership(AILLPlayerController* PlayerController, const bool bAndSave);
	
	//////////////////////////////////////////////////
	// Counselor
public:
	/** @return Profile for CounselorClass. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const FSCCounselorCharacterProfile& GetCounselorProfile(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const;

	/** @return Perks for CounselorClass. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const TArray<FSCPerkBackendData>& GetCounselorPerks(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const;

	/** @return Emotes for CounselorClass. */
	UFUNCTION(BlueprintPure, Category = "PlayerSettings|Game")
	const TArray<TSubclassOf<USCEmoteData>>& GetCounselorEmotes(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const;

	/** @return Outfit for CounselorClass. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const FSCCounselorOutfit& GetCounselorOutfit(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass) const;

	/** @return Profile for CounselorPick. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const FSCCounselorCharacterProfile& GetPickedCounselorProfile() const { return GetCounselorProfile(TSoftClassPtr<ASCCounselorCharacter>(CounselorPick)); }

	/** @return Perks for CounselorPick. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const TArray<FSCPerkBackendData>& GetPickedCounselorPerks() const { return GetCounselorPerks(TSoftClassPtr<ASCCounselorCharacter>(CounselorPick)); }

	/** @return Outfit for CounselorPick. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const FSCCounselorOutfit& GetPickedCounselorOutfit() const { return GetCounselorOutfit(TSoftClassPtr<ASCCounselorCharacter>(CounselorPick)); }

	/** @return Emotes for CounselorPick. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const TArray<TSubclassOf<USCEmoteData>>& GetPickedCounselorEmotes() const { return GetCounselorEmotes(TSoftClassPtr<ASCCounselorCharacter>(CounselorPick)); }

	// Last picked counselor
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Game")
	TSubclassOf<ASCCounselorCharacter> CounselorPick;

	// Counselor character profiles
	UPROPERTY()
	TArray<FSCCounselorCharacterProfile> CounselorCharacterProfiles;

	//////////////////////////////////////////////////
	// Killer
public:
	/** @return Profile for KillerClass. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const FSCKillerCharacterProfile& GetKillerProfile(const TSoftClassPtr<ASCKillerCharacter>& KillerClass) const;

	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const TArray<TSubclassOf<ASCContextKillActor>>& GetKillerGrabKills(const TSoftClassPtr<ASCKillerCharacter>& KillerClass) const;

	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	TSoftClassPtr<ASCWeapon> GetKillerSelectedWeapon(const TSoftClassPtr<ASCKillerCharacter>& KillerClass) const;

	/** @return Profile for KillerPick. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const FSCKillerCharacterProfile& GetPickedKillerProfile() const { return GetKillerProfile(TSoftClassPtr<ASCKillerCharacter>(KillerPick)); }

	/** @return Profile for KillerPick. */
	UFUNCTION(BlueprintPure, Category="PlayerSettings|Game")
	const TArray<TSubclassOf<ASCContextKillActor>>& GetPickedKillerGrabKills() const { return GetKillerGrabKills(TSoftClassPtr<ASCKillerCharacter>(KillerPick)); }

	// Last picked killer
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="PlayerSettings|Game")
	TSubclassOf<ASCKillerCharacter> KillerPick;

	// Killer character profiles
	UPROPERTY()
	TArray<FSCKillerCharacterProfile> KillerCharacterProfiles;
};
