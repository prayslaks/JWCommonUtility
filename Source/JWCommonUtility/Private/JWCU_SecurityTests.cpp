// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWCU_BFL_APIKeyStore.h"
#include "JWCU_BFL_PlatformCrypto.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_PlatformCryptoTest,
	"JWCommonUtility.Security.PlatformCrypto", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_PlatformCryptoTest::RunTest(const FString& Parameters)
{
	const TArray<uint8> Plain{0, 1, 2, 3, 255, 0, 9};
	TArray<uint8> Encrypted, Restored;
	TestTrue(TEXT("Windows protection is supported"), UJWCU_BFL_PlatformCrypto::IsProtectionSupported());
	if (!TestTrue(TEXT("Protect binary data"), UJWCU_BFL_PlatformCrypto::ProtectData(Plain, TEXT("test-context"), Encrypted))) return false;
	TestTrue(TEXT("Ciphertext differs from plaintext"), Encrypted != Plain);
	TestTrue(TEXT("Unprotect binary data"), UJWCU_BFL_PlatformCrypto::UnprotectData(Encrypted, TEXT("test-context"), Restored));
	TestTrue(TEXT("Embedded zero bytes survive"), Plain == Restored);
	TestFalse(TEXT("Wrong context cannot decrypt"), UJWCU_BFL_PlatformCrypto::UnprotectData(Encrypted, TEXT("wrong-context"), Restored));
	TestTrue(TEXT("Failed decryption clears output"), Restored.IsEmpty());
	Encrypted[Encrypted.Num() / 2] ^= 1;
	TestFalse(TEXT("Tampering is rejected"), UJWCU_BFL_PlatformCrypto::UnprotectData(Encrypted, TEXT("test-context"), Restored));
	TestFalse(TEXT("Empty input is rejected"), UJWCU_BFL_PlatformCrypto::ProtectData({}, TEXT("test-context"), Encrypted));
	TestTrue(TEXT("Failed protection clears output"), Encrypted.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWCU_APIKeyStoreTest,
	"JWCommonUtility.Security.APIKeyStore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWCU_APIKeyStoreTest::RunTest(const FString& Parameters)
{
	const FString App = TEXT("JWCU-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Name = TEXT("Provider");
	const FString Path = UJWCU_BFL_APIKeyStore::GetStoragePath(App, Name);
	ON_SCOPE_EXIT { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false); UJWCU_BFL_APIKeyStore::ForgetApiKey(App, Name); };
	FString Read = TEXT("stale-output");
	TestFalse(TEXT("Missing key returns false"), UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read));
	TestTrue(TEXT("Missing key clears output"), Read.IsEmpty());
	TestFalse(TEXT("Path traversal is rejected"), UJWCU_BFL_APIKeyStore::SetApiKey(TEXT("../escape"), Name, TEXT("fake"), true));
	TestTrue(TEXT("Runtime-only key accepted"), UJWCU_BFL_APIKeyStore::SetApiKey(App, Name, TEXT("fake-runtime"), false));
	TestFalse(TEXT("Runtime key creates no file"), UJWCU_BFL_APIKeyStore::HasStoredApiKey(App, Name));
	if (!TestTrue(TEXT("Remembered key saved"), UJWCU_BFL_APIKeyStore::SetApiKey(App, Name, TEXT("  fake-saved-한글  "), true))) return false;
	TestTrue(TEXT("Stored key is readable"), UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read));
	TestEqual(TEXT("UTF8 key roundtrip trims outer whitespace"), Read, FString(TEXT("fake-saved-한글")));
	TestFalse(TEXT("Other app cannot read this key"), UJWCU_BFL_APIKeyStore::GetApiKey(App + TEXT("-other"), Name, Read));
	TArray<uint8> Raw;
	FFileHelper::LoadFileToArray(Raw, *Path);
	const FTCHARToUTF8 Plain(TEXT("fake-saved-한글"));
	bool bContainsPlain = false;
	for (int32 Index = 0; Index + Plain.Length() <= Raw.Num(); ++Index)
		bContainsPlain |= FMemory::Memcmp(Raw.GetData() + Index, Plain.Get(), Plain.Length()) == 0;
	TestFalse(TEXT("Stored bytes do not contain the plaintext"), bContainsPlain);
	TestTrue(TEXT("Runtime override accepted"), UJWCU_BFL_APIKeyStore::SetApiKey(App, Name, TEXT("fake-override"), false));
	UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read);
	TestEqual(TEXT("Runtime override has priority"), Read, FString(TEXT("fake-override")));
	UJWCU_BFL_APIKeyStore::ClearMemoryKey(App, Name);
	UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read);
	TestEqual(TEXT("Clearing memory restores saved key"), Read, FString(TEXT("fake-saved-한글")));
	TestTrue(TEXT("Read-only fixture is established"), FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, true));
	TestFalse(TEXT("Failed replacement is reported"), UJWCU_BFL_APIKeyStore::SetApiKey(App, Name, TEXT("fake-replacement"), true));
	UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read);
	TestEqual(TEXT("Failed replacement preserves original"), Read, FString(TEXT("fake-saved-한글")));
	FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false);
	TestTrue(TEXT("Successful replacement works"), UJWCU_BFL_APIKeyStore::SetApiKey(App, Name, TEXT("fake-new"), true));
	UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read);
	TestEqual(TEXT("Replacement survives reload"), Read, FString(TEXT("fake-new")));
	FFileHelper::SaveArrayToFile(TArray<uint8>{1, 2, 3}, *Path);
	TestTrue(TEXT("Corrupt file is distinguished from absence"), UJWCU_BFL_APIKeyStore::HasStoredApiKey(App, Name));
	TestFalse(TEXT("Corrupt file fails closed"), UJWCU_BFL_APIKeyStore::GetApiKey(App, Name, Read));
	TestTrue(TEXT("Corruption returns no stale key"), Read.IsEmpty());
	TestTrue(TEXT("Forget removes corrupt file"), UJWCU_BFL_APIKeyStore::ForgetApiKey(App, Name));
	TestFalse(TEXT("Forget leaves no file"), UJWCU_BFL_APIKeyStore::HasStoredApiKey(App, Name));
	TestTrue(TEXT("Forget missing key is idempotent"), UJWCU_BFL_APIKeyStore::ForgetApiKey(App, Name));
	return true;
}

#endif
