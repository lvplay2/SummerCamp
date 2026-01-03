// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCBadgeMenuWidget.h"

#include "SCStatsAndScoringData.h"

void USCBadgeMenuWidget::GetAllBadgeInfo(TSubclassOf<USCStatBadge> BadgeClass, UTexture2D*& BadgeIcon, UTexture2D*& TwoStarIcon, UTexture2D*& ThreeStarIcon, FText& DisplayName, FText& Description, int32& OneStarUnlock, int32& TwoStarUnlock, int32& ThreeStarUnlock) const
{
	if (BadgeClass)
	{
		if (const USCStatBadge* const Badge = BadgeClass->GetDefaultObject<USCStatBadge>())
		{
			BadgeIcon = Badge->BadgeIcon.LoadSynchronous();
			TwoStarIcon = Badge->TwoStarBadgeIcon.LoadSynchronous();
			ThreeStarIcon = Badge->ThreeStarBadgeIcon.LoadSynchronous();
			DisplayName = Badge->DisplayName;
			Description = Badge->Description;
			OneStarUnlock = Badge->OneStarUnlock;
			TwoStarUnlock = Badge->TwoStarUnlock;
			ThreeStarUnlock = Badge->ThreeStarUnlock;
		}
	}
}

void USCBadgeMenuWidget::GetBadgeInfo(TSubclassOf<USCStatBadge> BadgeClass, const int32 Inlevel, FText& OutDisplayName, FText& OutDescription, int32& OutBadgeUnlock, UTexture2D*& OutBadgeIcon) const
{
	if (BadgeClass)
	{
		if (const USCStatBadge* const Badge = BadgeClass->GetDefaultObject<USCStatBadge>())
		{
			switch (Inlevel)
			{
			default:
			case 1:
				OutBadgeIcon = Badge->BadgeIcon.LoadSynchronous();
				OutBadgeUnlock = Badge->OneStarUnlock;
				break;
			case 2:
				OutBadgeIcon = Badge->BadgeIcon.LoadSynchronous();
				OutBadgeUnlock = Badge->TwoStarUnlock;
				break;
			case 3:
				OutBadgeIcon = Badge->BadgeIcon.LoadSynchronous();
				OutBadgeUnlock = Badge->ThreeStarUnlock;
				break;
			}
			OutDisplayName = Badge->DisplayName;
			OutDescription = Badge->Description;
		}
	}
}
