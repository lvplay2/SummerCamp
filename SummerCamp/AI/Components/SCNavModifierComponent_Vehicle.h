// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavModifierComponent.h"
#include "Navigation/CrowdAgentInterface.h"
#include "SCNavModifierComponent_Vehicle.generated.h"

/**
 * @class USCNavModifierComponent_Vehicle
 * @brief Nav Modifier for Vehicle specific handling of calculating bounds
 */
UCLASS()
class SUMMERCAMP_API USCNavModifierComponent_Vehicle
: public UNavModifierComponent, public ICrowdAgentInterface
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	// End UActorComponent Interface

	// Begin UNavRelevantComponent interface
	virtual FBox GetNavigationBounds() const override;
	virtual void CalcAndCacheBounds() const override;
	virtual void GetNavigationData(FNavigationRelevantData& Data) const override;
	// End UNavRelevantComponent interface

	// Begin ICrowdAgentInterface interface
	virtual FVector GetCrowdAgentLocation() const override;
	virtual FVector GetCrowdAgentVelocity() const override;
	virtual void GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const override;
	virtual float GetCrowdAgentMaxSpeed() const override;
	// End ICrowdAgentInterface interface

protected:
	/** Setting to 'true' will result in expanding lower bounding box of the nav
	 *	modifier by agent's height, before applying to navmesh */
	UPROPERTY(EditAnywhere, Category="Navigation")
	bool bIncludeAgentHeight;
};
