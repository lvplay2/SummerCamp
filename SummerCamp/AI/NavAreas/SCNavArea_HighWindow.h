// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "AI/Navigation/NavAreas/NavArea.h"
#include "SCNavArea_HighWindow.generated.h"

/**
* @class USCNavArea_HighWindow
* @brief Navigation area with a higher pathing cost due to a window being high enough to cause damage when climbing/jumping out of it.
*/
UCLASS()
class SUMMERCAMP_API USCNavArea_HighWindow 
: public UNavArea
{
	GENERATED_UCLASS_BODY()
};
