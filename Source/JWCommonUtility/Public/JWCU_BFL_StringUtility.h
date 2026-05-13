// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_StringUtility.generated.h"

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_StringUtility, Log, All);

/**
 * 문자열 관련 유틸리티 블루프린트 함수 라이브러리.
 * 숫자 포맷, 리치 텍스트, 템플릿 슬롯 교체 등을 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_StringUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Abbreviate Number \n 거대한 숫자를 단위 요약 문자열로 변환한다.
	 * @param InValue 변환할 숫자 값
	 * @param InThreshold 단위 요약 임계값 (K, M, B 등)
	 * @param InDecimalPlaces 소수점 자릿수 (기본값 1)
	 * @return 요약된 문자열 (예: "12.5K", "1.2M")
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString AbbreviateNumber(float InValue, EJWCU_NumberAbbreviationThreshold InThreshold, int32 InDecimalPlaces = 1);

	/**
	 * [ Blueprint Function Library ] \n Format Int With Commas \n 정수를 천 단위 콤마가 포함된 문자열로 변환한다.
	 * @param InValue 변환할 정수 값
	 * @return 콤마가 포함된 문자열 (예: "1,234,567")
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString FormatIntWithCommas(int32 InValue);

	/**
	 * [ Blueprint Function Library ] \n Format Float With Commas \n 실수를 천 단위 콤마가 포함된 문자열로 변환한다.
	 * @param InValue 변환할 실수 값
	 * @param InDecimalPlaces 소수점 자릿수 (기본값 2)
	 * @return 콤마가 포함된 문자열 (예: "1,234,567.89")
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString FormatFloatWithCommas(float InValue, int32 InDecimalPlaces = 2);

	/**
	 * [ Blueprint Function Library ] \n Wrap In Rich Text Style \n 문자열을 UMG 리치 텍스트 스타일 태그로 감싼다.
	 * @param InContent 감쌀 내용 문자열
	 * @param InStyleName 리치 텍스트 스타일 이름
	 * @return 태그가 감싸진 문자열 (예: "<Bold>Hello</>")
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString WrapInRichTextStyle(const FString& InContent, const FString& InStyleName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString WrapEachLineInRichTextStyle(const FString& InContent, const FString& InStyleName);

	/**
	 * [ Blueprint Function Library ] \n Replace Text Slots \n 템플릿 문자열에서 {Key} 플레이스홀더를 동적으로 교체한다.
	 * @param InTemplate 템플릿 문자열 (예: "Hello {Name}, you have {Count} items.")
	 * @param InReplacements 교체할 키-값 맵 (예: {"Name":"Player", "Count":"5"})
	 * @return 교체된 결과 문자열
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString ReplaceTextSlots(const FString& InTemplate, const TMap<FString, FString>& InReplacements);

	/**
	 * [ Blueprint Function Library ] \n Truncate String \n 문자열을 최대 길이로 자르고 접미사를 붙인다.
	 * @param InString 원본 문자열
	 * @param InMaxLength 최대 길이
	 * @param InSuffix 잘림 시 붙일 접미사 (기본값 "...")
	 * @return 잘린 문자열
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString TruncateString(const FString& InString, int32 InMaxLength, const FString& InSuffix = TEXT("..."));

	/**
	 * [ Blueprint Function Library ] \n Is Null Or Whitespace \n 문자열이 비어있거나 공백만 포함하는지 확인한다.
	 * @param InString 확인할 문자열
	 * @return 빈 문자열이거나 공백만 포함하면 true
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static bool IsNullOrWhitespace(const FString& InString);
};
