// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/Menu/SCMenuWidget.h"
#include "SCPamelaTapesWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCPamelaTapesWidget : public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "Pamela Tapes")
	bool IsPamelaTapeUnlocked(TSubclassOf<class USCPamelaTapeDataAsset> PamelaTape);

	UFUNCTION(BlueprintCallable, Category = "Tommy Tapes")
	bool IsTommyTapeUnlocked(TSubclassOf<class USCTommyTapeDataAsset> TommyTape);
};
