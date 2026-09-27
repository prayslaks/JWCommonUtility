// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_AIProviderSubsystem.h"

#include "JWCU_AIProviderSettings.h"
#include "JWCU_BFL_PlatformCrypto.h"
#include "Misc/ScopeExit.h"

void UJWCU_AIProviderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StorageNamespace = GetDefault<UJWCU_AIProviderSettings>()->GetStorageNamespace();
}

void UJWCU_AIProviderSubsystem::ClearRuntimeKeys()
{
	for (auto& Pair : RuntimeKeys) UJWCU_BFL_PlatformCrypto::ClearSensitiveString(Pair.Value);
	RuntimeKeys.Empty();
}

void UJWCU_AIProviderSubsystem::Deinitialize()
{
	ClearRuntimeKeys();
	Super::Deinitialize();
}

void UJWCU_AIProviderSubsystem::BeginDestroy()
{
	ClearRuntimeKeys();
	Super::BeginDestroy();
}

bool UJWCU_AIProviderSubsystem::PrepareProvider(const FString& Provider)
{
	check(IsInGameThread());
	if (StorageNamespace.IsEmpty()) StorageNamespace = GetDefault<UJWCU_AIProviderSettings>()->GetStorageNamespace();
	return UJWCU_AIProviderSettings::IsKnownProvider(Provider)
		&& UJWCU_BFL_APIKeyVault::MigrateSingleKey(StorageNamespace, Provider);
}

TArray<FString> UJWCU_AIProviderSubsystem::GetSupportedProviders() const
{
	TArray<FString> Providers;
	for (const FJWCU_AIProviderDefinition& Entry : UJWCU_AIProviderSettings::GetProviderCatalog())
		if (!Entry.bDeprecated) Providers.Add(Entry.Id);
	return Providers;
}

bool UJWCU_AIProviderSubsystem::ListKeys(const FString& Provider, TArray<FJWCU_APIKeyInfo>& OutKeys)
{
	OutKeys.Reset();
	return PrepareProvider(Provider) && UJWCU_BFL_APIKeyVault::ListKeys(StorageNamespace, Provider, OutKeys);
}

bool UJWCU_AIProviderSubsystem::SaveKey(const FString& Provider, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId)
{
	OutId.Invalidate();
	if (!UJWCU_AIProviderSettings::IsSupportedProvider(Provider) || !PrepareProvider(Provider) || !UJWCU_BFL_APIKeyVault::SaveKey(StorageNamespace, Provider, Id, Label, ApiKey, ExpiresUtc, OutId)) return false;
	TArray<FJWCU_APIKeyInfo> Keys;
	if (UJWCU_BFL_APIKeyVault::ListKeys(StorageNamespace, Provider, Keys)
		&& Keys.ContainsByPredicate([OutId](const FJWCU_APIKeyInfo& Info) { return Info.Id == OutId && Info.bActive; })) ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWCU_AIProviderSubsystem::SelectKey(const FString& Provider, FGuid Id)
{
	if (!UJWCU_AIProviderSettings::IsSupportedProvider(Provider) || !PrepareProvider(Provider) || !UJWCU_BFL_APIKeyVault::SelectKey(StorageNamespace, Provider, Id)) return false;
	ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWCU_AIProviderSubsystem::DeleteKey(const FString& Provider, FGuid Id)
{
	TArray<FJWCU_APIKeyInfo> Keys;
	if (!ListKeys(Provider, Keys)) return false;
	const bool bActive = Keys.ContainsByPredicate([Id](const FJWCU_APIKeyInfo& Info) { return Info.Id == Id && Info.bActive; });
	if (!UJWCU_BFL_APIKeyVault::DeleteKey(StorageNamespace, Provider, Id)) return false;
	if (bActive) ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWCU_AIProviderSubsystem::DeleteAllKeys(const FString& Provider)
{
	if (!PrepareProvider(Provider) || !UJWCU_BFL_APIKeyVault::DeleteAllKeys(StorageNamespace, Provider)) return false;
	ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWCU_AIProviderSubsystem::GetApiKey(const FString& Provider, FString& OutApiKey)
{
	check(IsInGameThread());
	UJWCU_BFL_PlatformCrypto::ClearSensitiveString(OutApiKey);
	if (!UJWCU_AIProviderSettings::IsSupportedProvider(Provider)) return false;
	if (const FString* Key = RuntimeKeys.Find(Provider))
	{
		OutApiKey = *Key;
		return !OutApiKey.IsEmpty();
	}
	return PrepareProvider(Provider) && UJWCU_BFL_APIKeyVault::GetActiveKey(StorageNamespace, Provider, OutApiKey);
}

bool UJWCU_AIProviderSubsystem::HasApiKey(const FString& Provider)
{
	FString Key;
	ON_SCOPE_EXIT { UJWCU_BFL_PlatformCrypto::ClearSensitiveString(Key); };
	return GetApiKey(Provider, Key);
}

bool UJWCU_AIProviderSubsystem::SetRuntimeApiKey(const FString& Provider, const FString& ApiKey)
{
	check(IsInGameThread());
	if (!UJWCU_AIProviderSettings::IsSupportedProvider(Provider) || ApiKey.Len() > 16384) return false;
	for (TCHAR Char : ApiKey) if (Char == 0) return false;
	ClearRuntimeApiKey(Provider);
	RuntimeKeys.Add(Provider, ApiKey.TrimStartAndEnd());
	return true;
}

void UJWCU_AIProviderSubsystem::ClearRuntimeApiKey(const FString& Provider)
{
	check(IsInGameThread());
	if (FString* Key = RuntimeKeys.Find(Provider)) UJWCU_BFL_PlatformCrypto::ClearSensitiveString(*Key);
	RuntimeKeys.Remove(Provider);
}

bool UJWCU_AIProviderSubsystem::HasRuntimeOverride(const FString& Provider) const
{
	return RuntimeKeys.Contains(Provider);
}
