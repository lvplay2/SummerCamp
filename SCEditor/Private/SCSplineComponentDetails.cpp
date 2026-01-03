// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

/****************************************
* RIPPED FROM Source/Editor/DetailCustomizations/Private/SplineComponentDetails.cpp
***************************************/

#include "SummerCamp.h"
#include "Camera/SCCameraSplineComponent.h"

#include "SCCameraSplineComponentVisualizer.h"
#include "SCSplineComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomNodeBuilder.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "ScopedTransaction.h"
#include "SNumericEntryBox.h"
#include "SRotatorInputBox.h"
#include "STextComboBox.h"
#include "SVectorInputBox.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "ILLFonic.F13.SCSplineComponentDetails"

class FSCSplinePointDetails
	: public IDetailCustomNodeBuilder
	, public TSharedFromThis<FSCSplinePointDetails>
{
public:
	FSCSplinePointDetails();

	//~ Begin IDetailCustomNodeBuilder interface
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override;
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool RequiresTick() const override { return true; }
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual FName GetName() const override;
	//~ End IDetailCustomNodeBuilder interface

private:

	template <typename T>
	struct TSharedValue
	{
		TSharedValue() : bInitialized(false) {}

		void Reset()
		{
			bInitialized = false;
		}

		void Add(T InValue)
		{
			if (!bInitialized)
			{
				Value = InValue;
				bInitialized = true;
			}
			else
			{
				if (Value.IsSet() && InValue != Value.GetValue()) { Value.Reset(); }
			}
		}

		TOptional<T> Value;
		bool bInitialized;
	};

	struct FSharedVectorValue
	{
		FSharedVectorValue() : bInitialized(false) {}

		void Reset()
		{
			bInitialized = false;
		}

		bool IsValid() const { return bInitialized; }

		void Add(const FVector& V)
		{
			if (!bInitialized)
			{
				X = V.X;
				Y = V.Y;
				Z = V.Z;
				bInitialized = true;
			}
			else
			{
				if (X.IsSet() && V.X != X.GetValue()) { X.Reset(); }
				if (Y.IsSet() && V.Y != Y.GetValue()) { Y.Reset(); }
				if (Z.IsSet() && V.Z != Z.GetValue()) { Z.Reset(); }
			}
		}

		TOptional<float> X;
		TOptional<float> Y;
		TOptional<float> Z;
		bool bInitialized;
	};

	struct FSharedRotatorValue
	{
		FSharedRotatorValue() : bInitialized(false) {}

		void Reset()
		{
			bInitialized = false;
		}

		bool IsValid() const { return bInitialized; }

		void Add(const FRotator& R)
		{
			if (!bInitialized)
			{
				Roll = R.Roll;
				Pitch = R.Pitch;
				Yaw = R.Yaw;
				bInitialized = true;
			}
			else
			{
				if (Roll.IsSet() && R.Roll != Roll.GetValue()) { Roll.Reset(); }
				if (Pitch.IsSet() && R.Pitch != Pitch.GetValue()) { Pitch.Reset(); }
				if (Yaw.IsSet() && R.Yaw != Yaw.GetValue()) { Yaw.Reset(); }
			}
		}

		TOptional<float> Roll;
		TOptional<float> Pitch;
		TOptional<float> Yaw;
		bool bInitialized;
	};

	EVisibility IsEnabled() const { return (SelectedKeys.Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed; }
	EVisibility IsDisabled() const { return (SelectedKeys.Num() == 0) ? EVisibility::Visible : EVisibility::Collapsed; }
	bool IsOnePointSelected() const { return SelectedKeys.Num() == 1; }
	TOptional<float> GetInputKey() const { return InputKey.Value; }
	TOptional<float> GetPositionX() const { return Position.X; }
	TOptional<float> GetPositionY() const { return Position.Y; }
	TOptional<float> GetPositionZ() const { return Position.Z; }
	TOptional<float> GetArriveTangentX() const { return ArriveTangent.X; }
	TOptional<float> GetArriveTangentY() const { return ArriveTangent.Y; }
	TOptional<float> GetArriveTangentZ() const { return ArriveTangent.Z; }
	TOptional<float> GetLeaveTangentX() const { return LeaveTangent.X; }
	TOptional<float> GetLeaveTangentY() const { return LeaveTangent.Y; }
	TOptional<float> GetLeaveTangentZ() const { return LeaveTangent.Z; }
	float GetRotationRoll() const { return Rotation.Roll.IsSet() ? Rotation.Roll.GetValue() : 0.f; }
	float GetRotationPitch() const { return Rotation.Pitch.IsSet() ? Rotation.Pitch.GetValue() : 0.f; }
	float GetRotationYaw() const { return Rotation.Yaw.IsSet() ? Rotation.Yaw.GetValue() : 0.f; }
	TOptional<float> GetScaleX() const { return Scale.X; }
	TOptional<float> GetScaleY() const { return Scale.Y; }
	TOptional<float> GetScaleZ() const { return Scale.Z; }
	TOptional<float> GetFOV() const { return FoV.Value; }
	TOptional<float> GetHoldTime() const { return HoldTime.Value; }
	TOptional<float> GetTimeToNext() const { return TimeToNext.Value; }
	TOptional<float> GetTimeAtPoint() const { return TimeAtPoint.Value; }
	FText GetPointRotationType() const;
	void OnSetInputKey(float NewValue, ETextCommit::Type CommitInfo);
	void OnSetPosition(float NewValue, ETextCommit::Type CommitInfo, int32 Axis);
	void OnSetArriveTangent(float NewValue, ETextCommit::Type CommitInfo, int32 Axis);
	void OnSetLeaveTangent(float NewValue, ETextCommit::Type CommitInfo, int32 Axis);
	void OnSetRotation(float NewValue, int32 Axis);
	void OnSetScale(float NewValue, ETextCommit::Type CommitInfo, int32 Axis);
	void OnSetFOV(float NewValue, ETextCommit::Type CommitInfo);
	void OnSetHoldTime(float NewValue, ETextCommit::Type CommitInfo);
	void OnSetTimeToNext(float NewValue, ETextCommit::Type CommitInfo);
	void OnSetTimeAtPoint(float NewValue, ETextCommit::Type CommitInfo);
	void OnSplinePointRotationTypeChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	FText GetPointType() const;
	void OnSplinePointTypeChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<FString> InComboString);

	void UpdateValues();

	USCCameraSplineComponent* SplineComp;
	TSet<int32> SelectedKeys;

	TSharedValue<float> InputKey;
	FSharedVectorValue Position;
	FSharedVectorValue ArriveTangent;
	FSharedVectorValue LeaveTangent;
	FSharedVectorValue Scale;
	FSharedRotatorValue Rotation;
	TSharedValue<float> FoV;
	TSharedValue<float> HoldTime;
	TSharedValue<float> TimeToNext;
	TSharedValue<float> TimeAtPoint;
	TSharedValue<ESplinePointType::Type> PointType;
	TSharedValue<ESplinePointType::Type> RotationType;

	FSCCameraSplineComponentVisualizer* SplineVisualizer;
	UProperty* SplineCurvesProperty;
	TArray<TSharedPtr<FString>> SplinePointTypes;
};

FSCSplinePointDetails::FSCSplinePointDetails()
{
	SplineComp = nullptr;
	TSharedPtr<FComponentVisualizer> Visualizer = GUnrealEd->FindComponentVisualizer(USCCameraSplineComponent::StaticClass());
	SplineVisualizer = (FSCCameraSplineComponentVisualizer*)Visualizer.Get();
	check(SplineVisualizer);

	SplineCurvesProperty = FindField<UProperty>(USCCameraSplineComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, SplineCurves));

	UEnum* SplinePointTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ESplinePointType"));
	check(SplinePointTypeEnum);
	for (int32 EnumIndex = 0; EnumIndex < SplinePointTypeEnum->NumEnums() - 1; ++EnumIndex)
	{
		SplinePointTypes.Add(MakeShareable(new FString(SplinePointTypeEnum->GetNameStringByIndex(EnumIndex))));
	}
}

void FSCSplinePointDetails::SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren)
{
}

void FSCSplinePointDetails::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
}


void FSCSplinePointDetails::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	// Message which is shown when no points are selected
	ChildrenBuilder.AddCustomRow(LOCTEXT("NoneSelected", "None selected"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsDisabled))
	[
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoPointsSelected", "No spline points are selected."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	];

	// Input key
	ChildrenBuilder.AddCustomRow(LOCTEXT("InputKey", "Input Key"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("InputKey", "Input Key"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(125.0f)
	[
		SNew(SNumericEntryBox<float>)
		.IsEnabled(TAttribute<bool>(this, &FSCSplinePointDetails::IsOnePointSelected))
		.Value(this, &FSCSplinePointDetails::GetInputKey)
		.UndeterminedString(LOCTEXT("Multiple", "Multiple"))
		.OnValueCommitted(this, &FSCSplinePointDetails::OnSetInputKey)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Position
	ChildrenBuilder.AddCustomRow(LOCTEXT("Position", "Position"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Position", "Position"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(375.0f)
	.MaxDesiredWidth(375.0f)
	[
		SNew(SVectorInputBox)
		.X(this, &FSCSplinePointDetails::GetPositionX)
		.Y(this, &FSCSplinePointDetails::GetPositionY)
		.Z(this, &FSCSplinePointDetails::GetPositionZ)
		.AllowResponsiveLayout(true)
		.OnXCommitted(this, &FSCSplinePointDetails::OnSetPosition, 0)
		.OnYCommitted(this, &FSCSplinePointDetails::OnSetPosition, 1)
		.OnZCommitted(this, &FSCSplinePointDetails::OnSetPosition, 2)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// ArriveTangent
	ChildrenBuilder.AddCustomRow(LOCTEXT("ArriveTangent", "Arrive Tangent"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ArriveTangent", "Arrive Tangent"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(375.0f)
	.MaxDesiredWidth(375.0f)
	[
		SNew(SVectorInputBox)
		.X(this, &FSCSplinePointDetails::GetArriveTangentX)
		.Y(this, &FSCSplinePointDetails::GetArriveTangentY)
		.Z(this, &FSCSplinePointDetails::GetArriveTangentZ)
		.AllowResponsiveLayout(true)
		.OnXCommitted(this, &FSCSplinePointDetails::OnSetArriveTangent, 0)
		.OnYCommitted(this, &FSCSplinePointDetails::OnSetArriveTangent, 1)
		.OnZCommitted(this, &FSCSplinePointDetails::OnSetArriveTangent, 2)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// LeaveTangent
	ChildrenBuilder.AddCustomRow(LOCTEXT("LeaveTangent", "Leave Tangent"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("LeaveTangent", "Leave Tangent"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(375.0f)
	.MaxDesiredWidth(375.0f)
	[
		SNew(SVectorInputBox)
		.X(this, &FSCSplinePointDetails::GetLeaveTangentX)
		.Y(this, &FSCSplinePointDetails::GetLeaveTangentY)
		.Z(this, &FSCSplinePointDetails::GetLeaveTangentZ)
		.AllowResponsiveLayout(true)
		.OnXCommitted(this, &FSCSplinePointDetails::OnSetLeaveTangent, 0)
		.OnYCommitted(this, &FSCSplinePointDetails::OnSetLeaveTangent, 1)
		.OnZCommitted(this, &FSCSplinePointDetails::OnSetLeaveTangent, 2)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Rotation
	ChildrenBuilder.AddCustomRow(LOCTEXT("Rotation", "Rotation"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Rotation", "Rotation"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(375.0f)
	.MaxDesiredWidth(375.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.33)
		[
			SNew(SSpinBox<float>)
			.MinValue(-180.0f)
			.MaxValue(180.0f)
			. Delta(0.01f)
			.Value(this, &FSCSplinePointDetails::GetRotationRoll)
			.OnValueChanged(this, &FSCSplinePointDetails::OnSetRotation, 0)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]

		+SHorizontalBox::Slot()
		.FillWidth(0.33)
		[
			SNew(SSpinBox<float>)
			.MinValue(-180.0f)
			.MaxValue(180.0f)
			. Delta(0.01f)
			.Value(this, &FSCSplinePointDetails::GetRotationPitch)
			.OnValueChanged(this, &FSCSplinePointDetails::OnSetRotation, 1)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]

		+SHorizontalBox::Slot()
		.FillWidth(0.33)
		[
			SNew(SSpinBox<float>)
			.MinValue(-180.0f)
			.MaxValue(180.0f)
			. Delta(0.01f)
			.Value(this, &FSCSplinePointDetails::GetRotationYaw)
			.OnValueChanged(this, &FSCSplinePointDetails::OnSetRotation, 2)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	];

	// Scale
	ChildrenBuilder.AddCustomRow(LOCTEXT("Scale", "Scale"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Scale", "Scale"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(375.0f)
	.MaxDesiredWidth(375.0f)
	[
		SNew(SVectorInputBox)
		.X(this, &FSCSplinePointDetails::GetScaleX)
		.Y(this, &FSCSplinePointDetails::GetScaleY)
		.Z(this, &FSCSplinePointDetails::GetScaleZ)
		.AllowResponsiveLayout(true)
		.OnXCommitted(this, &FSCSplinePointDetails::OnSetScale, 0)
		.OnYCommitted(this, &FSCSplinePointDetails::OnSetScale, 1)
		.OnZCommitted(this, &FSCSplinePointDetails::OnSetScale, 2)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// FOV
	ChildrenBuilder.AddCustomRow(LOCTEXT("FOV", "Field of View"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("FOV", "Field of View"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(125.0f)
	[
		SNew(SNumericEntryBox<float>)
		.Value(this, &FSCSplinePointDetails::GetFOV)
		.UndeterminedString(LOCTEXT("Multiple", "Multiple"))
		.OnValueCommitted(this, &FSCSplinePointDetails::OnSetFOV)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Hold Time
	ChildrenBuilder.AddCustomRow(LOCTEXT("HoldTime", "Hold Time"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("HoldTime", "Hold Time"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(125.0f)
	[
		SNew(SNumericEntryBox<float>)
		.Value(this, &FSCSplinePointDetails::GetHoldTime)
		.UndeterminedString(LOCTEXT("Multiple", "Multiple"))
		.OnValueCommitted(this, &FSCSplinePointDetails::OnSetHoldTime)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Time To Next
	ChildrenBuilder.AddCustomRow(LOCTEXT("TimeToNext", "Time To Next"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("TimeToNext", "Time To Next"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(125.0f)
	[
		SNew(SNumericEntryBox<float>)
		.Value(this, &FSCSplinePointDetails::GetTimeToNext)
		.UndeterminedString(LOCTEXT("Multiple", "Multiple"))
		.OnValueCommitted(this, &FSCSplinePointDetails::OnSetTimeToNext)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Time At Point
	ChildrenBuilder.AddCustomRow(LOCTEXT("TimeAtPoint", "Time At Point"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("TimeAtPoint", "Time At Point"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(125.0f)
	[
		SNew(SNumericEntryBox<float>)
		.Value(this, &FSCSplinePointDetails::GetTimeAtPoint)
		.UndeterminedString(LOCTEXT("Multiple", "Multiple"))
		.OnValueCommitted(this, &FSCSplinePointDetails::OnSetTimeAtPoint)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Type
	ChildrenBuilder.AddCustomRow(LOCTEXT("Type", "Type"))
	.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
	.NameContent()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Type", "Type"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(125.0f)
	[
		SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&SplinePointTypes)
		.OnGenerateWidget(this, &FSCSplinePointDetails::OnGenerateComboWidget)
		.OnSelectionChanged(this, &FSCSplinePointDetails::OnSplinePointTypeChanged)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FSCSplinePointDetails::GetPointType)
		]
	];

	// FOV Curve Type
	ChildrenBuilder.AddCustomRow(LOCTEXT("RotationType", "RotationType"))
		.Visibility(TAttribute<EVisibility>(this, &FSCSplinePointDetails::IsEnabled))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RotationType", "RotationType"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		.MinDesiredWidth(125.0f)
		.MaxDesiredWidth(125.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&SplinePointTypes)
		.OnGenerateWidget(this, &FSCSplinePointDetails::OnGenerateComboWidget)
		.OnSelectionChanged(this, &FSCSplinePointDetails::OnSplinePointRotationTypeChanged)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(this, &FSCSplinePointDetails::GetPointRotationType)
		]
		];
}

void FSCSplinePointDetails::Tick(float DeltaTime)
{
	UpdateValues();
}

void FSCSplinePointDetails::UpdateValues()
{
	SplineComp = SplineVisualizer->GetEditedSplineComponent();
	SelectedKeys = SplineVisualizer->GetSelectedKeys();

	// Cache values to be shown by the details customization.
	// An unset optional value represents 'multiple values' (in the case where multiple points are selected).
	InputKey.Reset();
	Position.Reset();
	ArriveTangent.Reset();
	LeaveTangent.Reset();
	Rotation.Reset();
	Scale.Reset();
	FoV.Reset();
	HoldTime.Reset();
	TimeToNext.Reset();
	TimeAtPoint.Reset();
	PointType.Reset();
	RotationType.Reset();

	for (int32 Index : SelectedKeys)
	{
		InputKey.Add(SplineComp->GetSplinePointsPosition().Points[Index].InVal);
		Position.Add(SplineComp->GetSplinePointsPosition().Points[Index].OutVal);
		ArriveTangent.Add(SplineComp->GetSplinePointsPosition().Points[Index].ArriveTangent);
		LeaveTangent.Add(SplineComp->GetSplinePointsPosition().Points[Index].LeaveTangent);
		Rotation.Add(SplineComp->GetSplinePointsRotation().Points[Index].OutVal.Rotator());
		Scale.Add(SplineComp->GetSplinePointsScale().Points[Index].OutVal);
		FoV.Add(SplineComp->GetSplinePointsFOV().Points[Index].OutVal);
		HoldTime.Add(SplineComp->GetSplinePointsHoldTime().Points[Index].OutVal);
		TimeToNext.Add(SplineComp->GetSplinePointsTimeToNext().Points[Index].OutVal);
		TimeAtPoint.Add(SplineComp->GetSplinePointsTimeAtPoint().Points[Index].OutVal);
		PointType.Add(ConvertInterpCurveModeToSplinePointType(SplineComp->GetSplinePointsPosition().Points[Index].InterpMode));
		RotationType.Add(ConvertInterpCurveModeToSplinePointType(SplineComp->GetSplinePointsRotation().Points[Index].InterpMode));
	}
}

FName FSCSplinePointDetails::GetName() const
{
	static const FName Name("SplinePointDetails");
	return Name;
}

void FSCSplinePointDetails::OnSetInputKey(float NewValue, ETextCommit::Type CommitInfo)
{
	if (CommitInfo != ETextCommit::OnEnter && CommitInfo != ETextCommit::OnUserMovedFocus)
	{
		return;
	}

	check(SelectedKeys.Num() == 1);
	const int32 Index = *SelectedKeys.CreateConstIterator();
	TArray<FInterpCurvePoint<FVector>>& Positions = SplineComp->GetSplinePointsPosition().Points;

	const int32 NumPoints = Positions.Num();

	bool bModifyOtherPoints = false;
	if ((Index > 0 && NewValue <= Positions[Index - 1].InVal) ||
		(Index < NumPoints - 1 && NewValue >= Positions[Index + 1].InVal))
	{
		const FText Title(LOCTEXT("InputKeyTitle", "Input key out of range"));
		const FText Message(LOCTEXT("InputKeyMessage", "Spline input keys must be numerically ascending. Would you like to modify other input keys in the spline in order to be able to set this value?"));

		// Ensure input keys remain ascending
		if (FMessageDialog::Open(EAppMsgType::YesNo, Message, &Title) == EAppReturnType::No)
		{
			return;
		}

		bModifyOtherPoints = true;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointInputKey", "Set spline point input key"));
	SplineComp->Modify();

	TArray<FInterpCurvePoint<FQuat>>& Rotations = SplineComp->GetSplinePointsRotation().Points;
	TArray<FInterpCurvePoint<FVector>>& Scales = SplineComp->GetSplinePointsScale().Points;

	if (bModifyOtherPoints)
	{
		// Shuffle the previous or next input keys down or up so the input value remains in sequence
		if (Index > 0 && NewValue <= Positions[Index - 1].InVal)
		{
			float Delta = (NewValue - Positions[Index].InVal);
			for (int32 PrevIndex = 0; PrevIndex < Index; PrevIndex++)
			{
				Positions[PrevIndex].InVal += Delta;
				Rotations[PrevIndex].InVal += Delta;
				Scales[PrevIndex].InVal += Delta;
			}
		}
		else if (Index < NumPoints - 1 && NewValue >= Positions[Index + 1].InVal)
		{
			float Delta = (NewValue - Positions[Index].InVal);
			for (int32 NextIndex = Index + 1; NextIndex < NumPoints; NextIndex++)
			{
				Positions[NextIndex].InVal += Delta;
				Rotations[NextIndex].InVal += Delta;
				Scales[NextIndex].InVal += Delta;
			}
		}
	}

	Positions[Index].InVal = NewValue;
	Rotations[Index].InVal = NewValue;
	Scales[Index].InVal = NewValue;

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetPosition(float NewValue, ETextCommit::Type CommitInfo, int32 Axis)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointPosition", "Set spline point position"));
	SplineComp->Modify();

	for (int32 Index : SelectedKeys)
	{
		FVector PointPosition = SplineComp->GetSplinePointsPosition().Points[Index].OutVal;
		PointPosition.Component(Axis) = NewValue;
		SplineComp->GetSplinePointsPosition().Points[Index].OutVal = PointPosition;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetArriveTangent(float NewValue, ETextCommit::Type CommitInfo, int32 Axis)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointTangent", "Set spline point tangent"));
	SplineComp->Modify();

	for (int32 Index : SelectedKeys)
	{
		FVector PointTangent = SplineComp->GetSplinePointsPosition().Points[Index].ArriveTangent;
		PointTangent.Component(Axis) = NewValue;
		SplineComp->GetSplinePointsPosition().Points[Index].ArriveTangent = PointTangent;
		SplineComp->GetSplinePointsPosition().Points[Index].InterpMode = CIM_CurveUser;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetLeaveTangent(float NewValue, ETextCommit::Type CommitInfo, int32 Axis)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointTangent", "Set spline point tangent"));
	SplineComp->Modify();

	for (int32 Index : SelectedKeys)
	{
		FVector PointTangent = SplineComp->GetSplinePointsPosition().Points[Index].LeaveTangent;
		PointTangent.Component(Axis) = NewValue;
		SplineComp->GetSplinePointsPosition().Points[Index].LeaveTangent = PointTangent;
		SplineComp->GetSplinePointsPosition().Points[Index].InterpMode = CIM_CurveUser;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetRotation(float NewValue, int32 Axis)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointRotation", "Set spline point rotation"));
	SplineComp->Modify();

	for (int32 Index : SelectedKeys)
	{
		FRotator PointRotation = SplineComp->GetSplinePointsRotation().Points[Index].OutVal.Rotator();

		switch (Axis)
		{
			case 0: PointRotation.Roll = NewValue; break;
			case 1: PointRotation.Pitch = NewValue; break;
			case 2: PointRotation.Yaw = NewValue; break;
		}

		SplineComp->GetSplinePointsRotation().Points[Index].OutVal = PointRotation.Quaternion();
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetScale(float NewValue, ETextCommit::Type CommitInfo, int32 Axis)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointScale", "Set spline point scale"));
	SplineComp->Modify();

	for (int32 Index : SelectedKeys)
	{
		FVector PointScale = SplineComp->GetSplinePointsScale().Points[Index].OutVal;
		PointScale.Component(Axis) = NewValue;
		SplineComp->GetSplinePointsScale().Points[Index].OutVal = PointScale;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetFOV(float NewValue, ETextCommit::Type CommitInfo)
{
	for (int32 Index : SelectedKeys)
	{
		SplineComp->GetSplinePointsFOV().Points[Index].OutVal = NewValue;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetHoldTime(float NewValue, ETextCommit::Type CommitInfo)
{
	int32 NumPoints = SplineComp->GetSplinePointsHoldTime().Points.Num();
	float TrueTime = 0.f;
	float TimeAtIndex = 0.f;
	float TimeAtNext = 0.f;
	for (int32 Index : SelectedKeys)
	{
		// if it's the last point we don't want to auto adjust the TimeToNext
		if (Index == NumPoints - 1)
			continue;

		TimeAtIndex = SplineComp->GetSplinePointsTimeAtPoint().Points[Index].OutVal;
		TimeAtNext = SplineComp->GetSplinePointsTimeAtPoint().Points[Index + 1].OutVal;

		TrueTime = TimeAtNext - TimeAtIndex;
		TrueTime -= NewValue;
		if (TrueTime < 0)
		{
			NewValue = FMath::Clamp(NewValue + TrueTime, 0.f, NewValue);
			TrueTime = 0;
		}

		SplineComp->GetSplinePointsTimeToNext().Points[Index].OutVal = TrueTime;
		SplineComp->GetSplinePointsHoldTime().Points[Index].OutVal = NewValue;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetTimeToNext(float NewValue, ETextCommit::Type CommitInfo)
{
	int32 NumPoints = SplineComp->GetSplinePointsHoldTime().Points.Num();
	for (int32 Index : SelectedKeys)
	{
		// Only allow manual adjustment for the last point, used for closed loop splines
		if (Index != NumPoints - 1)
			continue;

		SplineComp->GetSplinePointsTimeToNext().Points[Index].OutVal = NewValue;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSetTimeAtPoint(float NewValue, ETextCommit::Type CommitInfo)
{
	if (SelectedKeys.Num() > 1)
		return;

	FInterpCurveFloat TimeAtPointCurve = SplineComp->GetSplinePointsTimeAtPoint();
	int32 NumPoints = TimeAtPointCurve.Points.Num();
	float TimeAtPrevious = 0.f;
	float TimeAtNext = FLT_MAX;

	for (int32 Index : SelectedKeys)
	{
		// The first key has to be 0.
		if (Index <= 0)
			return;

		// If it's the last key we need to clamp the values lower bounds but not the upper.
		if (Index == NumPoints - 1)
		{
			TimeAtPrevious = TimeAtPointCurve.Points[Index - 1].OutVal;

			NewValue = FMath::Clamp(NewValue, TimeAtPrevious + KINDA_SMALL_NUMBER, TimeAtNext);
			SplineComp->GetSplinePointsTimeAtPoint().Points[Index].OutVal = NewValue;

			float OldHoldTime = SplineComp->GetSplinePointsHoldTime().Points[Index - 1].OutVal;
			float TrueTime = NewValue - TimeAtPrevious;
			TrueTime -= OldHoldTime;
			if (TrueTime < 0)
			{
				OldHoldTime = FMath::Clamp(OldHoldTime - TrueTime, 0.f, OldHoldTime);
				TrueTime = 0;
			}

			SplineComp->GetSplinePointsTimeToNext().Points[Index - 1].OutVal = TrueTime;
			SplineComp->GetSplinePointsHoldTime().Points[Index - 1].OutVal = OldHoldTime;
		}
		else
		{

			TimeAtPrevious = TimeAtPointCurve.Points[Index - 1].OutVal;
			TimeAtNext = TimeAtPointCurve.Points[Index + 1].OutVal;

			// Clamp our new value to the surrounding key times.
			NewValue = FMath::Clamp(NewValue, TimeAtPrevious + KINDA_SMALL_NUMBER, TimeAtNext - KINDA_SMALL_NUMBER);
			SplineComp->GetSplinePointsTimeAtPoint().Points[Index].OutVal = NewValue;

			// Update the previous key
			float OldHoldTime = SplineComp->GetSplinePointsHoldTime().Points[Index - 1].OutVal;
			float TrueTime = NewValue - TimeAtPrevious;
			TrueTime -= OldHoldTime;
			if (TrueTime < 0)
			{
				OldHoldTime = FMath::Clamp(OldHoldTime + TrueTime, 0.f, OldHoldTime);
				TrueTime = 0;
			}

			SplineComp->GetSplinePointsTimeToNext().Points[Index - 1].OutVal = TrueTime;
			SplineComp->GetSplinePointsHoldTime().Points[Index - 1].OutVal = OldHoldTime;

			// Update this key
			OldHoldTime = SplineComp->GetSplinePointsHoldTime().Points[Index].OutVal;

			TrueTime = TimeAtNext - NewValue;
			TrueTime -= OldHoldTime;
			if (TrueTime < 0)
			{
				OldHoldTime = FMath::Clamp(OldHoldTime - TrueTime, 0.f, OldHoldTime);
				TrueTime = 0;
			}

			SplineComp->GetSplinePointsTimeToNext().Points[Index].OutVal = TrueTime;
			SplineComp->GetSplinePointsHoldTime().Points[Index].OutVal = OldHoldTime;
		}

		SplineComp->bSplineHasBeenEdited = true;
		FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
		UpdateValues();
	}
}

FText FSCSplinePointDetails::GetPointType() const
{
	if (PointType.Value.IsSet())
	{
		return FText::FromString(*SplinePointTypes[PointType.Value.GetValue()]);
	}

	return LOCTEXT("MultipleTypes", "Multiple Types");
}

FText FSCSplinePointDetails::GetPointRotationType() const
{
	if (RotationType.Value.IsSet())
	{
		return FText::FromString(*SplinePointTypes[RotationType.Value.GetValue()]);
	}

	return LOCTEXT("MultipleTypes", "Multiple Types");
}

void FSCSplinePointDetails::OnSplinePointTypeChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointType", "Set spline point type"));
	SplineComp->Modify();

	EInterpCurveMode Mode = ConvertSplinePointTypeToInterpCurveMode((ESplinePointType::Type)SplinePointTypes.Find(NewValue));

	for (int32 Index : SelectedKeys)
	{
		SplineComp->GetSplinePointsPosition().Points[Index].InterpMode = Mode;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

void FSCSplinePointDetails::OnSplinePointRotationTypeChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	const FScopedTransaction Transaction(LOCTEXT("SetSplinePointRotationType", "Set spline point rotation interp type"));
	SplineComp->Modify();

	EInterpCurveMode Mode = ConvertSplinePointTypeToInterpCurveMode((ESplinePointType::Type)SplinePointTypes.Find(NewValue));

	for (int32 Index : SelectedKeys)
	{
		SplineComp->GetSplinePointsRotation().Points[Index].InterpMode = Mode;
	}

	SplineComp->UpdateSpline();
	SplineComp->bSplineHasBeenEdited = true;
	FComponentVisualizer::NotifyPropertyModified(SplineComp, SplineCurvesProperty);
	UpdateValues();
}

TSharedRef<SWidget> FSCSplinePointDetails::OnGenerateComboWidget(TSharedPtr<FString> InComboString)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InComboString))
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

////////////////////////////////////

TSharedRef<IDetailCustomization> FSCSplineComponentDetails::MakeInstance()
{
	return MakeShareable(new FSCSplineComponentDetails);
}

void FSCSplineComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Hide the SplineCurves property
	TSharedPtr<IPropertyHandle> SplineCurvesProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(USCCameraSplineComponent, SplineCurves));
	SplineCurvesProperty->MarkHiddenByCustomization();

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Selected Points");
	TSharedRef<FSCSplinePointDetails> SplinePointDetails = MakeShareable(new FSCSplinePointDetails);
	Category.AddCustomBuilder(SplinePointDetails);
}

#undef LOCTEXT_NAMESPACE
