// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendPlayerIdentifier.h"

FArchive& operator<<(FArchive& Ar, FILLBackendPlayerIdentifier& AccountId)
{
	int32 Size = AccountId.IsValid() ? AccountId.GetSize() : 0;
	Ar << Size;

	if (Size > 0)
	{
		if (Ar.IsSaving())
		{
			check(AccountId.IsValid());
			FString Contents = AccountId.GetIdentifier();
			Ar << Contents;
		}
		else if (Ar.IsLoading())
		{
			FString Contents;
			Ar << Contents;
			AccountId.FromString(Contents);
		}
	}
	else if (Ar.IsLoading())
	{
		AccountId.Reset();
	}

	return Ar;
}

void FILLBackendPlayerIdentifier::FromString(const FString& InIdentifier)
{
	if (InIdentifier.IsEmpty())
	{
		Reset();
	}
	else
	{
		Identifier = InIdentifier;
		Checksum = FCrc::StrCrc32(*Identifier, FNetworkVersion::GetLocalNetworkVersion());
		DebugString = FString::Printf(TEXT("0x%X"), Checksum);
	}
}

bool FILLBackendPlayerIdentifier::ExportTextItem(FString& ValueStr, FILLBackendPlayerIdentifier const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	if (0 != (PortFlags & EPropertyPortFlags::PPF_ExportCpp))
	{
		return false;
	}

	ValueStr += GetIdentifier();
	return true;
}

bool FILLBackendPlayerIdentifier::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
	const FString String(Buffer);
	FromString(String);
	return true;
}

bool FILLBackendPlayerIdentifier::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << *this;
	bOutSuccess = true;
	return true;
}

bool FILLBackendPlayerIdentifier::Serialize(FArchive& Ar)
{
	Ar << *this;
	return true;
}
