// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "JWCU_AIProviderKeysCustomization.h"
#include "StickyNotes/JWCU_StickyNoteStyle.h"

/** JWCU 설정 관리 화면과 포스트잇 SVG 스타일을 에디터 모듈 수명에 맞춰 등록·해제한다. */
class FJWCommonUtilityEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FJWCU_StickyNoteStyle::Initialize();
		FPropertyEditorModule& Editor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		Editor.RegisterCustomClassLayout(TEXT("JWCU_AIProviderSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FJWCU_AIProviderKeysCustomization::MakeInstance));
		Editor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		FJWCU_StickyNoteStyle::Shutdown();
		if (FPropertyEditorModule* Editor = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
			Editor->UnregisterCustomClassLayout(TEXT("JWCU_AIProviderSettings"));
	}
};

IMPLEMENT_MODULE(FJWCommonUtilityEditorModule, JWCommonUtilityEditor)
