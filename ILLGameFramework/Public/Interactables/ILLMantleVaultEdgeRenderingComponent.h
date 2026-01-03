// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "ILLMantleVaultEdgeRenderingComponent.generated.h"

/**
 * @class UILLMantleVaultEdgeRenderingComponent
 */
UCLASS(hidecategories=Object, editinlinenew)
class ILLGAMEFRAMEWORK_API UILLMantleVaultEdgeRenderingComponent
: public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
		
	// Begin UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }
#if WITH_EDITOR
	virtual bool ComponentIsTouchingSelectionBox(const FBox& InSelBBox, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const override { return false; }
	virtual bool ComponentIsTouchingSelectionFrustum(const FConvexVolume& InFrustum, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const override { return false; }
#endif
	// End UPrimitiveComponent interface

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// End USceneComponent interface
};
