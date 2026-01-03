// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCCharacterWidget.h"
#include "SCStengthOrWeaknessDataAsset.h"
#include "SCKillerCharacterWidget.generated.h"

class ASCActorPreview;
class ASCContextKillActor;
class ASCKillerCharacter;

class USCJasonSkin;

/**
 * @class USCKillerCharacterWidget
 */
UCLASS()
class SUMMERCAMP_API USCKillerCharacterWidget
: public USCCharacterWidget
{
	GENERATED_UCLASS_BODY()

	// Begin USCCharacterWidget interface
	virtual void SaveCharacterProfiles() override;
	virtual void LoadCharacterProfiles() override;
	// End USCCharacterWidget interface

	virtual void RemoveFromParent() override;

	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	TArray<FSCKillerCharacterProfile> GetUnsavedKillerProfile() const { return KillerCharacterProfiles; }

	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void SetUnsavedKillerProfile(TArray<FSCKillerCharacterProfile> _KillerCharacterProfiles) { KillerCharacterProfiles = _KillerCharacterProfiles; }

	/** Gets the weapon details for this killer. */
	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	void GetKillerWeaponDetails(TSoftClassPtr<ASCWeapon> KillerWeaponClass, FText& WeaponName, UTexture2D*& WeaponImage) const;

	/** Returns if this grab kill requires a weapon and it matches jasons weapon */
	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	bool GetGrabKillWeaponMatch(TSubclassOf<ASCContextKillActor> GrabKillClass, TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

	/** Checks to see if the passed in kill needs to be bought by the player. */
	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	bool DoesKillNeedBought(TSubclassOf<ASCContextKillActor> KillClass);

	/** Purchases the passed in kill from the backend for the current player. */
	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void PurchaseKill(TSubclassOf<ASCContextKillActor> KillClass);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnJasonKillsPurchased(bool bFailed);

	UFUNCTION()
	void OnJasonKillsPurchasedSuccess();

	/** Sets the killer's profile. */
	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void SetKillerCharacterProfile(const TSoftClassPtr<ASCKillerCharacter> KillerCharacter, const TSubclassOf<ASCContextKillActor> GrabKill, const int32 index);

	/** Gets the killer's profile. */
	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	FSCKillerCharacterProfile GetKillerCharacterProfile(const TSoftClassPtr<ASCKillerCharacter> KillerCharacter) const;

	/** Default amount of grab kills this character can have set. */
	UPROPERTY(EditDefaultsOnly, Category = "Killer Character UI")
	int32 AmountOfGrabKills;

	/** Sets the wanted skin for the provided Killer class in the killers profile. */
	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void SetKillerSkinProfile(const TSoftClassPtr<ASCKillerCharacter> KillerCharacter, const TSubclassOf<USCJasonSkin> SelectedSkin);

	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void SetActorPreviewClass(TSoftClassPtr<ASCKillerCharacter> KillerClass, FTransform Offset);

	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	ASCKillerCharacter* GetPreviewKillerActor() const;

	UPROPERTY(EditDefaultsOnly, Category = "Killer Character UI")
	TSubclassOf<ASCActorPreview> MenuPreviewActorClass;

	UFUNCTION(BlueprintNativeEvent, Category = "Killer Character UI")
	TSoftClassPtr<ASCKillerCharacter> GetBeautyPoseClass(const TSoftClassPtr<ASCKillerCharacter>& KillerClass);

	/** Checks whether the user is able to use the custom ability unlock feature. */
	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	ESlateVisibility GetAbilityUnlockButtonVisibility() const;

	/** Once all abilities have been selected, disable further selection until they back out, or hit reset. */
	UFUNCTION(BlueprintPure, Category = "Killer Character UI")
	bool AllowAbilitySelection() const { return CustomUnlockOrderIndex < (uint8)EKillerAbilities::MAX;	}

	/** Resets Ability order selection data. */
	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void ResetCurrentAbilitySelectionIndex();

	/** Stores the SelectedAbility in the current selection order index. (See uint8 CustomUnlockOrderIndex) */
	UFUNCTION(BlueprintCallable, Category = "Killer Character UI")
	void OnAbilityUnlockSelected(EKillerAbilities SelectedAbility, TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

protected:
	// Intermediate variable for tracking the user's custom ability unlock order index. 
	UPROPERTY(BlueprintReadOnly)
	uint8 CustomUnlockOrderIndex;

private:
	/** Array of all the different Killer Profiles. */
	TArray<FSCKillerCharacterProfile> KillerCharacterProfiles;

	/** Event that is fired when the backend gets back to us about purchasing the requested kill. */
	UFUNCTION()
	void OnPurchaseKill(int32 ResponseCode, const FString& ResponseContents);

	UPROPERTY()
	class ASCActorPreview* ActorPreview;
};
