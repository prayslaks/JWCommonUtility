// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "Modules/ModuleManager.h"
#include "StickyNotes/JWCU_StickyNoteStyle.h"

/** 포스트잇 SVG 스타일을 에디터 모듈 수명에 맞춰 등록·해제한다. */
class FJWCommonUtilityEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FJWCU_StickyNoteStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
		FJWCU_StickyNoteStyle::Shutdown();
	}
};

IMPLEMENT_MODULE(FJWCommonUtilityEditorModule, JWCommonUtilityEditor)
