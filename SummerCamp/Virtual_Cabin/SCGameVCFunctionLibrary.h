// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DateTime.h"
#include "SCGameVCFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API USCGameVCFunctionLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Virtual Cabin Library")
	static bool IsFridaythe13th();

	UFUNCTION(BlueprintCallable, Category = "Virtual Cabin Library")
	static bool IsFridaythe13th79();

	UFUNCTION(BlueprintCallable, Category = "Virtual Cabin Library")
	static ASCGVCPlayerController* GetVCPlayerController(UObject* Context);

	UFUNCTION(BlueprintCallable, Category = "Virtual Cabin Library")
	static ASCGVCCharacter* GetVCCharacter(UObject* Context);

};
