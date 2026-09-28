// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "EdGraphNode_Comment.h"
#include "JWCU_StickyNote.generated.h"

/** 포스트잇의 기본 색상과 용도를 선택하는 프리셋. */
UENUM()
enum class EJWCU_StickyNotePreset : uint8
{
	Note,
	Todo,
	Warning,
	Bug
};

/** 실행 핀 없이 BP 그래프에 저장되는 독립적인 포스트잇. */
UCLASS(HideCategories=(Comment))
class UJWCU_StickyNote : public UEdGraphNode_Comment
{
	GENERATED_BODY()

public:
	UJWCU_StickyNote();

	/** 여러 줄 메모 본문 필드. */
	UPROPERTY(EditAnywhere, Category="Sticky Note", meta=(MultiLine=true))
	FString Body;

	/** 카드 배경색 필드. */
	UPROPERTY(EditAnywhere, Category="Sticky Note", meta=(HideAlphaChannel))
	FLinearColor BackgroundColor;

	/** 제목과 본문 글자색 필드. */
	UPROPERTY(EditAnywhere, Category="Sticky Note", meta=(HideAlphaChannel))
	FLinearColor TextColor;

	/** 본문 글자 크기 필드. */
	UPROPERTY(EditAnywhere, Category="Sticky Note", meta=(ClampMin=10, ClampMax=48))
	int32 TextSize = 16;

	/** 펼친 크기를 보존하면서 제목만 표시하는 필드. */
	UPROPERTY(EditAnywhere, Category="Sticky Note")
	bool bCollapsed = false;

	/** 내용 편집은 허용하면서 위치와 크기를 잠그는 필드. */
	UPROPERTY(EditAnywhere, Category="Sticky Note", meta=(DisplayName="Lock Position and Size"))
	bool bLocked = false;

	virtual void PostPlacedNewNode() override;
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void ResizeNode(const FVector2f& NewSize) override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

	/** 사용자 입력 한 건으로 접힘 상태를 변경하는 함수. */
	void ToggleCollapsed();
	/** 사용자 입력 한 건으로 위치와 크기 잠금을 변경하는 함수. */
	void ToggleLocked();
	/** 사용자 지정 제목은 보존하고 기본 용도 제목과 프리셋 색상을 변경하는 함수. */
	void ApplyPreset(EJWCU_StickyNotePreset Preset);
	/** 제목과 본문을 보존한 엔진 기본 코멘트로 교체하는 함수. */
	UEdGraphNode_Comment* ConvertToComment();
};
