// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "ILLBackendPlayerIdentifier.generated.h"

/**
 * @struct FILLBackendPlayerIdentifier
 */
USTRUCT()
struct ILLGAMEFRAMEWORK_API FILLBackendPlayerIdentifier
{
	GENERATED_USTRUCT_BODY()

	FILLBackendPlayerIdentifier()
	: DebugString(TEXT("INVALID"))
	, Checksum(0) {}

	explicit FILLBackendPlayerIdentifier(const FString& InIdentifier)
	{
		FromString(InIdentifier);
	}

	FORCEINLINE FILLBackendPlayerIdentifier& operator =(const FILLBackendPlayerIdentifier& Other)
	{
		if (&Other != this)
		{
			Identifier = Other.Identifier;
			DebugString = Other.DebugString;
			Checksum = Other.Checksum;
		}
		return *this;
	}
	FORCEINLINE bool operator ==(const FILLBackendPlayerIdentifier& Other) const { return (Checksum == Other.Checksum && Identifier == Other.Identifier); }
	FORCEINLINE bool operator !=(const FILLBackendPlayerIdentifier& Other) const { return !operator ==(Other); }

	/** FArchive serialization operator. */
	ILLGAMEFRAMEWORK_API friend FArchive& operator<<(FArchive& Ar, FILLBackendPlayerIdentifier& UniqueNetId);

	/** Initializes this identifier from string. */
	void FromString(const FString& InIdentifier);

	/** Text exporting. */
	bool ExportTextItem(FString& ValueStr, FILLBackendPlayerIdentifier const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;

	/** Text importing. */
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	/** @return Static empty invalid instance. Intentionally not inlined to duck a ctor hit. */
	static const FILLBackendPlayerIdentifier& GetInvalid()
	{
		static FILLBackendPlayerIdentifier Instance;
		return Instance;
	}

	/** @return String version of this player account identifier. */
	FORCEINLINE const FString& GetIdentifier() const { return Identifier; }

	/** @return Byte size of this identifier. */
	FORCEINLINE int32 GetSize() const { return Identifier.GetCharArray().GetTypeSize() * Identifier.GetCharArray().Num(); }

	/** @return true if this player account identifier is valid. */
	FORCEINLINE bool IsValid() const { return !Identifier.IsEmpty(); }

	/** Network serialization. */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/** Invalidates this player account identifier. */
	FORCEINLINE void Reset()
	{
		*this = FILLBackendPlayerIdentifier();
	}

	/** FArchive serialize. */
	bool Serialize(FArchive& Ar);

	/** @return Debug name of this player, generally used for internal use like logging. */
	FORCEINLINE const FString& ToDebugString() const { return DebugString; }

private:
	// Account identifier
	FString Identifier;

	// Debug identifier
	FString DebugString;

	// Checksum of Identifier
	uint32 Checksum;
};

template<>
struct TStructOpsTypeTraits<FILLBackendPlayerIdentifier>
: public TStructOpsTypeTraitsBase2<FILLBackendPlayerIdentifier>
{
	enum 
	{
		// Can be copied via assignment operator
		WithCopy = true,
		// Requires custom serialization
		WithSerializer = true,
		// Requires custom net serialization
		WithNetSerializer = true,
		// Requires custom Identical operator for rep notifies in PostReceivedBunch()
		WithIdenticalViaEquality = true,
		// Export contents of this struct as a string (displayall, obj dump, etc)
		WithExportTextItem = true,
		// Import string contents as a unique id
		WithImportTextItem = true
	};
};
