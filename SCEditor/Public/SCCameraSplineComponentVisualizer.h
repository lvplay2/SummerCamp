// Copyright (C) 2015-2018 IllFonic, LLC. and Gun Media

/****************************************
* RIPPED FROM Source/Editor/ComponentVisualizers/Public/SplineComponentVisualizer.h
***************************************/

#pragma once

#include "ComponentVisualizer.h"
#include "Camera/SCCameraSplineComponent.h"

/** Base class for clickable spline editing proxies */
struct HSCSplineVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HSCSplineVisProxy(const UActorComponent* InComponent)
		: HComponentVisProxy(InComponent, HPP_Wireframe)
	{}
};

/** Proxy for a spline key */
struct HSCSplineKeyProxy : public HSCSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSCSplineKeyProxy(const UActorComponent* InComponent, int32 InKeyIndex) 
		: HSCSplineVisProxy(InComponent)
		, KeyIndex(InKeyIndex)
	{}

	int32 KeyIndex;
};

/** Proxy for a spline segment */
struct HSCSplineSegmentProxy : public HSCSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSCSplineSegmentProxy(const UActorComponent* InComponent, int32 InSegmentIndex)
		: HSCSplineVisProxy(InComponent)
		, SegmentIndex(InSegmentIndex)
	{}

	int32 SegmentIndex;
};

/** Proxy for a tangent handle */
struct HSCSplineTangentHandleProxy : public HSCSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSCSplineTangentHandleProxy(const UActorComponent* InComponent, int32 InKeyIndex, bool bInArriveTangent)
		: HSCSplineVisProxy(InComponent)
		, KeyIndex(InKeyIndex)
		, bArriveTangent(bInArriveTangent)
	{}

	int32 KeyIndex;
	bool bArriveTangent;
};

/** SCCameraSplineComponent visualizer/edit functionality */
class SCEDITOR_API FSCCameraSplineComponentVisualizer
	: public FComponentVisualizer
{
public:
	FSCCameraSplineComponentVisualizer();
	virtual ~FSCCameraSplineComponentVisualizer();

	//~ Begin FComponentVisualizer Interface
	virtual void OnRegister() override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual void EndEditing() override;
	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;
	virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override;
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual TSharedPtr<SWidget> GenerateContextMenu() const override;
	virtual bool IsVisualizingArchetype() const override;
	//~ End FComponentVisualizer Interface

	/** Get the spline component we are currently editing */
	USCCameraSplineComponent* GetEditedSplineComponent() const;

	TSet<int32> GetSelectedKeys() const { return SelectedKeys; }

protected:

	/** Update the key selection state of the visualizer */
	void ChangeSelectionState(int32 Index, bool bIsCtrlHeld);

	/** Duplicates the selected spline key(s) */
	void DuplicateKey();

	void OnDeleteKey();
	bool CanDeleteKey() const;

	void OnDuplicateKey();
	bool IsKeySelectionValid() const;

	void OnAddKey();
	bool CanAddKey() const;

	void OnResetToAutomaticTangent(EInterpCurveMode Mode);
	bool CanResetToAutomaticTangent(EInterpCurveMode Mode) const;

	void OnSetKeyType(EInterpCurveMode Mode);
	bool IsKeyTypeSet(EInterpCurveMode Mode) const;

	void OnSetVisualizeRollAndScale();
	bool IsVisualizingRollAndScale() const;

	void OnSetDiscontinuousSpline();
	bool IsDiscontinuousSpline() const;

	void OnResetToDefault();
	bool CanResetToDefault() const;

	/** Generate the submenu containing the available point types */
	void GenerateSplinePointTypeSubMenu(FMenuBuilder& MenuBuilder) const;

	/** Generate the submenu containing the available auto tangent types */
	void GenerateTangentTypeSubMenu(FMenuBuilder& MenuBuilder) const;

	/** Output log commands */
	TSharedPtr<FUICommandList> SplineComponentVisualizerActions;

	/** Actor that owns the currently edited spline */
	TWeakObjectPtr<AActor> SplineOwningActor;

	/** Name of property on the actor that references the spline we are editing */
	FPropertyNameAndIndex SplineCompPropName;

	/** Index of keys we have selected */
	TSet<int32> SelectedKeys;

	/** Index of the last key we selected */
	int32 LastKeyIndexSelected;

	/** Index of segment we have selected */
	int32 SelectedSegmentIndex;

	/** Index of tangent handle we have selected */
	int32 SelectedTangentHandle;

	struct ESelectedTangentHandle
	{
		enum Type
		{
			None,
			Leave,
			Arrive
		};
	};

	/** The type of the selected tangent handle */
	ESelectedTangentHandle::Type SelectedTangentHandleType;

	/** Position on spline we have selected */
	FVector SelectedSplinePosition;

	/** Cached rotation for this point */
	FQuat CachedRotation;

	/** Whether we currently allow duplication when dragging */
	bool bAllowDuplication;

private:
	UProperty* SplineCurvesProperty;
};
