// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCPlayerState.h"
#include "SCGVCPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class SUMMERCAMP_API ASCGVCPlayerState
: public ASCPlayerState
{
	GENERATED_BODY()
	
public:
	/** Update platform achievements */
	void UpdateAchievement(const FName AchievementName, const float AchievementProgress = 100.0f);

	/** Update platform achievements on the client */
	UFUNCTION(Reliable, Client)
	void ClientUpdateAchievement(const FName AchievementName, const float AchievementProgress);
};
