// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

class ASCActorPreview;
class ASCCounselorCharacter;

class USCEmoteData;

#include "SCMenuWidget.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCCounselorEmoteMenuWidget.generated.h"

/**
 * @struct FSCCounselorEmoteData
 */
USTRUCT(BlueprintType)
struct SUMMERCAMP_API FSCCounselorEmoteData
{
	GENERATED_USTRUCT_BODY()

public:
	FSCCounselorEmoteData() {}

	FSCCounselorEmoteData(USCEmoteData* InEmoteData, bool bInIsLocked)
	: EmoteData(InEmoteData)
	, bIsLocked(bInIsLocked) {}

	UPROPERTY(BlueprintReadOnly, Category = "Defaults")
	USCEmoteData* EmoteData;

	UPROPERTY(BlueprintReadOnly, Category = "Defaults")
	bool bIsLocked;
};

/**
 * @class USCCounselorEmoteWidget
 */
UCLASS()
class SUMMERCAMP_API USCCounselorEmoteMenuWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

public:
	// BEGIN UObject interface
	virtual void PostInitProperties() override;
	// END UObject interface

	// BEGIN UWidget interface
	virtual void RemoveFromParent() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	// END UWidget interface

	// BEGIN UILLUserWidget interface
	virtual bool MenuInputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;
	// END UILLUserWidget interface

	UFUNCTION(BlueprintCallable, Category = "Default")
	TArray<FSCCounselorEmoteData> GetAvailableEmotesForCounselor(TSoftClassPtr<ASCCounselorCharacter> CounselorClass) const;

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SetUnsavedCounselorProfiles(TArray<FSCCounselorCharacterProfile> _CounselorCharacterProfiles) { CounselorCharacterProfiles = _CounselorCharacterProfiles; }

	UFUNCTION(BlueprintPure, Category = "Counselor Character UI")
	TArray<FSCCounselorCharacterProfile> GetUnsavedCounselorProfiles() const { return CounselorCharacterProfiles; }

	UFUNCTION(BlueprintCallable, Category = "Counselor Character UI")
	void SetEmote(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, int32 EmoteIndex, TSubclassOf<USCEmoteData> InEmoteData);

	UFUNCTION(BlueprintPure, Category = "Emote Data")
	UTexture2D* GetEmoteIcon(TSubclassOf<USCEmoteData> EmoteData) const;

	UFUNCTION(BlueprintPure, Category = "Emote Data")
	UAnimMontage* GetEmoteMontage(TSubclassOf<USCEmoteData> EmoteData, TSoftClassPtr<ASCCounselorCharacter> CounselorClass) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	ESlateVisibility GetGamepadVisibility() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenStorePage() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Default")
	bool bShowLockedEmotes;

	UPROPERTY(BlueprintReadWrite, Category = "Input")
	bool bBlockMenuInputAxis;

	UPROPERTY(BlueprintReadWrite, Category = "Preview")
	bool bPreviewMouseDown;

	UPROPERTY(BlueprintReadWrite, Category = "Preveiw")
	class UImage* PreviewImage;

	UPROPERTY(BlueprintReadWrite, Category = "Select")
	class USCCounselorEmoteSelectWidget* EmoteSelect;

private:
	/** Object Library to load all of our emote data at runtime */
	UPROPERTY()
	UObjectLibrary* EmoteObjectLibrary;

	/** Array of Counselor Profiles because we're gross */
	UPROPERTY()
	TArray<FSCCounselorCharacterProfile> CounselorCharacterProfiles;

	/////////////////////////////////////////////////
	// Actor Preview
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	TSoftClassPtr<ASCActorPreview> PreviewClass;

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewCounselorClass(TSoftClassPtr<ASCCounselorCharacter> CounselorClass);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void PlayPreviewEmote(TSubclassOf<USCEmoteData> EmoteClass);

private:
	UPROPERTY()
	ASCActorPreview* ActorPreview;

	float PreviewRelativeRotation;

	FSCCounselorCharacterProfile* GetProfileData(TSoftClassPtr<ASCCounselorCharacter> CounselorClass);
};
