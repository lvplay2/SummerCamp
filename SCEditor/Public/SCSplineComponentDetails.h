// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media


/****************************************
* RIPPED FROM Source/Editor/DetailCustomizations/Private/SplineComponentDetails.h
***************************************/

#include "IDetailCustomization.h"

#pragma once

class FSCSplineComponentDetails
	: public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	// Begin IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End IDetailCustomization interface
};
