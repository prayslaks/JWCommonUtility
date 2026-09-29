// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWCU_BFL_PlatformCrypto.h"
#include "Misc/AutomationTest.h"

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


#endif
