// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCUserWidget.h"
#include "SCErrorDialogWidget.h"
#include "SCOnlineSessionClient.h"
#include "SCQuickPlayOverlayWidget.generated.h"

/**
 * @class USCQuickPlayOverlayWidget
 */
UCLASS(Abstract)
class SUMMERCAMP_API USCQuickPlayOverlayWidget
: public USCUserWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** Stops matchmaking if currently running. Automatically closed when RemoveFromParent is hit, so no need to do that when this is destroyed. */
	UFUNCTION(BlueprintCallable, Category = "QuickPlay")
	void StopMatchmaking();
};
