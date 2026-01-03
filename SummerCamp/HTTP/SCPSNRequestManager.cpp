// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPSNRequestManager.h"

USCPSNRequestManager::USCPSNRequestManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClientId = FGuid(0xbaf7de9b, 0xea00481a, 0x81c69f72, 0xc5e1cb6f);
	ClientSecret = { '6', 'N', '6', 'K', '3', 'J', 'h', 't', 'm', 'G', '0', '5', 'E', 'U', 'Z', 'C' };
	DevelopmentProxyAddress = TEXT("https://52.8.238.42:3128");
	DevelopmentProxyUserPass = TEXT("sc_dedi:scdedibeefcake");
}
