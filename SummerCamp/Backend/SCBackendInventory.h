// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#pragma once

#include "SCBackendPlayer.h"
#include "SCCharacterSelectionsSaveGame.h"
#include "SCBackendInventory.generated.h"

class ASCContextKillActor;
class ASCCounselorCharacter;
class ASCKillerCharacter;

/**
 * @struct FSCCounselorCharacterSaveHelper
 */
USTRUCT()
struct FSCCounselorCharacterProfileSaveHelper
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FSCCounselorCharacterProfile> CounselorCharacterProfiles;
};

/**
 * @struct FSCKillerCharacterProfileSaveHelper
 */
USTRUCT()
struct FSCKillerCharacterProfileSaveHelper
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FSCKillerCharacterProfile> KillerCharacterProfiles;
};

DECLARE_DELEGATE_OneParam(FSCBackendProfileLoadedDelegate, bool);

/**
 * @class USCBackendInventory
 */
UCLASS(Blueprintable, Within=SCBackendPlayer)
class SUMMERCAMP_API USCBackendInventory
: public UObject
{
	GENERATED_UCLASS_BODY()

	/** */
	void SetCounselorProfiles(TArray<FSCCounselorCharacterProfile> _CounselorCharacterProfiles) { CounselorCharacterProfiles = _CounselorCharacterProfiles; }

	/** */
	TArray<FSCCounselorCharacterProfile> GetConselorProfiles() { return CounselorCharacterProfiles; }

	/** */
	void SetKillerProfiles(TArray<FSCKillerCharacterProfile> _KillerCharacterProfiles) { KillerCharacterProfiles = _KillerCharacterProfiles; }

	/** */
	TArray<FSCKillerCharacterProfile> GetKillerProfiles() { return KillerCharacterProfiles; }

	/** */
	UPROPERTY()
	bool bIsDirty;

private:
	/** */
	UPROPERTY()
	TArray<FSCCounselorCharacterProfile> CounselorCharacterProfiles;

	/** */
	UPROPERTY()
	TArray<FSCKillerCharacterProfile> KillerCharacterProfiles;
};
