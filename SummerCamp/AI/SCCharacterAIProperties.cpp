// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCCharacterAIProperties.h"

USCCharacterAIProperties::USCCharacterAIProperties(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, HearingThreshold(1500.f)
, LOSHearingThreshold(2500.f)
, SightRadius(2000.f)
, PeripheralVisionAngle(62.f)
, MinWiggleInterval(.15f)
, MaxWiggleInterval(.2f)
{
}

USCCounselorAIProperties::USCCounselorAIProperties(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bUseDefaultPreferencesForOfflineLogic(true)
, TimeBetweenLockingInteractions(3.f)
, MinTimeToHide(10.f)
, HideJasonHeardMinTime(45.f)
, bCheckJasonLOSBeforeHiding(true)
, MinDistanceFromJasonToHide(1000.f)
, WeaponsOverPartsMinStrength(3)
, MinHealthToUseMedSpray(50)
, MinDistanceFromJasonToUseMedSpray(500.f)
, MaxDistanceFromJasonToUseFirecrackers(200.f)
, JasonJukeDistance(300.f)
, JasonThreatDistance(1000.f)
, JasonThreatChasingRecently(30.f)
, JasonThreatSeenRecently(15.f)
, JasonThreatSeenTimeout(40.f)
, FightBackStunDelayMinimum(15.f)
, FightBackStunDelayMaximum(20.f)
, ArmedFightBackMinimumDistance(500.f)
, ArmedFightBackMaximumDistance(1500.f)
, MeleeFightBackCooldownMinimum(1.f)
, MeleeFightBackCooldownMaximum(5.f)
, MeleeFightBackCloseFacingAngle(30.f)
, MeleeFightBackMinimumDistance(300.f)
, MeleeFightBackMaximumDistance(700.f)
, MeleeFightBackMaximumDistanceWounded(500.f)
, MinFearForFlashlightIndoors(50.f)
, MinFearForFlashlightOutdoors(15.f)
, MinRunSprintStateEaseOff(2.f)
, MaxRunSprintStateEaseOff(5.f)
, MinStaminaToRun(10.f)
, MinFearToRun(10.f)
, MinStaminaToSprint(30.f)
, MinFearToSprint(30.f)
, ShareJasonNoiseStimulusDistance(1500.f)
, ShareJasonVisualStimulusDistance(2000.f)
, MinFearForWindowDive(15.f)
, MinHealthForWindowDive(60.f)
{
}

USCJasonAIProperties::USCJasonAIProperties(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, MaxGrabDistance(1000.f)
, MinThrowingKnifeDistance(500.f)
, MaxThrowingKnifeDistance(2500.f)
{
}
