// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_ColorUtility.generated.h"

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_ColorUtility, Log, All);

/**
 * 색상/비주얼 유틸리티 블루프린트 함수 라이브러리.
 * 색상 블렌딩, 컴포넌트 조작, Hex 변환 등을 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_ColorUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Blend Colors \n 두 색상을 지정된 블렌딩 모드로 혼합한다.
	 * @param InColorA 첫 번째 색상
	 * @param InColorB 두 번째 색상
	 * @param InAlpha 혼합 비율 (0.0 = A, 1.0 = B)
	 * @param InBlendMode 블렌딩 모드
	 * @return 혼합된 색상
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FLinearColor BlendColors(FLinearColor InColorA, FLinearColor InColorB, float InAlpha, EJWCU_ColorBlendMode InBlendMode);

	/**
	 * [ Blueprint Function Library ] \n Get Color Component \n 색상에서 특정 컴포넌트 값을 추출한다.
	 * @param InColor 대상 색상
	 * @param InComponent 추출할 컴포넌트 (R/G/B/A/H/S/V)
	 * @return 컴포넌트 값
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static float GetColorComponent(FLinearColor InColor, EJWCU_ColorComponent InComponent);

	/**
	 * [ Blueprint Function Library ] \n Set Color Component \n 색상의 특정 컴포넌트 값을 설정한다.
	 * @param InColor 대상 색상
	 * @param InComponent 설정할 컴포넌트 (R/G/B/A/H/S/V)
	 * @param InValue 설정할 값
	 * @return 수정된 색상
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FLinearColor SetColorComponent(FLinearColor InColor, EJWCU_ColorComponent InComponent, float InValue);

	/**
	 * [ Blueprint Function Library ] \n Lighten Color \n 색상을 밝게 만든다.
	 * @param InColor 원본 색상
	 * @param InFactor 밝기 증가 비율 (0.0 = 변화 없음, 1.0 = 완전 흰색)
	 * @return 밝아진 색상
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FLinearColor LightenColor(FLinearColor InColor, float InFactor);

	/**
	 * [ Blueprint Function Library ] \n Darken Color \n 색상을 어둡게 만든다.
	 * @param InColor 원본 색상
	 * @param InFactor 어두움 증가 비율 (0.0 = 변화 없음, 1.0 = 완전 검정)
	 * @return 어두워진 색상
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FLinearColor DarkenColor(FLinearColor InColor, float InFactor);

	/**
	 * [ Blueprint Function Library ] \n Hex To Linear Color \n Hex 문자열을 FLinearColor로 변환한다.
	 * @param InHex Hex 문자열 (예: "#FF5500" 또는 "FF5500")
	 * @param OutColor 변환된 색상
	 * @return 변환 성공 여부
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static bool HexToLinearColor(const FString& InHex, FLinearColor& OutColor);

	/**
	 * [ Blueprint Function Library ] \n Linear Color To Hex \n FLinearColor를 Hex 문자열로 변환한다.
	 * @param InColor 변환할 색상
	 * @param bIncludeAlpha 알파 채널 포함 여부
	 * @return Hex 문자열 (예: "#FF5500" 또는 "#FF550080")
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FString LinearColorToHex(FLinearColor InColor, bool bIncludeAlpha = false);

	/**
	 * [ Blueprint Function Library ] \n Desaturate Color \n 색상의 채도를 감소시킨다.
	 * @param InColor 원본 색상
	 * @param InDesaturation 채도 감소 비율 (0.0 = 원본, 1.0 = 완전 회색)
	 * @return 채도가 감소된 색상
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FLinearColor DesaturateColor(FLinearColor InColor, float InDesaturation);
};
