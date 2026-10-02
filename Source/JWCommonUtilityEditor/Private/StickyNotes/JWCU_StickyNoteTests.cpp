// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "StickyNotes/JWCU_StickyNote.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "StickyNotes/JWCU_StickyNoteActions.h"
#include "StickyNotes/SJWCU_StickyNote.h"
#include "SGraphPanel.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

namespace JWCU_StickyNoteTests
{
	inline UBlueprint* CreateBlueprint(const FString& PackageName)
	{
		UPackage* Package = CreatePackage(*PackageName);
		return FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, TEXT("NoteTest"), BPTYPE_Normal);
	}

	inline UJWCU_StickyNote* AddNote(UBlueprint* Blueprint)
	{
		UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
		UJWCU_StickyNote* Note = NewObject<UJWCU_StickyNote>(Graph, NAME_None, RF_Transactional);
		Note->CreateNewGuid();
		Note->PostPlacedNewNode();
		Graph->AddNode(Note, false, false);
		return Note;
	}

	inline TSharedPtr<SWidget> FindWidget(const TSharedRef<SWidget>& Root, const FString& Type)
	{
		if (Root->GetTypeAsString() == Type)
			return Root;
		FChildren* Children = Root->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (TSharedPtr<SWidget> Found = FindWidget(Children->GetChildAt(Index), Type))
				return Found;
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_StickyNoteGraphTest,
	"JWCommonUtility.Editor.StickyNotes.Graph", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_StickyNoteGraphTest::RunTest(const FString& Parameters)
{
	UBlueprint* Blueprint = JWCU_StickyNoteTests::CreateBlueprint(TEXT("/Temp/JWCUNotes_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	UJWCU_StickyNote* Note = JWCU_StickyNoteTests::AddNote(Blueprint);
	UEdGraph* Graph = Note->GetGraph();
	TestEqual(TEXT("No group movement after placement"), Note->MoveMode.GetValue(), ECommentBoxMode::NoGroupMovement);
	TestFalse(TEXT("No bubble after placement"), !!Note->bCommentBubbleVisible);
	TestEqual(TEXT("No execution or data pins"), Note->Pins.Num(), 0);
	TestTrue(TEXT("K2 graph accepted"), Note->IsCompatibleWithGraph(Graph));
	UEdGraph* OtherGraph = NewObject<UEdGraph>();
	OtherGraph->Schema = UEdGraphSchema::StaticClass();
	TestFalse(TEXT("Unrelated graph rejected"), Note->CanPasteHere(OtherGraph));

	Note->Body = TEXT("한글 메모\nSecond line\n[] TODO");
	Note->NodeComment = TEXT("Custom title");
	Note->ResizeNode(FVector2f(480, 320));
	Note->ToggleLocked();
	Note->ResizeNode(FVector2f(900, 800));
	TestEqual(TEXT("Locked width preserved"), Note->NodeWidth, 480);
	GEditor->UndoTransaction();
	TestFalse(TEXT("Undo lock"), Note->bLocked);
	GEditor->RedoTransaction();
	TestTrue(TEXT("Redo lock"), Note->bLocked);
	Note->ToggleCollapsed();
	TestEqual(TEXT("Fold preserves expanded height"), Note->NodeHeight, 320);
	Note->ApplyPreset(EJWCU_StickyNotePreset::Bug);
	TestEqual(TEXT("Preset preserves custom title"), Note->NodeComment, FString(TEXT("Custom title")));

	FString Clipboard;
	FEdGraphUtilities::ExportNodesToText(TSet<UObject*>{ Note }, Clipboard);
	TestTrue(TEXT("Clipboard accepted"), FEdGraphUtilities::CanImportNodesFromText(Graph, Clipboard));
	TSet<UEdGraphNode*> Imported;
	FEdGraphUtilities::ImportNodesFromText(Graph, Clipboard, Imported);
	TestEqual(TEXT("One pasted node"), Imported.Num(), 1);
	for (UEdGraphNode* Node : Imported)
	{
		const UJWCU_StickyNote* Copy = Cast<UJWCU_StickyNote>(Node);
		if (TestNotNull(TEXT("Pasted type"), Copy))
		{
			TestEqual(TEXT("Pasted multiline body"), Copy->Body, Note->Body);
			TestTrue(TEXT("Pasted color within clipboard decimal precision"), Copy->BackgroundColor.Equals(Note->BackgroundColor, 0.00001f));
			TestTrue(TEXT("Pasted fold flag"), Copy->bCollapsed);
			TestTrue(TEXT("Pasted lock flag"), Copy->bLocked);
		}
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	TestTrue(TEXT("Blueprint compiles with sticky notes"), Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);
	TestTrue(TEXT("Compile retains authored note"), Graph->Nodes.Contains(Note));

	FBlueprintActionDatabase& Database = FBlueprintActionDatabase::Get();
	const FBlueprintActionDatabase::FActionList* Actions = Database.GetAllActions().Find(FObjectKey(UJWCU_StickyNoteActions::StaticClass()));
	if (TestNotNull(TEXT("Search action registered automatically"), Actions) && TestEqual(TEXT("Single creation action"), Actions->Num(), 1))
	{
		FBlueprintActionFilter Filter;
		Filter.Context.Blueprints.Add(Blueprint);
		Filter.Context.Graphs.Add(Graph);
		FBlueprintActionInfo ActionInfo(UJWCU_StickyNoteActions::StaticClass(), (*Actions)[0]);
		TestFalse(TEXT("Action visible in context-sensitive graph search"), Filter.IsFiltered(ActionInfo));
		UEdGraphNode* Spawned = (*Actions)[0]->Invoke(Graph, IBlueprintNodeBinder::FBindingSet(), FVector2D(768, 512));
		TestTrue(TEXT("Search spawns a note, not the K2 adapter"), Spawned && Spawned->IsA<UJWCU_StickyNote>());
		if (Spawned)
		{
			TestEqual(TEXT("Spawn uses cursor X"), Spawned->NodePosX, 768);
			TestEqual(TEXT("Spawn uses cursor Y"), Spawned->NodePosY, 512);
		}
	}

	const FString Body = Note->Body;
	UEdGraphNode_Comment* Comment = Note->ConvertToComment();
	if (TestNotNull(TEXT("Conversion result"), Comment))
	{
		TestTrue(TEXT("Conversion removes plugin type"), Comment->GetClass() == UEdGraphNode_Comment::StaticClass());
		TestTrue(TEXT("Conversion preserves entire body"), Comment->NodeComment.Contains(Body));
		TestFalse(TEXT("Original removed"), Graph->Nodes.Contains(Note));
		GEditor->UndoTransaction();
		TestTrue(TEXT("Undo restores original note"), Graph->Nodes.Contains(Note));
		TestFalse(TEXT("Undo removes converted comment"), Graph->Nodes.Contains(Comment));
		GEditor->RedoTransaction();
		TestFalse(TEXT("Redo removes original note"), Graph->Nodes.Contains(Note));
		TestTrue(TEXT("Redo restores converted comment"), Graph->Nodes.Contains(Comment));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_StickyNoteWidgetTest,
	"JWCommonUtility.Editor.StickyNotes.Widget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_StickyNoteWidgetTest::RunTest(const FString& Parameters)
{
	UBlueprint* Blueprint = JWCU_StickyNoteTests::CreateBlueprint(TEXT("/Temp/JWCUWidget_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	UJWCU_StickyNote* Note = JWCU_StickyNoteTests::AddNote(Blueprint);
	UJWCU_StickyNote* Neighbor = JWCU_StickyNoteTests::AddNote(Blueprint);
	Neighbor->NodePosX = 100;
	TSharedRef<SGraphPanel> Panel = SNew(SGraphPanel).GraphObj(Note->GetGraph());
	TSharedRef<SJWCU_StickyNote> Widget = SNew(SJWCU_StickyNote, Note);
	Widget->SetOwner(Panel);
	Widget->SetIsEditable(true);
	Widget->SlatePrepass();
	TestTrue(TEXT("Expanded widget size"), FVector2f(Widget->GetDesiredSize()).Equals(FVector2f(360, 240)));
	Note->ToggleCollapsed();
	Widget->SlatePrepass();
	TestEqual(TEXT("Collapsed widget height"), Widget->GetDesiredSize().Y, 42.f);
	Note->ToggleCollapsed();
	Widget->SlatePrepass();
	TestEqual(TEXT("Expanded widget restores height"), Widget->GetDesiredSize().Y, 240.f);
	SGraphNode::FNodeSet Filter;
	Widget->MoveTo(FVector2f(200, 300), Filter);
	TestEqual(TEXT("Widget moves own node"), Note->NodePosX, 200);
	TestEqual(TEXT("Widget does not move neighbor"), Neighbor->NodePosX, 100);
	Note->ToggleLocked();
	Filter.Empty();
	Widget->MoveTo(FVector2f(600, 700), Filter);
	TestEqual(TEXT("Widget respects position lock"), Note->NodePosX, 200);
	Note->ToggleLocked();
	Widget->SetIsEditable(false);
	Filter.Empty();
	Widget->MoveTo(FVector2f(600, 700), Filter);
	TestEqual(TEXT("Read-only graph blocks movement"), Note->NodePosX, 200);
	Widget->SetIsEditable(true);

	const FGeometry Geometry = FGeometry::MakeRoot(FVector2f(360, 240), FSlateLayoutTransform());
	const FPointerEvent DoubleClick(0, FVector2D(70, 90), FVector2D(70, 90), TSet<FKey>{ EKeys::LeftMouseButton }, EKeys::LeftMouseButton, 0.f, FModifierKeysState());
	TestTrue(TEXT("Body double-click handled"), Widget->OnMouseButtonDoubleClick(Geometry, DoubleClick).IsEventHandled());
	TSharedPtr<SWidget> FoundEditor = JWCU_StickyNoteTests::FindWidget(Widget, TEXT("SMultiLineEditableTextBox"));
	if (TestTrue(TEXT("Inline body editor exists"), FoundEditor.IsValid()))
	{
		TSharedPtr<SMultiLineEditableTextBox> Editor = StaticCastSharedPtr<SMultiLineEditableTextBox>(FoundEditor);
		Editor->SetText(FText::FromString(TEXT("한글 입력\nSecond line")));
		TestEqual(TEXT("Inline edit updates persistent body before focus loss"), Note->Body, FString(TEXT("한글 입력\nSecond line")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_StickyNoteCommentMovementTest,
	"JWCommonUtility.Editor.StickyNotes.CommentMovement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_StickyNoteCommentMovementTest::RunTest(const FString& Parameters)
{
	UBlueprint* Blueprint = JWCU_StickyNoteTests::CreateBlueprint(TEXT("/Temp/JWCUCommentMovement_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	UJWCU_StickyNote* Note = JWCU_StickyNoteTests::AddNote(Blueprint);
	UJWCU_StickyNote* Neighbor = JWCU_StickyNoteTests::AddNote(Blueprint);
	Note->NodePosX = 80;
	Note->NodePosY = 100;
	Neighbor->NodePosX = 140;
	Neighbor->NodePosY = 160;
	Neighbor->ResizeNode(FVector2f(220, 120));
	UEdGraph* Graph = Note->GetGraph();
	UEdGraphNode_Comment* Outer = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
	Outer->CreateNewGuid();
	Outer->NodeWidth = 1000;
	Outer->NodeHeight = 800;
	Outer->MoveMode = ECommentBoxMode::GroupMovement;
	Graph->AddNode(Outer, false, false);
	UEdGraphNode_Comment* Inner = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
	Inner->CreateNewGuid();
	Inner->NodePosX = 40;
	Inner->NodePosY = 60;
	Inner->NodeWidth = 700;
	Inner->NodeHeight = 600;
	Inner->MoveMode = ECommentBoxMode::GroupMovement;
	Graph->AddNode(Inner, false, false);

	TSharedRef<SGraphPanel> Panel = SNew(SGraphPanel).GraphObj(Graph);
	// 실제 생성 경로의 위젯을 패널에 등록하고 엔진의 이동 종료 처리를 실행한다.
	TSharedRef<SGraphNode> NoteWidget = Note->CreateVisualWidget().ToSharedRef();
	TSharedRef<SGraphNode> NeighborWidget = Neighbor->CreateVisualWidget().ToSharedRef();
	TSharedRef<SGraphNodeComment> OuterWidget = SNew(SGraphNodeComment, Outer);
	TSharedRef<SGraphNodeComment> InnerWidget = SNew(SGraphNodeComment, Inner);
	Panel->AddGraphNode(NoteWidget);
	Panel->AddGraphNode(NeighborWidget);
	Panel->AddGraphNode(OuterWidget);
	Panel->AddGraphNode(InnerWidget);
	Panel->SlatePrepass();
	OuterWidget->GetShadowBrush(true);
	InnerWidget->GetShadowBrush(true);
	TestTrue(TEXT("Outer comment contains note and nested comment"), Outer->GetNodesUnderComment().Contains(Note) && Outer->GetNodesUnderComment().Contains(Inner));
	TestTrue(TEXT("Inner comment contains note"), Inner->GetNodesUnderComment().Contains(Note));

	SGraphNode::FNodeSet Filter;
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("Move comment containing sticky notes")));
		StaticCastSharedRef<SGraphNode>(OuterWidget)->MoveTo(FVector2f(160, 80), Filter);
		// 수정 전에는 여기서 StickyNote를 SGraphNodeComment로 잘못 처리하여 크래시가 발생한다.
		OuterWidget->EndUserInteraction();
	}
	TestEqual(TEXT("Grouped note X"), Note->NodePosX, 240);
	TestEqual(TEXT("Grouped note Y"), Note->NodePosY, 180);
	TestEqual(TEXT("Nested comment moves once"), Inner->NodePosX, 200);
	TestEqual(TEXT("Note does not collect overlapping neighbor"), Note->GetNodesUnderComment().Num(), 0);
	GEditor->UndoTransaction();
	TestEqual(TEXT("Undo grouped note movement"), Note->NodePosX, 80);
	GEditor->RedoTransaction();
	TestEqual(TEXT("Redo grouped note movement"), Note->NodePosX, 240);

	Filter.Empty();
	NoteWidget->MoveTo(FVector2f(256, 192), Filter);
	NoteWidget->EndUserInteraction();
	TestEqual(TEXT("Independent note movement preserves neighbor"), Neighbor->NodePosX, 300);
	TestTrue(TEXT("Body remains selectable"), NoteWidget->CanBeSelected(FVector2f(100, 100)));
	TestTrue(TEXT("Marquee uses entire note"), NoteWidget->GetDesiredSizeForMarquee2f().Equals(FVector2f(360, 240)));

	Note->ToggleCollapsed();
	NoteWidget->SlatePrepass();
	Panel->SelectionManager.SelectSingleNode(Note);
	Filter.Empty();
	StaticCastSharedRef<SGraphNode>(OuterWidget)->MoveTo(FVector2f(192, 96), Filter);
	TestEqual(TEXT("Selected folded note waits for selection movement"), Note->NodePosX, 256);
	NoteWidget->MoveTo(FVector2f(288, 208), Filter);
	OuterWidget->EndUserInteraction();
	InnerWidget->EndUserInteraction();
	TestEqual(TEXT("Selected folded note moves once"), Note->NodePosX, 288);
	TestEqual(TEXT("Folded note retains expanded height"), Note->NodeHeight, 240);
	TestEqual(TEXT("Folded note does not collect neighbors"), Note->GetNodesUnderComment().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_StickyNotePersistenceTest,
	"JWCommonUtility.Editor.StickyNotes.SaveReload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_StickyNotePersistenceTest::RunTest(const FString& Parameters)
{
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/JWCUStickyNotes/"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	FPackageName::RegisterMountPoint(TEXT("/JWCUStickyNoteTests/"), Directory);
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString PackageName = TEXT("/JWCUStickyNoteTests/Note_") + Id;
	UBlueprint* Blueprint = JWCU_StickyNoteTests::CreateBlueprint(PackageName);
	UJWCU_StickyNote* Note = JWCU_StickyNoteTests::AddNote(Blueprint);
	Note->NodeComment = TEXT("저장 테스트");
	Note->Body = TEXT("첫 번째 줄\n두 번째 줄\nSave / reload");
	Note->NodePosX = 288;
	Note->NodePosY = -144;
	Note->NodeWidth = 512;
	Note->NodeHeight = 352;
	Note->bCollapsed = true;
	Note->bLocked = true;
	Note->TextSize = 22;
	Note->BackgroundColor = FLinearColor::Green;
	Note->TextColor = FLinearColor::Black;
	const FGuid NoteGuid = Note->NodeGuid;
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	const FString Filename = Directory / (TEXT("Note_") + Id + TEXT(".uasset"));
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	UPackage* Package = Blueprint->GetOutermost();
	if (TestTrue(TEXT("Save BP containing note"), UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs)))
	{
		// 같은 메모리 객체를 재사용하지 않고 디스크에서 새 패키지 인스턴스를 읽는다.
		ResetLoaders(Package);
		Package->Rename(*(TEXT("/Temp/JWCUOld_") + Id), nullptr, REN_DontCreateRedirectors | REN_NonTransactional);
		UPackage* ReloadedPackage = LoadPackage(nullptr, *PackageName, LOAD_None);
		UBlueprint* Reloaded = ReloadedPackage ? FindObject<UBlueprint>(ReloadedPackage, TEXT("NoteTest")) : nullptr;
		if (TestNotNull(TEXT("Reloaded blueprint"), Reloaded))
		{
			UJWCU_StickyNote* LoadedNote = nullptr;
			for (UEdGraphNode* Node : FBlueprintEditorUtils::FindEventGraph(Reloaded)->Nodes)
			{
				if (Node->NodeGuid == NoteGuid)
					LoadedNote = Cast<UJWCU_StickyNote>(Node);
			}
			if (TestNotNull(TEXT("Saved note restored"), LoadedNote))
			{
				TestTrue(TEXT("Fresh deserialized instance"), LoadedNote != Note);
				TestEqual(TEXT("Saved body"), LoadedNote->Body, Note->Body);
				TestEqual(TEXT("Saved title"), LoadedNote->NodeComment, Note->NodeComment);
				TestEqual(TEXT("Saved width"), LoadedNote->NodeWidth, 512);
				TestEqual(TEXT("Saved expanded height"), LoadedNote->NodeHeight, 352);
				TestEqual(TEXT("Saved X"), LoadedNote->NodePosX, 288);
				TestEqual(TEXT("Saved Y"), LoadedNote->NodePosY, -144);
				TestEqual(TEXT("Saved font size"), LoadedNote->TextSize, 22);
				TestTrue(TEXT("Saved flags and colors"), LoadedNote->bCollapsed && LoadedNote->bLocked && LoadedNote->BackgroundColor == FLinearColor::Green && LoadedNote->TextColor == FLinearColor::Black);
				FKismetEditorUtilities::CompileBlueprint(Reloaded);
				TestTrue(TEXT("Reloaded BP compiles"), Reloaded->Status == BS_UpToDate || Reloaded->Status == BS_UpToDateWithWarnings);
			}
		}
	}
	FPackageName::UnRegisterMountPoint(TEXT("/JWCUStickyNoteTests/"), Directory);
	return true;
}

#endif
