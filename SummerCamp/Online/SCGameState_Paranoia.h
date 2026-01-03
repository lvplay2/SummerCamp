// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCGameState_Hunt.h"
#include "SCGameState_Paranoia.generated.h"

class ASCPlayerState;

/**
* @class ASCGameState_Paranoia
*/
UCLASS(Config = Game)
class SUMMERCAMP_API ASCGameState_Paranoia
	: public ASCGameState_Hunt
{
	GENERATED_UCLASS_BODY()
public:
	// we dont want these objectives to ever show up
	virtual ASCDriveableVehicle* GetCarVehicle(bool Car4Seats) const override { return nullptr; }
	virtual ASCDriveableVehicle* GetBoatVehicle() const override { return nullptr; }
	virtual ASCPhoneJunctionBox* GetPhone() const override { return nullptr; }
};