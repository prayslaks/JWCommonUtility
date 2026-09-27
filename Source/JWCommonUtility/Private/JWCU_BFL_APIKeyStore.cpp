// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_BFL_APIKeyStore.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "JWCU_BFL_PlatformCrypto.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

TMap<FString, FString> UJWCU_BFL_APIKeyStore::MemoryKeys;

bool UJWCU_BFL_APIKeyStore::IsValidIdentifier(const FString& Value)
{
	if (Value.IsEmpty() || Value.Len() > 128) return false;
	for (TCHAR Char : Value)
	{
		if (!(Char >= 'a' && Char <= 'z') && !(Char >= 'A' && Char <= 'Z')
			&& !(Char >= '0' && Char <= '9') && Char != '-' && Char != '_') return false;
	}
	return true;
}

FString UJWCU_BFL_APIKeyStore::GetContext(const FString& AppId, const FString& KeyName)
{
	return TEXT("JWCU.APIKey.v1/") + AppId + TEXT("/") + KeyName;
}

FString UJWCU_BFL_APIKeyStore::GetStoragePath(const FString& AppId, const FString& KeyName)
{
	if (!IsValidIdentifier(AppId) || !IsValidIdentifier(KeyName)) return FString();
	const FTCHARToUTF8 Context(*GetContext(AppId, KeyName));
	uint8 Hash[20];
	FSHA1::HashBuffer(Context.Get(), Context.Length(), Hash);
	// 해시는 암호화가 아니라 파일 예약명·경로 구분자·대소문자 충돌을 피하기 위한 식별자다.
	return FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("JWCommonUtility/APIKeys"), BytesToHex(Hash, 20) + TEXT(".bin"));
}

bool UJWCU_BFL_APIKeyStore::SetApiKey(const FString& AppId, const FString& KeyName, const FString& ApiKey, bool bRemember)
{
	check(IsInGameThread());
	const FString Path = GetStoragePath(AppId, KeyName);
	if (Path.IsEmpty() || ApiKey.Len() > 131072) return false;
	for (TCHAR Char : ApiKey) if (Char == 0) return false;
	FString Key = ApiKey.TrimStartAndEnd();
	ON_SCOPE_EXIT { UJWCU_BFL_PlatformCrypto::ClearSensitiveString(Key); };
	if (bRemember)
	{
		if (Key.IsEmpty()) return ForgetApiKey(AppId, KeyName);
		const FString Context = GetContext(AppId, KeyName);
		TArray<uint8> Plain, Encrypted, Verified;
		ON_SCOPE_EXIT { UJWCU_BFL_PlatformCrypto::ClearSensitiveBytes(Plain); UJWCU_BFL_PlatformCrypto::ClearSensitiveBytes(Verified); };
		const FTCHARToUTF8 Utf8(*Key);
		Plain.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length() + 1);
		if (!UJWCU_BFL_PlatformCrypto::ProtectData(Plain, Context, Encrypted)) return false;
		IFileManager& Files = IFileManager::Get();
		if (!Files.MakeDirectory(*FPaths::GetPath(Path), true)) return false;
		const FString TempPath = Path + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
		ON_SCOPE_EXIT { Files.Delete(*TempPath, false, true, true); };
		// 완전한 파일을 다시 읽어 복원한 값이 같을 때만 기존 저장본을 교체한다.
		if (!FFileHelper::SaveArrayToFile(Encrypted, *TempPath)) return false;
		Encrypted.Reset();
		if (!FFileHelper::LoadFileToArray(Encrypted, *TempPath)
			|| !UJWCU_BFL_PlatformCrypto::UnprotectData(Encrypted, Context, Verified) || Verified != Plain) return false;
		// UE의 일반 Move는 목적 파일을 먼저 삭제하므로 Windows의 교체 연산을 직접 사용한다.
#if PLATFORM_WINDOWS
		if (!MoveFileExW(*TempPath, *Path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return false;
#else
		return false;
#endif
		ClearMemoryKey(AppId, KeyName);
		return true;
	}
	ClearMemoryKey(AppId, KeyName);
	MemoryKeys.Add(GetContext(AppId, KeyName), Key);
	return true;
}

bool UJWCU_BFL_APIKeyStore::GetStoredApiKey(const FString& AppId, const FString& KeyName, FString& OutApiKey)
{
	check(IsInGameThread());
	UJWCU_BFL_PlatformCrypto::ClearSensitiveString(OutApiKey);
	const FString Path = GetStoragePath(AppId, KeyName);
	if (Path.IsEmpty()) return false;
	const int64 Size = IFileManager::Get().FileSize(*Path);
	if (Size <= 0 || Size > 2 * 1024 * 1024) return false;
	TArray<uint8> Encrypted, Plain;
	ON_SCOPE_EXIT { UJWCU_BFL_PlatformCrypto::ClearSensitiveBytes(Plain); };
	if (!FFileHelper::LoadFileToArray(Encrypted, *Path)
		|| !UJWCU_BFL_PlatformCrypto::UnprotectData(Encrypted, GetContext(AppId, KeyName), Plain)
		|| Plain.Num() < 2 || Plain.Last() != 0) return false;
	for (int32 Index = 0; Index < Plain.Num() - 1; ++Index) if (Plain[Index] == 0) return false;
	OutApiKey = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(Plain.GetData()));
	return !OutApiKey.IsEmpty();
}

bool UJWCU_BFL_APIKeyStore::GetApiKey(const FString& AppId, const FString& KeyName, FString& OutApiKey)
{
	check(IsInGameThread());
	UJWCU_BFL_PlatformCrypto::ClearSensitiveString(OutApiKey);
	if (!IsValidIdentifier(AppId) || !IsValidIdentifier(KeyName)) return false;
	if (const FString* Key = MemoryKeys.Find(GetContext(AppId, KeyName)))
	{
		OutApiKey = *Key;
		return !OutApiKey.IsEmpty();
	}
	return GetStoredApiKey(AppId, KeyName, OutApiKey);
}

bool UJWCU_BFL_APIKeyStore::HasApiKey(const FString& AppId, const FString& KeyName)
{
	FString Key;
	const bool bFound = GetApiKey(AppId, KeyName, Key);
	UJWCU_BFL_PlatformCrypto::ClearSensitiveString(Key);
	return bFound;
}

bool UJWCU_BFL_APIKeyStore::HasStoredApiKey(const FString& AppId, const FString& KeyName)
{
	check(IsInGameThread());
	const FString Path = GetStoragePath(AppId, KeyName);
	return !Path.IsEmpty() && IFileManager::Get().FileExists(*Path);
}

void UJWCU_BFL_APIKeyStore::ClearMemoryKey(const FString& AppId, const FString& KeyName)
{
	check(IsInGameThread());
	const FString Context = GetContext(AppId, KeyName);
	if (FString* Key = MemoryKeys.Find(Context)) UJWCU_BFL_PlatformCrypto::ClearSensitiveString(*Key);
	MemoryKeys.Remove(Context);
}

bool UJWCU_BFL_APIKeyStore::ForgetApiKey(const FString& AppId, const FString& KeyName)
{
	check(IsInGameThread());
	const FString Path = GetStoragePath(AppId, KeyName);
	if (Path.IsEmpty()) return false;
	if (!IFileManager::Get().Delete(*Path, false, true, true)) return false;
	ClearMemoryKey(AppId, KeyName);
	return true;
}

void UJWCU_BFL_APIKeyStore::ClearAllMemoryKeys()
{
	for (auto& Pair : MemoryKeys) UJWCU_BFL_PlatformCrypto::ClearSensitiveString(Pair.Value);
	MemoryKeys.Empty();
}

bool UJWCU_BFL_APIKeyStore::HasMemoryOverride(const FString& AppId, const FString& KeyName)
{
	check(IsInGameThread());
	return MemoryKeys.Contains(GetContext(AppId, KeyName));
}
