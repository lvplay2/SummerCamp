// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCEditorModule.h"

#include "MessageLogModule.h"
#include "PropertyEditorModule.h"

#include "Camera/SCCameraSplineComponent.h"
#include "SCCameraSplineComponentVisualizer.h"
#include "SCSplineComponentDetails.h"

IMPLEMENT_GAME_MODULE(FSCEditorModule, SCEditor);

void FSCEditorModule::StartupModule()
{
	if (GUnrealEd)
	{
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FSCCameraSplineComponentVisualizer());

		if (Visualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(USCCameraSplineComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomClassLayout(TEXT("SCCameraSplineComponent"), FOnGetDetailCustomizationInstance::CreateStatic(&FSCSplineComponentDetails::MakeInstance));
	}

	// Make a log specifically for spawning errors
	auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.RegisterLogListing("Spawning", NSLOCTEXT("SC.Spawning", "SpawningInfo", "Spawning Info"));
}

void FSCEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(USCCameraSplineComponent::StaticClass()->GetFName());
	}

	auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.UnregisterLogListing("EnlightenProjection");
}
