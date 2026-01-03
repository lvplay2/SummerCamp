// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/Menu/SCMenuWidget.h"
#include "SCCreditsMenuWidget.generated.h"

class UMediaPlayer;

/**
 * @class USCCreditsMenuWidget
 */
UCLASS()
class SUMMERCAMP_API USCCreditsMenuWidget
: public USCMenuWidget
{
	GENERATED_UCLASS_BODY()

	virtual bool Initialize() override;

	UFUNCTION(BlueprintCallable, Category = "Video Player")
	void PlayVideo(UMediaSource* MediaSource);

	UFUNCTION(BlueprintCallable, Category = "Video Player")
	void CloseVideo();

	UPROPERTY(EditAnywhere, Category = "Video Player")
	UMediaPlayer* MediaPlayer;

private:
	UFUNCTION()
	void OnPlayBackStopped();

	UFUNCTION()
	void OnPlaybackResumed();
};
