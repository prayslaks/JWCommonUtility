// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "BlueprintNodeSpawner.h"
#include "JWCU_StickyNoteSpawner.generated.h"

/** 검색용 K2 템플릿과 실제 코멘트 계열 메모 생성을 분리하는 액션 생성기. */
UCLASS(Transient)
class UJWCU_StickyNoteSpawner : public UBlueprintNodeSpawner
{
	GENERATED_BODY()

public:
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location) const override;
};
