// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "GameFramework/Actor.h"
#include "ILLDynamicCamera.generated.h"

/**
 * Dynamic camera supports basic movement based offsets and ellipsoidal rotation for a more cinematic presentation
 */
UCLASS(ClassGroup="Camera", BlueprintType, Blueprintable)
class ILLGAMEFRAMEWORK_API AILLDynamicCamera
: public AActor
{
	GENERATED_UCLASS_BODY()

public:
	/** Dynamic Camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UILLDynamicCameraComponent* DynamicCamera;
};
