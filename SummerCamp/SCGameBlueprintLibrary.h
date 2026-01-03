// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "ILLGameBlueprintLibrary.h"
#include "SCPerkData.h"
#include "SCWorldSettings.h"
#include "SCConversationManager.h"
#include "SCStatsAndScoringData.h"
#include "SCGameBlueprintLibrary.generated.h"

class AILLPlayerController;

class ASCCharacter;
class ASCContextKillActor;
class ASCCounselorCharacter;
class ASCKillerCharacter;

class USCCounselorWardrobe;
class USCJasonSkin;

/**
 * @class USCGameBlueprintLibrary
 */
UCLASS()
class SUMMERCAMP_API USCGameBlueprintLibrary
: public UILLGameBlueprintLibrary
{
	GENERATED_BODY()

	//////////////////////////////////////////////////
	// Manager Accessors
public:
	UFUNCTION(BlueprintPure, Category = "Character UI", meta=(WorldContext="WorldContextObject"))
	static USCCounselorWardrobe* GetCounselorWardrobe(UObject* WorldContextObject);

	//////////////////////////////////////////////////
	// Utilities
public:
	/** Converts a soft class pointer from a CharacterClass to a KillerClass provided that the soft object path includes "Characters/Killers" */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	static TSoftClassPtr<ASCKillerCharacter> CastAsKillerCharacter(TSoftClassPtr<ASCCharacter> CharacterClass, bool& bSuccess);

	/** Converts a soft class pointer from a CharacterClass to a CounselorClass provided that the soft object path includes "Characters/Counselors" */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	static TSoftClassPtr<ASCCounselorCharacter> CastAsCounselorCharacter(TSoftClassPtr<ASCCharacter> CharacterClass, bool& bSuccess);

	/** Checks if this game is running the Japanese SKU (only ever true on PS4) */
	UFUNCTION(BlueprintPure, Category = "Utility", meta=(WorldContext="WorldContextObject"))
	static bool IsJapaneseSKU(const UObject* WorldContextObject);

	//////////////////////////////////////////////////
	// Character
public:
	/** @return Primary name for CharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	static FText GetCharacterPrimaryName(TSoftClassPtr<ASCCharacter> CharacterClass);

	/** @return Trope name for CharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	static FText GetCharacterTropeName(TSoftClassPtr<ASCCharacter> CharacterClass);

	/** @return Profile image for CharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	static UTexture2D* GetCharacterLargeImage(TSoftClassPtr<ASCCharacter> CharacterClass);

	/** @return Level at which CharacterClass unlocks. */
	UFUNCTION(BlueprintCallable, Category = "Character UI")
	static int32 GetCharacterUnlockLevel(TSoftClassPtr<ASCCharacter> CharacterClass);

	/** @return true if CharacterClass is locked out for Player. */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	static bool IsCharacterLocked(AILLPlayerController* Player, TSoftClassPtr<ASCCharacter> CharacterClass);

	/** @return true if CharacterClass is locked out for Player. */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	static bool IsCharacterLockedForPlayer(ASCPlayerState* PlayerState, TSoftClassPtr<ASCCharacter> CharacterClass);

	/** @return Unlock Entitlement string required for CharacterClass */
	UFUNCTION(BlueprintPure, Category = "Character UI")
	static FString GetUnlockEntitlement(TSoftClassPtr<ASCCharacter> CharacterClass);

	/** Apply custom Score event */
	UFUNCTION(BlueprintCallable, Category = "Character Scoring")
	static void AddEXPEvent(class ASCPlayerState* PlayerState, TSubclassOf<USCScoringEvent> ScoreEventClass);
	
	//////////////////////////////////////////////////
	// Character: Counselor
public:
	/** @return Name for CounselorClass ability. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorAbilityName(TSoftClassPtr<ASCCounselorCharacter> CounselorClass);

	/** @return Description for CounselorClass ability. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorAbilityDescription(TSoftClassPtr<ASCCounselorCharacter> CounselorClass);

	/** @return Icon for CounselorClass ability. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static UTexture2D* GetCounselorAbilityIcon(TSoftClassPtr<ASCCounselorCharacter> CounselorClass);

	/** @return Name for PerkBackendData. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorPerkName(FSCPerkBackendData PerkBackendData);

	/** @return Description for the perk. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorPerkDescription(FSCPerkBackendData PerkBackendData);

	/** @return Description for positive perk modifer. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorPerkPositiveDescription(FSCPerkBackendData PerkBackendData);

	/** @return Description for negative perk modifer. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorPerkNegativeDescription(FSCPerkBackendData PerkBackendData);

	/** @return Description for legendary perk modifer. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static FText GetCounselorPerkLegendaryDescription(FSCPerkBackendData PerkBackendData);

	/** @return Name for PerkBackendData. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static UTexture2D* GetCounselorPerkIcon(FSCPerkBackendData PerkBackendData);

	/** @return Stat value for CounselorClass' Stat. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Counselor")
	static uint8 GetCounselorStatAmount(TSoftClassPtr<ASCCounselorCharacter> CounselorClass, const ECounselorStats Stat);

	//////////////////////////////////////////////////
	// Character: Killer
public:
	/** @return true if GrabKillClass is compatible with KillerCharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static bool CanKillerUseGrabKill(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass, TSubclassOf<ASCContextKillActor> GrabKillClass);

	/** @return Cost for GrabKillClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static int32 GetGrabKillCost(TSubclassOf<ASCContextKillActor> GrabKillClass);

	/** @return Level requirement for GrabKillClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static int32 GetGrabKillLevel(TSubclassOf<ASCContextKillActor> GrabKillClass);

	/** @return Description for GrabKillClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static FText GetGrabKillDescription(TSubclassOf<ASCContextKillActor> GrabKillClass);

	/** @return Name for GrabKillClass.  */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static FText GetGrabKillName(TSubclassOf<ASCContextKillActor> GrabKillClass);

	/** @return Thumnail image for GrabKillClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static UTexture2D* GetGrabKillImage(TSubclassOf<ASCContextKillActor> GrabKillClass, bool bLargeIcon = true);

	/** @return Weapon name for KillerCharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static FText GetKillerWeaponName(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

	/** @return Weapon name for KillerCharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static UTexture2D* GetKillerWeaponThumbnail(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

	/** @return Strengths for KillerCharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> GetKillerStrengths(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

	/** @return Weaknesses for KillerCharacterClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static TArray<TSubclassOf<USCStengthOrWeaknessDataAsset>> GetKillerWeaknesses(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

	/** @return Default weapon class for KillerCharacterClass */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static TSoftClassPtr<ASCWeapon> GetKillerDefaultWeaponClass(TSoftClassPtr<ASCKillerCharacter> KillerCharacterClass);

	/** @return Title for StrengthWeaknessClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static FText GetKillerStrengthWeaknessTitle(TSubclassOf<USCStengthOrWeaknessDataAsset> StrengthWeaknessClass);

	/** @return Description for StrengthWeaknessClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static FText GetKillerStrengthWeaknessDescription(TSubclassOf<USCStengthOrWeaknessDataAsset> StrengthWeaknessClass);

	/** @return Description for StrengthWeaknessClass. */
	UFUNCTION(BlueprintPure, Category = "Character UI|Killer")
	static UTexture2D* GetKillerStrengthWeaknessIcon(TSubclassOf<USCStengthOrWeaknessDataAsset> StrengthWeaknessClass);

	/** Gets the default character and mask meshes based on the passed in class */
	UFUNCTION(BlueprintPure, Category = "Killer")
	static void GetKillerDefaultMeshAndMask(const TSubclassOf<ASCKillerCharacter>& KillerClass, USkeletalMesh*& CharacterMesh, USkeletalMesh*& MaskMesh);

	/** Applies the skin override to the passed in killer character and mask meshes */
	UFUNCTION(BlueprintCallable, Category = "Killer", meta=(WorldContext="WorldContextObject"))
	static void ApplySkinToKillerMeshAndMask(UObject* WorldContextObject, const TSubclassOf<USCJasonSkin>& NewSkin, USkeletalMeshComponent* CharacterMesh, USkeletalMeshComponent* MaskMesh);

	/** Gets the static mesh for the picked killer weapon from the player state */
	UFUNCTION(BlueprintPure, Category = "Killer")
	static UStaticMesh* GetPickedKillerWeapon(ASCPlayerState* KillerPlayerState);

	//////////////////////////////////////////////////
	// Voice overs
public:
	UFUNCTION(BlueprintCallable, Category = "Music")
	static bool PlayMusic(ASCCharacter* Character, FName MusicName);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	static bool PlayVoiceOver(ASCCounselorCharacter* CounselorCharacter, FName VoiceOverLine);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	static bool PlayVoiceOverBySoundCue(ASCCounselorCharacter* CounselorCharacter, USoundCue *VoiceOverCue);

	/** Attempt to play a conversation and return the newly created Conversation object ***CAN BE NULL*** */
	UFUNCTION(BlueprintCallable, Category = "Voice Over", meta = (WorldContext = "WorldContextObject"))
	static USCConversation* PlayConversation(UObject* WorldContextObject, FSCConversationData ConversationData);

	/** Attempt to play Voice Over for a character that could potentially be in a conversation. Returns true if VO was played */
	UFUNCTION(BlueprintCallable, Category = "Voice Over", meta=(WorldContext="WorldContextObject", AdvancedDisplay="bAbortConversation"))
	static bool PlayConversationBiasedVoiceOver(UObject* WorldContextObject, ASCCounselorCharacter* CounselorCharacter, USoundCue* VoiceOverCue, bool bAbortConversation = false);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	static bool StopVoiceOver(ASCCounselorCharacter* CounselorCharacter);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	static bool IsVoiceOverPlaying(ASCCounselorCharacter* CounselorCharacter, FName VoiceOverLine);

	UFUNCTION(BlueprintCallable, Category = "Voice Over")
	static bool PlayPamelaVoiceOver(ASCKillerCharacter* KillerCharacter, FName PamelaVOLine);

	/** Returns an int value for the index in the array that is of the same class as our TestClass, comparison is equality not child of. -1 is default */
	UFUNCTION(BlueprintCallable, Category = "Something")
	static int32 SwitchOnClass(TSubclassOf<UObject> TestClass, TArray<TSubclassOf<UObject>> SwitchCases);

	//////////////////////////////////////////////////
	// Debug
public:

	/** Print the current callstack */
	UFUNCTION(BlueprintCallable, Category = "Debug", meta=(WorldContext="WorldContextObject"))
	static void PrintCallstack(UObject* WorldContextObject);

public:
	//Additions To Support Latest Pak
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static bool SetSubtitleLanguage(FString& Language);
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static void SetSubtitleFontSize(int FontSize);
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static bool IsVoiceMenuSupported();
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static bool IsVoiceLanguageJapanese(UObject* WorldContextObject);
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static bool IsLanguageMenuSupported();
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static bool IsChineseSKU(UObject* WorldContextObject);
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static TArray<FText> GetVoiceLanguageText();
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static TArray<FString> GetVoiceLanguages();
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static TArray<FText> GetSubtitleLanguageText();
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static TArray<FString> GetSubtitleLanguages();
	UFUNCTION(BlueprintCallable, Category = "Additions")
	static int GetSubtitleFontSize();
};
