// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_CollectionUtility.generated.h"

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_CollectionUtility, Log, All);

/**
 * 배열/컬렉션 유틸리티 블루프린트 함수 라이브러리.
 * 셔플, 랜덤 추출, 중복 제거, 정렬 등을 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_CollectionUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Shuffle String Array \n 문자열 배열을 무작위로 섞는다 (Fisher-Yates).
	 * @param InArray 셔플할 배열
	 * @return 셔플된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<FString> ShuffleStringArray(const TArray<FString>& InArray);

	/**
	 * [ Blueprint Function Library ] \n Shuffle Int Array \n 정수 배열을 무작위로 섞는다 (Fisher-Yates).
	 * @param InArray 셔플할 배열
	 * @return 셔플된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<int32> ShuffleIntArray(const TArray<int32>& InArray);

	/**
	 * [ Blueprint Function Library ] \n Shuffle Actor Array \n 액터 배열을 무작위로 섞는다 (Fisher-Yates).
	 * @param InArray 셔플할 배열
	 * @return 셔플된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<AActor*> ShuffleActorArray(const TArray<AActor*>& InArray);

	/**
	 * [ Blueprint Function Library ] \n Pick Random Strings \n 문자열 배열에서 중복 없이 N개를 무작위로 추출한다.
	 * @param InArray 원본 배열
	 * @param InCount 추출할 개수
	 * @return 추출된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<FString> PickRandomStrings(const TArray<FString>& InArray, int32 InCount);

	/**
	 * [ Blueprint Function Library ] \n Remove Duplicate Strings \n 문자열 배열에서 중복을 제거한다 (순서 유지).
	 * @param InArray 원본 배열
	 * @return 중복이 제거된 배열
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static TArray<FString> RemoveDuplicateStrings(const TArray<FString>& InArray);

	/**
	 * [ Blueprint Function Library ] \n Remove Null Actors \n 액터 배열에서 null/invalid 항목을 제거한다.
	 * @param InArray 원본 배열
	 * @return 유효한 액터만 포함된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<AActor*> RemoveNullActors(const TArray<AActor*>& InArray);

	/**
	 * [ Blueprint Function Library ] \n Sort Float Array \n 실수 배열을 정렬한다.
	 * @param InArray 정렬할 배열
	 * @param InOrder 정렬 순서 (Ascending / Descending)
	 * @return 정렬된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<float> SortFloatArray(const TArray<float>& InArray, EJWCU_SortOrder InOrder);

	/**
	 * [ Blueprint Function Library ] \n Sort Int Array \n 정수 배열을 정렬한다.
	 * @param InArray 정렬할 배열
	 * @param InOrder 정렬 순서 (Ascending / Descending)
	 * @return 정렬된 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static TArray<int32> SortIntArray(const TArray<int32>& InArray, EJWCU_SortOrder InOrder);
};
