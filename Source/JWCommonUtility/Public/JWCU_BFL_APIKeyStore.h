// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_APIKeyStore.generated.h"

/** 앱 식별자·키 이름별 개인 API 키를 메모리 또는 사용자 로컬 암호문으로 보관한다. 게임 스레드 전용이다. */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_APIKeyStore : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 키를 적용하며 Remember가 true이면 검증한 OS 암호문을 저장하는 함수. 빈 키는 저장 시 삭제, 메모리 모드에서는 임시 차단한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool SetApiKey(const FString& AppId, const FString& KeyName, const FString& ApiKey, bool bRemember = false);
	/** 메모리 우선으로 키를 조회하고 없으면 저장본을 복호화하는 함수. 실패·미존재 시 출력은 비운다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool GetApiKey(const FString& AppId, const FString& KeyName, FString& OutApiKey);
	/** 비어 있지 않은 키를 실제로 읽을 수 있는지 반환하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool HasApiKey(const FString& AppId, const FString& KeyName);
	/** 저장 파일이 존재하는지 반환하는 함수. 손상·다른 계정의 파일도 존재로 취급한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool HasStoredApiKey(const FString& AppId, const FString& KeyName);
	/** 메모리 덮어쓰기를 무시하고 암호화 저장본만 복호화하는 함수. */
	static bool GetStoredApiKey(const FString& AppId, const FString& KeyName, FString& OutApiKey);
	/** 저장본과 메모리 키를 제거하는 함수. 저장본 삭제 실패 시 false를 반환한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool ForgetApiKey(const FString& AppId, const FString& KeyName);
	/** 실행 중 덮어쓰기만 지워 다음 조회부터 저장본을 사용하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static void ClearMemoryKey(const FString& AppId, const FString& KeyName);
	/** 모듈 종료 때 모든 실행 중 키 버퍼를 덮어쓰는 함수. */
	static void ClearAllMemoryKeys();
	/** 빈 값 차단을 포함한 실행 중 덮어쓰기 존재 여부를 반환하는 함수. */
	static bool HasMemoryOverride(const FString& AppId, const FString& KeyName);

	/** 저장소 식별자의 허용 문자와 길이를 검증하는 함수. */
	static bool IsValidIdentifier(const FString& Value);

private:
	friend class FJWCU_APIKeyStoreTest;
	static FString GetStoragePath(const FString& AppId, const FString& KeyName);
	static FString GetContext(const FString& AppId, const FString& KeyName);
	static TMap<FString, FString> MemoryKeys;
};
