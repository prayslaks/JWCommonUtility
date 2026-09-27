// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "JWCU_AIProviderKeysCustomization.h"

/** JWCU 설정의 관리 화면을 에디터 수명에 맞춰 등록한다. */
class FJWCommonUtilityEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& Editor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		Editor.RegisterCustomClassLayout(TEXT("JWCU_AIProviderSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FJWCU_AIProviderKeysCustomization::MakeInstance));
		Editor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FPropertyEditorModule* Editor = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
			Editor->UnregisterCustomClassLayout(TEXT("JWCU_AIProviderSettings"));
	}
};

IMPLEMENT_MODULE(FJWCommonUtilityEditorModule, JWCommonUtilityEditor)
