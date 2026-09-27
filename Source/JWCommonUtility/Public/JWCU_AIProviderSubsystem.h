// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWCU_BFL_APIKeyVault.h"
#include "JWCU_AIProviderSubsystem.generated.h"

/** 개인 저장 키 관리와 GameInstance별 임시 키 수명을 BP에 제공한다. 공급자 ID는 JWCU 코드에 등록된 OpenAI·Gemini 등 문자열이다. */
UCLASS(meta=(DisplayName="JWCU AI Provider Subsystem"))
class JWCOMMONUTILITY_API UJWCU_AIProviderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	/** 현재 지원하는 고정 공급자 ID 목록을 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWCU|AI Providers")
	TArray<FString> GetSupportedProviders() const;

	/** 비밀 원문을 제외한 공급자의 저장 키 목록을 반환하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool ListKeys(const FString& Provider, TArray<FJWCU_APIKeyInfo>& OutKeys);
	/** 키를 추가·수정하는 함수. 빈 Id는 추가, 빈 ApiKey는 수정 시 기존 값 유지이며 사용 키 저장은 이 인스턴스의 임시 키를 해제한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool SaveKey(const FString& Provider, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId);
	/** 저장 키를 선택하고 이 GameInstance의 임시 덮어쓰기를 해제하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool SelectKey(const FString& Provider, FGuid Id);
	/** 로컬 저장 키를 삭제하는 함수. 사용 키 삭제 시 이 인스턴스의 임시 키도 해제한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool DeleteKey(const FString& Provider, FGuid Id);
	/** 공급자의 로컬 저장 키 전체와 이 인스턴스의 임시 키를 삭제하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool DeleteAllKeys(const FString& Provider);
	/** 임시 키 우선으로 사용 키를 반환하는 함수. 비밀값을 로그·에셋에 기록하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool GetApiKey(const FString& Provider, FString& OutApiKey);
	/** 현재 인스턴스가 비어 있지 않은 사용 키를 읽을 수 있는지 확인하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool HasApiKey(const FString& Provider);
	/** 이 GameInstance에서만 사용할 임시 키를 지정하는 함수. 빈 값은 저장 키 조회를 임시 차단한다. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	bool SetRuntimeApiKey(const FString& Provider, const FString& ApiKey);
	/** 이 인스턴스의 임시 키를 해제하고 저장 키 조회로 복귀하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWCU|AI Providers")
	void ClearRuntimeApiKey(const FString& Provider);
	/** 이 인스턴스에 빈 값 차단을 포함한 임시 키 설정이 있는지 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWCU|AI Providers")
	bool HasRuntimeOverride(const FString& Provider) const;

private:
	friend class FJWCU_AIProviderSubsystemTest;
	FString StorageNamespace;
	TMap<FString, FString> RuntimeKeys;
	bool PrepareProvider(const FString& Provider);
	void ClearRuntimeKeys();
};
