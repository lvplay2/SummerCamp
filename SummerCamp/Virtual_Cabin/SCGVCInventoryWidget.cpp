// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "HorizontalBox.h"
#include "SCGVCInventoryWidget.h"

UHorizontalBox* USCGVCInventoryWidget::CreateNewHorizontalBox()
{
	UHorizontalBox* newHorizBox;

	newHorizBox = NewObject<UHorizontalBox>();

	return newHorizBox;
}