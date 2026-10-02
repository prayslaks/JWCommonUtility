// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "SGraphNodeComment.h"
#include "Styling/SlateTypes.h"

class UJWCU_StickyNote;
class SMultiLineEditableTextBox;

/** 그래프 배율을 따르는 포스트잇의 본문 편집과 독립 이동 UI. */
// 엔진은 UEdGraphNode_Comment의 위젯을 이동 종료 시 SGraphNodeComment로 캐스팅한다.
class SJWCU_StickyNote : public SGraphNodeComment
{
public:
	SLATE_BEGIN_ARGS(SJWCU_StickyNote) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, UJWCU_StickyNote* Note);
	virtual void UpdateGraphNode() override;
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FVector2D ComputeDesiredSize(float LayoutScale) const override;
	virtual bool CanBeSelected(const FVector2f& Position) const override { return SGraphNode::CanBeSelected(Position); }
	virtual FVector2f GetDesiredSizeForMarquee2f() const override { return SGraphNode::GetDesiredSizeForMarquee2f(); }
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override { return SGraphNode::GetShadowBrush(bSelected); }
	virtual void GetOverlayBrushes(bool bSelected, const FVector2f& WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override
	{
		SGraphNode::GetOverlayBrushes(bSelected, WidgetSize, Brushes);
	}
	virtual void MoveTo(const FVector2f& Position, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;

protected:
	/** 외부 코멘트의 강제 갱신에서도 메모 자체는 주변 노드를 수집하지 않는다. */
	virtual bool IsNodeUnderComment(UEdGraphNode_Comment* Comment, const TSharedRef<SGraphNode> NodeWidget) const override { return false; }
	virtual FSlateRect GetHitTestingBorder() const override { return SGraphNodeResizable::GetHitTestingBorder(); }
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
