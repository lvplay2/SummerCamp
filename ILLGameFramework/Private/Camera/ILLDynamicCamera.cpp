// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "Camera/ILLDynamicCamera.h"
#include "Camera/ILLDynamicCameraComponent.h"

AILLDynamicCamera::AILLDynamicCamera(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetTickGroup(TG_PostUpdateWork);

	DynamicCamera = CreateDefaultSubobject<UILLDynamicCameraComponent>(TEXT("DynamicCamera"));
	RootComponent = DynamicCamera;
}
