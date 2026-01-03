// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

#include "SummerCamp.h"
#include "SCNavModifierComponent_Vehicle.h"

#include "AI/NavigationModifier.h"
#include "AI/NavigationOctree.h"
#include "Navigation/CrowdManager.h"
#include "PhysicsEngine/BodySetup.h"

#include "NavAreas/SCNavArea_CarOnly.h"
#include "SCDriveableVehicle.h"
#include "SCWheeledVehicleMovementComponent.h"

USCNavModifierComponent_Vehicle::USCNavModifierComponent_Vehicle(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	AreaClass = USCNavArea_CarOnly::StaticClass();
	bIncludeAgentHeight = true;
	bNavigationRelevant = true;
}

void USCNavModifierComponent_Vehicle::OnRegister()
{
	Super::OnRegister();

	// Register ourself with the CrowdManager
	if (UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(this))
	{
		ICrowdAgentInterface* IAgent = Cast<ICrowdAgentInterface>(this);
		CrowdManager->RegisterAgent(IAgent);
	}
}

void USCNavModifierComponent_Vehicle::OnUnregister()
{
	Super::OnUnregister();

	// Unregister ourself with the CrowdManager
	if (UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(this))
	{
		ICrowdAgentInterface* IAgent = Cast<ICrowdAgentInterface>(this);
		CrowdManager->UnregisterAgent(IAgent);
	}
}

FBox USCNavModifierComponent_Vehicle::GetNavigationBounds() const
{
	CalcAndCacheBounds();
	return Bounds;
}

void USCNavModifierComponent_Vehicle::CalcAndCacheBounds() const
{
	const ASCDriveableVehicle* MyVehicle = Cast<ASCDriveableVehicle>(GetOwner());
	if (MyVehicle)
	{
		USkeletalMeshComponent* Mesh = MyVehicle->GetMesh();
		
		Bounds = FBox(ForceInit);
		ComponentBounds.Reset();
		if (Mesh && Mesh->IsRegistered() && Mesh->IsCollisionEnabled())
		{
			UBodySetup* BodySetup = Mesh->GetBodySetup();
			if (BodySetup)
			{
				FTransform ParentTM = Mesh->GetComponentToWorld();
				const FVector Scale3D = ParentTM.GetScale3D();
				ParentTM.RemoveScaling();
				Bounds += Mesh->Bounds.GetBox();

				for (int32 SphereIdx = 0; SphereIdx < BodySetup->AggGeom.SphereElems.Num(); ++SphereIdx)
				{
					const FKSphereElem& ElemInfo = BodySetup->AggGeom.SphereElems[SphereIdx];
					FTransform ElemTM = ElemInfo.GetTransform();
					ElemTM.ScaleTranslation(Scale3D);
					ElemTM *= ParentTM;

					const FBox SphereBounds = FBox::BuildAABB(ElemTM.GetLocation(), ElemInfo.Radius * Scale3D);
					ComponentBounds.Add(FRotatedBox(SphereBounds, ElemTM.GetRotation()));
				}

				for (int32 BoxIdx = 0; BoxIdx < BodySetup->AggGeom.BoxElems.Num(); ++BoxIdx)
				{
					const FKBoxElem& ElemInfo = BodySetup->AggGeom.BoxElems[BoxIdx];
					FTransform ElemTM = ElemInfo.GetTransform();
					ElemTM.ScaleTranslation(Scale3D);
					ElemTM *= ParentTM;

					const FBox BoxBounds = FBox::BuildAABB(ElemTM.GetLocation(), FVector(ElemInfo.X, ElemInfo.Y, ElemInfo.Z) * Scale3D * 0.5f);
					ComponentBounds.Add(FRotatedBox(BoxBounds, ElemTM.GetRotation()));
				}

				for (int32 SphylIdx = 0; SphylIdx < BodySetup->AggGeom.SphylElems.Num(); ++SphylIdx)
				{
					const FKSphylElem& ElemInfo = BodySetup->AggGeom.SphylElems[SphylIdx];
					FTransform ElemTM = ElemInfo.GetTransform();
					ElemTM.ScaleTranslation(Scale3D);
					ElemTM *= ParentTM;

					const FBox SphylBounds = FBox::BuildAABB(ElemTM.GetLocation(), FVector(ElemInfo.Radius, ElemInfo.Radius, ElemInfo.Length) * Scale3D);
					ComponentBounds.Add(FRotatedBox(SphylBounds, ElemTM.GetRotation()));
				}

				for (int32 ConvexIdx = 0; ConvexIdx < BodySetup->AggGeom.ConvexElems.Num(); ++ConvexIdx)
				{
					const FKConvexElem& ElemInfo = BodySetup->AggGeom.ConvexElems[ConvexIdx];
					FTransform ElemTM = ElemInfo.GetTransform();
					ElemTM.ScaleTranslation(Scale3D);
					ElemTM *= FTransform(ParentTM.GetLocation());

					const FBox ConvexBounds = ElemInfo.CalcAABB(ElemTM, Scale3D);
					ComponentBounds.Add(FRotatedBox(ConvexBounds, ElemTM.GetRotation() * ParentTM.GetRotation()));
				}
			}
		}

		if (ComponentBounds.Num() == 0)
		{
			Bounds = FBox::BuildAABB(MyVehicle->GetActorLocation(), FailsafeExtent);
			ComponentBounds.Add(FRotatedBox(Bounds, MyVehicle->GetActorQuat()));
		}
		else
		{
			for (int32 Idx = 0; Idx < ComponentBounds.Num(); ++Idx)
			{
				const FVector BoxOrigin = ComponentBounds[Idx].Box.GetCenter();
				const FVector BoxExtent = ComponentBounds[Idx].Box.GetExtent();

				const FVector NavModBoxOrigin = FTransform(ComponentBounds[Idx].Quat).InverseTransformPosition(BoxOrigin);
				ComponentBounds[Idx].Box = FBox::BuildAABB(NavModBoxOrigin, BoxExtent);
			}
		}
	}
}

void USCNavModifierComponent_Vehicle::GetNavigationData(FNavigationRelevantData& Data) const
{
	for (int32 Idx = 0; Idx < ComponentBounds.Num(); Idx++)
	{
		FAreaNavModifier NavModifier(ComponentBounds[Idx].Box, FTransform(ComponentBounds[Idx].Quat), AreaClass);
		NavModifier.SetIncludeAgentHeight(bIncludeAgentHeight);
		Data.Modifiers.Add(NavModifier);
	}
}

FVector USCNavModifierComponent_Vehicle::GetCrowdAgentLocation() const
{
	if (const ASCDriveableVehicle* MyVehicle = Cast<ASCDriveableVehicle>(GetOwner()))
	{
		return MyVehicle->GetActorLocation();
	}

	return FNavigationSystem::InvalidLocation;
}

FVector USCNavModifierComponent_Vehicle::GetCrowdAgentVelocity() const
{
	const ASCDriveableVehicle* MyVehicle = Cast<ASCDriveableVehicle>(GetOwner());
	const USCWheeledVehicleMovementComponent* VehicleMovement = MyVehicle ? Cast<USCWheeledVehicleMovementComponent>(MyVehicle->GetMovementComponent()) : nullptr;
	if (VehicleMovement)
	{
		return VehicleMovement->Velocity;
	}

	return FVector::ZeroVector;
}

void USCNavModifierComponent_Vehicle::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	const ASCDriveableVehicle* MyVehicle = Cast<ASCDriveableVehicle>(GetOwner());
	const USCWheeledVehicleMovementComponent* VehicleMovement = MyVehicle ? Cast<USCWheeledVehicleMovementComponent>(MyVehicle->GetMovementComponent()) : nullptr;
	if (VehicleMovement && VehicleMovement->UpdatedComponent)
	{
		VehicleMovement->UpdatedComponent->CalcBoundingCylinder(CylinderRadius, CylinderHalfHeight);
	}
}

float USCNavModifierComponent_Vehicle::GetCrowdAgentMaxSpeed() const
{
	const ASCDriveableVehicle* MyVehicle = Cast<ASCDriveableVehicle>(GetOwner());
	const USCWheeledVehicleMovementComponent* VehicleMovement = MyVehicle ? Cast<USCWheeledVehicleMovementComponent>(MyVehicle->GetMovementComponent()) : nullptr;
	if (VehicleMovement)
	{
		return VehicleMovement->GetMaxSpeed();
	}

	return 0.f;
}
