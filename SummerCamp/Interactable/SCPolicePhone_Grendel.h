// Copyright (C) 2015-2017 IllFonic, LLC. and Gun Media

#pragma once

#include "CoreMinimal.h"
#include "Interactable/SCPolicePhone.h"
#include "SCPolicePhone_Grendel.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API ASCPolicePhone_Grendel : public ASCPolicePhone
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = PolicePhone)
	void OnEnabled(bool bEnable);

	////////////////////////////////
	// ASCPolicePhone interface
	virtual void UpdateShimmer() override;
	
	
};
