// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_APIKeyVault.generated.h"

/** 원문을 노출하지 않는 저장 키의 관리 정보다. 만료일은 사용자가 지정한 관리 화면 표시용 날짜다. */
USTRUCT(BlueprintType)
struct JWCOMMONUTILITY_API FJWCU_APIKeyInfo
{
	GENERATED_BODY()
	/** 저장소 안에서 키를 식별하는 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FGuid Id;
	/** 사용자가 붙인 키 이름 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FString Label;
	/** 원문 대신 보여 주는 마스킹된 끝자리 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FString MaskedKey;
	/** 해당 공급자의 기본 사용 키인지 나타내는 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") bool bActive = false;
	/** 이 저장소 등록 시각(UTC) 필드. 이전 형식에서 이관한 키는 알 수 없어 최소 날짜를 사용한다. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FDateTime CreatedUtc;
	/** 마지막 편집 시각(UTC) 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FDateTime UpdatedUtc;
	/** 관리용 만료 시각(UTC) 필드. 최소 날짜는 미지정이며 API 사용을 차단하지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FDateTime ExpiresUtc;
};

/** 공급자별 여러 키와 사용 키 선택·메타데이터를 하나의 OS 암호문으로 저장한다. 게임 스레드 전용이다. */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_APIKeyVault : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 저장 키 목록을 원문 없이 조회하는 함수. 손상 시 false이며 빈 목록으로 덮어쓰지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool ListKeys(const FString& AppId, const FString& Group, TArray<FJWCU_APIKeyInfo>& OutKeys);
	/** 키를 추가·편집하는 함수. 새 항목은 빈 Id를 사용하며 편집 시 빈 ApiKey는 기존 원문을 유지한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool SaveKey(const FString& AppId, const FString& Group, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId);
	/** 해당 그룹의 사용 키를 선택하고 APIKeyStore의 전역 임시 덮어쓰기를 해제하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool SelectKey(const FString& AppId, const FString& Group, FGuid Id);
	/** 로컬 키를 삭제하는 함수. 사용 키를 삭제하면 자동 대체하지 않고 선택을 비운다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool DeleteKey(const FString& AppId, const FString& Group, FGuid Id);
	/** 그룹의 모든 저장 키와 APIKeyStore의 전역 임시 덮어쓰기를 제거하는 함수. 공급자 서버의 키는 폐기하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool DeleteAllKeys(const FString& AppId, const FString& Group);
	/** APIKeyStore의 전역 임시 덮어쓰기 또는 선택한 저장 키의 원문을 반환하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|Security|API Keys")
	static bool GetActiveKey(const FString& AppId, const FString& Group, FString& OutKey);
	/** 이전 단일 키 암호문을 목록으로 옮기는 함수. 기존 목록이 있으면 우선하며 검증 저장 후 원본을 삭제한다. */
	static bool MigrateSingleKey(const FString& AppId, const FString& Group);

private:
	struct FEntry { FJWCU_APIKeyInfo Info; FString Secret; };
	static bool Load(const FString& AppId, const FString& Group, TArray<FEntry>& Entries, FGuid& Active);
	static bool Save(const FString& AppId, const FString& Group, const TArray<FEntry>& Entries, const FGuid& Active);
	static void Clear(TArray<FEntry>& Entries);
};
