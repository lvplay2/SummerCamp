// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/PlayerStart.h"
#include "ILLPlayerStart.generated.h"

/**
 * @class AILLPlayerStart
 */
UCLASS()
class ILLGAMEFRAMEWORK_API AILLPlayerStart
: public APlayerStart
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void CheckForErrors() override;
#endif
	// End AActor interface

	// Begin ANavigationObjectBase interface
	virtual void Validate() override;
	// End ANavigationObjectBase interface

	/** @return true if this spawn is encroaching blocking geometry. */
	virtual bool IsEncroached(APawn* PawnToFit = nullptr) const;

protected:
	// Furthest this spawn can be from the ground before throwing a map check warning
	UPROPERTY(EditDefaultsOnly, Category="Spawning")
	float GroundDistanceLimit;
};
