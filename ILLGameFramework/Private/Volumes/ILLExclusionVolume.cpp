// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "ILLGameFramework.h"
#include "IllExclusionVolume.h"


FBoxSphereBounds AIllExclusionVolume::GetVolumeExtents()
{
	return GetBrushComponent()->GetPlacementExtent();
}

