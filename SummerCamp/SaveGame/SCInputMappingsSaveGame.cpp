// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCInputMappingsSaveGame.h"

USCInputMappingsSaveGame::USCInputMappingsSaveGame(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool USCInputMappingsSaveGame::RemapPCInputActionByCategory(AILLPlayerController* PlayerController, const FName ActionName, const FInputChord NewChord, FILLCustomKeyBinding Binding)
{
	const FString CategoryPrefix = CategoryToLiteral(Binding.Category);

	// remove any actions currently bound to the key.
	for (int32 idx = ActionMappings.Num() - 1; idx >= 0; --idx)
	{
		FInputActionKeyMapping& Mapping = ActionMappings[idx];

		// Make sure we're looking at the right category.
		if (CategoryPrefix != "GLB_" && !Mapping.ActionName.ToString().StartsWith(CategoryPrefix) && !Mapping.ActionName.ToString().StartsWith("GLB_"))
			continue;

		// Ignore the virtual cabin bindings....
		if (Mapping.ActionName.ToString().Contains("VC_"))
			continue;

		// If the other action is also aprt of this binding don't remove it, it will get updated as well.
		if (Binding.BoundActions.Contains(Mapping.ActionName))
			continue;

		// If the chords are the same then remove that shit.
		if (Mapping.Key == NewChord.Key &&
			Mapping.bCtrl == NewChord.bCtrl &&
			Mapping.bAlt == NewChord.bAlt &&
			Mapping.bShift == NewChord.bShift &&
			Mapping.bCmd == NewChord.bCmd)
		{
			// RemoveAtSwap by index is faster and cheaper than RemoveSwap by object...
			ActionMappings.RemoveAtSwap(idx, 1, false);
		}
	}

	// remove any axes currently bound to the key.
	for (int32 idx = AxisMappings.Num() - 1; idx >= 0; --idx)
	{
		FInputAxisKeyMapping& Mapping = AxisMappings[idx];

		// Make sure we're looking at the right category.
		if (CategoryPrefix != "GLB_" && !Mapping.AxisName.ToString().StartsWith(CategoryPrefix) && !Mapping.AxisName.ToString().StartsWith("GLB_"))
			continue;

		// Ignore the virtual cabin bindings....
		if (Mapping.AxisName.ToString().Contains("VC_"))
			continue;

		// We don't want to remove gamepad controls here.
		if (Mapping.Key == NewChord.Key &&
			!NewChord.HasAnyModifierKeys() &&
			Mapping.Scale == Binding.Scale)
		{
			// RemoveAtSwap by index is faster and cheaper than RemoveSwap by object...
			AxisMappings.RemoveAtSwap(idx, 1, false);
		}
	}

	return RemapPCInputAction(PlayerController, ActionName, NewChord);
}

bool USCInputMappingsSaveGame::RemapPCInputAxisByCategory(AILLPlayerController* PlayerController, const FName AxisName, const FKey Key, FILLCustomKeyBinding Binding)
{
	const FString CategoryPrefix = CategoryToLiteral(Binding.Category);

	// remove any actions currently bound to the key.
	for (int idx = ActionMappings.Num() - 1; idx >= 0; --idx)
	{
		FInputActionKeyMapping& Mapping = ActionMappings[idx];

		// Make sure we're looking at the right category.
		if (CategoryPrefix != "GLB_" && !Mapping.ActionName.ToString().StartsWith(CategoryPrefix) && !Mapping.ActionName.ToString().StartsWith("GLB_"))
			continue;

		// Ignore the virtual cabin bindings....
		if (Mapping.ActionName.ToString().Contains("VC_"))
			continue;

		// If the chords are the same then remove that shit.
		if (Mapping.Key == Key &&
			!Mapping.bCtrl && !Mapping.bAlt && !Mapping.bShift && !Mapping.bCmd)
		{
			// RemoveAtSwap by index is faster and cheaper than RemoveSwap by object...
			ActionMappings.RemoveAtSwap(idx, 1, false);
		}
	}

	// remove any axes currently bound to the key.
	for (int idx = AxisMappings.Num() - 1; idx >= 0; --idx)
	{
		FInputAxisKeyMapping& Mapping = AxisMappings[idx];

		// Make sure we're looking at the right category.
		if (CategoryPrefix != "GLB_" && !Mapping.AxisName.ToString().StartsWith(CategoryPrefix) && !Mapping.AxisName.ToString().StartsWith("GLB_"))
			continue;

		// Ignore the virtual cabin bindings....
		if (Mapping.AxisName.ToString().Contains("VC_"))
			continue;

		// If the other action is also aprt of this binding don't remove it, it will get updated as well.
		if (Binding.BoundActions.Contains(Mapping.AxisName))
			continue;

		// We don't want to remove gamepad controls here.
		if (Mapping.Key == Key && Mapping.Scale == Binding.Scale)
		{
			// RemoveAtSwap by index is faster and cheaper than RemoveSwap by object...
			AxisMappings.RemoveAtSwap(idx, 1, false);
		}
	}

	return RemapPCInputAxis(PlayerController, AxisName, Key, Binding.Scale);
}

FString USCInputMappingsSaveGame::CategoryToLiteral(ESCCustomKeybindCategory InCategory)
{
	// return the category string literal that begins each input action
	switch(InCategory)
	{
	case ESCCustomKeybindCategory::Global:
		return FString(TEXT("GLB_"));
	case ESCCustomKeybindCategory::Counselor:
		return FString(TEXT("CSLR_"));
	case ESCCustomKeybindCategory::CounselorCombat:
		return FString(TEXT("CMBT_CSLR_"));
	case ESCCustomKeybindCategory::Killer:
		return FString(TEXT("KLR_"));
	case ESCCustomKeybindCategory::KillerCombat:
		return FString(TEXT("CMBT_KLR_"));
	case ESCCustomKeybindCategory::Vehicle:
		return FString(TEXT("VHCL_"));
	case ESCCustomKeybindCategory::Spectator:
		return FString(TEXT("SPEC_"));
	case ESCCustomKeybindCategory::VirtualCabin:
		return FString(TEXT("VC_"));
	case ESCCustomKeybindCategory::Minigame:
		return FString(TEXT("MG_"));
	default:
		return FString(TEXT("INVALID"));
	}
}