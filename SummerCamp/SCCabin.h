// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "GameFramework/Actor.h"
#include "SCCabin.generated.h"

/**
 * @class ASCCabin
 */
UCLASS()
class SUMMERCAMP_API ASCCabin
: public AActor
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	/** Toggles sense effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
	void SetSenseEffects(bool bEnable);

	/** Toggles rain effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Effects")
	void SetRaining(bool bRaining);

	/** @return Child actors. */
	UFUNCTION(BlueprintPure, Category = "Cabin")
	TArray<AActor*> GetChildren() const { return Children; } // OPTIMIZE: pjackson: Shouldn't this be a const-ref return?

	/** @return true if this is a Jason shack. */
	UFUNCTION(BlueprintPure, Category="Jason Shack")
	bool IsJasonShack() const {	return bIsJasonsShack; }

protected:
	// Is this a Jason shack?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Jason Shack")
	bool bIsJasonsShack;
};
