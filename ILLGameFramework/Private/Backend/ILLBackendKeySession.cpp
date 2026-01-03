// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLBackendKeySession.h"

#include "ILLGameInstance.h"

bool FILLBackendKeySession::ParseFromJSON(UILLGameInstance* GameInstance, int32 ResponseCode, const FString& ResponseContent, FILLBackendKeySession& OutSession)
{
	if (ResponseCode != 200 || ResponseContent.IsEmpty())
		return false;

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		FString KeySessionId, AESKey, InitVector, HMACKey;
		if(JsonObject->TryGetStringField(TEXT("KeySessionID"), KeySessionId) && !KeySessionId.IsEmpty()
		&& JsonObject->TryGetStringField(TEXT("AESKey"), AESKey) && AESKey.Len() == ILLNETENCRYPTION_AESKEY_BYTES*2
		&& JsonObject->TryGetStringField(TEXT("InitVector"), InitVector) && InitVector.Len() == ILLNETENCRYPTION_AESIV_BYTES*2
		&& JsonObject->TryGetStringField(TEXT("HMACKey"), HMACKey) && HMACKey.Len() == ILLNETENCRYPTION_HMACKEY_BYTES*2)
		{
			// Translate the strings into our format
			if (GameInstance->KeySessionToEncryptionToken(KeySessionId, OutSession.EncryptionToken))
			{
				for (int32 Index = 0; Index < AESKey.Len(); Index += 2)
					OutSession.EncryptionKey.Add((uint8)(FParse::HexDigit(AESKey[Index])*16 + FParse::HexDigit(AESKey[Index+1])));
				for (int32 Index = 0; Index < InitVector.Len(); Index += 2)
					OutSession.InitializationVector.Add((uint8)(FParse::HexDigit(InitVector[Index])*16 + FParse::HexDigit(InitVector[Index+1])));
				for (int32 Index = 0; Index < HMACKey.Len(); Index += 2)
					OutSession.HashKey.Add((uint8)(FParse::HexDigit(HMACKey[Index])*16 + FParse::HexDigit(HMACKey[Index+1])));

				return true;
			}
		}
	}

	return false;
}
