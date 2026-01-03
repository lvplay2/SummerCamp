// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCharacterWidget.h"
#include "SCCounselorCharacter.h"
#include "SCCounselorCharacterWidget.generated.h"

class ASCActorPreview;

class USCEmoteData;

/**
 * @class USCCounselorCharacterWidget
 */
UCLASS()
class SUMMERCAMP_API USCCounselorCharacterWidget
: public USCCharacterWidget
{
	GENERATED_UCLASS_BODY()

	// Begin USCCharacterWidget interface
	virtual void SaveCharacterProfiles() override;
	virtual void LoadCharacterProfiles() override;
	// End USCCharacterWidget interface

	virtual void RemoveFromParent() override;

	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	TArray<FSCCounselorCharacterProfile> GetUnsavedCounselorProfile() { return CounselorCharacterProfiles; }

	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	bool IsPerkSlotEmpty(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, int32 PerkIndex) const;

	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	bool IsEmoteSlotEmpty(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, int32 EmoteIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SetUnsavedCounselorProfile(TArray<FSCCounselorCharacterProfile> _CounselorCharacterProfiles);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor Character UI")
	void OnPerkRollPurchased(bool bSuccess, const FSCPerkBackendData& RolledPerk = FSCPerkBackendData());

	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor Character UI")
	void OnPerkRollPurchasedDone(bool bSuccess);

	UFUNCTION()
	void OnPerkRollPurchasedSuccess();

	UFUNCTION()
	void OnSellBackPerkSuccess();

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SellBackPerk(FSCPerkBackendData PerkClass);

	UFUNCTION(BlueprintImplementableEvent, Category = "Counselor Character UI")
	void OnSellBackPerkDone(bool bSuccess);

	/** Gets an array of perks for the passed in counselor. */
	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	TArray<FSCPerkBackendData> GetCounselorCharacterPerks(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter) const;

	// Returns true if the passed in perk is currently equipped on the character.
	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	bool GetCounselorPerkEquipped(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter, const FSCPerkBackendData PerkCompare) const;

	/** Gets the outfit for the passed in counselor */
	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	void GetCounselorCharacterClothes(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter, FSCCounselorOutfit& Outfit) const;

	/** Gets the emotes for the passed in counselor */
	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	TArray<TSubclassOf<USCEmoteData>> GetCounselorCharacterEmotes(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter) const;

	/** Sets the counselor's profile. */
	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SetCounselorCharacterProfile(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter, const TArray<FSCPerkBackendData> Perks, const FSCCounselorOutfit Outfit, const TArray<TSubclassOf<USCEmoteData>> Emotes);

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void PurchasePerkRoll();

	/** Gets the counselor's profile. */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	FSCCounselorCharacterProfile GetCounselorCharacterProfile(const TSoftClassPtr<ASCCounselorCharacter> CounselorCharacter) const;

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SetActorPreviewClass(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, FTransform Offset);

	UPROPERTY(EditDefaultsOnly, Category = "Character UI")
	TSubclassOf<ASCActorPreview> MenuPreviewActorClass;

	UFUNCTION(BlueprintNativeEvent, Category = "Character UI")
	TSoftClassPtr<ASCCounselorCharacter> GetBeautyPoseClass(const TSoftClassPtr<ASCCounselorCharacter>& CounselorClass);

private:

	UFUNCTION()
	void OnPurchasePerkRoll(int32 ResponseCode, const FString& ResponseContents);

	UFUNCTION()
	void OnSellBackPerk(int32 ResponseCode, const FString& ResponseContents);

	void OnLoadCharacterProfilesFromBackend(bool bSucceded);

	/** Array of all the different Counselor Profiles. */
	TArray<FSCCounselorCharacterProfile> CounselorCharacterProfiles;

	/** Stored off Instance ID of the recent Perk that was sold back. */
	UPROPERTY()
	FString CurrentInstanceID;

	UPROPERTY()
	ASCActorPreview* ActorPreview;
};
