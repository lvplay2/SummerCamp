// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCMusicObject.h"

USCMusicObject::USCMusicObject()
{
	StingerCooldownTime = 15.f;
	AddLayerChangeDelayTime = 2.0f;
	RemoveLayerChangeDelayTime = 10.f;
}

FMusicTrack USCMusicObject::GetMusicDataByEventName(FName EventName)
{
	FMusicTrack MusicTrackData;
	for (int i = 0; i < MusicData.Num(); ++i)
	{
		if (MusicData[i].EventName == EventName)
		{
			MusicTrackData = MusicData[i];
			break;
		}
	}

	return MusicTrackData;
}
