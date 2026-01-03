// Copyright (C) 2015-2018 IllFonic, LLC.

// This is a helper struct to make quantizing float data easier. It attempts to replace the regular float value with as little work as possible.
// There are a couple of caveats. First the max value should be set on construction and not changed once set. Changing the max value during use may cause the 
// quantized data to become distorted and incorrect. Secondly while it can be used in most cases as a float just fine there are a couple uses where you will have to explicitly 
// cast it to a float. This is most prevalent when using it with FMath functions.

#pragma once

#include "ILLNetSerialization.generated.h"

/**
 * @struct FFloat_NetQuantize
 */
USTRUCT(BlueprintType)
struct FFloat_NetQuantize
{
	friend class UILLNetQuantizeLibrary;

	GENERATED_USTRUCT_BODY()

	FORCEINLINE FFloat_NetQuantize() {}

	explicit FORCEINLINE FFloat_NetQuantize(EForceInit)
	: RawData(0.f), MaxValue(1.f), MinValue(0.f) {}

	FORCEINLINE FFloat_NetQuantize(float Value, float Max = 1.f, float Min = 0.f)
	: RawData(Value), MaxValue(Max), MinValue(Min) {}

	FORCEINLINE operator float() const { return RawData; }

	FORCEINLINE operator int() const { return FMath::TruncToInt(RawData); }

	/**
	 * Used to set the maximum value needed for the quantization.
	 * @param Max - The highest value this float should be able to be
	 */
	FORCEINLINE void SetMaxValue(float Max) { MaxValue = Max; }

	/**
	 * Used to set the minimum value needed for the quantization.
	 * @param Min - The lowest value this float should be able to be
	 */
	FORCEINLINE void SetMinValue(float Min) { MinValue = Min; }

	/**
	 * Used to set the maximum and minimum value needed for the quantization.
	 * @param Max - The highest value this float should be able to be
	 * @param Min - The lowest value this float should be able to be
	 */
	FORCEINLINE void SetValueWidth(float Max, float Min) { MaxValue = Max; MinValue = Min; }

	/* Serialize function used to quantize and serialize the float data. */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		const float ScaledMax = MaxValue - MinValue;
		// convert raw float to quantized byte
		uint8 QuantizeData = FMath::RoundToInt( (RawData-MinValue) * 255.f / ScaledMax) & 0xFF;

		// make sure value isn't zero
		uint8 B = (QuantizeData!=0);
		Ar.SerializeBits(&B, 1); // check bit to see if we should read/write
		if (B) Ar << QuantizeData; else QuantizeData = 0;

		// un-quantize byte back into float data so that the server and clients match
		if (Ar.IsLoading())
		{
			RawData = (QuantizeData * ScaledMax / 255.f) + MinValue;
			RawData = FMath::Clamp(RawData, MinValue, MaxValue);
		}

		return true;
	}

	/* Assignment operators */
	FORCEINLINE FFloat_NetQuantize& operator=(const FFloat_NetQuantize& Other);
	FORCEINLINE FFloat_NetQuantize& operator=(const float Other);

	/* Comparison operators */
	FORCEINLINE bool operator==(const FFloat_NetQuantize& Other) const;
	FORCEINLINE bool operator==(const float Other) const;
	FORCEINLINE friend bool operator==(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE bool operator!=(const FFloat_NetQuantize& Other) const;
	FORCEINLINE bool operator!=(const float Other) const;
	FORCEINLINE friend bool operator!=(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE bool operator>=(const FFloat_NetQuantize& Other) const;
	FORCEINLINE bool operator>=(const float Other) const;
	FORCEINLINE friend bool operator>=(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE bool operator<=(const FFloat_NetQuantize& Other) const;
	FORCEINLINE bool operator<=(const float Other) const;
	FORCEINLINE friend bool operator<=(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE bool operator>(const FFloat_NetQuantize& Other) const;
	FORCEINLINE bool operator>(const float Other) const;
	FORCEINLINE friend bool operator>(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE bool operator<(const FFloat_NetQuantize& Other) const;
	FORCEINLINE bool operator<(const float Other) const;
	FORCEINLINE friend bool operator<(const float A, const FFloat_NetQuantize& B);

	/** Arithmatic operators */
	FORCEINLINE FFloat_NetQuantize& operator+=(const FFloat_NetQuantize& Other);
	FORCEINLINE FFloat_NetQuantize& operator+=(const float Other);
	FORCEINLINE friend float& operator+=(float& A, const FFloat_NetQuantize& B);

	FORCEINLINE FFloat_NetQuantize& operator-=(const FFloat_NetQuantize& Other);
	FORCEINLINE FFloat_NetQuantize& operator-=(const float Other);
	FORCEINLINE friend float& operator-=(float& A, const FFloat_NetQuantize& B);

	FORCEINLINE FFloat_NetQuantize& operator*=(const FFloat_NetQuantize& Other);
	FORCEINLINE FFloat_NetQuantize& operator*=(const float Other);
	FORCEINLINE friend float& operator*=(float& A, const FFloat_NetQuantize& B);

	FORCEINLINE FFloat_NetQuantize& operator/=(const FFloat_NetQuantize& Other);
	FORCEINLINE FFloat_NetQuantize& operator/=(const float Other);
	FORCEINLINE friend float& operator/=(float& A, const FFloat_NetQuantize& B);

	FORCEINLINE float operator+(const FFloat_NetQuantize& Other) const;
	FORCEINLINE float operator+(const float Other) const;
	FORCEINLINE friend float operator+(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE float operator-(const FFloat_NetQuantize& Other) const;
	FORCEINLINE float operator-(const float Other) const;
	FORCEINLINE friend float operator-(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE float operator*(const FFloat_NetQuantize& Other) const;
	FORCEINLINE float operator*(const float Other) const;
	FORCEINLINE friend float operator*(const float A, const FFloat_NetQuantize& B);

	FORCEINLINE float operator/(const FFloat_NetQuantize& Other) const;
	FORCEINLINE float operator/(const float Other) const;
	FORCEINLINE friend float operator/(const float A, const FFloat_NetQuantize& B);

protected:
	UPROPERTY()
	float RawData;

	UPROPERTY()
	float MaxValue;

	UPROPERTY()
	float MinValue;
};

template<>
struct TStructOpsTypeTraits<FFloat_NetQuantize>
: public TStructOpsTypeTraitsBase2<FFloat_NetQuantize>
{
	enum
	{
		WithNetSerializer = true
	};
};


/**
 * @class UILLNetQuantizeLibrary
 */
UCLASS()
class ILLGAMEFRAMEWORK_API UILLNetQuantizeLibrary
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** returns the float value for a quantized float struct */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "ToFloat (QuantizedFloat)", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities|Float")
	static float ToFloat(const FFloat_NetQuantize& QF) { return QF.RawData; }
};

FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator=(const FFloat_NetQuantize& Other)
{
	RawData = Other.RawData;
	return *this;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator=(const float Other)
{
	RawData = Other;
	return *this;
}
FORCEINLINE bool FFloat_NetQuantize::operator==(const FFloat_NetQuantize& Other) const
{
	return FMath::IsNearlyEqual(RawData, Other.RawData);
}
FORCEINLINE bool FFloat_NetQuantize::operator==(const float Other) const
{
	return FMath::IsNearlyEqual(RawData, Other);
}
FORCEINLINE bool operator==(const float A, const FFloat_NetQuantize& B)
{
	return FMath::IsNearlyEqual(A, B.RawData);
}
FORCEINLINE bool FFloat_NetQuantize::operator!=(const FFloat_NetQuantize& Other) const
{
	return !FMath::IsNearlyEqual(RawData, Other.RawData);
}
FORCEINLINE bool FFloat_NetQuantize::operator!=(const float Other) const
{
	return !FMath::IsNearlyEqual(RawData, Other);
}
FORCEINLINE bool operator!=(const float A, const FFloat_NetQuantize& B)
{
	return !FMath::IsNearlyEqual(A, B.RawData);
}
FORCEINLINE bool FFloat_NetQuantize::operator>=(const FFloat_NetQuantize& Other) const
{
	return RawData >= Other.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator>=(const float Other) const
{
	return RawData >= Other;
}
FORCEINLINE bool operator>=(const float A, const FFloat_NetQuantize& B)
{
	return A >= B.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator<=(const FFloat_NetQuantize& Other) const
{
	return RawData <= Other.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator<=(const float Other) const
{
	return RawData <= Other;
}
FORCEINLINE bool operator<=(const float A, const FFloat_NetQuantize& B)
{
	return A <= B.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator>(const FFloat_NetQuantize& Other) const
{
	return RawData > Other.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator>(const float Other) const
{
	return RawData > Other;
}
FORCEINLINE bool operator>(const float A, const FFloat_NetQuantize& B)
{
	return A > B.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator<(const FFloat_NetQuantize& Other) const
{
	return RawData < Other.RawData;
}
FORCEINLINE bool FFloat_NetQuantize::operator<(const float Other) const
{
	return RawData < Other;
}
FORCEINLINE bool operator<(const float A, const FFloat_NetQuantize& B)
{
	return A < B.RawData;
}

FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator+=(const FFloat_NetQuantize& Other)
{
	RawData += Other.RawData;
	return *this;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator+=(const float Other)
{
	RawData += Other;
	return *this;
}
FORCEINLINE float& operator+=(float& A, const FFloat_NetQuantize& B)
{
	A += B.RawData;
	return A;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator-=(const FFloat_NetQuantize& Other)
{
	RawData -= Other.RawData;
	return *this;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator-=(const float Other)
{
	RawData -= Other;
	return *this;
}
FORCEINLINE float& operator-=(float& A, const FFloat_NetQuantize& B)
{
	A -= B.RawData;
	return A;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator*=(const FFloat_NetQuantize& Other)
{
	RawData *= Other.RawData;
	return *this;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator*=(const float Other)
{
	RawData *= Other;
	return *this;
}
FORCEINLINE float& operator*=(float& A, const FFloat_NetQuantize& B)
{
	A *= B.RawData;
	return A;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator/=(const FFloat_NetQuantize& Other)
{
	RawData /= Other.RawData;
	return *this;
}
FORCEINLINE FFloat_NetQuantize& FFloat_NetQuantize::operator/=(const float Other)
{
	RawData /= Other;
	return *this;
}
FORCEINLINE float& operator/=(float& A, const FFloat_NetQuantize& B)
{
	A /= B.RawData;
	return A;
}
FORCEINLINE float FFloat_NetQuantize::operator+(const FFloat_NetQuantize& Other) const
{
	return RawData + Other.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator+(const float Other) const
{
	return RawData + Other;
}
FORCEINLINE float operator+(const float A, const FFloat_NetQuantize& B)
{
	return A + B.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator-(const FFloat_NetQuantize& Other) const
{
	return RawData - Other.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator-(const float Other) const
{
	return RawData - Other;
}
FORCEINLINE float operator-(const float A, const FFloat_NetQuantize& B)
{
	return A - B.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator*(const FFloat_NetQuantize& Other) const
{
	return RawData * Other.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator*(const float Other) const
{
	return RawData * Other;
}
FORCEINLINE float operator*(const float A, const FFloat_NetQuantize& B)
{
	return A * B.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator/(const FFloat_NetQuantize& Other) const
{
	return RawData / Other.RawData;
}
FORCEINLINE float FFloat_NetQuantize::operator/(const float Other) const
{
	return RawData / Other;
}
FORCEINLINE float operator/(const float A, const FFloat_NetQuantize& B)
{
	return A / B.RawData;
}