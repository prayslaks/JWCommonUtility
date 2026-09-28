// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ISlateStyle;

/** 플러그인에 포함된 포스트잇 SVG 브러시의 등록과 수명을 관리한다. */
class FJWCU_StickyNoteStyle
{
public:
	/** 플러그인 리소스 경로에서 아이콘 스타일을 등록하는 함수. */
	static void Initialize();
	/** 에디터 모듈 종료 시 스타일 등록을 해제하는 함수. */
	static void Shutdown();
	/** 등록된 포스트잇 스타일을 반환하는 함수. */
	static const ISlateStyle& Get();

private:
	static TSharedPtr<FSlateStyleSet> Style;
};
