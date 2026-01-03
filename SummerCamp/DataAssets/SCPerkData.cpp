// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCPerkData.h"

USCPerkData* USCPerkData::CreatePerkFromBackendData(const FSCPerkBackendData& PerkData)
{
	USCPerkData* CreatedPerk = nullptr;

	if (PerkData.PerkClass)
	{
		auto SetPerkProperty = [](USCPerkData* Perk, const FSCPerkModifier& PerkModifier, const bool bAdditiveModifier = false)
		{
			if (Perk)
			{
				// Multiple properties have values set
				if (PerkModifier.ModifierVariableName.Contains(TEXT("|")))
				{
					FString FirstPropertyName, SecondPropertyName;
					PerkModifier.ModifierVariableName.Split(TEXT("|"), &FirstPropertyName, &SecondPropertyName);

					if (UProperty* FirstProperty = FindField<UProperty>(Perk->GetClass(), *FirstPropertyName))
					{
						if (float* FloatProperty = FirstProperty->ContainerPtrToValuePtr<float>(Perk))
						{
							if (bAdditiveModifier)
								*FloatProperty += PerkModifier.ModifierPercentage / 100.0f;
							else
								*FloatProperty = PerkModifier.ModifierPercentage / 100.0f;
						}
					}
					if (UProperty* SecondProperty = FindField<UProperty>(Perk->GetClass(), *SecondPropertyName))
					{
						if (float* FloatProperty = SecondProperty->ContainerPtrToValuePtr<float>(Perk))
						{
							if (bAdditiveModifier)
								*FloatProperty += PerkModifier.ModifierPercentage / 100.0f;
							else
								*FloatProperty = PerkModifier.ModifierPercentage / 100.0f;
						}
					}
				}
				// StatModifiers has a value set
				else if (PerkModifier.ModifierVariableName.Contains(TEXT("StatModifiers.")))
				{
					FString PropertyArrayName, PropertyIndex;
					PerkModifier.ModifierVariableName.Split(TEXT("."), &PropertyArrayName, &PropertyIndex);

					if (UProperty* StatModifierProperty = FindField<UProperty>(Perk->GetClass(), *PropertyArrayName))
					{
						if (float* FloatProperty = StatModifierProperty->ContainerPtrToValuePtr<float>(Perk))
						{
							const uint8 IndexValue = (uint8)FCString::Atoi(*PropertyIndex);
							if (IndexValue >= 0 && IndexValue < (uint8)ECounselorStats::MAX)
							{
								if (bAdditiveModifier)
									FloatProperty[IndexValue] += PerkModifier.ModifierPercentage / 100.0f;
								else
									FloatProperty[IndexValue] = PerkModifier.ModifierPercentage / 100.0f;
							}
						}
					}
				}
				// Single property set
				else
				{
					if (UProperty* PerkProperty = FindField<UProperty>(Perk->GetClass(), *PerkModifier.ModifierVariableName))
					{
						if (float* FloatProperty = PerkProperty->ContainerPtrToValuePtr<float>(Perk))
						{
							if (bAdditiveModifier)
								*FloatProperty += PerkModifier.ModifierPercentage / 100.0f;
							else
								*FloatProperty = PerkModifier.ModifierPercentage / 100.0f;
						}
					}
				}
			}
		};

		// Create a new instance of the perk class and set it's properties to match the data from the backend
		CreatedPerk = NewObject<USCPerkData>((UObject*)GetTransientPackage(), PerkData.PerkClass);
		if (CreatedPerk)
		{
			SetPerkProperty(CreatedPerk, PerkData.PositivePerkModifiers);
			SetPerkProperty(CreatedPerk, PerkData.NegativePerkModifiers);
			SetPerkProperty(CreatedPerk, PerkData.LegendaryPerkModifiers, true);
		}
	}

	return CreatedPerk;
}