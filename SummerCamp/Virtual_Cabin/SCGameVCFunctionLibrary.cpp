// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "Virtual_Cabin/SCGVCCharacter.h"
#include "Virtual_Cabin/SCGVCPlayerController.h"
#include "SCGameVCFunctionLibrary.h"

bool USCGameVCFunctionLibrary::IsFridaythe13th()
{
	FDateTime curDateTime = FDateTime::Today(); 
	EDayOfWeek dayofWeek = curDateTime.GetDayOfWeek();
	int32 day = curDateTime.GetDay();

	if (dayofWeek == EDayOfWeek::Friday && day == 13)
		return true;
		
	return false;
}


bool USCGameVCFunctionLibrary::IsFridaythe13th79()
{
	FDateTime curDateTime = FDateTime::Today();
	EDayOfWeek dayofWeek = curDateTime.GetDayOfWeek();
	int32 day = curDateTime.GetDay();
	int32 year = curDateTime.GetYear();

	if (dayofWeek == EDayOfWeek::Friday && day == 13 && year == 1979)
		return true;

	return false;
}

ASCGVCPlayerController* USCGameVCFunctionLibrary::GetVCPlayerController(UObject* Context)
{
	UWorld* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		ASCGVCPlayerController* PCC = Cast<ASCGVCPlayerController>(UGameplayStatics::GetPlayerController(World, 0));

		return PCC;
	}

	return nullptr;
}

ASCGVCCharacter* USCGameVCFunctionLibrary::GetVCCharacter(UObject* Context)
{
	UWorld* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		ASCGVCPlayerController* PCC = Cast<ASCGVCPlayerController>(UGameplayStatics::GetPlayerController(World, 0));

		return Cast<ASCGVCCharacter>(PCC->GetCharacter());
	}
	return nullptr;
}
