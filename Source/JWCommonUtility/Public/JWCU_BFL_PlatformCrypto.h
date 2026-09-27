// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_PlatformCrypto.generated.h"

/** OS 사용자 계정에 결합된 데이터 보호를 제공한다. 현재 Windows DPAPI를 지원한다. */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_PlatformCrypto : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 현재 플랫폼이 사용자 계정 기반 암호화를 지원하는지 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWCU|Security")
	static bool IsProtectionSupported();
	/** 바이트를 현재 사용자 계정으로 암호화하는 함수. Context는 복호화 때도 같아야 하며 비밀키가 아니다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security")
	static bool ProtectData(const TArray<uint8>& PlainData, const FString& Context, TArray<uint8>& OutProtectedData);
	/** 암호문을 현재 사용자 계정으로 복호화하는 함수. 실패 시 출력은 비운다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security")
	static bool UnprotectData(const TArray<uint8>& ProtectedData, const FString& Context, TArray<uint8>& OutPlainData);
	/** 사용이 끝난 민감한 바이트 버퍼를 덮어쓰고 해제하는 함수. */
	static void ClearSensitiveBytes(TArray<uint8>& Data);
	/** 사용이 끝난 민감한 문자열 버퍼를 덮어쓰고 해제하는 함수. 다른 복사본은 지우지 않는다. */
	static void ClearSensitiveString(FString& Value);
};
