// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Engine/PointLight.h"
#include "SCPoweredObject.h"
#include "PoweredPointLight.generated.h"

/**
 * @class APoweredPointLight
 */
UCLASS(ClassGroup=(Lights, PointLights), MinimalAPI, ComponentWrapperClass, meta=(ChildCanTick))
class APoweredPointLight
	: public APointLight
	, public ISCPoweredObject
{
	GENERATED_BODY()

public:
	APoweredPointLight(const FObjectInitializer& ObjectInitializer);

	// Begin AActor interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	// End AActor interface

	// Begin ISCPoweredObject interface
	virtual void SetPowered(bool bHasPower, UCurveFloat* Curve) override;
	virtual void SetPowerCurve(UCurveFloat* Curve) override { PowerCurve = Curve; }
	// End ISCPoweredObject interface

	void SetVisibility(bool bVisible);

	UFUNCTION(BlueprintNativeEvent, Category = "Power")
	void OnActivate();

	UFUNCTION(BlueprintNativeEvent, Category = "Power")
	void OnDeactivate();

protected:
	UPROPERTY(EditAnywhere, Category = "Power")
	bool bStartPowered;

	/** Sphere for overlap testing */
	UPROPERTY()
	USphereComponent* CollisionVolume;

	/** Only applicable if PowerCurve is non-null, if true we're powering up, otherwise we're powering down */
	bool bIsApplyingPower;

	/** Intensity of our light component at startup */
	float InitialIntensity;

	/** Cumulative time for use in the PowerCurve when turning on/off */
	float CurrentPowerTime;

	/** Curve used to turn lights on/off in a dynamic manner */
	UPROPERTY(transient)
	UCurveFloat* PowerCurve;

	/** List of child components glowing materials */
	UPROPERTY(transient)
	TArray<UMaterialInstanceDynamic*> GlowMaterials;
};
