// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCGameBlueprintLibrary.h"

#include "OnlineSubsystemUtils.h"

#include "ILLPlayerController.h"

#include "SCCounselorCharacter.h"
#include "SCGameInstance.h"
#include "SCKillerCharacter.h"
#include "SCMusicComponent.h"
#include "SCPlayerState.h"
#include "SCVoiceOverComponent.h"
#include "SCWeapon.h"
#include "SCGameState.h"

USCCounselorWardrobe* USCGameBlueprintLibrary::GetCounselorWardrobe(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	USCGameInstance* GameInstance = Cast<USCGameInstance>(World ? World->GetGameInstance() : nullptr);
	return GameInstance ? GameInstance->GetCounselorWardrobe() : nullptr;
}

TSoftClassPtr<ASCKillerCharacter> USCGameBlueprintLibrary::CastAsKillerCharacter(TSoftClassPtr<ASCCharacter> CharacterClass, bool& bSuccess)
{
	if (CharacterClass.ToSoftObjectPath().ToString().Contains(TEXT("Characters/Killers")))
	{
		bSuccess = true;
		return TSoftClassPtr<ASCKillerCharacter>(CharacterClass.ToSoftObjectPath());
	}

	bSuccess = false;
	return nullptr;
}

TSoftClassPtr<ASCCounselorCharacter> USCGameBlueprintLibrary::CastAsCounselorCharacter(TSoftClassPtr<ASCCharacter> CharacterClass, bool& bSuccess)
{
	if (CharacterClass.ToSoftObjectPath().ToString().Contains(TEXT("Characters/Counselors")))
	{
		bSuccess = true;
		return TSoftClassPtr<ASCCounselorCharacter>(CharacterClass.ToSoftObjectPath());
	}

	bSuccess = false;
	return nullptr;
}

FText USCGameBlueprintLibrary::GetCharacterPrimaryName(TSoftClassPtr<ASCCharacter> CharacterClass)
{
	CharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCharacter* const DefaultCharacter = CharacterClass.Get() ? CharacterClass.Get()->GetDefaultObject<ASCCharacter>() : nullptr)
	{
		return DefaultCharacter->GetCharacterName();
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetCharacterTropeName(TSoftClassPtr<ASCCharacter> CharacterClass)
{
	CharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCharacter* const DefaultCharacter = CharacterClass.Get() ? CharacterClass.Get()->GetDefaultObject<ASCCharacter>() : nullptr)
	{
		return DefaultCharacter->GetCharacterTropeName();
	}

	return FText();
}

UTexture2D* USCGameBlueprintLibrary::GetCharacterLargeImage(TSoftClassPtr<ASCCharacter> CharacterClass)
{
	CharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCharacter* const DefaultCharacter = CharacterClass.Get() ? CharacterClass.Get()->GetDefaultObject<ASCCharacter>() : nullptr)
	{
		return DefaultCharacter->GetLargeImage();
	}

	return nullptr;
}

int32 USCGameBlueprintLibrary::GetCharacterUnlockLevel(TSoftClassPtr<ASCCharacter> CharacterClass)
{
	CharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCharacter* const DefaultCharacter = CharacterClass.Get() ? CharacterClass.Get()->GetDefaultObject<ASCCharacter>() : nullptr)
	{
		return DefaultCharacter->UnlockLevel;
	}

	return 0;
}

bool USCGameBlueprintLibrary::IsCharacterLocked(AILLPlayerController* Player, TSoftClassPtr<ASCCharacter> CharacterClass)
{
	if (ASCPlayerState* PlayerState = Player ? Cast<ASCPlayerState>(Player->PlayerState) : nullptr)
	{
		return IsCharacterLockedForPlayer(PlayerState, CharacterClass);
	}

	return true;
}

bool USCGameBlueprintLibrary::IsCharacterLockedForPlayer(ASCPlayerState* PlayerState, TSoftClassPtr<ASCCharacter> CharacterClass)
{
	CharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCharacter* const DefaultCharacter = CharacterClass.Get() ? CharacterClass.Get()->GetDefaultObject<ASCCharacter>() : nullptr)
	{
		if (PlayerState && PlayerState->GetPlayerLevel() >= DefaultCharacter->UnlockLevel)
		{
			return false;
		}
	}

	return true;
}


bool USCGameBlueprintLibrary::IsJapaneseSKU(const UObject* WorldContextObject)
{
#if PLATFORM_PS4
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	IOnlineSubsystem* OSS = Online::GetSubsystem(World);
	const FString AppID = OSS ? OSS->GetAppId() : TEXT("");
	if (AppID == TEXT("CUSA11523_00")) // Japanese Disc SKU
		return true;
	return false;
#else
	return false;
#endif
}

FString USCGameBlueprintLibrary::GetUnlockEntitlement(TSoftClassPtr<ASCCharacter> CharacterClass)
{
	CharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCharacter* const DefaultCharacter = CharacterClass.Get() ? CharacterClass.Get()->GetDefaultObject<ASCCharacter>() : nullptr)
	{
		return DefaultCharacter->UnlockEntitlement;
	}

	return TEXT("");
}

void USCGameBlueprintLibrary::AddEXPEvent(ASCPlayerState* PlayerState, TSubclassOf<USCScoringEvent> ScoreEventClass)
{
	if (ScoreEventClass && PlayerState && PlayerState->Role == ROLE_Authority)
		PlayerState->HandleScoreEvent(ScoreEventClass);
}

FText USCGameBlueprintLibrary::GetCounselorAbilityName(TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	CounselorClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCounselorCharacter* const DefaultCounselor = CounselorClass.Get() ? CounselorClass.Get()->GetDefaultObject<ASCCounselorCharacter>() : nullptr)
	{
		if (USCCounselorActiveAbility* ActiveAbility = DefaultCounselor->ActiveAbilityClass ? DefaultCounselor->ActiveAbilityClass->GetDefaultObject<USCCounselorActiveAbility>() : nullptr)
		{
			return ActiveAbility->AbilityName;
		}
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetCounselorAbilityDescription(TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	CounselorClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCounselorCharacter* const DefaultCounselor = CounselorClass.Get() ? CounselorClass.Get()->GetDefaultObject<ASCCounselorCharacter>() : nullptr)
	{
		if (USCCounselorActiveAbility* ActiveAbility = DefaultCounselor->ActiveAbilityClass ? DefaultCounselor->ActiveAbilityClass->GetDefaultObject<USCCounselorActiveAbility>() : nullptr)
		{
			return ActiveAbility->AbilityDiscription;
		}
	}

	return FText();
}

UTexture2D* USCGameBlueprintLibrary::GetCounselorAbilityIcon(TSoftClassPtr<ASCCounselorCharacter> CounselorClass)
{
	CounselorClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCounselorCharacter* const DefaultCounselor = CounselorClass.Get() ? CounselorClass.Get()->GetDefaultObject<ASCCounselorCharacter>() : nullptr)
	{
		if (USCCounselorActiveAbility* ActiveAbility = DefaultCounselor->ActiveAbilityClass ? DefaultCounselor->ActiveAbilityClass->GetDefaultObject<USCCounselorActiveAbility>() : nullptr)
		{
			return ActiveAbility->UIIcon;
		}
	}

	return nullptr;
}

FText USCGameBlueprintLibrary::GetCounselorPerkName(FSCPerkBackendData PerkBackendData)
{
	if (const USCPerkData* const DefaultPerk = PerkBackendData.PerkClass ? PerkBackendData.PerkClass->GetDefaultObject<USCPerkData>() : nullptr)
	{
		return DefaultPerk->PerkName;
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetCounselorPerkDescription(FSCPerkBackendData PerkBackendData)
{
	if (const USCPerkData* const DefaultPerk = PerkBackendData.PerkClass ? PerkBackendData.PerkClass->GetDefaultObject<USCPerkData>() : nullptr)
	{
		return DefaultPerk->Description;
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetCounselorPerkPositiveDescription(FSCPerkBackendData PerkBackendData)
{
	if (const USCPerkData* const DefaultPerk = PerkBackendData.PerkClass ? PerkBackendData.PerkClass->GetDefaultObject<USCPerkData>() : nullptr)
	{
		return DefaultPerk->PositiveDescription;
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetCounselorPerkNegativeDescription(FSCPerkBackendData PerkBackendData)
{
	if (const USCPerkData* const DefaultPerk = PerkBackendData.PerkClass ? PerkBackendData.PerkClass->GetDefaultObject<USCPerkData>() : nullptr)
	{
		return DefaultPerk->NegativeDescription;
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetCounselorPerkLegendaryDescription(FSCPerkBackendData PerkBackendData)
{
	if (const USCPerkData* const DefaultPerk = PerkBackendData.PerkClass ? PerkBackendData.PerkClass->GetDefaultObject<USCPerkData>() : nullptr)
	{
		return DefaultPerk->LegendaryDescription;
	}

	return FText();
}

UTexture2D* USCGameBlueprintLibrary::GetCounselorPerkIcon(FSCPerkBackendData PerkBackendData)
{
	if (const USCPerkData* const DefaultPerk = PerkBackendData.PerkClass ? PerkBackendData.PerkClass->GetDefaultObject<USCPerkData>() : nullptr)
	{
		return DefaultPerk->LoadoutIcon;
	}

	return nullptr;
}

uint8 USCGameBlueprintLibrary::GetCounselorStatAmount(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, const ECounselorStats Stat)
{
	CounselorClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCCounselorCharacter* const DefaultCounselor = CounselorClass.Get() ? CounselorClass.Get()->GetDefaultObject<ASCCounselorCharacter>() : nullptr)
	{
		return DefaultCounselor->Stats[uint8(Stat)];
	}

	return 0;
}

bool USCGameBlueprintLibrary::CanKillerUseGrabKill(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass, TSubclassOf<ASCContextKillActor> GrabKillClass)
{
	if (!KillerCharacterClass.IsNull() && GrabKillClass)
	{
		if (const ASCContextKillActor* const ContextKill = GrabKillClass->GetDefaultObject<ASCContextKillActor>())
		{
			if (ContextKill->GetContextWeaponType() == nullptr)
				return true;

			KillerCharacterClass.LoadSynchronous();
			if (const ASCKillerCharacter* const KillerCharacter = KillerCharacterClass.Get()->GetDefaultObject<ASCKillerCharacter>())
			{
				if (TSoftClassPtr<ASCWeapon> WeaponClass = KillerCharacter->GetDefaultWeaponClass())
				{
					if (ContextKill->GetContextWeaponType() == WeaponClass.Get())
						return true;
				}
			}
		}
	}

	return false;
}

int32 USCGameBlueprintLibrary::GetGrabKillCost(TSubclassOf<ASCContextKillActor> GrabKillClass)
{
	if (const ASCContextKillActor* const GrabKill = GrabKillClass ? GrabKillClass->GetDefaultObject<ASCContextKillActor>() : nullptr)
	{
		return GrabKill->GetCost();
	}

	return 0;
}

int32 USCGameBlueprintLibrary::GetGrabKillLevel(TSubclassOf<ASCContextKillActor> GrabKillClass)
{
	if (const ASCContextKillActor* const GrabKill = GrabKillClass ? GrabKillClass->GetDefaultObject<ASCContextKillActor>() : nullptr)
	{
		return GrabKill->LevelRequirement;
	}

	return 0;
}

FText USCGameBlueprintLibrary::GetGrabKillDescription(TSubclassOf<ASCContextKillActor> GrabKillClass)
{
	if (const ASCContextKillActor* const GrabKill = GrabKillClass ? GrabKillClass->GetDefaultObject<ASCContextKillActor>() : nullptr)
	{
		return GrabKill->GetDescription();
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetGrabKillName(TSubclassOf<ASCContextKillActor> GrabKillClass)
{
	if (const ASCContextKillActor* const GrabKill = GrabKillClass ? GrabKillClass->GetDefaultObject<ASCContextKillActor>() : nullptr)
	{
		return GrabKill->GetFriendlyName();
	}

	return FText();
}

UTexture2D* USCGameBlueprintLibrary::GetGrabKillImage(TSubclassOf<ASCContextKillActor> GrabKillClass, bool bLargeIcon/* = true*/)
{
	if (const ASCContextKillActor* const GrabKill = GrabKillClass ? GrabKillClass->GetDefaultObject<ASCContextKillActor>() : nullptr)
	{
		return bLargeIcon ? GrabKill->GetContextKillMenuIcon() : GrabKill->GetContextKillHUDIcon();
	}

	return nullptr;
}

FText USCGameBlueprintLibrary::GetKillerWeaponName(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	KillerCharacterClass.LoadSynchronous(); // TODO: Make Async
	if (ASCKillerCharacter* KillerCharacter = KillerCharacterClass.Get() ? KillerCharacterClass.Get()->GetDefaultObject<ASCKillerCharacter>() : nullptr)
	{
		TSoftClassPtr<ASCWeapon> WeaponClass = KillerCharacter->GetDefaultWeaponClass();
		WeaponClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCWeapon* const Weapon = WeaponClass.Get() ? WeaponClass.Get()->GetDefaultObject<ASCWeapon>() : nullptr)
		{
			return Weapon->GetFriendlyName();
		}
	}

	return FText();
}

UTexture2D* USCGameBlueprintLibrary::GetKillerWeaponThumbnail(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	KillerCharacterClass.LoadSynchronous(); // TODO: Make Async
	if (ASCKillerCharacter* KillerCharacter = KillerCharacterClass.Get() ? KillerCharacterClass.Get()->GetDefaultObject<ASCKillerCharacter>() : nullptr)
	{
		TSoftClassPtr<ASCWeapon> WeaponClass = KillerCharacter->GetDefaultWeaponClass();
		WeaponClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCWeapon* const Weapon = WeaponClass.Get() ? WeaponClass.Get()->GetDefaultObject<ASCWeapon>() : nullptr)
		{
			return Weapon->GetThumbnailImage();
		}
	}

	return nullptr;
}

TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> USCGameBlueprintLibrary::GetKillerStrengths(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	KillerCharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCKillerCharacter* const DefaultKiller = KillerCharacterClass.Get() ? KillerCharacterClass.Get()->GetDefaultObject<ASCKillerCharacter>() : nullptr)
	{
		return DefaultKiller->GetStrengths();
	}

	return TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>>();
}

TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> USCGameBlueprintLibrary::GetKillerWeaknesses(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	KillerCharacterClass.LoadSynchronous(); // TODO: Make Async
	if (const ASCKillerCharacter* const DefaultKiller = KillerCharacterClass.Get() ? KillerCharacterClass.Get()->GetDefaultObject<ASCKillerCharacter>() : nullptr)
	{
		return DefaultKiller->GetWeaknesses();
	}

	return TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>>();
}

TSoftClassPtr<ASCWeapon> USCGameBlueprintLibrary::GetKillerDefaultWeaponClass(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass)
{
	if (!KillerCharacterClass.IsNull())
	{
		KillerCharacterClass.LoadSynchronous(); // TODO: Make Async
		if (const ASCKillerCharacter* const Killer = KillerCharacterClass.Get() ? KillerCharacterClass.Get()->GetDefaultObject<ASCKillerCharacter>() : nullptr)
			return Killer->GetDefaultWeaponClass();
	}

	return nullptr;
}

FText USCGameBlueprintLibrary::GetKillerStrengthWeaknessTitle(TSubclassOf<USCStengthOrWeaknessDataAsset> StrengthWeaknessClass)
{
	if (USCStengthOrWeaknessDataAsset* StrengthWeakness = StrengthWeaknessClass ? StrengthWeaknessClass->GetDefaultObject<USCStengthOrWeaknessDataAsset>() : nullptr)
	{
		return StrengthWeakness->Title;
	}

	return FText();
}

FText USCGameBlueprintLibrary::GetKillerStrengthWeaknessDescription(TSubclassOf<USCStengthOrWeaknessDataAsset> StrengthWeaknessClass)
{
	if (USCStengthOrWeaknessDataAsset* StrengthWeakness = StrengthWeaknessClass ? StrengthWeaknessClass->GetDefaultObject<USCStengthOrWeaknessDataAsset>() : nullptr)
	{
		return StrengthWeakness->Description;
	}

	return FText();
}

UTexture2D* USCGameBlueprintLibrary::GetKillerStrengthWeaknessIcon(TSubclassOf<USCStengthOrWeaknessDataAsset> StrengthWeaknessClass)
{
	if (USCStengthOrWeaknessDataAsset* StrengthWeakness = StrengthWeaknessClass ? StrengthWeaknessClass->GetDefaultObject<USCStengthOrWeaknessDataAsset>() : nullptr)
	{
		return StrengthWeakness->Icon;
	}

	return nullptr;
}

void USCGameBlueprintLibrary::GetKillerDefaultMeshAndMask(const TSubclassOf<ASCKillerCharacter>& KillerClass, USkeletalMesh*& CharacterMesh, USkeletalMesh*& MaskMesh)
{
	CharacterMesh = nullptr;
	MaskMesh = nullptr;
	if (!KillerClass)
		return;

	// Get the default meshes
	const ASCKillerCharacter* const KillerCharacter = KillerClass.GetDefaultObject();
	CharacterMesh = KillerCharacter->GetMesh()->SkeletalMesh;
	MaskMesh = KillerCharacter->GetKillerMaskMesh()->SkeletalMesh;
}

void USCGameBlueprintLibrary::ApplySkinToKillerMeshAndMask(UObject* WorldContextObject, const TSubclassOf<USCJasonSkin>& NewSkin, USkeletalMeshComponent* CharacterMesh, USkeletalMeshComponent* MaskMesh)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
		return;

	// Apply the skin!
	if (const USCJasonSkin* const Skin = Cast<USCJasonSkin>(NewSkin.GetDefaultObject()))
	{
		if (!Skin->MeshMaterial.IsNull())
		{
			Skin->MeshMaterial.LoadSynchronous(); // TODO: Make Async
			USCMaterialOverride::ApplyMaterialOverride(World, Cast<USCMaterialOverride>(Skin->MeshMaterial.Get()->GetDefaultObject()), CharacterMesh);
		}

		if (!Skin->MaskMaterial.IsNull())
		{
			Skin->MaskMaterial.LoadSynchronous(); // TODO: Make Async
			USCMaterialOverride::ApplyMaterialOverride(World, Cast<USCMaterialOverride>(Skin->MaskMaterial.Get()->GetDefaultObject()), MaskMesh);
		}
	}
}

UStaticMesh* USCGameBlueprintLibrary::GetPickedKillerWeapon(ASCPlayerState* KillerPlayerState)
{
	if (!IsValid(KillerPlayerState))
		return nullptr;

	TSoftClassPtr<ASCWeapon> KillerWeaponClass = nullptr;

	// Upgrade to a picked weapon from the player state (if available)
	if (!KillerPlayerState->GetKillerWeapon().IsNull())
	{
		KillerWeaponClass = KillerPlayerState->GetKillerWeapon();
	}
	// Grab the default weapon from the killer class
	else if (TSubclassOf<ASCCharacter> KillerClass = KillerPlayerState->GetActiveCharacterClass())
	{
		if (const ASCKillerCharacter* const Killer = Cast<const ASCKillerCharacter>(KillerClass.GetDefaultObject()))
		{
			KillerWeaponClass = Killer->GetDefaultWeaponClass();
		}
	}

	KillerWeaponClass.LoadSynchronous(); // TODO: Make Async

	// Validate
	const ASCWeapon* const KillerWeapon = KillerWeaponClass.Get() ? Cast<ASCWeapon>(KillerWeaponClass.Get()->GetDefaultObject()) : nullptr;
	if (!IsValid(KillerWeapon))
		return nullptr;

	// Grab that mesh!
	return KillerWeapon->GetMesh<UStaticMeshComponent>()->GetStaticMesh();
}

bool USCGameBlueprintLibrary::PlayMusic(class ASCCharacter* Character, FName MusicName)
{
	if (Character && Character->MusicComponent)
	{
		Character->MusicComponent->PlayMusic(MusicName);
		return true;
	}

	return false;
}

bool USCGameBlueprintLibrary::PlayVoiceOver(ASCCounselorCharacter* CounselorCharacter, FName VoiceOverLine)
{
	if (CounselorCharacter && CounselorCharacter->VoiceOverComponent)
	{
		CounselorCharacter->VoiceOverComponent->PlayVoiceOver(VoiceOverLine);

		return true;
	}

	return false;
}

bool USCGameBlueprintLibrary::PlayVoiceOverBySoundCue(ASCCounselorCharacter* CounselorCharacter, USoundCue *VoiceOverCue)
{
	if (CounselorCharacter && CounselorCharacter->VoiceOverComponent && VoiceOverCue)
	{
		CounselorCharacter->VoiceOverComponent->PlayVoiceOverBySoundCue(VoiceOverCue);

		return true;
	}

	return false;
}

USCConversation* USCGameBlueprintLibrary::PlayConversation(UObject* WorldContextObject, FSCConversationData ConversationData)
{
	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		if (ConversationData.VOData.Num() > 0)
		{
			if (ASCGameState* GS = World->GetGameState<ASCGameState>())
			{
				return GS->GetConversationManager()->PlayConversation(ConversationData);
			}
		}
	}
	return nullptr;
}

bool USCGameBlueprintLibrary::PlayConversationBiasedVoiceOver(UObject* WorldContextObject, ASCCounselorCharacter* CounselorCharacter, USoundCue* VoiceOverCue, bool bAbortConversation /*= false*/)
{
	if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
	{
		if (ASCGameState* GS = World->GetGameState<ASCGameState>())
		{
			return GS->GetConversationManager()->PlayVoiceLine(CounselorCharacter, VoiceOverCue, bAbortConversation);
		}
	}
	return false;
}

bool USCGameBlueprintLibrary::StopVoiceOver(ASCCounselorCharacter* CounselorCharacter)
{
	if (CounselorCharacter && CounselorCharacter->VoiceOverComponent)
	{
		CounselorCharacter->VoiceOverComponent->StopVoiceOver();

		return true;
	}

	return false;
}

bool USCGameBlueprintLibrary::IsVoiceOverPlaying(ASCCounselorCharacter* CounselorCharacter, FName VoiceOverLine)
{
	if (CounselorCharacter && CounselorCharacter->VoiceOverComponent)
	{
		return CounselorCharacter->VoiceOverComponent->IsVoiceOverPlaying(VoiceOverLine);
	}

	return false;
}

bool USCGameBlueprintLibrary::PlayPamelaVoiceOver(ASCKillerCharacter* KillerCharacter, FName PamelaVOLine)
{
	if (KillerCharacter && KillerCharacter->VoiceOverComponent)
	{
		KillerCharacter->VoiceOverComponent->PlayVoiceOver(PamelaVOLine);

		return true;
	}

	return false;
}

int32 USCGameBlueprintLibrary::SwitchOnClass(TSubclassOf<UObject> TestClass, TArray<TSubclassOf<UObject>> SwitchCases)
{
	for (int32 index = 0; index < SwitchCases.Num(); ++index)
	{
		TSubclassOf<UObject> Case = SwitchCases[index];
		if (TestClass == Case)
		{
			return index;
		}
	}

	return -1;
}

void USCGameBlueprintLibrary::PrintCallstack(UObject* WorldContextObject)
{

	const SIZE_T StackTraceSize = 65535;
	ANSICHAR* StackTrace = (ANSICHAR*)FMemory::Malloc(StackTraceSize);
	StackTrace[0] = 0;

	// Walk the stack and dump it to the allocated memory.
	FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, 0);
	UE_LOG(LogSC, Log, TEXT("%s"), ANSI_TO_TCHAR(StackTrace));
	FMemory::Free(StackTrace);
}

//Additions To Support Latest Pak

bool USCGameBlueprintLibrary::SetSubtitleLanguage(FString& Language) {
	return true;
}

void USCGameBlueprintLibrary::SetSubtitleFontSize(int FontSize) {

}

bool USCGameBlueprintLibrary::IsVoiceMenuSupported() {
	return false;
}

bool USCGameBlueprintLibrary::IsVoiceLanguageJapanese(UObject* WorldContextObject) {
	return false;
}

bool USCGameBlueprintLibrary::IsLanguageMenuSupported() {
	return false;
}

bool USCGameBlueprintLibrary::IsChineseSKU(UObject* WorldContextObject) {
	return false;
}

TArray<FText> USCGameBlueprintLibrary::GetVoiceLanguageText() {
	TArray<FText> voiceLanguageText;
	return voiceLanguageText;
}

TArray<FString> USCGameBlueprintLibrary::GetVoiceLanguages() {
	TArray<FString> voiceLanguages;
	return voiceLanguages;
}

TArray<FText> USCGameBlueprintLibrary::GetSubtitleLanguageText() {
	TArray<FText> subtitleLanguageText;
	return subtitleLanguageText;
}

TArray<FString> USCGameBlueprintLibrary::GetSubtitleLanguages() {
	TArray<FString> subtitleLanguages;
	return subtitleLanguages;
}

int USCGameBlueprintLibrary::GetSubtitleFontSize() {
	return 0;
}