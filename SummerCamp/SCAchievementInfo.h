// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

namespace SCAchievements
{
//////////////////////////////////////////////////
// Achievement Info

#define ACH_SUMMERCAMP				"ACH_SUMMERCAMP"
#define ACH_CAMP_COUNSELOR			"ACH_CAMP_COUNSELOR"
#define ACH_HEAD_COUNSELOR			"ACH_HEAD_COUNSELOR"
#define ACH_FRIDAY_THE_13TH			"ACH_FRIDAY_THE_13TH"
#define ACH_THE_SEQUEL				"ACH_THE_SEQUEL"
#define ACH_FINAL_CHAPTER			"ACH_FINAL_CHAPTER"
#define ACH_ROLL_CREDITS			"ACH_ROLL_CREDITS"
#define ACH_FIRST_BLOOD				"ACH_FIRST_BLOOD"
#define ACH_MY_LUCKY_NUMBER			"ACH_MY_LUCKY_NUMBER"
#define ACH_EVIL_LURKS				"ACH_EVIL_LURKS"
#define ACH_GOTTA_KILL_EM_ALL		"ACH_GOTTA_KILL_EM_ALL"
#define ACH_NO_HAPPY_ENDINGS		"ACH_NO_HAPPY_ENDINGS"
#define ACH_ONE_FOR_GOOD_MEASURE	"ACH_ONE_FOR_GOOD_MEASURE"
#define ACH_SEEN_EVERY_MOVIE		"ACH_SEEN_EVERY_MOVIE"
#define ACH_SUPER_FAN				"ACH_SUPER_FAN"
#define ACH_GREASE_MONKEY			"ACH_GREASE_MONKEY"
#define ACH_SHIPWRIGHT				"ACH_SHIPWRIGHT"
#define ACH_SHOCK_JOCKEY			"ACH_SHOCK_JOCKEY"
#define ACH_OPERATOR				"ACH_OPERATOR"
#define ACH_DISC_JOCKEY				"ACH_DISC_JOCKEY"
#define ACH_FRIDAY_DRIVER			"ACH_FRIDAY_DRIVER"
#define ACH_ALONG_FOR_THE_RIDE		"ACH_ALONG_FOR_THE_RIDE"
#define ACH_ON_A_BOAT				"ACH_ON_A_BOAT"
#define ACH_AYE_AYE_CAPTAIN			"ACH_AYE_AYE_CAPTAIN"
#define ACH_ITS_A_TRAP				"ACH_ITS_A_TRAP"
#define ACH_HERE_IS_TOMMY			"ACH_HERE_IS_TOMMY"
#define ACH_GOOD_BOY				"ACH_GOOD_BOY"
#define ACH_VOODOO_DOLL				"ACH_VOODOO_DOLL"
#define ACH_EENIE_MEENIE			"ACH_EENIE_MEENIE"
#define ACH_YOU_DIED				"ACH_YOU_DIED"
#define ACH_YOU_DIED_ALOT			"ACH_YOU_DIED_ALOT"
#define ACH_SLAM_JAM				"ACH_SLAM_JAM"
#define ACH_CRYSTAL_LAKE_FIVE_O		"ACH_CRYSTAL_LAKE_FIVE_O"
#define ACH_FLARING_UP				"ACH_FLARING_UP"
#define ACH_SNAP_CRACK_BOOM			"ACH_SNAP_CRACK_BOOM"
#define ACH_GET_OUT_OF_JAIL_FREE	"ACH_GET_OUT_OF_JAIL_FREE"
#define ACH_FACE_OFF				"ACH_FACE_OFF"
#define ACH_NEW_THREADS				"ACH_NEW_THREADS"
#define ACH_GOALIE					"ACH_GOALIE"
#define ACH_DOOR_WONT_CLOSE			"ACH_DOOR_WONT_CLOSE"
#define ACH_DOOR_WONT_OPEN			"ACH_DOOR_WONT_OPEN"
#define ACH_WINDOWS_99				"ACH_WINDOWS_99"
#define ACH_A_CLASSIC				"ACH_A_CLASSIC"
#define ACH_COOKING_WITH_JASON		"ACH_COOKING_WITH_JASON"
#define ACH_SMASH_BROS				"ACH_SMASH_BROS"
#define ACH_I_NEED_A_MEDIC			"ACH_I_NEED_A_MEDIC"
#define ACH_THIS_IS_MY_BOOMSTICK	"ACH_THIS_IS_MY_BOOMSTICK"
#define ACH_LETS_SPLIT_UP			"ACH_LETS_SPLIT_UP"
#define ACH_CHAD_IS_A_DICK			"ACH_CHAD_IS_A_DICK"
#define ACH_PHD_IN_MURDER			"ACH_PHD_IN_MURDER"
#define ACH_KILLER_FRANCHISE		"ACH_KILLER_FRANCHISE"
#define ACH_UNLIKELY_GIRL			"ACH_UNLIKELY_GIRL"
#define ACH_H20_DELIRIOUS			"ACH_H20_DELIRIOUS"

// Achievement stat unlock values
static const int32 CampCounselorMatchCount		= 500;
static const int32 HeadCounselorMatchCount		= 1000;
static const int32 SequelMatchCount				= 500;
static const int32 FinalChapterMatchCount		= 1000;
static const int32 LuckNumberKillCount			= 13;
static const int32 EvilLurksKillCount			= 666;
static const int32 KillEmAllKillCount			= 1313;
static const int32 NoHappyEndingKillCount		= 7;
static const int32 GreaseMonkeyRepairCount		= 100;
static const int32 ShipwrightRepairCount		= 100;
static const int32 ShockJockeyRepairCount		= 100;
static const int32 OperatorCallCount			= 13;
static const int32 DiscJockeyCallCount			= 13;
static const int32 AlongForTheRideEscapeCount	= 13;
static const int32 ImOnABoatEscapeCount			= 13;
static const int32 ItsATrapCount				= 13;
static const int32 TommyPlayCount				= 13;
static const int32 GoodBoyStunCount				= 13;
static const int32 ThrowingKnifeHitCount		= 13;
static const int32 EenieMeenieTrapCount			= 13;
static const int32 YouDiedALotCount				= 100;
static const int32 SlamJamCount					= 13;
static const int32 CrystalLakeFiveOCount		= 13;
static const int32 FlaringUpCount				= 13;
static const int32 SnapCrackleBoomCount			= 13;
static const int32 GetOutOfJailFreeCount		= 13;
static const int32 FaceOffCount					= 13;
static const int32 ThisDoorWontOpenCount		= 100;
static const int32 ClosedWindowJumpCount		= 99;
static const int32 SmashBrosCount				= 100;
static const int32 INeedAMedicCount				= 100;
static const int32 BoomStickCount				= 13;
static const int32 PhDInMurderCount				= 56;


// Platform leaderboard stats
#define LEADERBOARD_STAT_COUNSELOR_MATCHES		"CounselorMatchesPlayed"
#define LEADERBOARD_STAT_JASON_MATCHES			"JasonMatchesPlayed"
#define LEADERBOARD_STAT_COUNSELORS_KILLED		"CounselorsKilled"
#define LEADERBOARD_STAT_CAR_REPAIRS			"CarPartsRepaired"
#define LEADERBOARD_STAT_BOAT_REPAIRS			"BoatPartsRepaired"
#define LEADERBOARD_STAT_GRID_REPAIRS			"GridPartsRepaired"
#define LEADERBOARD_STAT_POLICE_CALLED			"PoliceCallCount"
#define LEADERBOARD_STAT_TJ_CALLED				"TommyJarvisCallCount"
#define LEADERBOARD_STAT_CAR_PASSENGER_ESCAPE	"CarPassengerEscapeCount"
#define LEADERBOARD_STAT_BOAT_PASSENGER_ESCAPE	"BoatPassengerEscapeCount"
#define LEADERBOARD_STAT_TRAP_JASON				"TrappedJasonCount"
#define LEADERBOARD_STAT_PLAY_AS_TJ				"TommyJarvisMatchesPlayed"
#define LEADERBOARD_STAT_SWEATER_STUN			"SweaterStunCount"
#define LEADERBOARD_STAT_THROWING_KNIFE_HIT		"ThrowingKnifeHitCount"
#define LEADERBOARD_STAT_TRAP_COUNSELOR			"TrappedCounselorCount"
#define LEADERBOARD_STAT_DEATH_BY_JASON			"DeathsByJasonCount"
#define LEADERBOARD_STAT_SLAM_JAM				"SlamJamCount"
#define LEADERBOARD_STAT_POLICE_ESCAPES			"PoliceEscapeCount"
#define LEADERBOARD_STAT_FLARE_HITS				"FlareGunHits"
#define LEADERBOARD_STAT_FIRECRACKER_STUNS		"FirecrackerStunCount"
#define LEADERBOARD_STAT_POCKET_KNIFE_ESCAPE	"PocketKnifeEscapeCount"
#define LEADERBOARD_STAT_KNOCKED_MASK_OFF		"KnockedMaskOffCount"
#define LEADERBOARD_STAT_DOOR_BREAKS			"DoorsBrokenDownCount"
#define LEADERBOARD_STAT_CLOSED_WINDOWS			"ClosedWindowJumpCount"
#define LEADERBOARD_STAT_BASEBALL_BAT_HITS		"BaseballBatHitsOnJason"
#define LEADERBOARD_STAT_FIRST_AID_USES			"FirstAidSprayUses"
#define LEADERBOARD_STAT_SHOTGUN_JASON			"ShotgunHitsOnJason"
#define LEADERBOARD_STAT_ROLL_CREDITS			"RollCredits"
#define LEADERBOARD_STAT_NO_HAPPY_ENDINGS		"NoHappyEndings"
#define LEADERBOARD_STAT_ONE_FOR_GOOD_MEASURE	"OneForGoodMeasure"
#define LEADERBOARD_STAT_SEEN_EVERY_MOVIE		"IveSeenEveryMovie"
#define LEADERBOARD_STAT_SUPER_FAN				"SuperFan"
#define LEADERBOARD_STAT_FRIDAY_DRIVER			"FridayDriver"
#define LEADERBOARD_STAT_AYE_AYE_CAPTAIN		"AyeAyeCaptain"
#define LEADERBOARD_STAT_NEW_THREADS			"NewThreads"
#define LEADERBOARD_STAT_GOALIE					"Goalie"
#define LEADERBOARD_STAT_THIS_DOOR_WONT_CLOSE	"ThisDoorWontClose"
#define LEADERBOARD_STAT_A_CLASSIC				"AClassic"
#define LEADERBOARD_STAT_COOKING_WITH_JASON		"CookingWithJasonVorhees"
#define LEADERBOARD_STAT_LETS_SPLIT_UP			"LetsSplitUp"
#define LEADERBOARD_STAT_CHAD_IS_A_DICK			"ChadIsADick"
#define LEADERBOARD_STAT_PHD_IN_MURDER			"APhDInMurder"
#define LEADERBOARD_STAT_PLAYER_LEVEL			"PlayerLevel"
#define LEADERBOARD_STAT_SOLO_FLIRT_SURVIVES	"SoloFlirtSurvives"
#define LEADERBOARD_STAT_TEDDY_BEARS_SEEN		"TeddyBearsSeen"

}