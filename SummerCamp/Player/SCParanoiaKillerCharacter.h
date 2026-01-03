// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCKillerCharacter.h"
#include "SCParanoiaKillerCharacter.generated.h"

/**
 * @class ASCParanoiaKillerCharacter
 */
UCLASS()
class SUMMERCAMP_API ASCParanoiaKillerCharacter
: public ASCKillerCharacter
{
	GENERATED_BODY()

public:
	ASCParanoiaKillerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void SetupPlayerInputComponent(class UInputComponent* InInputComponent) override;
	virtual void OnReceivedPlayerState() override;

	virtual ASCCounselorCharacter* GetLocalCounselor() const override;

	UFUNCTION()
	void OnChangeIntoCounselor();

protected:
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, ASCPlayerState* KillerPlayerState, AActor* DamageCauser) override {}
};