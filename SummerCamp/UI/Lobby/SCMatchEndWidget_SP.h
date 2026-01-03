// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCMenuWidget.h"
#include "SCEmoteData.h"
#include "SCMatchEndWidget_SP.generated.h"

/**
 * @Class USCMatchEndWidget_SP 
 */
UCLASS()
class SUMMERCAMP_API USCMatchEndWidget_SP
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	// Begin UUserWidget interface
	virtual bool Initialize() override;
	// End UUserWidget interface

protected:
	// Cache the emote unlock for getting the score skull
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	USCEmoteData* XPEmoteUnlock;

	// Cache the emote unlock for getting the all kills skull
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	USCEmoteData* KillsEmoteUnlock;

	// Cache the emote unlock for getting the stealth skull
	UPROPERTY(BlueprintReadOnly, Category = "Default")
	USCEmoteData* StealthEmoteUnlock;

	// Called once all the data is loaded.
	UFUNCTION(BlueprintImplementableEvent, Category = "Initialize")
	void DataLoaded();

	// Called if the data fails to load.
	UFUNCTION(BlueprintImplementableEvent, Category = "Initialize")
	void DataLoadFailed();

	/** Return the Texture to display for unlocking the XP emote */
	UFUNCTION(BlueprintPure, Category = "Unlock")
	UTexture2D* GetXPUnlockImg() const { return XPEmoteUnlock ? XPEmoteUnlock->EmoteUnlockImg.LoadSynchronous() : nullptr; }

	/** Return the Texture to display for unlocking the Kills emote */
	UFUNCTION(BlueprintPure, Category = "Unlock")
	UTexture2D* GetKillsUnlockImg() const { return KillsEmoteUnlock ? KillsEmoteUnlock->EmoteUnlockImg.LoadSynchronous() : nullptr; }

	/** Return the Texture to display for unlocking the Stealth emote */
	UFUNCTION(BlueprintPure, Category = "Unlock")
	UTexture2D* GetStealthUnlockImg() const { return StealthEmoteUnlock ? StealthEmoteUnlock->EmoteUnlockImg.LoadSynchronous() : nullptr; }
};
