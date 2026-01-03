// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "Components/WidgetComponent.h"
#include "ILLWidgetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDistanceNotifySignature);
/**
 * @class UILLWidgetComponent
 */
UCLASS(ClassGroup=UI, meta=(BlueprintSpawnableComponent))
class ILLGAMEFRAMEWORK_API UILLWidgetComponent
: public UWidgetComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UActorComponent interface
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	/** Ensures the user widget is initialized */
	virtual void InitWidget() override;
	// End UActorComponent interface

	// Consider this out of range when the game state does not report as in-progress?
	UPROPERTY(EditAnywhere, Category = "Culling")
	bool bOutOfRangeWhenMatchNotInProgress;

	// Will broadcast range events if true
	UPROPERTY(EditAnywhere, Category = "Culling")
	bool bUseDistanceNotify;

	// The distance at which the range notifies are called
	UPROPERTY(EditAnywhere, Category = "Culling", meta = (EditCondition = "bUseDistanceNotify"))
	float DistanceToNotify;

	// How long after the OnLocalCharacterOutOfRange gets called before the widget is destroyed, < 0 for no destruction
	UPROPERTY(EditAnywhere, Category = "Culling", meta = (EditConditoin = "bUseDistanceNotify"))
	float DestructionTimer;

protected:
	// Should the widget hug screen edges instead of going off-screen?
	UPROPERTY(EditAnywhere, Category=UserInterface)
	bool bHugScreenEdges;

	//////////////////////////////////////////////////
	// Distance culling
protected:
	// Delegate that is called when the locally controlled pawn is in range of the component
	UPROPERTY(BlueprintAssignable)
	FDistanceNotifySignature OnLocalCharacterInRange;

	// Delegate that is called when the locally controlled pawn is out of range of the component
	UPROPERTY(BlueprintAssignable)
	FDistanceNotifySignature OnLocalCharacterOutOfRange;

private:
	bool bLastWasNear;

	float DestroyTimer;
};
