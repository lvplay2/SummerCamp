// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "SHA256.h"

void FSHA256::Reset()
{
	ProcessedBytes = 0;
	UnprocessedBytes = 0;

	// according to RFC 1321
	CurrentHash[0] = 0x6a09e667;
	CurrentHash[1] = 0xbb67ae85;
	CurrentHash[2] = 0x3c6ef372;
	CurrentHash[3] = 0xa54ff53a;
	CurrentHash[4] = 0x510e527f;
	CurrentHash[5] = 0x9b05688c;
	CurrentHash[6] = 0x1f83d9ab;
	CurrentHash[7] = 0x5be0cd19;
}

namespace
{
	inline uint32_t rotate(uint32_t a, uint32_t c)
	{
		return (a >> c) | (a << (32 - c));
	}

	// Mix functions for ProcessBlock
	inline uint32_t f1(uint32_t e, uint32_t f, uint32_t g)
	{
		uint32_t term1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
		uint32_t term2 = (e & f) ^ (~e & g); //(g ^ (e & (f ^ g)))
		return term1 + term2;
	}

	inline uint32_t f2(uint32_t a, uint32_t b, uint32_t c)
	{
		uint32_t term1 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
		uint32_t term2 = ((a | b) & c) | (a & b); //(a & (b ^ c)) ^ (b & c);
		return term1 + term2;
	}
}

void FSHA256::ProcessBlock(const void* Data)
{
	// get last hash
	uint32_t a = CurrentHash[0];
	uint32_t b = CurrentHash[1];
	uint32_t c = CurrentHash[2];
	uint32_t d = CurrentHash[3];
	uint32_t e = CurrentHash[4];
	uint32_t f = CurrentHash[5];
	uint32_t g = CurrentHash[6];
	uint32_t h = CurrentHash[7];

	const uint32_t* input = (uint32_t*)Data;
	// convert to big endian
	uint32_t words[64];
	int i;
	for (i = 0; i < 16; i++)
#if defined(__BYTE_ORDER) && (__BYTE_ORDER != 0) && (__BYTE_ORDER == __BIG_ENDIAN)
		words[i] =      input[i];
#else
		words[i] = BYTESWAP_ORDER32(input[i]);
#endif

	uint32_t x,y; // temporaries

	// first round
	x = h + f1(e,f,g) + 0x428a2f98 + words[ 0]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0x71374491 + words[ 1]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0xb5c0fbcf + words[ 2]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0xe9b5dba5 + words[ 3]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0x3956c25b + words[ 4]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0x59f111f1 + words[ 5]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0x923f82a4 + words[ 6]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0xab1c5ed5 + words[ 7]; y = f2(b,c,d); e += x; a = x + y;

	// second round
	x = h + f1(e,f,g) + 0xd807aa98 + words[ 8]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0x12835b01 + words[ 9]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0x243185be + words[10]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0x550c7dc3 + words[11]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0x72be5d74 + words[12]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0x80deb1fe + words[13]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0x9bdc06a7 + words[14]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0xc19bf174 + words[15]; y = f2(b,c,d); e += x; a = x + y;

	// extend to 24 words
	for (; i < 24; i++)
		words[i] = words[i-16] +
			(rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7] +
			(rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

	// third round
	x = h + f1(e,f,g) + 0xe49b69c1 + words[16]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0xefbe4786 + words[17]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0x0fc19dc6 + words[18]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0x240ca1cc + words[19]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0x2de92c6f + words[20]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0x4a7484aa + words[21]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0x5cb0a9dc + words[22]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0x76f988da + words[23]; y = f2(b,c,d); e += x; a = x + y;

	// extend to 32 words
	for (; i < 32; i++)
		words[i] = words[i-16] +
			(rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7] +
			(rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

	// fourth round
	x = h + f1(e,f,g) + 0x983e5152 + words[24]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0xa831c66d + words[25]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0xb00327c8 + words[26]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0xbf597fc7 + words[27]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0xc6e00bf3 + words[28]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0xd5a79147 + words[29]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0x06ca6351 + words[30]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0x14292967 + words[31]; y = f2(b,c,d); e += x; a = x + y;

	// extend to 40 words
	for (; i < 40; i++)
		words[i] = words[i-16] +
			(rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7] +
			(rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

	// fifth round
	x = h + f1(e,f,g) + 0x27b70a85 + words[32]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0x2e1b2138 + words[33]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0x4d2c6dfc + words[34]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0x53380d13 + words[35]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0x650a7354 + words[36]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0x766a0abb + words[37]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0x81c2c92e + words[38]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0x92722c85 + words[39]; y = f2(b,c,d); e += x; a = x + y;

	// extend to 48 words
	for (; i < 48; i++)
		words[i] = words[i-16] +
			(rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7] +
			(rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

	// sixth round
	x = h + f1(e,f,g) + 0xa2bfe8a1 + words[40]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0xa81a664b + words[41]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0xc24b8b70 + words[42]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0xc76c51a3 + words[43]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0xd192e819 + words[44]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0xd6990624 + words[45]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0xf40e3585 + words[46]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0x106aa070 + words[47]; y = f2(b,c,d); e += x; a = x + y;

	// extend to 56 words
	for (; i < 56; i++)
		words[i] = words[i-16] +
			(rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7] +
			(rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

	// seventh round
	x = h + f1(e,f,g) + 0x19a4c116 + words[48]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0x1e376c08 + words[49]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0x2748774c + words[50]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0x34b0bcb5 + words[51]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0x391c0cb3 + words[52]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0x4ed8aa4a + words[53]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0x5b9cca4f + words[54]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0x682e6ff3 + words[55]; y = f2(b,c,d); e += x; a = x + y;

	// extend to 64 words
	for (; i < 64; i++)
		words[i] = words[i-16] +
			(rotate(words[i-15],  7) ^ rotate(words[i-15], 18) ^ (words[i-15] >>  3)) +
			words[i-7] +
			(rotate(words[i- 2], 17) ^ rotate(words[i- 2], 19) ^ (words[i- 2] >> 10));

	// eighth round
	x = h + f1(e,f,g) + 0x748f82ee + words[56]; y = f2(a,b,c); d += x; h = x + y;
	x = g + f1(d,e,f) + 0x78a5636f + words[57]; y = f2(h,a,b); c += x; g = x + y;
	x = f + f1(c,d,e) + 0x84c87814 + words[58]; y = f2(g,h,a); b += x; f = x + y;
	x = e + f1(b,c,d) + 0x8cc70208 + words[59]; y = f2(f,g,h); a += x; e = x + y;
	x = d + f1(a,b,c) + 0x90befffa + words[60]; y = f2(e,f,g); h += x; d = x + y;
	x = c + f1(h,a,b) + 0xa4506ceb + words[61]; y = f2(d,e,f); g += x; c = x + y;
	x = b + f1(g,h,a) + 0xbef9a3f7 + words[62]; y = f2(c,d,e); f += x; b = x + y;
	x = a + f1(f,g,h) + 0xc67178f2 + words[63]; y = f2(b,c,d); e += x; a = x + y;

	// update hash
	CurrentHash[0] += a;
	CurrentHash[1] += b;
	CurrentHash[2] += c;
	CurrentHash[3] += d;
	CurrentHash[4] += e;
	CurrentHash[5] += f;
	CurrentHash[6] += g;
	CurrentHash[7] += h;
}

void FSHA256::Update(const void* Data, size_t NumBytes)
{
	const uint8_t* current = (const uint8_t*)Data;

	if (UnprocessedBytes > 0)
	{
		while (NumBytes > 0 && UnprocessedBytes < BLOCK_SIZE)
		{
			UnprocessedBuffer[UnprocessedBytes++] = *current++;
			NumBytes--;
		}
	}

	// Full buffer
	if (UnprocessedBytes == BLOCK_SIZE)
	{
		ProcessBlock(UnprocessedBuffer);
		ProcessedBytes += BLOCK_SIZE;
		UnprocessedBytes = 0;
	}

	if (NumBytes == 0)
		return;

	// Process full blocks
	while (NumBytes >= BLOCK_SIZE)
	{
		ProcessBlock(current);
		current += BLOCK_SIZE;
		ProcessedBytes += BLOCK_SIZE;
		NumBytes -= BLOCK_SIZE;
	}

	// Keep remaining bytes in buffer
	while (NumBytes > 0)
	{
		UnprocessedBuffer[UnprocessedBytes++] = *current++;
		NumBytes--;
	}
}

void FSHA256::ProcessBuffer()
{
	// the input bytes are considered as bits strings, where the first bit is the most significant bit of the byte

	// - append "1" bit to message
	// - append "0" bits until message length in bit mod 512 is 448
	// - append length as 64 bit integer

	// number of bits
	size_t paddedLength = UnprocessedBytes * 8;

	// plus one bit set to 1 (always appended)
	paddedLength++;

	// number of bits must be (numBits % 512) = 448
	size_t lower11Bits = paddedLength & 511;
	if (lower11Bits <= 448)
		paddedLength +=       448 - lower11Bits;
	else
		paddedLength += 512 + 448 - lower11Bits;
	// convert from bits to bytes
	paddedLength /= 8;

	// only needed if additional data flows over into a second block
	uint8 extra[BLOCK_SIZE];

	// append a "1" bit, 128 => binary 10000000
	if (UnprocessedBytes < BLOCK_SIZE)
		UnprocessedBuffer[UnprocessedBytes] = 128;
	else
		extra[0] = 128;

	size_t i;
	for (i = UnprocessedBytes + 1; i < BLOCK_SIZE; i++)
		UnprocessedBuffer[i] = 0;
	for (; i < paddedLength; i++)
		extra[i - BLOCK_SIZE] = 0;

	// add message length in bits as 64 bit number
	uint64_t msgBits = 8 * (ProcessedBytes + UnprocessedBytes);
	// find right position
	uint8* addLength;
	if (paddedLength < BLOCK_SIZE)
		addLength = UnprocessedBuffer + paddedLength;
	else
		addLength = extra + paddedLength - BLOCK_SIZE;

	// must be big endian
	*addLength++ = (uint8)((msgBits >> 56) & 0xFF);
	*addLength++ = (uint8)((msgBits >> 48) & 0xFF);
	*addLength++ = (uint8)((msgBits >> 40) & 0xFF);
	*addLength++ = (uint8)((msgBits >> 32) & 0xFF);
	*addLength++ = (uint8)((msgBits >> 24) & 0xFF);
	*addLength++ = (uint8)((msgBits >> 16) & 0xFF);
	*addLength++ = (uint8)((msgBits >>  8) & 0xFF);
	*addLength   = (uint8)( msgBits        & 0xFF);

	// process blocks
	ProcessBlock(UnprocessedBuffer);

	// flowed over into a second block ?
	if (paddedLength > BLOCK_SIZE)
		ProcessBlock(extra);
}

FString FSHA256::GetLatestHash()
{
	// compute hash (as raw bytes)
	uint8 rawHash[HASH_BYTES];
	GetLatestHash(rawHash);

	// convert to hex string
	FString result;
	result.Reserve(2 * HASH_BYTES);
	for (int i = 0; i < HASH_BYTES; i++)
	{
		static const char dec2hex[16+1] = "0123456789abcdef";
		result += dec2hex[(rawHash[i] >> 4) & 15];
		result += dec2hex[ rawHash[i]       & 15];
	}

	return result;
}

void FSHA256::GetLatestHash(uint8 (&OutDigest)[FSHA256::HASH_BYTES])
{
	// Save old hash if Buffer is partially filled
	uint32_t OldHash[HASH_VALUES];
	for (int i = 0; i < HASH_VALUES; i++)
		OldHash[i] = CurrentHash[i];

	// Process remaining bytes
	ProcessBuffer();

	uint8* Scan = OutDigest;
	for (int i = 0; i < HASH_VALUES; i++)
	{
		*Scan++ = (CurrentHash[i] >> 24) & 0xFF;
		*Scan++ = (CurrentHash[i] >> 16) & 0xFF;
		*Scan++ = (CurrentHash[i] >>  8) & 0xFF;
		*Scan++ =  CurrentHash[i]        & 0xFF;

		// Restore old hash
		CurrentHash[i] = OldHash[i];
	}
}
