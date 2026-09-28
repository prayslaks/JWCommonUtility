// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "StickyNotes/JWCU_StickyNoteSpawner.h"

#include "BlueprintNodeTemplateCache.h"
#include "StickyNotes/JWCU_StickyNote.h"

UEdGraphNode* UJWCU_StickyNoteSpawner::Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location) const
{
	// 액션 DB는 등록 클래스와 템플릿 타입이 일치해야 한다. 실제 그래프에만 메모를 생성한다.
	if (FBlueprintNodeTemplateCache::IsTemplateOuter(ParentGraph))
		return Super::Invoke(ParentGraph, Bindings, Location);
	if (!GetDefault<UJWCU_StickyNote>()->IsCompatibleWithGraph(ParentGraph))
		return nullptr;
	return SpawnNode<UJWCU_StickyNote>(ParentGraph, Bindings, Location, FCustomizeNodeDelegate());
}
