// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLRadialSelectWidget.h"

#include "SCCharacterSelectionsSaveGame.h"

#include "SCCounselorEmoteSelectWidget.generated.h"

class ASCCounselorCharacter;

class USCEmoteData;

/**
 * @class USCCounselorEmoteSelectWidget
 */
UCLASS()
class SUMMERCAMP_API USCCounselorEmoteSelectWidget
: public UILLRadialSelectWidget
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UILLUserWidget interface
	virtual bool Initialize() override;
	// End UILLUserWidget interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defaults")
	bool bIsInGame;

	UPROPERTY(BlueprintReadWrite, Category = "Defaults")
	TSoftClassPtr<ASCCounselorCharacter> CurrentCounselorClass;

	UPROPERTY(BlueprintReadWrite, Category = "Defaults")
	USCEmoteData* PendingEmote;

	/** Sets the emote data at a specific index */
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void UpdateEmote(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Highlight")
	void EnableHighlight(const bool bInShowHighlight) { bShowHighlight = bInShowHighlight; }

protected:
	/** Plays the selected emote */
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void PlayEmote(int32 Index);

	/** Updates UI for a recently changed emote */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void OnUpdateEmote(int32 Index, USCEmoteData* InEmote);

	/** Blueprint event to handle initialization when widget is created from code */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void OnInitialize();

	UFUNCTION(BlueprintCallable, Category = "Defaults")
	UTexture2D* GetEmoteIcon(TSubclassOf<USCEmoteData> EmoteData) const;
};
