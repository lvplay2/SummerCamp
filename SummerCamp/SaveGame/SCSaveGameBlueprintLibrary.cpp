// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCSaveGameBlueprintLibrary.h"

#include "ILLPlayerController.h"
#include "SCInputMappingsSaveGame.h"

#include "SCLocalPlayer.h"

USCSaveGameBlueprintLibrary::USCSaveGameBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

TArray<FSCCounselorCharacterProfile> USCSaveGameBlueprintLibrary::GetCounselorProfiles(AILLPlayerController* PlayerController)
{
	TArray<FSCCounselorCharacterProfile> CounselorProfiles;
	if (PlayerController)
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				return CounselorProfiles = Settings->CounselorCharacterProfiles;
			}
		}
	}

	return CounselorProfiles;
}


FSCCounselorCharacterProfile USCSaveGameBlueprintLibrary::GetCounselorProfile(AILLPlayerController* PlayerController, TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	for (const auto CounselorProfile : GetCounselorProfiles(PlayerController))
	{
		if (CounselorProfile.CounselorCharacterClass == CounselorClass)
			return CounselorProfile;
	}

	FSCCounselorCharacterProfile Default;
	Default.CounselorCharacterClass = CounselorClass;
	return Default;
}



void USCSaveGameBlueprintLibrary::SaveCounselorProfile(AILLPlayerController* PlayerController, const FSCCounselorCharacterProfile& CounselorProfile)
{
	if (PlayerController)
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			if (USCCharacterSelectionsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCCharacterSelectionsSaveGame>())
			{
				for (auto& Profile : Settings->CounselorCharacterProfiles)
				{
					if (Profile.CounselorCharacterClass == CounselorProfile.CounselorCharacterClass)
					{
						Profile = CounselorProfile;
						break;
					}
				}
			}
		}
	}
}

TArray<TSubclassOf<USCEmoteData>> USCSaveGameBlueprintLibrary::GetEmotesForCounselor(AILLPlayerController* PlayerController, TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	FSCCounselorCharacterProfile Profile = GetCounselorProfile(PlayerController, CounselorClass);
	if (Profile.Emotes.Num() <= 0)
	{
		CounselorClass.LoadSynchronous(); // TODO: Make Async
		if (ASCCounselorCharacter* Counselor = Cast<ASCCounselorCharacter>(CounselorClass.Get()->GetDefaultObject()))
		{
			return Counselor->DefaultEmotes;
		}
	}

	return Profile.Emotes;
}

bool USCSaveGameBlueprintLibrary::HasCustomKeybinding(AILLPlayerController* PlayerController)
{
	if (PlayerController)
	{
		if (USCLocalPlayer* LocalPlayer = Cast<USCLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			if (USCInputMappingsSaveGame* Settings = LocalPlayer->GetLoadedSettingsSave<USCInputMappingsSaveGame>())
			{
				return Settings->HasCustomKeybinds();
			}
		}
	}

	return false;
}
