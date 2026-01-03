// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/ActorComponent.h"
#include "Navigation/CrowdAgentInterface.h"
#include "ILLCrowdAgentInterfaceComponent.generated.h"

/**
* @class UILLCrowdAgentInterfaceComponent
* @brief Component that can be added to characters that registers them with the Navigation System's Crowd Manager. This is useful if you need AI using CrowdFollowing to avoid player characters.
*/
UCLASS()
class ILLGAMEFRAMEWORK_API UILLCrowdAgentInterfaceComponent 
: public UActorComponent, public ICrowdAgentInterface
{
	GENERATED_UCLASS_BODY()
	
	// Begin UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	// End UActorComponent interface
	
	// Begin ICrowdAgentInterface interface
	virtual FVector GetCrowdAgentLocation() const override;
	virtual FVector GetCrowdAgentVelocity() const override;
	virtual void GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const override;
	virtual float GetCrowdAgentMaxSpeed() const override;
	// End ICrowdAgentInterface interface
};
