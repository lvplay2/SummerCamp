// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCreditsMenuWidget.h"

#include "MediaPlayer.h"

USCCreditsMenuWidget::USCCreditsMenuWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool USCCreditsMenuWidget::Initialize()
{
	if (MediaPlayer)
	{
		MediaPlayer->OnPlaybackResumed.AddDynamic(this, &USCCreditsMenuWidget::OnPlayBackStopped);
		MediaPlayer->OnEndReached.AddDynamic(this, &USCCreditsMenuWidget::OnPlayBackStopped);
		MediaPlayer->OnPlaybackResumed.AddDynamic(this, &USCCreditsMenuWidget::OnPlaybackResumed);
	}

	return Super::Initialize();
}

void USCCreditsMenuWidget::PlayVideo(UMediaSource* MediaSource)
{
	if (MediaPlayer && MediaSource)
	{
		MediaPlayer->OpenSource(MediaSource);
	}
}

void USCCreditsMenuWidget::CloseVideo()
{
	if (MediaPlayer)
	{
		MediaPlayer->OnPlaybackResumed.RemoveAll(this);
		MediaPlayer->OnEndReached.RemoveAll(this);
		MediaPlayer->OnPlaybackResumed.RemoveAll(this);

		MediaPlayer->Close();
	}
}

void USCCreditsMenuWidget::OnPlayBackStopped()
{
}

void USCCreditsMenuWidget::OnPlaybackResumed()
{
}
