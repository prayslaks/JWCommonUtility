// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "StickyNotes/SJWCU_StickyNote.h"

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "ScopedTransaction.h"
#include "StickyNotes/JWCU_StickyNote.h"
#include "StickyNotes/JWCU_StickyNoteStyle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "JWCUStickyNote"

void SJWCU_StickyNote::Construct(const FArguments& Args, UJWCU_StickyNote* Note)
{
	GraphNode = Note;
	MouseZone = CRWZ_NotInWindow;
	bUserIsDragging = false;
	UserSize = FVector2f(Note->NodeWidth, Note->NodeHeight);
	// 기본 입력창의 어두운 브러시는 BackgroundColor와 곱해진다. 흰 브러시로 메모색을 그대로 표시한다.
	BodyEditorStyle = FAppStyle::GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
	const FSlateBrush& BackgroundBrush = *FAppStyle::GetBrush("WhiteBrush");
	BodyEditorStyle.SetBackgroundImageNormal(BackgroundBrush)
		.SetBackgroundImageHovered(BackgroundBrush)
		.SetBackgroundImageFocused(BackgroundBrush)
		.SetBackgroundImageReadOnly(BackgroundBrush);
	UpdateGraphNode();
	CacheDesiredSize(1.f);
}

UJWCU_StickyNote* SJWCU_StickyNote::GetNote() const
{
	return CastChecked<UJWCU_StickyNote>(GraphNode);
}

FSlateFontInfo SJWCU_StickyNote::GetBodyFont() const
{
	FSlateFontInfo Font = FAppStyle::GetFontStyle("NormalFont");
	Font.Size = GetNote()->TextSize;
	return Font;
}

void SJWCU_StickyNote::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	LeftNodeBox.Reset();
	RightNodeBox.Reset();
	GetOrAddSlot(ENodeZone::Center)
	[
		SNew(SBox)
		.WidthOverride_Lambda([this]() { return ComputeDesiredSize(1.f).X; })
		.HeightOverride_Lambda([this]() { return ComputeDesiredSize(1.f).Y; })
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([this]() { return IsSelectedExclusively() ? FLinearColor(1.f, 0.45f, 0.f) : FLinearColor(0.08f, 0.08f, 0.08f); })
			.Padding(2.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this]() { return GetNote()->BackgroundColor; })
				.Padding(8.f)
				.Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(2.f, 0.f, 6.f, 0.f)
						[
							SAssignNew(InlineEditableText, SInlineEditableTextBlock)
							.Text_Lambda([this]() { return FText::FromString(GetNote()->NodeComment); })
							.Font(FAppStyle::GetFontStyle("BoldFont"))
							.ColorAndOpacity_Lambda([this]() { return GetNote()->TextColor; })
							.IsReadOnly_Lambda([this]() { return !IsNodeEditable(); })
							.IsSelected_Lambda([this]() { return IsSelectedExclusively(); })
							.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
							{
								if (IsNodeEditable() && GetNote()->NodeComment != Text.ToString())
								{
									const FScopedTransaction Transaction(LOCTEXT("TitleTransaction", "Edit Sticky Note Title"));
									GetNote()->Modify();
									GetNote()->OnRenameNode(Text.ToString());
								}
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
						[
							SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(3.f)
								.HAlign(HAlign_Center).VAlign(VAlign_Center)
								.ToolTipText_Lambda([this]() { return GetNote()->bLocked ? LOCTEXT("UnlockButtonTip", "Unlock position and size") : LOCTEXT("LockButtonTip", "Lock position and size. Text remains editable."); })
								.IsEnabled(this, &SGraphNode::IsNodeEditable)
								.OnClicked_Lambda([this]() { GetNote()->ToggleLocked(); return FReply::Handled(); })
								[
									SNew(SImage)
									.Image_Lambda([this]() { return FJWCU_StickyNoteStyle::Get().GetBrush(GetNote()->bLocked ? "LockClosed" : "LockOpen"); })
									.DesiredSizeOverride(FVector2D(16.f, 16.f))
									.ColorAndOpacity_Lambda([this]() { return GetNote()->TextColor; })
								]
							]
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(3.f)
								.HAlign(HAlign_Center).VAlign(VAlign_Center)
								.ToolTipText_Lambda([this]() { return GetNote()->bCollapsed ? LOCTEXT("UnfoldButtonTip", "Unfold the note") : LOCTEXT("FoldButtonTip", "Fold the note"); })
								.IsEnabled(this, &SGraphNode::IsNodeEditable)
								.OnClicked_Lambda([this]() { GetNote()->ToggleCollapsed(); return FReply::Handled(); })
								[
									SNew(SImage)
									.Image_Lambda([this]() { return FJWCU_StickyNoteStyle::Get().GetBrush(GetNote()->bCollapsed ? "Expand" : "Collapse"); })
									.DesiredSizeOverride(FVector2D(16.f, 16.f))
									.ColorAndOpacity_Lambda([this]() { return GetNote()->TextColor; })
								]
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 0.f)
					[
						SNew(SOverlay)
						.Visibility_Lambda([this]() { return GetNote()->bCollapsed ? EVisibility::Collapsed : EVisibility::Visible; })
						+ SOverlay::Slot()
						[
							SNew(SScrollBox)
							.Visibility_Lambda([this]() { return bEditingBody ? EVisibility::Collapsed : EVisibility::Visible; })
							+ SScrollBox::Slot()
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return GetNote()->Body.IsEmpty() ? LOCTEXT("Placeholder", "Double-click to write a note...") : FText::FromString(GetNote()->Body); })
								.Font(this, &SJWCU_StickyNote::GetBodyFont)
								.ColorAndOpacity_Lambda([this]() { return GetNote()->TextColor; })
								.AutoWrapText(true)
							]
						]
						+ SOverlay::Slot()
						[
							SAssignNew(BodyEditor, SMultiLineEditableTextBox)
							.Style(&BodyEditorStyle)
							.Text_Lambda([this]() { return FText::FromString(GetNote()->Body); })
							.Font(this, &SJWCU_StickyNote::GetBodyFont)
							.ForegroundColor_Lambda([this]() { return GetNote()->TextColor; })
							.ReadOnlyForegroundColor_Lambda([this]() { return GetNote()->TextColor; })
							.FocusedForegroundColor_Lambda([this]() { return GetNote()->TextColor; })
							.BackgroundColor_Lambda([this]() { return GetNote()->BackgroundColor; })
							.Visibility_Lambda([this]() { return bEditingBody ? EVisibility::Visible : EVisibility::Collapsed; })
							.IsReadOnly_Lambda([this]() { return !IsNodeEditable(); })
							.AutoWrapText(true)
							.Padding(2.f)
							.OnTextChanged(this, &SJWCU_StickyNote::OnBodyChanged)
							.OnTextCommitted(this, &SJWCU_StickyNote::OnBodyCommitted)
							.OnKeyDownHandler(this, &SJWCU_StickyNote::OnBodyKeyDown)
						]
					]
				]
			]
		]
	];
}

void SJWCU_StickyNote::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	if (!bUserIsDragging)
		UserSize = FVector2f(GetNote()->NodeWidth, GetNote()->NodeHeight);
	SGraphNodeResizable::Tick(Geometry, CurrentTime, DeltaTime);
}

FVector2D SJWCU_StickyNote::ComputeDesiredSize(float LayoutScale) const
{
	const FVector2f Size = bUserIsDragging ? FVector2f(UserSize) : FVector2f(GetNote()->NodeWidth, GetNote()->NodeHeight);
	return FVector2D(Size.X, GetNote()->bCollapsed ? 42.f : Size.Y);
}

void SJWCU_StickyNote::MoveTo(const FVector2f& Position, FNodeSet& NodeFilter, bool bMarkDirty)
{
	if (!GetNote()->bLocked && IsNodeEditable())
		SGraphNode::MoveTo(Position, NodeFilter, bMarkDirty);
}

SGraphNodeResizable::EResizableWindowZone SJWCU_StickyNote::FindMouseZone(const FVector2f& Position) const
{
	if (GetNote()->bLocked || GetNote()->bCollapsed || !IsNodeEditable())
		return CRWZ_TitleBar;
	return SGraphNodeResizable::FindMouseZone(Position);
}

FReply SJWCU_StickyNote::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	MouseZone = FindMouseZone(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
	if (!GetNote()->bLocked && !GetNote()->bCollapsed && IsNodeEditable())
	{
		FReply Reply = SGraphNodeResizable::OnMouseButtonDown(Geometry, Event);
		if (Reply.IsEventHandled())
			return Reply;
	}
	return SGraphNode::OnMouseButtonDown(Geometry, Event);
}

FReply SJWCU_StickyNote::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !GetNote()->bCollapsed && IsNodeEditable()
		&& Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).Y > GetTitleBarHeight())
	{
		bEditingBody = true;
		return FReply::Handled().SetUserFocus(BodyEditor.ToSharedRef(), EFocusCause::Mouse);
	}
	return FReply::Handled();
}

void SJWCU_StickyNote::OnBodyChanged(const FText& Text)
{
	if (!bEditingBody || !IsNodeEditable() || GetNote()->Body == Text.ToString())
		return;
	if (!BodyTransaction)
	{
		BodyTransaction = MakeShared<FScopedTransaction>(LOCTEXT("BodyTransaction", "Edit Sticky Note Body"));
		GetNote()->Modify();
	}
	// 포커스가 남은 채 저장하더라도 마지막 입력까지 에셋에 기록한다.
	GetNote()->Body = Text.ToString();
	GetNote()->MarkPackageDirty();
}

void SJWCU_StickyNote::OnBodyCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	OnBodyChanged(Text);
	BodyTransaction.Reset();
	bEditingBody = false;
}

FReply SJWCU_StickyNote::OnBodyKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape || (Event.IsControlDown() && Event.GetKey() == EKeys::Enter))
	{
		BodyTransaction.Reset();
		bEditingBody = false;
		return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
