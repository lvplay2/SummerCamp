// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCWaypoint.h"


ASCWaypoint::ASCWaypoint(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	UMaterialInterface* BillboardTextMaterial = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder(TEXT("/Game/UI/Materials/M_Billboard_Text"));
	BillboardTextMaterial = ObjectFinder.Object;

	if (WaypointTextComponent && BillboardTextMaterial)
		WaypointTextComponent->SetTextMaterial(BillboardTextMaterial);
}
