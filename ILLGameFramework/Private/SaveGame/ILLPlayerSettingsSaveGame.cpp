// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLPlayerSettingsSaveGame.h"

#include "Misc/Compression.h"
#include "Misc/SHA256.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"

#include "ILLLocalPlayer.h"
#include "ILLPlayerController.h"

#define LOCTEXT_NAMESPACE "ILLFonic.IGF.ILLPlayerSettingsSaveGame"

// Magic int32 header used for identification
#ifndef ILLPLAYERSETTINGS_MAGIC_HEADER
# define ILLPLAYERSETTINGS_MAGIC_HEADER				0xC120F111
#endif

// Flags to use when compressing/decompressing
#ifndef ILLPLAYERSETTINGS_COMPRESSION_FLAGS
# define ILLPLAYERSETTINGS_COMPRESSION_FLAGS		(ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory)
#endif

// Maximum block size needed for PS4 player settings saves
#ifndef ILLPLAYERSETTINGS_PS4_MAX_SIZE
# define ILLPLAYERSETTINGS_PS4_MAX_SIZE				(4*1024*1024) // 4MB
#endif

// Salt appended to the hash performed on the body contents
// This can be an array of any size consisting of uint32s
#ifndef ILLPLAYERSETTINGS_BODY_HASH_SALT
# define ILLPLAYERSETTINGS_BODY_HASH_SALT			{ 0xca8a58c9, 0xf6452c7b, 0x9b360231, 0xe150e39b }
#endif

// Salt appended to the owner platform identifier before being hashed
// This can be an array of any size consisting of uint32s
#ifndef ILLPLAYERSETTINGS_OWNER_HASH_SALT
# define ILLPLAYERSETTINGS_OWNER_HASH_SALT			{ 0x16f7cc34, 0x42b9840a, 0xa590cbc4, 0xa2b3d30a }
#endif

// Slot name to use when player settings are saved/loaded
static FString SettingsSaveSlotName[static_cast<uint8>(EILLPlayerSettingsSaveType::MAX)];

// Title to use for player settings saves
static FText SettingsSaveTitle[static_cast<uint8>(EILLPlayerSettingsSaveType::MAX)];

/**
 * @enum FILLPlayerSettingsSaveGameFileVersion
 * @brief Version values for player settings save games.
 */
enum class FILLPlayerSettingsSaveGameFileVersion : uint8
{
	InitialVersion = 1,
	OwnerSalting = 2,
	BodyHashSalting = 3,
	SoftObjectPtrSupport = 4,
	SaveBodySizeBecausePS4IsAnAsshole = 5,

	// !!! New versions can be added above this line !!!
	LatestPlusOne,
	LatestVersion = LatestPlusOne-1,
	MinimumVersion = SaveBodySizeBecausePS4IsAnAsshole // Bump this if you want a settings reset for any older settings files
};

/**
 * @struct FSCSoftObjectPtrSupportProxyArchive
 * @brief Archive proxy to support saving of FSoftObjectPtr. Code stolen from FArchiveUObject.
 */
struct FSCSoftObjectPtrSupportProxyArchive
: public FArchiveProxy
{
	FSCSoftObjectPtrSupportProxyArchive(FArchive& InInnerArchive)
	: FArchiveProxy(InInnerArchive)
	{ }

	// Stolen from FArchiveUObject
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override
	{
		FArchive& Ar = *this;
		if (Ar.IsSaving() || Ar.IsLoading())
		{
			// Reset before serializing to clear the internal weak pointer. 
			Value.ResetWeakPtr();
			Ar << Value.GetUniqueID();
		}
		else if (!IsObjectReferenceCollector() || IsModifyingWeakAndStrongReferences())
		{
			// Treat this like a weak pointer object, as we are doing something like replacing references in memory
			UObject* Object = Value.Get();

			Ar << Object;

			if (IsLoading() || (Object && IsModifyingWeakAndStrongReferences()))
			{
				Value = Object;
			}
		}

		return Ar;
	}

	// Stolen from FArchiveUObject
	virtual FArchive& operator<<(FSoftObjectPath& Value) override
	{
		Value.SerializePath(*this);
		return *this;
	}
};

void ExtendedSaveGameInfo(const TCHAR* SaveName, const EGameDelegates_SaveGame Key, FString& Value)
{
	if (Key == EGameDelegates_SaveGame::MaxSize)
	{
		Value = FString::FromInt(ILLPLAYERSETTINGS_PS4_MAX_SIZE);
		return;
	}
	else if (Key == EGameDelegates_SaveGame::Icon)
	{
		// NOTE: If you want to support a custom save icon for PS4, add a 24-bit 228x128 PNG to the location bellow
		// and add "+DirectoriesToAlwaysStageAsNonUFS=(Path="Icons")" to DefaultGame.ini
		Value = TEXT("../../..") / FString(FApp::GetProjectName()) / TEXT("Content/Icons/save_data.png");
		return;
	}

	if (!SaveName || !SaveName[0])
		return;

	if (Key == EGameDelegates_SaveGame::Title)
	{
		for (uint8 Index = 0; Index < static_cast<uint8>(EILLPlayerSettingsSaveType::MAX); ++Index)
		{
			if (SettingsSaveSlotName[Index] == SaveName)
			{
				Value = SettingsSaveTitle[Index].ToString();
				break;
			}
		}
	}
}

FILLPlayerSettingsSaveGameRunnable::FILLPlayerSettingsSaveGameRunnable(UILLLocalPlayer* InLocalPlayer, EILLPlayerSettingsSaveType InSaveGameType)
: LocalPlayer(InLocalPlayer)
, SaveGameType(InSaveGameType)
, SaveGameThread(nullptr)
, bAsyncCompleted(false)
{
	SaveSlotName = SettingsSaveSlotName[static_cast<uint8>(SaveGameType)];
	check(!SaveSlotName.IsEmpty());
}

FILLPlayerSettingsSaveGameRunnable::~FILLPlayerSettingsSaveGameRunnable()
{
	if (SaveGameThread)
	{
		delete SaveGameThread;
	}
}

void FILLPlayerSettingsSaveGameRunnable::StartWork(const int32 ThreadNumber)
{
	// Listen for extended save game info requests (PS4 specific)
	FGameDelegates::Get().GetExtendedSaveGameInfoDelegate().BindStatic(&ExtendedSaveGameInfo);

	// Start a thread for the asynchronous work
	SaveGameThread = FRunnableThread::Create(this, *FString::Printf(TEXT("ILLPlayerSettingsSaveGameThread_%i"), ThreadNumber), 0, TPri_BelowNormal);
	if (!SaveGameThread)
	{
		bAsyncCompleted = true;
	}
}

void FILLPlayerSettingsSaveGameRunnable::FinishWork()
{
	FGameDelegates::Get().GetExtendedSaveGameInfoDelegate().Unbind();
}

uint32 FILLPlayerLoadSaveGameRunnable::Run()
{
	FGCScopeGuard LockGC;

	// Ensure that bAsyncCompleted is always flagged true when this function returns
	struct FScopeFinishWork
	{
		FScopeFinishWork(bool& bInAsyncCompleted)
		: bOuterAsyncCompleted(bInAsyncCompleted) {}

		~FScopeFinishWork()
		{
			bOuterAsyncCompleted = true;
		}

		bool& bOuterAsyncCompleted;
	} AsyncCompletedSetter(bAsyncCompleted);

	// Grab the save game system
	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	if (!LocalPlayer || !SaveSystem)
		return 0;

	// NOTE: Do not call to DoesSaveGameExist to confirm the existence of this first, since that returns false when it is exists and corrupt, and only LoadGame will show the error dialog for that on PS4
	// Load raw data from slot
	// @see UGameplayStatics::LoadGameFromSlot
	TArray<uint8> ObjectBytes;
	if (!SaveSystem->LoadGame(false, *SaveSlotName, LocalPlayer->GetControllerId(), ObjectBytes))
		return 0;
	
	FMemoryReader MemoryReader(ObjectBytes, true);
	MemoryReader.ArMaxSerializeSize = ObjectBytes.Num();

	// Check our magic number
	int32 FileMagic;
	MemoryReader << FileMagic;
	if (!ensure(!MemoryReader.IsError()))
		return 0;
	if (FileMagic != ILLPLAYERSETTINGS_MAGIC_HEADER)
		return 0;

	// Grab hash
	FString BodyHash;
	MemoryReader << BodyHash;
	if (!ensure(!MemoryReader.IsError()))
		return 0;

	// Have to read in how big the file size actually is due to PS4 allocating a fixed file size
	// rather than the specific number of bytes we write
	int32 SaveBodySize;
	MemoryReader << SaveBodySize;
	SaveBodySize = FMath::Min(SaveBodySize, ObjectBytes.Num());
	if (!ensure(!MemoryReader.IsError()))
		return 0;

	// Generate and compare hashes
	const uint32 BodySaltBytes[] = ILLPLAYERSETTINGS_BODY_HASH_SALT;
	FSHA256 BodyDigest;
	BodyDigest.Update(ObjectBytes.GetData()+MemoryReader.Tell(), SaveBodySize);
	const FString BodySaltString = FString::FromBlob((uint8*)BodySaltBytes, sizeof(BodySaltBytes));
	BodyDigest.Update(*BodySaltString, BodySaltString.Len());
	FString CalculatedHash = BodyDigest.GetLatestHash();
	if (BodyHash != CalculatedHash)
		return 0;

	// Uncompress the body for parsing
	bool bCompressed;
	MemoryReader << bCompressed;
	if (!ensure(!MemoryReader.IsError()))
	{
		return 0;
	}
	else if (bCompressed)
	{
		int32 CompressedSize, UncompressedSize;
		MemoryReader << CompressedSize;
		MemoryReader << UncompressedSize;

		// Decompress ObjectBytes
		TArray<uint8> UncompressedData;
		UncompressedData.SetNumUninitialized(UncompressedSize);
		const uint8* CompressedBytes = ObjectBytes.GetData()+MemoryReader.Tell();
		if (ensure(FCompression::UncompressMemory(
			ILLPLAYERSETTINGS_COMPRESSION_FLAGS,
			UncompressedData.GetData(), UncompressedSize,
			CompressedBytes, CompressedSize,
			false, FPlatformMisc::GetPlatformCompression()->GetCompressionBitWindow())))
		{
			ObjectBytes = UncompressedData;
		}
		else
		{
			// Failed to decompress
			return 0;
		}
	}
	else if (!ensure(MemoryReader.Tell() < ObjectBytes.Num()))
	{
		return 0;
	}
	else
	{
		// Remove header information before FinishWork runs
		ObjectBytes.RemoveAt(0, MemoryReader.Tell());
	}
	if (!ensure(ObjectBytes.Num() > 0))
		return 0;
	
	// ObjectBytes is now the uncompressed body data
	FMemoryReader BodyReader(ObjectBytes, true);
	BodyReader.ArMaxSerializeSize = ObjectBytes.Num();

	// Read version for this file
	uint32 SavegameFileVersion = static_cast<uint32>(FILLPlayerSettingsSaveGameFileVersion::InitialVersion);
	BodyReader << SavegameFileVersion;
	if (!ensure(!BodyReader.IsError()))
		return 0;
	if (SavegameFileVersion < static_cast<uint32>(FILLPlayerSettingsSaveGameFileVersion::MinimumVersion))
		return 0;
	if (SavegameFileVersion > static_cast<uint32>(FILLPlayerSettingsSaveGameFileVersion::LatestVersion))
		return 0;

	// Automatically re-save if out of date
	bNeedsReSave = bNeedsReSave || (SavegameFileVersion != static_cast<uint32>(FILLPlayerSettingsSaveGameFileVersion::LatestVersion));

	// Read package, licensee and UE4 version information
	int32 PackageFileUE4Version;
	BodyReader << PackageFileUE4Version;
	BodyReader.SetUE4Ver(PackageFileUE4Version);

	int32 LicenseeFileUE4Version;
	BodyReader << LicenseeFileUE4Version;
	BodyReader.SetLicenseeUE4Ver(LicenseeFileUE4Version);

	FEngineVersion SavedEngineVersion;
	BodyReader << SavedEngineVersion;
	BodyReader.SetEngineVer(SavedEngineVersion);

	// Read custom version information
	int32 CustomVersionFormat;
	BodyReader << CustomVersionFormat;
	FCustomVersionContainer CustomVersions;
	CustomVersions.Serialize(BodyReader, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
	BodyReader.SetCustomVersions(CustomVersions);
	if (!ensure(!BodyReader.IsError()))
		return 0;

	// Re-confirm that our local player and their net ID is valid
	if (!LocalPlayer || !LocalPlayer->GetCachedUniqueNetId().IsValid())
		return 0;

	// Hash the salted owner unique ID to confirm ownership
	const FString OwnerUniqueId = LocalPlayer->GetCachedUniqueNetId()->ToString();
	uint32 OwnerSalt[] = ILLPLAYERSETTINGS_OWNER_HASH_SALT;
	const FString OwnerHash = FSHA256::Hash(OwnerUniqueId + FString::FromBlob((uint8*)OwnerSalt, sizeof(OwnerSalt)));

	// Read the owner hash from file and confirm it matches before accepting the LoadedSaveEntries
	FString ParsedOwnerHash;
	BodyReader << ParsedOwnerHash;
	if (!ensure(!BodyReader.IsError()))
		return 0;
	if (ParsedOwnerHash != OwnerHash)
		return 0;

	// Parse the child save game entries
	BodyReader << LoadedSaveEntries;

	return 0;
}

void FILLPlayerLoadSaveGameRunnable::FinishWork()
{
	// Parse save games bytes into objects
	TArray<UILLPlayerSettingsSaveGame*> LoadedSaveGames;
	for (FILLPlayerSettingsSaveGameEntry& SaveEntry : LoadedSaveEntries)
	{
		// Make sure we are allowed to take this entry
		bool bAllowedEntry = false;
		TSubclassOf<UILLPlayerSettingsSaveGame> SaveClass = nullptr;
		for (TSubclassOf<UILLPlayerSettingsSaveGame> AllowedSaveGameClass : LocalPlayer->SettingsSaveGameClasses)
		{
			if (SaveEntry.SaveClassName == AllowedSaveGameClass->GetName())
			{
				SaveClass = AllowedSaveGameClass;
				bAllowedEntry = (AllowedSaveGameClass->GetDefaultObject<UILLPlayerSettingsSaveGame>()->SaveType == SaveGameType);
				break;
			}
		}
		if (!bAllowedEntry)
		{
			// Queue a re-save to cull out this bad entry
			bNeedsReSave = true;
			continue;
		}

		// Spawn a save game instance and serialize into it
		FMemoryReader SubReader(SaveEntry.ObjectBytes, true);
		FObjectAndNameAsStringProxyArchive SubAr(SubReader, true);
		FSCSoftObjectPtrSupportProxyArchive SoftObjectAr(SubAr);
		SubReader.ArMaxSerializeSize = SubAr.ArMaxSerializeSize = SoftObjectAr.ArMaxSerializeSize = SaveEntry.ObjectBytes.Num();
		UILLPlayerSettingsSaveGame* NewSaveGame = NewObject<UILLPlayerSettingsSaveGame>(LocalPlayer, SaveClass);
		NewSaveGame->Serialize(SoftObjectAr);
		if (SoftObjectAr.IsError())
		{
			// Queue a re-save because this entry is damaged
			bNeedsReSave = true;
		}
		else
		{
			LoadedSaveGames.Add(NewSaveGame);
		}
	}

	if (LoadedSaveGames.Num() != 0)
	{
		// Initialize the LoadedSaveGames set
		for (UILLPlayerSettingsSaveGame* NewSaveGame : LoadedSaveGames)
		{
			NewSaveGame->SaveGameLoaded(CastChecked<AILLPlayerController>(LocalPlayer->PlayerController));
		}
	}

	// Create entries for any new classes that are missing from this load, applying their settings immediately
	for (TSubclassOf<UILLPlayerSettingsSaveGame> AllowedSaveGameClass : LocalPlayer->SettingsSaveGameClasses)
	{
		if (AllowedSaveGameClass->GetDefaultObject<UILLPlayerSettingsSaveGame>()->SaveType == SaveGameType
		&& !LoadedSaveGames.ContainsByPredicate([&](UILLPlayerSettingsSaveGame* Entry) -> bool { return Entry->IsA(AllowedSaveGameClass); }))
		{
			UILLPlayerSettingsSaveGame* NewSaveGame = LocalPlayer->CreateSettingsSave(AllowedSaveGameClass);
			NewSaveGame->ApplyPlayerSettings(CastChecked<AILLPlayerController>(LocalPlayer->PlayerController));
			LoadedSaveGames.Add(NewSaveGame);

			// Queue a re-save to get the new entry in there
			bNeedsReSave = true;
		}
	}

	// Update the save game cache
	for (UILLPlayerSettingsSaveGame* NewSaveGame : LoadedSaveGames)
	{
		// Replace existing entries in the cache
		LocalPlayer->SettingsSaveGameCache.RemoveAll([&](UILLPlayerSettingsSaveGame* Entry) { return Entry->IsA(NewSaveGame->GetClass()); });
		LocalPlayer->SettingsSaveGameCache.Add(NewSaveGame);
	}

	// Queue a save to write out changes/upgrades to disk
	if (bNeedsReSave)
	{
		LocalPlayer->QueuePlayerSettingsSaveGameWrite(SaveGameType);
	}

	// Done!
	Super::FinishWork();
}

uint32 FILLPlayerWriteSaveGameRunnable::Run()
{
	FGCScopeGuard LockGC;

	if (ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem())
	{
		// Start a header buffer
		TArray<uint8> HeaderBytes;
		FMemoryWriter HeaderWriter(HeaderBytes, true);

		// Write our magic number
		int32 FileMagic = ILLPLAYERSETTINGS_MAGIC_HEADER;
		HeaderWriter << FileMagic;

		// Generate UncompressedBytes
		TArray<uint8> UncompressedBytes;
		FMemoryWriter UncompressedWriter(UncompressedBytes, true);

		// Write version for this file format
		uint32 SavegameFileVersion = static_cast<uint32>(FILLPlayerSettingsSaveGameFileVersion::LatestVersion);
		UncompressedWriter << SavegameFileVersion;

		// Write out package, licensee and UE4 version information
		int32 PackageFileUE4Version = GPackageFileUE4Version;
		UncompressedWriter << PackageFileUE4Version;
		int32 LicenseeFileUE4Version = GPackageFileLicenseeUE4Version;
		UncompressedWriter << LicenseeFileUE4Version;
		FEngineVersion SavedEngineVersion = FEngineVersion::Current();
		UncompressedWriter << SavedEngineVersion;

		// Write out custom version data
		ECustomVersionSerializationFormat::Type const CustomVersionFormat = ECustomVersionSerializationFormat::Latest;
		int32 CustomVersionFormatInt = static_cast<int32>(CustomVersionFormat);
		UncompressedWriter << CustomVersionFormatInt;
		FCustomVersionContainer CustomVersions = FCustomVersionContainer::GetRegistered();
		CustomVersions.Serialize(UncompressedWriter, CustomVersionFormat);

		// Write the owner platform unique ID
		const FString OwnerUniqueId = LocalPlayer->GetCachedUniqueNetId()->ToString();
		uint32 OwnerSalt[] = ILLPLAYERSETTINGS_OWNER_HASH_SALT;
		FString OwnerHash = FSHA256::Hash(OwnerUniqueId + FString::FromBlob((uint8*)OwnerSalt, sizeof(OwnerSalt)));
		UncompressedWriter << OwnerHash;

		// Write out save game entries collected in StartWork
		UncompressedWriter << WrittenSaveEntries;

		// Compress UncompressedBytes
		TArray<uint8> CompressedBytes;
		int32 UncompressedSize = UncompressedBytes.Num();
		int32 CompressedSize = UncompressedSize; // Do not let it get any bigger, obfuscation is more important than savings
		CompressedBytes.SetNumUninitialized(UncompressedSize);
		bool bCompressed = FCompression::CompressMemory(
			ILLPLAYERSETTINGS_COMPRESSION_FLAGS,
			CompressedBytes.GetData(), CompressedSize,
			UncompressedBytes.GetData(), UncompressedSize,
			FPlatformMisc::GetPlatformCompression()->GetCompressionBitWindow());

		// Write the compressed body
		TArray<uint8> BodyBytes;
		FMemoryWriter BodyWriter(BodyBytes, true);
		BodyWriter << bCompressed;
		if (bCompressed)
		{
			BodyWriter << CompressedSize;
			BodyWriter << UncompressedSize;
			BodyWriter.Serialize(CompressedBytes.GetData(), CompressedSize);
		}
		else
		{
			BodyWriter.Serialize(UncompressedBytes.GetData(), UncompressedBytes.Num());
		}

		// Write a hash of the BodyBytes to the header
		const uint32 BodySaltBytes[] = ILLPLAYERSETTINGS_BODY_HASH_SALT;
		FSHA256 BodyDigest;
		BodyDigest.Update(BodyBytes.GetData(), BodyBytes.Num());
		const FString BodySaltString = FString::FromBlob((uint8*)BodySaltBytes, sizeof(BodySaltBytes));
		BodyDigest.Update(*BodySaltString, BodySaltString.Len());
		FString BodyHash = BodyDigest.GetLatestHash();
		HeaderWriter << BodyHash;

		// Need to write out the size of our body in case the file gets 0 filled to fit some arbitrary "block size" (LOOKING AT YOU SONY)
		int32 SaveBodySize = BodyBytes.Num();
		HeaderWriter << SaveBodySize;

		// Combine the header and compressed body
		TArray<uint8> SaveBytes;
		SaveBytes.SetNumUninitialized(HeaderBytes.Num()+BodyBytes.Num());
		memcpy(SaveBytes.GetData(), HeaderBytes.GetData(), HeaderBytes.Num());
		memcpy(SaveBytes.GetData()+HeaderBytes.Num(), BodyBytes.GetData(), BodyBytes.Num());

		// Stuff that data into the save system with the desired file name
		if (!SaveSystem->SaveGame(false, *SaveSlotName, LocalPlayer->GetControllerId(), SaveBytes))
		{
			UE_LOG(LogPlayerManagement, Warning, TEXT("Failed to save %s for player %i!"), *SaveSlotName, LocalPlayer->GetControllerId());
		}
	}

	// Asynchronous work completed, allow FinishWork to process
	bAsyncCompleted = true;
	return 0;
}

void FILLPlayerWriteSaveGameRunnable::StartWork(const int32 ThreadNumber)
{
	if (LocalPlayer && ensure(LocalPlayer->HasPlatformLogin()))
	{
		// Serialize all of our child save game entries
		for (UILLPlayerSettingsSaveGame* SaveGame : LocalPlayer->SettingsSaveGameCache)
		{
			if (SaveGame->SaveType == SaveGameType)
			{
				FILLPlayerSettingsSaveGameEntry& NewEntry = WrittenSaveEntries[WrittenSaveEntries.AddDefaulted()];
				NewEntry.SaveClassName = SaveGame->GetClass()->GetName();

				FMemoryWriter SubWriter(NewEntry.ObjectBytes, true);
				FObjectAndNameAsStringProxyArchive SubAr(SubWriter, false);
				FSCSoftObjectPtrSupportProxyArchive SoftObjectAr(SubAr);
				SaveGame->Serialize(SoftObjectAr);
			}
		}

		// Start the thread
		FILLPlayerSettingsSaveGameRunnable::StartWork(ThreadNumber);
	}
	else
	{
		bAsyncCompleted = true;
	}
}

UILLPlayerSettingsSaveGame::UILLPlayerSettingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	if (SettingsSaveSlotName[0].IsEmpty()) // CLEANUP: pjackson: Gross way to initialize these
	{
		// Order of these matters, Platform aliases to be the same index as User on platforms that do not need the separation
		SettingsSaveSlotName[static_cast<uint8>(EILLPlayerSettingsSaveType::Platform)] = TEXT("PlatformSettings");
		SettingsSaveSlotName[static_cast<uint8>(EILLPlayerSettingsSaveType::User)] = TEXT("UserSettings");
		SettingsSaveTitle[static_cast<uint8>(EILLPlayerSettingsSaveType::Platform)] = LOCTEXT("PlatformSettingsTitle", "Platform Settings");
		SettingsSaveTitle[static_cast<uint8>(EILLPlayerSettingsSaveType::User)] = LOCTEXT("UserSettingsTitle", "User Settings");
	}

	SaveType = EILLPlayerSettingsSaveType::User;
}

void UILLPlayerSettingsSaveGame::ApplyPlayerSettings(AILLPlayerController* PlayerController, const bool bAndSave/* = true*/)
{
	check(PlayerController && PlayerController->GetLocalPlayer());

	if (bAndSave)
	{
		UILLLocalPlayer* LocalPlayer = CastChecked<UILLLocalPlayer>(PlayerController->GetLocalPlayer());

		// Queue an asynchronous save of this
		LocalPlayer->QueuePlayerSettingsSaveGameWrite(this);
	}
}

void UILLPlayerSettingsSaveGame::SaveGameLoaded(AILLPlayerController* PlayerController)
{
	// Apply user settings to the system, but there's no need to save it right after it was loaded
	ApplyPlayerSettings(PlayerController, false);
}

void UILLPlayerSettingsSaveGame::SaveGameNewlyCreated(AILLPlayerController* PlayerController)
{
}

UILLPlayerSettingsSaveGame* UILLPlayerSettingsSaveGame::SpawnDuplicate() const
{
	return static_cast<UILLPlayerSettingsSaveGame*>(StaticDuplicateObject(this, GetOuterUILLLocalPlayer()));
}

#undef LOCTEXT_NAMESPACE
