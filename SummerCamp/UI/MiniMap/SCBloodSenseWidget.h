// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "IllMinimapWidget.h"
#include "SCBloodSenseWidget.generated.h"

/**
 * @class USCBloodSenseWidget
 */
UCLASS()
class SUMMERCAMP_API USCBloodSenseWidget
: public UILLMinimapWidget
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool UpdateMapWidget_Implementation(UPARAM(ref) FVector& currentPosition, UPARAM(ref) FRotator& currentRotation) override;
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|BloodSense")
	FVector Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|BloodSense")
	float DefaultRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|BloodSense")
	float NextPingTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|BloodSense")
	float TotalPingTime;

	float CurrentPingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|BloodSense")
	FVector2D PingMinMaxDelay;

	bool bIsActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap|BloodSense")
	class UImage* Blip;

	void BeginPing();
	void EndPing();
};
