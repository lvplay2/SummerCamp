// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once
#include "SCMenuWidget.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCCounselorWardrobeWidget.generated.h"

class ASCActorPreview;

/**
* @class USCCounselorWardrobeWidget
*/
UCLASS()
class SUMMERCAMP_API USCCounselorWardrobeWidget
	: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual bool MenuInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;

	virtual void RemoveFromParent() override;
	/** Returns the USCBackendInventory object from the LocalPlayer. */
	UFUNCTION()
	USCBackendInventory* GetInventoryManager() const;

	/** Saves clothing options out to character profile. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	virtual void SaveCharacterProfiles();

	/** Loads clothing options from current profile save. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	virtual void LoadCharacterProfiles();

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SetUnsavedCounselorProfile(TArray<FSCCounselorCharacterProfile> _CounselorCharacterProfiles) { CounselorCharacterProfiles = _CounselorCharacterProfiles; }

	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	TArray<FSCCounselorCharacterProfile> GetUnsavedCounselorProfile() { return CounselorCharacterProfiles; }

	FSCCounselorCharacterProfile* GetProfileData();

	/** Returns if the BackendInventory has been modified and should be saved. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool AreSettingsDirty() const;

	/** Clears the dirty flag on the BackendInventory object for the LocalPlayer. */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	void ClearDirty();

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewClass(TSoftClassPtr<ASCCounselorCharacter> CharacterClass);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewOutfit(TSubclassOf<USCClothingData> OutfitClass, bool bFromClick);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewSwatch(int32 SlotIndex, TSubclassOf<USCClothingSlotMaterialOption> Swatch);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	TSubclassOf<ASCActorPreview> PreviewClass;

	UPROPERTY(BlueprintReadOnly, Category = "Preview")
	TSoftClassPtr<ASCCounselorCharacter> CounselorClass;

	UFUNCTION(BlueprintPure, Category = "UI")
	ESlateVisibility GetGamepadVisibility() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FSCClothingMaterialPair GetSelectedSwatchFromSave(int32 SwatchSlot) const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenStorePage() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RandomizeOutfit();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool AllowedToSetOutfit() const { return OutfitSelectedTimer <= 0.0f; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Preview")
	FString PreviewClassFirstName;

	UPROPERTY(BlueprintReadOnly, Category = "Preview")
	FString PreviewClassLastName;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class USCOutfitListWidget* OutfitListWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Preview")
	bool bPreviewMouseDown;

	UPROPERTY(BlueprintReadWrite, Category = "Preveiw")
	class UImage* PreviewImage;

private:
	/** Array of all the different Counselor Profiles. */
	UPROPERTY()
	TArray<FSCCounselorCharacterProfile> CounselorCharacterProfiles;

	UPROPERTY()
	ASCActorPreview* ActorPreview;

	UPROPERTY()
	float ControllerInputDelta;

	UPROPERTY()
	float OutfitSelectedTimer;
};