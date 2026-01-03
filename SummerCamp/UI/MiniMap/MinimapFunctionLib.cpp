// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "MinimapFunctionLib.h"

FColor UMinimapFunctionLib::GetTextureColorAtUV(UTexture2D* texture, FVector2D coord)
{
	FTexture2DMipMap* MyMipMap = &texture->PlatformData->Mips[0];

	FColor* FormattedImageData = static_cast<FColor*>(MyMipMap->BulkData.Lock(LOCK_READ_ONLY));

	uint32 PixelX = MyMipMap->SizeX * coord.X;
	uint32 PixelY = MyMipMap->SizeY * coord.Y;

	uint32 index = PixelY * MyMipMap->SizeX + PixelX;

	FColor PixelColor = FormattedImageData[index];

	MyMipMap->BulkData.Unlock();
	return PixelColor;
}

FColor UMinimapFunctionLib::GetTextureColorAtPixel(UTexture2D* texture, FVector2D coord)
{
	FTexture2DMipMap* MyMipMap = &texture->PlatformData->Mips[0];

	FColor* FormattedImageData = static_cast<FColor*>(MyMipMap->BulkData.Lock(LOCK_READ_ONLY));

	if (coord.X > MyMipMap->SizeX || coord.Y > MyMipMap->SizeY)
	{
		return FColor();
	}

	uint32 index = coord.Y * MyMipMap->SizeX + coord.X;

	FColor PixelColor = FormattedImageData[index];

	MyMipMap->BulkData.Unlock();
	return PixelColor;
}
