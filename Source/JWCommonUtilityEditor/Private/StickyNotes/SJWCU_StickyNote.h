// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "SGraphNodeResizable.h"
#include "Styling/SlateTypes.h"

class UJWCU_StickyNote;
class SMultiLineEditableTextBox;

/** 그래프 배율을 따르는 포스트잇의 본문 편집과 독립 이동 UI. */
class SJWCU_StickyNote : public SGraphNodeResizable
{
public:
	SLATE_BEGIN_ARGS(SJWCU_StickyNote) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, UJWCU_StickyNote* Note);
	virtual void UpdateGraphNode() override;
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FVector2D ComputeDesiredSize(float LayoutScale) const override;
	virtual void MoveTo(const FVector2f& Position, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;

protected:
	virtual EResizableWindowZone FindMouseZone(const FVector2f& Position) const override;
	virtual float GetTitleBarHeight() const override { return 42.f; }
	virtual FVector2f GetNodeMinimumSize2f() const override { return FVector2f(220.f, 120.f); }
	virtual FVector2f GetNodeMaximumSize2f() const override { return FVector2f(1600.f, 1600.f); }

private:
	UJWCU_StickyNote* GetNote() const;
	void OnBodyChanged(const FText& Text);
	void OnBodyCommitted(const FText& Text, ETextCommit::Type CommitType);
	FReply OnBodyKeyDown(const FGeometry& Geometry, const FKeyEvent& Event);
	FSlateFontInfo GetBodyFont() const;
	/** 입력창이 보관하는 스타일 포인터의 수명을 위젯과 일치시키는 필드. */
	FEditableTextBoxStyle BodyEditorStyle;
	TSharedPtr<SMultiLineEditableTextBox> BodyEditor;
	TSharedPtr<FScopedTransaction> BodyTransaction;
	bool bEditingBody = false;
};
