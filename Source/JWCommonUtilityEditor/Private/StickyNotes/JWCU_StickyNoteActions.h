// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "K2Node.h"
#include "JWCU_StickyNoteActions.generated.h"

/** K2 액션 DB에 코멘트 계열 포스트잇 생성기를 등록하며 그래프에는 배치되지 않는 어댑터. */
UCLASS(Transient)
class UJWCU_StickyNoteActions : public UK2Node
{
	GENERATED_BODY()

public:
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual bool IsNodePure() const override { return true; }
	virtual bool IsNodeSafeToIgnore() const override { return true; }
};
