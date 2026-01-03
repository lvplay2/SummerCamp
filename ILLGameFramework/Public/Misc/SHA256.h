// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

/**
 * @class FSHA256
 */
class FSHA256
{
public:
	// Split into 64 byte blocks (=> 512 bits)
	enum { BLOCK_SIZE = 512/8 };

	// Hash is 32 bytes long
	enum { HASH_BYTES = 32 };
	enum { HASH_VALUES = HASH_BYTES / 4 };

	FSHA256()
	{
		Reset();
	}

	/**
	 * Compute SHA256 of a memory block.
	 *
	 * @param Data Byte data to hash.
	 * @param NumBytes Number of bytes in Data.
	 * @return String SHA256 result.
	 */
	static FString Hash(const void* Data, size_t NumBytes)
	{
		FSHA256 Instance;
		Instance.Update(Data, NumBytes);
		return Instance.GetLatestHash();
	}

	/**
	 * Compute SHA256 of a string, excluding final zero.
	 *
	 * @param Text String to hash.
	 * @return String SHA256 result.
	 */
	static FString Hash(const FString& Text)
	{
		FSHA256 Instance;
		Instance.Update(*Text, Text.Len() != 0 ? Text.Len()+1 : 0);
		return Instance.GetLatestHash();
	}

	/**
	 * Computes a SHA256 of a memory block.
	 *
	 * @param Data Byte data to hash.
	 * @param NumBytes Number of bytes in Data.
	 * @param OutDigest Bytes to store the digest in.
	 */
	static void Hash(const uint8* Data, size_t NumBytes, uint8 (&OutDigest)[HASH_BYTES])
	{
		FSHA256 Instance;
		Instance.Update(Data, NumBytes);
		Instance.GetLatestHash(OutDigest);
	}

	/** Used to add an arbitrary number of bytes. */
	void Update(const void* Data, size_t NumBytes);

	/** @return Latest hash as 64 hex characters. */
	FString GetLatestHash();

	/** @return Latest hash as bytes. */
	void GetLatestHash(uint8 (&OutDigest)[HASH_BYTES]);

	/** Resets this instance. */
	void Reset();

private:
	/** Process 64 bytes. */
	void ProcessBlock(const void* Data);

	/** Process everything left in the internal buffer. */
	void ProcessBuffer();

	// Size of processed data in bytes
	uint64_t ProcessedBytes;

	// Valid bytes in UnprocessedBuffer
	size_t UnprocessedBytes;

	// Bytes not processed yet
	uint8_t UnprocessedBuffer[BLOCK_SIZE];

	// Hash, stored as integers
	uint32_t CurrentHash[HASH_VALUES];
};
