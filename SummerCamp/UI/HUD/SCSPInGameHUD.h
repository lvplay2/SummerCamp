// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCInGameHUD.h"
#include "SCSPInGameHUD.generated.h"

enum class ESCSkullTypes : uint8;

/**
 * @class ASCSPInGameHUD
 */
UCLASS()
class SUMMERCAMP_API ASCSPInGameHUD
: public ASCInGameHUD
{
	GENERATED_UCLASS_BODY()

	/** Show a HUD notification for completing an objective. */
	UFUNCTION()
	void OnShowObjectiveEvent(const FString& Message);

	/** Show a HUD notification for achieving a skull. */
	UFUNCTION()
	void OnSkullCompleted(const ESCSkullTypes skullType) const;

	/** Show a HUD notification for failing a skull. */
	void OnSkullFailed(const ESCSkullTypes skullType) const;
};
