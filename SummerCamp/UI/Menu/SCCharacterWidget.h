// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBackendInventory.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCSettingsMenuWidget.h"

#include "SCCharacterWidget.generated.h"

enum class ECounselorStats : uint8;

/**
 * @class USCCharacterWidget
 */
UCLASS()
class SUMMERCAMP_API USCCharacterWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "Character UI")
	virtual void SaveCharacterProfiles() {}

	UFUNCTION(BlueprintCallable, Category = "Character UI")
	virtual void LoadCharacterProfiles() {}

	UFUNCTION(BlueprintCallable, Category = "Character UI")
	void SetCurrentCharacterClass(const TSoftClassPtr<ASCCharacter> CharacterClass) { CurrentSelectedCharacterClass = CharacterClass; }

	UFUNCTION(BlueprintPure, Category = "Character UI")
	TSoftClassPtr<ASCCharacter> GetCurrentCharacterClass() const { return CurrentSelectedCharacterClass; }

	USCBackendInventory* GetInventoryManager();

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnSaveCharacterProfiles(bool bSucceded);

	UFUNCTION(BlueprintCallable, Category = "Character UI")
	bool AreSettingsDirty();

	UFUNCTION(BlueprintCallable, Category = "Character UI")
	void ClearDirty();

protected:

	/** */
	TSoftClassPtr<ASCCharacter> CurrentSelectedCharacterClass;
};
