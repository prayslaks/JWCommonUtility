// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_AIProviderSettings.h"

#include "JWCU_BFL_PlatformCrypto.h"
#include "Misc/App.h"

const TArray<FJWCU_AIProviderDefinition>& UJWCU_AIProviderSettings::GetProviderCatalog()
{
	// 공급자 ID는 저장소 식별자이므로 변경하지 않는다. 지원 종료 시 항목 대신 상태를 바꾼다.
	static const TArray<FJWCU_AIProviderDefinition> Catalog{{TEXT("OpenAI"), false}, {TEXT("Gemini"), false}};
	return Catalog;
}

bool UJWCU_AIProviderSettings::IsKnownProvider(const FString& Provider)
{
	return GetProviderCatalog().ContainsByPredicate([&Provider](const FJWCU_AIProviderDefinition& Entry) { return Entry.Id == Provider; });
}

bool UJWCU_AIProviderSettings::IsSupportedProvider(const FString& Provider)
{
	return GetProviderCatalog().ContainsByPredicate([&Provider](const FJWCU_AIProviderDefinition& Entry) { return Entry.Id == Provider && !Entry.bDeprecated; });
}

FString UJWCU_AIProviderSettings::GetStorageNamespace() const
{
	return FApp::GetProjectName();
}

bool UJWCU_AIProviderSettings::GetSavedKeys(const FString& Provider, TArray<FJWCU_APIKeyInfo>& OutKeys) const
{
	OutKeys.Reset();
	if (!IsKnownProvider(Provider)) return false;
	const FString AppId = GetStorageNamespace();
	return UJWCU_BFL_APIKeyVault::MigrateSingleKey(AppId, Provider) && UJWCU_BFL_APIKeyVault::ListKeys(AppId, Provider, OutKeys);
}

bool UJWCU_AIProviderSettings::SaveManagedKey(const FString& Provider, FGuid Id, const FString& Label, const FString& Key, FDateTime Expiry, FGuid& OutId) const
{
	OutId.Invalidate();
	if (!IsSupportedProvider(Provider)) return false;
	const FString AppId = GetStorageNamespace();
	return UJWCU_BFL_APIKeyVault::MigrateSingleKey(AppId, Provider) && UJWCU_BFL_APIKeyVault::SaveKey(AppId, Provider, Id, Label, Key, Expiry, OutId);
}

bool UJWCU_AIProviderSettings::SelectManagedKey(const FString& Provider, FGuid Id) const
{
	return IsSupportedProvider(Provider) && UJWCU_BFL_APIKeyVault::SelectKey(GetStorageNamespace(), Provider, Id);
}

bool UJWCU_AIProviderSettings::DeleteManagedKey(const FString& Provider, FGuid Id) const
{
	return IsKnownProvider(Provider) && UJWCU_BFL_APIKeyVault::DeleteKey(GetStorageNamespace(), Provider, Id);
}

bool UJWCU_AIProviderSettings::GetApiKey(const FString& Provider, FString& OutKey) const
{
	UJWCU_BFL_PlatformCrypto::ClearSensitiveString(OutKey);
	if (!IsSupportedProvider(Provider)) return false;
	const FString AppId = GetStorageNamespace();
	return UJWCU_BFL_APIKeyVault::MigrateSingleKey(AppId, Provider) && UJWCU_BFL_APIKeyVault::GetActiveKey(AppId, Provider, OutKey);
}
