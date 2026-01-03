// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPoweredObject.h"

#include "SCGameState.h"
#include "SCPowerPlant.h"

USCPoweredObject::USCPoweredObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ISCPoweredObject::ISCPoweredObject()
	: bIsRegisteredWithPowerPlant(false)
	, bObjectHasPower(true)
{

}

void ISCPoweredObject::SetPowered(bool bHasPower, UCurveFloat* Curve)
{
	bObjectHasPower = bHasPower;
	Execute_OnSetPowered(_getUObject(), bHasPower, Curve);
}

bool ISCPoweredObject::RegisterWithPowerPlant()
{
	if (bIsRegisteredWithPowerPlant)
		return true;

	UWorld* World = _getUObject()->GetWorld();
	if (World == nullptr)
		return false;

	ASCGameState* SCGameState = Cast<ASCGameState>(World->GetGameState());
	if (SCGameState == nullptr)
		return false;

	// Sign up!
	if (AActor* Actor = Cast<AActor>(_getUObject()))
	{
		SCGameState->GetPowerPlant()->Register(Actor);
	}
	else if (USceneComponent* Component = Cast<USceneComponent>(_getUObject()))
	{
		SCGameState->GetPowerPlant()->Register(Component);
	}
	else
	{
#if WITH_EDITOR
		// TODO: PIE Error -- We gotta have a position to handle power!
#endif
		return false;
	}

	bIsRegisteredWithPowerPlant = true;
	return true;
}
