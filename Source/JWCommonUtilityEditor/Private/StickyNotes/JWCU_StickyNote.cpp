// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "StickyNotes/JWCU_StickyNote.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "ScopedTransaction.h"
#include "StickyNotes/SJWCU_StickyNote.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "JWCUStickyNote"

UJWCU_StickyNote::UJWCU_StickyNote()
{
	NodeWidth = 360;
	NodeHeight = 240;
	NodeComment = TEXT("Note");
	BackgroundColor = FLinearColor(FColor(255, 231, 137));
	TextColor = FLinearColor(FColor(35, 31, 24));
	MoveMode = ECommentBoxMode::NoGroupMovement;
	bCommentBubbleVisible = false;
	bCommentBubblePinned = false;
	bCommentBubbleVisible_InDetailsPanel = false;
}

void UJWCU_StickyNote::PostPlacedNewNode()
{
	// 기본 코멘트의 사용자 설정 적용은 그룹 이동과 말풍선을 다시 켜므로 건너뛴다.
	UEdGraphNode::PostPlacedNewNode();
}

void UJWCU_StickyNote::PostEditUndo()
{
	Super::PostEditUndo();
	if (UEdGraph* Graph = GetGraph())
		Graph->NotifyGraphChanged();
}

void UJWCU_StickyNote::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	TextSize = FMath::Clamp(TextSize, 10, 48);
	Super::PostEditChangeProperty(Event);
	if (UEdGraph* Graph = GetGraph())
		Graph->NotifyGraphChanged();
}

void UJWCU_StickyNote::ResizeNode(const FVector2f& NewSize)
{
	if (!bLocked && !bCollapsed)
		Super::ResizeNode(FVector2f(FMath::Clamp(NewSize.X, 220.f, 1600.f), FMath::Clamp(NewSize.Y, 120.f, 1600.f)));
}

bool UJWCU_StickyNote::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return TargetGraph && TargetGraph->GetSchema() && TargetGraph->GetSchema()->IsA<UEdGraphSchema_K2>();
}

FText UJWCU_StickyNote::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(NodeComment);
}

FText UJWCU_StickyNote::GetTooltipText() const
{
	return FText::FromString(NodeComment + TEXT("\n") + Body);
}

TSharedPtr<SGraphNode> UJWCU_StickyNote::CreateVisualWidget()
{
	return SNew(SJWCU_StickyNote, this);
}

void UJWCU_StickyNote::ToggleCollapsed()
{
	const FScopedTransaction Transaction(LOCTEXT("CollapseTransaction", "Fold Sticky Note"));
	Modify();
	bCollapsed = !bCollapsed;
}

void UJWCU_StickyNote::ToggleLocked()
{
	const FScopedTransaction Transaction(LOCTEXT("LockTransaction", "Lock Sticky Note Position and Size"));
	Modify();
	bLocked = !bLocked;
}

void UJWCU_StickyNote::ApplyPreset(EJWCU_StickyNotePreset Preset)
{
	const FScopedTransaction Transaction(LOCTEXT("PresetTransaction", "Style Sticky Note"));
	Modify();
	const TCHAR* Title = TEXT("Note");
	FColor Color(255, 231, 137);
	switch (Preset)
	{
	case EJWCU_StickyNotePreset::Todo: Title = TEXT("TODO"); Color = FColor(161, 218, 255); break;
	case EJWCU_StickyNotePreset::Warning: Title = TEXT("Warning"); Color = FColor(255, 187, 117); break;
	case EJWCU_StickyNotePreset::Bug: Title = TEXT("Bug"); Color = FColor(255, 163, 175); break;
	default: break;
	}
	if (NodeComment == TEXT("Note") || NodeComment == TEXT("TODO") || NodeComment == TEXT("Warning") || NodeComment == TEXT("Bug"))
		NodeComment = Title;
	BackgroundColor = FLinearColor(Color);
	TextColor = FLinearColor(FColor(35, 31, 24));
}

UEdGraphNode_Comment* UJWCU_StickyNote::ConvertToComment()
{
	UEdGraph* Graph = GetGraph();
	if (!Graph || !Graph->Nodes.Contains(this))
		return nullptr;
	const FScopedTransaction Transaction(LOCTEXT("ConvertTransaction", "Convert Sticky Note to Comment"));
	Graph->Modify();
	Modify();
	UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
	Comment->CreateNewGuid();
	Comment->NodePosX = NodePosX;
	Comment->NodePosY = NodePosY;
	Comment->NodeWidth = NodeWidth;
	Comment->NodeHeight = NodeHeight;
	Comment->NodeComment = Body.IsEmpty() ? NodeComment : NodeComment + TEXT("\n\n") + Body;
	Comment->CommentColor = BackgroundColor;
	Comment->FontSize = TextSize;
	Comment->MoveMode = ECommentBoxMode::NoGroupMovement;
	Comment->bCommentBubbleVisible = false;
	Comment->bCommentBubblePinned = false;
	Comment->bCommentBubbleVisible_InDetailsPanel = false;
	Graph->AddNode(Comment, false, false);
	DestroyNode();
	Graph->NotifyGraphChanged();
	return Comment;
}

void UJWCU_StickyNote::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	if (!Context || Context->bIsDebugging)
		return;
	UJWCU_StickyNote* Note = const_cast<UJWCU_StickyNote*>(this);
	FToolMenuSection& Section = Menu->AddSection("JWCUStickyNote", LOCTEXT("Menu", "Sticky Note"));
	Section.AddMenuEntry("Fold", LOCTEXT("Fold", "Fold / Unfold"), LOCTEXT("FoldTip", "Keep the expanded size and show only the title."), FSlateIcon(),
		FUIAction(FExecuteAction::CreateUObject(Note, &UJWCU_StickyNote::ToggleCollapsed)));
	Section.AddMenuEntry("Lock", LOCTEXT("Lock", "Lock / Unlock Position and Size"), LOCTEXT("LockTip", "Text remains editable while the card is locked."), FSlateIcon(),
		FUIAction(FExecuteAction::CreateUObject(Note, &UJWCU_StickyNote::ToggleLocked)));
	const FText Labels[] = { LOCTEXT("NotePreset", "Style: Note"), LOCTEXT("TodoPreset", "Style: TODO"), LOCTEXT("WarningPreset", "Style: Warning"), LOCTEXT("BugPreset", "Style: Bug") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
	{
		Section.AddMenuEntry(FName(*FString::Printf(TEXT("Preset%d"), Index)), Labels[Index], LOCTEXT("PresetTip", "Apply preset colors; preserve a custom title."), FSlateIcon(),
			FUIAction(FExecuteAction::CreateUObject(Note, &UJWCU_StickyNote::ApplyPreset, static_cast<EJWCU_StickyNotePreset>(Index))));
	}
	Section.AddMenuEntry("Convert", LOCTEXT("Convert", "Convert to Standard Comment"), LOCTEXT("ConvertTip", "Preserve title and body in an Unreal comment. Folding and locking are removed. Undo is supported."), FSlateIcon(),
		FUIAction(FExecuteAction::CreateWeakLambda(Note, [Note]() { Note->ConvertToComment(); })));
}

#undef LOCTEXT_NAMESPACE
