// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "StickyNotes/JWCU_StickyNoteActions.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "StickyNotes/JWCU_StickyNote.h"
#include "StickyNotes/JWCU_StickyNoteSpawner.h"
#include "Styling/AppStyle.h"

bool UJWCU_StickyNoteActions::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return GetDefault<UJWCU_StickyNote>()->IsCompatibleWithGraph(TargetGraph);
}

void UJWCU_StickyNoteActions::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	if (!ActionRegistrar.IsOpenForRegistration(GetClass()))
		return;
	// 실제 저장 타입은 UEdGraphNode_Comment 계열이다. K2 어댑터는 검색 등록만 담당한다.
	UJWCU_StickyNoteSpawner* Spawner = NewObject<UJWCU_StickyNoteSpawner>();
	Spawner->NodeClass = GetClass();
	Spawner->DefaultMenuSignature.MenuName = NSLOCTEXT("JWCUStickyNote", "Add", "Add Sticky Note");
	Spawner->DefaultMenuSignature.Category = NSLOCTEXT("JWCUStickyNote", "Category", "JWCommonUtility|Notes");
	Spawner->DefaultMenuSignature.Tooltip = NSLOCTEXT("JWCUStickyNote", "AddTip", "Add an independent, resizable note at the cursor. Does not wrap selected nodes.");
	Spawner->DefaultMenuSignature.Keywords = FText::FromString(TEXT("JWCU sticky note memo post-it 포스트잇 메모 메모장"));
	Spawner->DefaultMenuSignature.Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Comment");
	ActionRegistrar.AddBlueprintAction(GetClass(), Spawner);
}
