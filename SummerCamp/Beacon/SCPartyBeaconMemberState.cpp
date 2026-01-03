// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPartyBeaconMemberState.h"

#include "OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"

#include "SCBackendRequestManager.h"
#include "SCGameInstance.h"
#include "SCPlayerState.h"

ASCPartyBeaconMemberState::ASCPartyBeaconMemberState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ASCPartyBeaconMemberState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPartyBeaconMemberState, TotalExperience);
	DOREPLIFETIME(ASCPartyBeaconMemberState, PlayerLevel);
	DOREPLIFETIME(ASCPartyBeaconMemberState, XPAmountToCurrentLevel);
	DOREPLIFETIME(ASCPartyBeaconMemberState, XPAmountToNextLevel);
}

UTexture2D* ASCPartyBeaconMemberState::GetAvatar()
{
	if (!CachedAvatar)
	{
		if (IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get())
		{
			auto IdentityInt = OnlineSub->GetIdentityInterface();
			if (IdentityInt.IsValid() && UniqueId.IsValid())
			{
				// FIXME: pjackson: Refactor
			//	CachedAvatar = IdentityInt->GetAvatar(*UniqueId);
			}
		}
	}

	return CachedAvatar;
}

void ASCPartyBeaconMemberState::UpdateBackendProfileFrom(ASCPlayerState* PlayerState)
{
	if (PlayerState)
	{
		TotalExperience = PlayerState->GetTotalExperience();
		PlayerLevel = PlayerState->GetPlayerLevel();
		XPAmountToCurrentLevel = PlayerState->GetXPAmountToCurrentLevel();
		XPAmountToNextLevel = PlayerState->GetXPAmountToNextLevel();
	}
}

void ASCPartyBeaconMemberState::RequestBackendProfileUpdate()
{
	if (Role == ROLE_Authority && AccountId.IsValid())
	{
		USCGameInstance* GI = Cast<USCGameInstance>(GetGameInstance());
		USCBackendRequestManager* RequestManager = GI->GetBackendRequestManager<USCBackendRequestManager>();
		RequestManager->GetPlayerAccountProfile(AccountId, FOnILLBackendSimpleRequestDelegate::CreateUObject(this, &ThisClass::OnRetrieveBackendProfile));
	}
}

float ASCPartyBeaconMemberState::GetCurrentLevelPercentage() const
{
	if (TotalExperience > 0)
	{
		const double ExperienceIntoLevel = TotalExperience - XPAmountToCurrentLevel;
		const double TotalExperienceForLevel = (TotalExperience + XPAmountToNextLevel) - XPAmountToCurrentLevel;
		if (TotalExperienceForLevel > FLT_EPSILON)
		{
			return (float)(ExperienceIntoLevel / TotalExperienceForLevel);
		}
	}

	return 0.f;
}

void ASCPartyBeaconMemberState::OnRep_AccountId()
{
	Super::OnRep_AccountId();

	RequestBackendProfileUpdate();
}

void ASCPartyBeaconMemberState::OnRetrieveBackendProfile(int32 ResponseCode, const FString& ResponseContents)
{
	if (ResponseCode != 200)
	{
		UE_LOG(LogSC, Warning, TEXT("OnRetrieveBackendProfile: Code %i"), ResponseCode);
		return;
	}

	// Parse Json
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContents);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Get Account Object
		const TSharedPtr<FJsonObject>* JAccountObj;
		if (JsonObject->TryGetObjectField(TEXT("Account"), JAccountObj))
		{
			// Get Experience and Player level info from the Profile
			const TSharedPtr<FJsonObject>* JExpObj;
			if ((*JAccountObj)->TryGetObjectField(TEXT("ExperienceProfile"), JExpObj))
			{
				(*JExpObj)->TryGetNumberField(TEXT("TotalExperience"), TotalExperience);
				(*JExpObj)->TryGetNumberField(TEXT("Level"), PlayerLevel);
				(*JExpObj)->TryGetNumberField(TEXT("ToCurrentLevel"), XPAmountToCurrentLevel);
				(*JExpObj)->TryGetNumberField(TEXT("ToNextLevel"), XPAmountToNextLevel);
			}
		}
	}
}
