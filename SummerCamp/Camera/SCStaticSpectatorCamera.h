// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "Camera/CameraActor.h"
#include "SCStaticSpectatorCamera.generated.h"

/**
 * @class ASCStaticSpectatorCamera
 */
UCLASS()
class SUMMERCAMP_API ASCStaticSpectatorCamera 
: public ACameraActor
{
	GENERATED_UCLASS_BODY()

	/** The display name of the camera */
	UPROPERTY(EditInstanceOnly, Category = "SpectatorCamera")
	FName DisplayName;
};
