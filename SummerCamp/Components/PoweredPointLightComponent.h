// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Components/PointLightComponent.h"
#include "SCPoweredObject.h"
#include "PoweredPointLightComponent.generated.h"

/**
 * @class UPoweredPointLightComponent
 */
UCLASS(Blueprintable, ClassGroup=(Lights,Common), hidecategories=(Object, LightShafts), editinlinenew, meta=(BlueprintSpawnableComponent))
class SUMMERCAMP_API UPoweredPointLightComponent
	: public UPointLightComponent
	, public ISCPoweredObject
{
	GENERATED_BODY()

public:
	UPoweredPointLightComponent(const FObjectInitializer& ObjectInitializer);

	// Begin UActorComponent interface
public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UActorComponent interface

	// Begin USceneComponent interface
protected:
	virtual void OnVisibilityChanged() override;
	// End USceneComponent interface

	// Begin ISCPoweredObject interface
public:
	virtual void SetPowered(bool bHasPower, UCurveFloat* Curve) override;
	virtual void SetPowerCurve(UCurveFloat* Curve) override { PowerCurve = Curve; }
	// End ISCPoweredObject interface

	UFUNCTION()
	void OnActivate();

	UFUNCTION()
	void OnDeactivate();

	UPROPERTY(BlueprintAssignable, Category = "Power")
	FOnActivateDelegate OnActivateDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Power")
	FOnDeactivateDelegate OnDeactivateDelegate;

	/** Curve used to turn lights on/off in a dynamic manner */
	UPROPERTY(transient)
	UCurveFloat* PowerCurve;

protected:
	UPROPERTY(EditAnywhere, CAtegory = "Power")
	bool bStartPowered;

	/** Only applicable if PowerCurve is non-null, if true we're powering up, otherwise we're powering down */
	bool bIsApplyingPower;

	/** Intensity of our light component at startup */
	float InitialIntensity;

	/** Cumulative time for use in the PowerCurve when turning on/off */
	float CurrentPowerTime;

	/** List of child components glowing materials */
	UPROPERTY(transient)
	TArray<UMaterialInstanceDynamic*> GlowMaterials;
};
