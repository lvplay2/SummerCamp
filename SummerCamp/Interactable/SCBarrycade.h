// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBarricade.h"
#include "SCBarrycade.generated.h"

class USCScoringEvent;

enum class EILLInteractMethod : uint8;

/**
 * @class ASCBarrycade
 */
UCLASS()
class SUMMERCAMP_API ASCBarrycade
: public ASCBarricade
{
	GENERATED_BODY()

public:
	ASCBarrycade(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Default")
		UStaticMeshComponent* ForceFieldMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
		FRotator RotationOn;



/*	// specific blueprint implementable events because I'm lazy
	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnBarricadeEnabled(bool bEnable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Default")
	void OnBarricadeEnabled(bool bEnable);
	*/
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UParticleSystemComponent* ActivateParticleSystem;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UParticleSystemComponent* DeactivateParticleSystem;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UParticleSystemComponent* DestroyedParticleSystem;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UParticleSystemComponent* ActivateParticleSystem2;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UParticleSystemComponent* DeactivateParticleSystem2;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UParticleSystemComponent* DestroyedParticleSystem2;

	/*
	UPROPERTY(EditDefaultsOnly, Category = "Default")
		UParticleSystem* ActivateParticleSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
		UParticleSystem* DeactivateParticleSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
		UParticleSystem* DestroyedParticleSystem;
		*/

	// Begin ASCBarricade Interface
	virtual void OnBarricadeAborted(ASCCharacter* Interactor) override;
	virtual void UseBarricadeDriverTick(FAnimNotifyData NotifyData) override;

protected:
	virtual void OnRep_Enabled() override;
	virtual void OnRep_Destroyed() override;
	// End ASCBarricade Interface
};
