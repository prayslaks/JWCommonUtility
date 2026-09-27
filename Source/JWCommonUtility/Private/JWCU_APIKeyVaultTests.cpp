// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWCU_BFL_APIKeyVault.h"
#include "JWCU_BFL_APIKeyStore.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_APIKeyVaultTest,
	"JWCommonUtility.Security.APIKeyVault", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_APIKeyVaultTest::RunTest(const FString& Parameters)
{
	const FString App = TEXT("JWCU-VaultTest-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Group = TEXT("Provider");
	ON_SCOPE_EXIT { UJWCU_BFL_APIKeyStore::ForgetApiKey(App, Group); UJWCU_BFL_APIKeyStore::ForgetApiKey(App, TEXT("Vault-") + Group); };
	TArray<FJWCU_APIKeyInfo> Keys;
	TestFalse(TEXT("Invalid path identifier rejected on read"), UJWCU_BFL_APIKeyVault::ListKeys(TEXT("../bad"), Group, Keys));
	TestTrue(TEXT("New vault is empty"), UJWCU_BFL_APIKeyVault::ListKeys(App, Group, Keys) && Keys.IsEmpty());
	FGuid First, Second, Edited;
	const FDateTime Expired(2020, 1, 1);
	TestTrue(TEXT("First key saved"), UJWCU_BFL_APIKeyVault::SaveKey(App, Group, {}, TEXT("개발"), TEXT("fake-vault-first-1234"), {}, First));
	TestTrue(TEXT("Second key saved"), UJWCU_BFL_APIKeyVault::SaveKey(App, Group, {}, TEXT("테스트"), TEXT("fake-vault-second-5678"), Expired, Second));
	TestTrue(TEXT("List includes both keys"), UJWCU_BFL_APIKeyVault::ListKeys(App, Group, Keys) && Keys.Num() == 2);
	if (Keys.Num() != 2) return false;
	TestTrue(TEXT("Only first key starts active"), Keys[0].bActive && !Keys[1].bActive);
	TestTrue(TEXT("Display metadata masks full key"), Keys[0].MaskedKey.EndsWith(TEXT("1234")) && !Keys[0].MaskedKey.Contains(TEXT("fake-vault")));
	const FDateTime Created = Keys[1].CreatedUtc;
	TestTrue(TEXT("Registration time is recorded"), Created.GetTicks() > 0);
	TestTrue(TEXT("Select second key"), UJWCU_BFL_APIKeyVault::SelectKey(App, Group, Second));
	FString Read;
	TestTrue(TEXT("Expired management date does not block API access"), UJWCU_BFL_APIKeyVault::GetActiveKey(App, Group, Read));
	TestEqual(TEXT("Selection changes actual returned key"), Read, FString(TEXT("fake-vault-second-5678")));
	TestTrue(TEXT("Edit metadata without secret replacement"), UJWCU_BFL_APIKeyVault::SaveKey(App, Group, Second, TEXT("새 이름"), TEXT(""), {}, Edited));
	TestTrue(TEXT("Editing keeps ID"), Edited == Second);
	UJWCU_BFL_APIKeyVault::ListKeys(App, Group, Keys);
	TestTrue(TEXT("Editing preserves creation time and clears expiry"), Keys[1].CreatedUtc == Created && Keys[1].ExpiresUtc.GetTicks() == 0);
	UJWCU_BFL_APIKeyVault::GetActiveKey(App, Group, Read);
	TestEqual(TEXT("Empty edit preserves secret"), Read, FString(TEXT("fake-vault-second-5678")));
	UJWCU_BFL_APIKeyStore::SetApiKey(App, Group, TEXT(""), false);
	TestFalse(TEXT("Empty runtime override masks active key"), UJWCU_BFL_APIKeyVault::GetActiveKey(App, Group, Read));
	TestTrue(TEXT("Selecting clears runtime override"), UJWCU_BFL_APIKeyVault::SelectKey(App, Group, Second) && !UJWCU_BFL_APIKeyStore::HasMemoryOverride(App, Group));
	TestTrue(TEXT("Delete active key"), UJWCU_BFL_APIKeyVault::DeleteKey(App, Group, Second));
	TestFalse(TEXT("Deleting active key does not select another"), UJWCU_BFL_APIKeyVault::GetActiveKey(App, Group, Read));
	TestFalse(TEXT("Unknown ID cannot be edited"), UJWCU_BFL_APIKeyVault::SaveKey(App, Group, Second, TEXT("gone"), TEXT("fake"), {}, Edited));
	TestFalse(TEXT("Unknown ID cannot be selected"), UJWCU_BFL_APIKeyVault::SelectKey(App, Group, Second));
	TestTrue(TEXT("Delete all persists empty vault"), UJWCU_BFL_APIKeyVault::DeleteAllKeys(App, Group));
	UJWCU_BFL_APIKeyStore::SetApiKey(App, Group, TEXT("fake-old-key"), true);
	TestTrue(TEXT("Existing empty vault wins over legacy key"), UJWCU_BFL_APIKeyVault::MigrateSingleKey(App, Group));
	TestTrue(TEXT("Old key cannot resurrect after deletion"), UJWCU_BFL_APIKeyVault::ListKeys(App, Group, Keys) && Keys.IsEmpty());
	UJWCU_BFL_APIKeyStore::ForgetApiKey(App, TEXT("Vault-") + Group);
	UJWCU_BFL_APIKeyStore::SetApiKey(App, Group, TEXT("fake-migration-key"), true);
	TestTrue(TEXT("Single encrypted key migrates"), UJWCU_BFL_APIKeyVault::MigrateSingleKey(App, Group));
	TestFalse(TEXT("Migrated scalar is removed"), UJWCU_BFL_APIKeyStore::HasStoredApiKey(App, Group));
	TestTrue(TEXT("Migration is idempotent"), UJWCU_BFL_APIKeyVault::MigrateSingleKey(App, Group));
	UJWCU_BFL_APIKeyVault::ListKeys(App, Group, Keys);
	TestTrue(TEXT("Migrated key active with unknown original dates"), Keys.Num() == 1 && Keys[0].bActive && Keys[0].CreatedUtc.GetTicks() == 0);
	UJWCU_BFL_APIKeyVault::GetActiveKey(App, Group, Read);
	TestEqual(TEXT("Migration preserves actual key"), Read, FString(TEXT("fake-migration-key")));
	UJWCU_BFL_APIKeyStore::SetApiKey(App, TEXT("Vault-") + Group, TEXT("invalid-json"), true);
	TestFalse(TEXT("Corrupt vault cannot be silently overwritten"), UJWCU_BFL_APIKeyVault::SaveKey(App, Group, {}, TEXT("new"), TEXT("fake"), {}, Edited));
	UJWCU_BFL_APIKeyStore::GetStoredApiKey(App, TEXT("Vault-") + Group, Read);
	TestEqual(TEXT("Corrupt original is retained"), Read, FString(TEXT("invalid-json")));
	return true;
}

#endif
