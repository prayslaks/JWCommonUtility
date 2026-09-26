// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_DateTimeUtility.generated.h"

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_DateTimeUtility, Log, All);

/**
 * 날짜/시간 관련 유틸리티 블루프린트 함수 라이브러리.
 * FDateTime을 다양한 포맷의 FText로 변환하여 UMG에서 활용할 수 있도록 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_DateTimeUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Format Date \n FDateTime을 지정된 날짜 포맷의 FText로 변환한다.
	 * @param InDateTime 변환할 날짜/시간
	 * @param InFormat 날짜 출력 포맷
	 * @return 포맷된 날짜 텍스트
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FText FormatDate(const FDateTime& InDateTime, EJWCU_DateFormat InFormat);

	/**
	 * [ Blueprint Function Library ] \n Format Time \n FDateTime을 지정된 시간 포맷의 FText로 변환한다.
	 * @param InDateTime 변환할 날짜/시간
	 * @param InFormat 시간 출력 포맷
	 * @return 포맷된 시간 텍스트
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FText FormatTime(const FDateTime& InDateTime, EJWCU_TimeFormat InFormat);

	/**
	 * [ Blueprint Function Library ] \n Format Date Time \n FDateTime을 지정된 날짜+시간 결합 포맷의 FText로 변환한다.
	 * @param InDateTime 변환할 날짜/시간
	 * @param InFormat 날짜+시간 출력 포맷
	 * @return 포맷된 날짜+시간 텍스트
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FText FormatDateTime(const FDateTime& InDateTime, EJWCU_DateTimeFormat InFormat);

	/**
	 * [ Blueprint Function Library ] \n Get Relative Time \n FDateTime을 현재 시간 기준 상대 시간 텍스트로 변환한다.
	 * @param InDateTime 비교할 날짜/시간
	 * @param InGranularity 시간 단위 세분화 옵션
	 * @return 상대 시간 결과 구조체 (표시 텍스트, 초 단위 차이, 과거 여부)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FJWCU_RelativeTimeResult GetRelativeTime(const FDateTime& InDateTime, EJWCU_RelativeTimeGranularity InGranularity);

	/**
	 * [ Blueprint Function Library ] \n Get Now UTC \n 현재 UTC 시간을 FDateTime으로 반환한다.
	 * @return 현재 UTC FDateTime
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FDateTime GetNowUTC();

	/**
	 * [ Blueprint Function Library ] \n Get Now Local \n 현재 로컬 시간을 FDateTime으로 반환한다.
	 * @return 현재 로컬 FDateTime
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FDateTime GetNowLocal();

	/**
	 * [ Blueprint Function Library ] \n Unix Timestamp To DateTime \n Unix 타임스탬프를 FDateTime으로 변환한다.
	 * @param InUnixTimestamp Unix 타임스탬프 (초 단위)
	 * @return 변환된 FDateTime
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FDateTime UnixTimestampToDateTime(int64 InUnixTimestamp);

	/**
	 * [ Blueprint Function Library ] \n DateTime To Unix Timestamp \n FDateTime을 Unix 타임스탬프로 변환한다.
	 * @param InDateTime 변환할 FDateTime
	 * @return Unix 타임스탬프 (초 단위)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static int64 DateTimeToUnixTimestamp(const FDateTime& InDateTime);

private:

	/** 월 이름 약어를 반환하는 내부 헬퍼. */
	static FString GetMonthAbbreviation(int32 InMonth);

	/** 요일 전체 이름을 반환하는 내부 헬퍼. */
	static FString GetWeekdayName(EDayOfWeek InDayOfWeek);

	/** 12시간제 변환 내부 헬퍼. */
	static void ConvertTo12Hour(int32 InHour24, int32& OutHour12, FString& OutAmPm);
};
