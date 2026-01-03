// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPoweredObject.generated.h"

/**
 * @class USCPoweredObject
 */
UINTERFACE(NotBlueprintable)
class SUMMERCAMP_API USCPoweredObject
	: public UInterface
{
	GENERATED_UINTERFACE_BODY()

};

/**
 * @class ISCPoweredObject
 */
class SUMMERCAMP_API ISCPoweredObject
{
	GENERATED_IINTERFACE_BODY()

public:
	ISCPoweredObject();

	/** Called when power is applied or removed from this object, if you're not getting this make sure Register was called */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Power")
	void OnSetPowered(bool bHasPower, UCurveFloat* Curve);

	/** Function to be override in C++, calls blueprint version automatically */
	virtual void SetPowered(bool bHasPower, UCurveFloat* Curve);

	/** Used to generically set a power curve for a given object */
	virtual void SetPowerCurve(UCurveFloat* Curve) { }

	/**
	 * Call to sign us up for the power plant! MUST be called to recieve and SetPowered events
	 * @return If true, we successfully registered (or had done previously)! False if we couldn't get the World or GameState, try calling from BeginPlay
	 */
	bool RegisterWithPowerPlant();

	/** @return true if this object has power */
	FORCEINLINE bool HasPower() const { return bObjectHasPower; }

private:
	/** Prevents double registering */
	bool bIsRegisteredWithPowerPlant;

	/** Does this object currently have power */
	bool bObjectHasPower;
};
