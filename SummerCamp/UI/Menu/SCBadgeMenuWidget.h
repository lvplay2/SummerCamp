// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "UI/Menu/SCMenuWidget.h"
#include "SCBadgeMenuWidget.generated.h"

class USCStatBadge;

/**
 * USCBadgeMenuWidget
 */
UCLASS()
class SUMMERCAMP_API USCBadgeMenuWidget 
: public USCMenuWidget
{
	GENERATED_BODY()
public:
	// Gets all textures and data from a badge
	UFUNCTION(BlueprintPure, Category = "Badges")
	void GetAllBadgeInfo(TSubclassOf<USCStatBadge> BadgeClass, UTexture2D*& BadgeIcon, UTexture2D*& TwoStarIcon, UTexture2D*& ThreeStarIcon, FText& DisplayName, FText& Description, int32& OneStarUnlock, int32& TwoStarUnlock, int32& ThreeStarUnlock) const;

	
	// Gets a specific texture and data from a given badge level
	UFUNCTION(BlueprintPure, Category = "Badges")
	void GetBadgeInfo(TSubclassOf<USCStatBadge> BadgeClass, const int32 Inlevel, FText& DisplayName, FText& Description, int32& OutBadgeUnlock, UTexture2D*& OutBadgeIcon ) const;
	
};
