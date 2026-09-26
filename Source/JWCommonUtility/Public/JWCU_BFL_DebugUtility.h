// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_DebugUtility.generated.h"

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_DebugUtility, Log, All);

/**
 * 디버그 시각화 유틸리티 블루프린트 함수 라이브러리.
 * 통합 디버그 도형 그리기, 스크린 출력 등을 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_DebugUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Draw Debug Shape At Location \n 지정된 위치에 디버그 도형을 그린다.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InShape 도형 타입
	 * @param InLocation 위치 (Line/Arrow의 경우 시작점)
	 * @param InSize 크기 (Sphere=X가 반지름, Box=Half Extent, Line/Arrow=끝점, Point=X가 크기, Capsule=X반지름 Z높이)
	 * @param InRotation 회전 (Box에 적용)
	 * @param InColor 색상
	 * @param InDuration 표시 시간 (초, 0이면 1프레임)
	 * @param InThickness 선 두께
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(WorldContext="WorldContextObject", DevelopmentOnly))
	static void DrawDebugShapeAtLocation(const UObject* WorldContextObject, EJWCU_DebugShape InShape, FVector InLocation, FVector InSize, FRotator InRotation, FLinearColor InColor, float InDuration = 2.0f, float InThickness = 1.0f);

	/**
	 * [ Blueprint Function Library ] \n Print Key Value To Screen \n 키-값 쌍을 스크린에 출력한다.
	 * @param InKey 키 문자열
	 * @param InValue 값 문자열
	 * @param InColor 표시 색상
	 * @param InDuration 표시 시간 (초)
	 * @param InScreenId 스크린 메시지 ID (-1이면 매번 새 줄)
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(DevelopmentOnly))
	static void PrintKeyValueToScreen(const FString& InKey, const FString& InValue, FColor InColor, float InDuration = 5.0f, int32 InScreenId = -1);

	/**
	 * [ Blueprint Function Library ] \n Print Vector To Screen \n FVector를 포맷팅하여 스크린에 출력한다.
	 * @param InLabel 라벨 문자열
	 * @param InVector 출력할 벡터
	 * @param InDecimalPlaces 소수점 자릿수
	 * @param InColor 표시 색상
	 * @param InDuration 표시 시간 (초)
	 * @param InScreenId 스크린 메시지 ID
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(DevelopmentOnly))
	static void PrintVectorToScreen(const FString& InLabel, FVector InVector, int32 InDecimalPlaces = 2, FColor InColor = FColor::Green, float InDuration = 5.0f, int32 InScreenId = -1);

	/**
	 * [ Blueprint Function Library ] \n Print Rotator To Screen \n FRotator를 포맷팅하여 스크린에 출력한다.
	 * @param InLabel 라벨 문자열
	 * @param InRotator 출력할 로테이터
	 * @param InDecimalPlaces 소수점 자릿수
	 * @param InColor 표시 색상
	 * @param InDuration 표시 시간 (초)
	 * @param InScreenId 스크린 메시지 ID
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(DevelopmentOnly))
	static void PrintRotatorToScreen(const FString& InLabel, FRotator InRotator, int32 InDecimalPlaces = 2, FColor InColor = FColor::Yellow, float InDuration = 5.0f, int32 InScreenId = -1);

	/**
	 * [ Blueprint Function Library ] \n Draw Debug Text At Location \n 월드 좌표에 3D 디버그 텍스트를 표시한다.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InText 표시할 텍스트
	 * @param InWorldLocation 월드 좌표
	 * @param InColor 텍스트 색상
	 * @param InDuration 표시 시간 (초)
	 * @param InFontScale 폰트 스케일
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(WorldContext="WorldContextObject", DevelopmentOnly))
	static void DrawDebugTextAtLocation(const UObject* WorldContextObject, const FString& InText, FVector InWorldLocation, FColor InColor, float InDuration = 5.0f, float InFontScale = 1.0f);
};
