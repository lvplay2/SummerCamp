// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLEditorFramework.h"
#include "ILLInteractableComponentVisualizer.h"

#include "ILLInteractableComponent.h"

#include "SceneManagement.h"

void FILLInteractableComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UILLInteractableComponent* Interactable = Cast<const UILLInteractableComponent>(Component);
	if (Interactable == nullptr)
		return;

	const FColor DebugColor(142, 86, 255); // Violet
	const FVector ComponentLocation = Interactable->GetComponentLocation();
	const float Radius = Interactable->DistanceLimit;

	// No limits, draw a sphere
	if (Interactable->bUseYawLimit == false && Interactable->bUsePitchLimit == false)
	{
		DrawWireSphereAutoSides(PDI, ComponentLocation, DebugColor, Radius, SDPG_World);
	}
	else
	{
		// May have pitch or yaw limits, draw arcs
		const FTransform ComponentTransform = Interactable->GetComponentTransform();
		const FRotator ComponentRotation = Interactable->GetComponentRotation();
		const FVector X = ComponentTransform.GetUnitAxis(EAxis::X);
		const FVector ForwardPos = ComponentLocation + X * Radius;

		if (Interactable->bUseYawLimit)
		{
			const float Yaw = Interactable->MaxYawOffset;
			const FRotator YawRotation(0.f, Yaw, 0.f);
			const FVector RightPos = ComponentLocation + (ComponentRotation + YawRotation).Vector() * Radius;
			const FVector LeftPos = ComponentLocation + (ComponentRotation - YawRotation).Vector() * Radius;
			DrawArc(PDI, ComponentLocation, X, ComponentTransform.GetUnitAxis(EAxis::Y), -Yaw, Yaw, Radius, 8, DebugColor, SDPG_World);
			PDI->DrawLine(ComponentLocation, LeftPos, DebugColor, SDPG_World);
			PDI->DrawLine(ComponentLocation, RightPos, DebugColor, SDPG_World);
			PDI->DrawLine(ComponentLocation, ForwardPos, DebugColor, SDPG_World);
		}

		if (Interactable->bUsePitchLimit)
		{
			const float Pitch = Interactable->MaxPitchOffset;
			const FRotator PitchRotation(Pitch, 0.f, 0.f);
			const FVector TopPos = ComponentLocation + (ComponentRotation + PitchRotation).Vector() * Radius;
			const FVector BottomPos = ComponentLocation + (ComponentRotation - PitchRotation).Vector() * Radius;
			DrawArc(PDI, ComponentLocation, X, ComponentTransform.GetUnitAxis(EAxis::Z), -Pitch, Pitch, Radius, 8, DebugColor, SDPG_World);
			PDI->DrawLine(ComponentLocation, TopPos, DebugColor, SDPG_World);
			PDI->DrawLine(ComponentLocation, BottomPos, DebugColor, SDPG_World);
			PDI->DrawLine(ComponentLocation, ForwardPos, DebugColor, SDPG_World);
		}
	}
}
