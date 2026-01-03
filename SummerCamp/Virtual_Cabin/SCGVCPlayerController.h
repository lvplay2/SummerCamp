// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLPlayerController.h"
#include "SCGVCPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API ASCGVCPlayerController
: public AILLPlayerController
{
	GENERATED_BODY()

public:
	virtual void InitInputSystem() override;
	virtual void Possess(APawn* VCPawn) override;

	void HideInventory();

	void ShowInventory();

	void ShowDebugMenu();

	void HideDebugMenu();
};
