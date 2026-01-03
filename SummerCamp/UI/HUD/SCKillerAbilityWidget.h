// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/SCUserWidget.h"
#include "SCKillerCharacter.h"
#include "SCKillerAbilityWidget.generated.h"

/**
* @class USCKillerAbilityWidget
*/
UCLASS()
class SUMMERCAMP_API USCKillerAbilityWidget
	: public USCUserWidget
{
	GENERATED_BODY()

public:
	USCKillerAbilityWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Color")
	FLinearColor GetStalkIconColor() const;

	UFUNCTION(BlueprintPure, Category = "Color")
	FLinearColor GetSenseIconColor() const;

	UFUNCTION(BlueprintPure, Category = "Color")
	FLinearColor GetMorphIconColor() const;

	UFUNCTION(BlueprintPure, Category = "Color")
	FLinearColor GetShiftIconColor() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetStalkGlowVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetSenseGlowVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetMorphGlowVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetShiftGlowVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetStalkActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetSenseActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetMorphActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetShiftActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetPowersActionMapVisibility() const;

	UFUNCTION(BlueprintPure, Category = "Visibility")
	ESlateVisibility GetWidgetVisibility() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilityUnlock")
	void OnAbilityUnlocked(EKillerAbilities UnlockedAbility);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilityAvailable")
	void OnAbilityAvailable(EKillerAbilities AvailableAbility);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilitiesAvailable")
	void OnAbilityUsed(EKillerAbilities UsedAbility);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilitiesAvailable")
	void OnAbilityActive(EKillerAbilities UsedAbility);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilitiesAvailable")
	void OnAbilityEnded(EKillerAbilities UsedAbility);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "AbilityValue")
	float MorphAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityValue")
	float SenseAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityValue")
	float ShiftAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityValue")
	float StalkAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityUnlock")
	float SenseUnlockAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityUnlock")
	float ShiftUnlockAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityUnlock")
	float StalkUnlockAmount;

	UPROPERTY(BlueprintReadOnly, Category = "AbilityUnlock")
	float MorphUnlockAmount;

	UPROPERTY(EditDefaultsOnly, Category = "Color")
	FLinearColor IconUnusableColor;

	UPROPERTY(EditDefaultsOnly, Category = "Color")
	FLinearColor IconUsableColor;

	UPROPERTY(EditDefaultsOnly, Category = "Color")
	FLinearColor IconLockedColor;

	FLinearColor GetAbilityIconColor(EKillerAbilities Ability) const;

	bool AvailableAbilities[(uint8)EKillerAbilities::MAX];
	bool ActiveAbilities[(uint8)EKillerAbilities::MAX];
};
