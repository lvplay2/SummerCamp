// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerController_Offline.h"
#include "SCDossier.h"
#include "SCPlayerController_SPChallenges.generated.h"

enum class ESCSkullTypes : uint8;
/**
 * @class ASCPlayerController_SPChallenges
 */
UCLASS(Config=Game)
class SUMMERCAMP_API ASCPlayerController_SPChallenges
: public ASCPlayerController_Offline
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void PostInitializeComponents() override;
	// End AActor interface

	/** Callback for the Dossier when an objective is marked completed. */
	UFUNCTION()
	void OnObjectiveCompleted(const FSCObjectiveState ObjectiveState);

	/** Callback (from the Dossier) when a skull is completed. */
	UFUNCTION()
	void OnSkullCompleted(const ESCSkullTypes SkullType);

	/** Callback (from the Dossier) when a skull is failed. */
	UFUNCTION()
	void OnSkullFailed(const ESCSkullTypes SkullType);
};
