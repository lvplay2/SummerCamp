// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLCrowdAgentInterfaceComponent.h"

#include "Navigation/CrowdManager.h"

#include "ILLCharacter.h"

UILLCrowdAgentInterfaceComponent::UILLCrowdAgentInterfaceComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
}

void UILLCrowdAgentInterfaceComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Register ourself with the CrowdManager
	if (UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(this))
	{
		ICrowdAgentInterface* IAgent = Cast<ICrowdAgentInterface>(this);
		CrowdManager->RegisterAgent(IAgent);
	}
}

void UILLCrowdAgentInterfaceComponent::UninitializeComponent()
{
	Super::UninitializeComponent();

	// Unregister ourself with the CrowdManager
	if (UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(this))
	{
		ICrowdAgentInterface* IAgent = Cast<ICrowdAgentInterface>(this);
		CrowdManager->UnregisterAgent(IAgent);
	}
}

FVector UILLCrowdAgentInterfaceComponent::GetCrowdAgentLocation() const
{
	if (AILLCharacter* Character = Cast<AILLCharacter>(GetOwner()))
	{
		if (UPawnMovementComponent* MovementComponent = Character->GetMovementComponent())
		{
			return MovementComponent->GetActorFeetLocation();
		}
	}

	return FVector::ZeroVector;
}

FVector UILLCrowdAgentInterfaceComponent::GetCrowdAgentVelocity() const
{
	if (AILLCharacter* Character = Cast<AILLCharacter>(GetOwner()))
	{
		if (UPawnMovementComponent* MovementComponent = Character->GetMovementComponent())
		{
			return MovementComponent->Velocity;
		}
	}

	return FVector::ZeroVector;
}

void UILLCrowdAgentInterfaceComponent::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	if (AILLCharacter* Character = Cast<AILLCharacter>(GetOwner()))
	{
		UPawnMovementComponent* MovementComponent = Character->GetMovementComponent();
		if (MovementComponent && MovementComponent->UpdatedComponent)
		{
			MovementComponent->UpdatedComponent->CalcBoundingCylinder(CylinderRadius, CylinderHalfHeight);
		}
	}
}

float UILLCrowdAgentInterfaceComponent::GetCrowdAgentMaxSpeed() const
{
	if (AILLCharacter* Character = Cast<AILLCharacter>(GetOwner()))
	{
		if (UPawnMovementComponent* MovementComponent = Character->GetMovementComponent())
		{
			return MovementComponent->GetMaxSpeed();
		}
	}

	return 0.f;
}
