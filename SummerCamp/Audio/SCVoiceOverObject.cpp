// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCVoiceOverObject.h"

FVoiceOverData USCVoiceOverObject::GetVoiceOverDataByVOName(const FName VOName) const
{
	FVoiceOverData VOData;

	// Check for the VO Data
	for (int i = 0; i < VoiceOverData.Num(); ++i)
	{
		if (VoiceOverData[i].VoiceOverName == VOName)
		{
			VOData = VoiceOverData[i];
			break;
		}
	}

	return VOData;
}
