// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCRebindingWidget.h"

#include "ILLLocalPlayer.h"
#include "ILLPlayerInput.h"

USCRebindingWidget::USCRebindingWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TiedBinding(FILLCustomKeyBinding())
{
}

void USCRebindingWidget::BeginRebind(FBPInputChord NewChord)
{
	// We don't have an owner?!?!?!
	if (GetOwningILLPlayer() == nullptr)
		return;

	bool bRebound = false;
	
	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(GetOwningILLPlayer()->GetLocalPlayer()))
	{
		if (USCInputMappingsSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCInputMappingsSaveGame>())
		{

			if (UILLPlayerInput* Input = Cast<UILLPlayerInput>(GetOwningPlayer()->PlayerInput))
			{
				if (Input->IsUsingGamepad())
				{
					// do something...
				}
				else
				{
					// rebind PC controls
					for (FName Action : TiedBinding.BoundActions)
					{

						bRebound = (TiedBinding.bIsAxisBinding ? SaveGame->RemapPCInputAxisByCategory(GetOwningILLPlayer(), Action, NewChord.Key, TiedBinding) : SaveGame->RemapPCInputActionByCategory(GetOwningILLPlayer(), Action, NewChord.ToChord(), TiedBinding)) || bRebound;
					}
				}
			}
		}
	}
}

#define ISKEY(OtherKey) Key == EKeys::OtherKey

FText USCRebindingWidget::GetBoundKeyName()
{
	if (TiedBinding.BoundActions.Num() == 0)
		return FText();

	if (UILLLocalPlayer* LocalPlayer = Cast<UILLLocalPlayer>(GetOwningILLPlayer()->GetLocalPlayer()))
	{
		if (USCInputMappingsSaveGame* SaveGame = LocalPlayer->GetLoadedSettingsSave<USCInputMappingsSaveGame>())
		{
			if (UILLPlayerInput* PInput = Cast<UILLPlayerInput>(GetOwningPlayer()->PlayerInput))
			{
				if (PInput->IsUsingGamepad())
				{
					// do something...
				}
				else
				{
					if (TiedBinding.bIsAxisBinding)
					{
						for (FName Axis : TiedBinding.BoundActions)
						{
							for (FInputAxisKeyMapping Map : SaveGame->AxisMappings)
							{
								if (Map.AxisName == Axis && !Map.Key.IsGamepadKey() && Map.Scale == TiedBinding.Scale)
								{
									const FKey& Key = Map.Key;

									if (ISKEY(Delete))
										return FText::FromString(TEXT("Del"));
									else if (ISKEY(Insert))
										return FText::FromString(TEXT("Ins"));
									else if (ISKEY(Multiply))
										return FText::FromString(TEXT("*"));
									else if (ISKEY(Add))
										return FText::FromString(TEXT("+"));
									else if (ISKEY(Subtract) || ISKEY(Hyphen))
										return FText::FromString(TEXT("-"));
									else if (ISKEY(Decimal) || ISKEY(Period))
										return FText::FromString(TEXT("."));
									else if (ISKEY(Divide) || ISKEY(Slash))
										return FText::FromString(TEXT("/"));
									else if (ISKEY(Semicolon))
										return FText::FromString(TEXT(";"));
									else if (ISKEY(Equals))
										return FText::FromString(TEXT("="));
									else if (ISKEY(Comma))
										return FText::FromString(TEXT(","));
									else if (ISKEY(LeftBracket))
										return FText::FromString(TEXT("["));
									else if (ISKEY(RightBracket))
										return FText::FromString(TEXT("]"));
									else if (ISKEY(Backslash))
										return FText::FromString(TEXT("\\"));
									else if (ISKEY(Apostrophe))
										return FText::FromString(TEXT("'"));
									else
										return Key.GetDisplayName();
								}
							}
						}
					}
					else
					{
						FString RetVal = FString();
						for (FName Action : TiedBinding.BoundActions)
						{
							for (FInputActionKeyMapping Map : SaveGame->ActionMappings)
							{
								if (!Map.Key.IsGamepadKey() && Map.ActionName == Action)
								{
									// Compile the full chord name
									RetVal += Map.bCtrl ? "Ctrl + " : "";
									RetVal += Map.bAlt ? "Alt + " : "";
									RetVal += Map.bShift ? "Shift + " : "";
									RetVal += Map.bCmd ? "Cmd + " : "";

									const FKey& Key = Map.Key;
									FText KeyText = Key.GetDisplayName();
									if (ISKEY(Delete))
										KeyText = FText::FromString(TEXT("Del"));
									else if (ISKEY(Insert))
										KeyText = FText::FromString(TEXT("Ins"));
									else if (ISKEY(Multiply))
										KeyText = FText::FromString(TEXT("*"));
									else if (ISKEY(Add))
										KeyText = FText::FromString(TEXT("+"));
									else if (ISKEY(Subtract) || ISKEY(Hyphen))
										KeyText = FText::FromString(TEXT("-"));
									else if (ISKEY(Decimal) || ISKEY(Period))
										KeyText = FText::FromString(TEXT("."));
									else if (ISKEY(Divide) || ISKEY(Slash))
										KeyText = FText::FromString(TEXT("/"));
									else if (ISKEY(Semicolon))
										KeyText = FText::FromString(TEXT(";"));
									else if (ISKEY(Equals))
										KeyText = FText::FromString(TEXT("="));
									else if (ISKEY(Comma))
										KeyText = FText::FromString(TEXT(","));
									else if (ISKEY(LeftBracket))
										KeyText = FText::FromString(TEXT("["));
									else if (ISKEY(RightBracket))
										KeyText = FText::FromString(TEXT("]"));
									else if (ISKEY(Backslash))
										KeyText = FText::FromString(TEXT("\\"));
									else if (ISKEY(Apostrophe))
										KeyText = FText::FromString(TEXT("'"));

									RetVal += KeyText.ToString();

									return FText::FromString(RetVal);
								}
							}
						}
					}
				}
			}
		}
	}

	return FText::FromString("INVALID");
}

void USCRebindingWidget::InitializeBinding(FILLCustomKeyBinding InKeyBinding)
{
	TiedBinding = InKeyBinding;

	if (TiedBinding.BoundActions.Num() == 0)
		return;
}
