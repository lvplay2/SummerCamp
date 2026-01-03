// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

class UILLGameInstance;

/**
 * @struct FILLBackendKeySession
 */
struct ILLGAMEFRAMEWORK_API FILLBackendKeySession
{
	FString EncryptionToken;
	TArray<uint8> EncryptionKey;
	TArray<uint8> InitializationVector;
	TArray<uint8> HashKey;

	/** Resets any encryption key data stored. */
	FORCEINLINE void Reset()
	{
		EncryptionToken.Empty();
		EncryptionKey.Empty();
		InitializationVector.Empty();
		HashKey.Empty();
	}

	/**
	 * Parses a JSON response into this structure's format.
	 *
	 * @param GameInstance Game instance object for generating the EncryptionToken.
	 * @param ResponseCode HTTP status code received from backend.
	 * @param ResponseContent Response body.
	 * @param OutSession Parsed result.
	 * @return true if parsing was successful.
	 */
	static bool ParseFromJSON(UILLGameInstance* GameInstance, int32 ResponseCode, const FString& ResponseContent, FILLBackendKeySession& OutSession);
};
