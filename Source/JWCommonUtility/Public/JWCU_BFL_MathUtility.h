// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_MathUtility.generated.h"

class UCurveFloat;

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_MathUtility, Log, All);

/**
 * 수학 관련 유틸리티 블루프린트 함수 라이브러리.
 * 확률, 커브 매핑, 거리 계산, 회전 등을 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_MathUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Roll Percent Chance \n 0~100 사이의 확률을 판정하여 true/false를 반환한다.
	 * @param InPercent 확률 값 (0.0 ~ 100.0)
	 * @return 확률 판정 결과
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static bool RollPercentChance(float InPercent);

	/**
	 * [ Blueprint Function Library ] \n Curve Map Range Clamped \n 입력 범위를 출력 범위로 매핑하되, 선택적으로 UCurveFloat를 통한 비선형 보간을 지원한다.
	 * @param InValue 입력 값
	 * @param InRangeMin 입력 범위 최소값
	 * @param InRangeMax 입력 범위 최대값
	 * @param OutRangeMin 출력 범위 최소값
	 * @param OutRangeMax 출력 범위 최대값
	 * @param InOptionalCurve 비선형 매핑을 위한 커브 에셋 (nullptr이면 선형 매핑)
	 * @return 매핑된 출력 값
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static float CurveMapRangeClamped(float InValue, float InRangeMin, float InRangeMax, float OutRangeMin, float OutRangeMax, UCurveFloat* InOptionalCurve);

	/**
	 * [ Blueprint Function Library ] \n Get Horizontal Distance \n 두 벡터 사이의 수평 거리를 계산한다 (Z축 무시).
	 * @param InA 첫 번째 위치
	 * @param InB 두 번째 위치
	 * @return 수평 거리 (XY 평면)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static float GetHorizontalDistance(FVector InA, FVector InB);

	/**
	 * [ Blueprint Function Library ] \n Get Look At Rotation \n 원점에서 타겟을 바라보기 위한 회전값을 계산한다.
	 * @param InOrigin 시작 위치
	 * @param InTarget 바라볼 대상 위치
	 * @param InAxis 회전 축 제어 (Full/YawOnly/PitchOnly)
	 * @return 계산된 회전값
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FRotator GetLookAtRotation(FVector InOrigin, FVector InTarget, EJWCU_LookAtAxis InAxis);

	/**
	 * [ Blueprint Function Library ] \n Quat Multiply Rotators \n 두 Rotator를 Quaternion으로 변환해 LeftQuat * RightQuat 순서로 곱한 뒤 Rotator로 반환한다.
	 * @param InLeft 왼쪽 Quaternion 피연산자
	 * @param InRight 오른쪽 Quaternion 피연산자
	 * @return (InLeft.Quaternion() * InRight.Quaternion()).Rotator()
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FRotator QuatMultiplyRotators(FRotator InLeft, FRotator InRight);

	/**
	 * [ Blueprint Function Library ] \n Quat Multiply Rotators Forward Vector \n 두 Rotator를 Quaternion으로 변환해 LeftQuat * RightQuat 순서로 곱한 뒤 Forward Vector를 반환한다.
	 * @param InLeft 왼쪽 Quaternion 피연산자
	 * @param InRight 오른쪽 Quaternion 피연산자
	 * @return (InLeft.Quaternion() * InRight.Quaternion()).GetForwardVector()
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static FVector QuatMultiplyRotatorsForwardVector(FRotator InLeft, FRotator InRight);

	/**
	 * [ Blueprint Function Library ] \n Normalize Angle 180 \n 각도를 -180 ~ 180 범위로 정규화한다.
	 * @param InAngle 정규화할 각도
	 * @return 정규화된 각도 (-180 ~ 180)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static float NormalizeAngle180(float InAngle);

	/**
	 * [ Blueprint Function Library ] \n Weighted Random Index \n 가중치 배열을 기반으로 랜덤 인덱스를 선택한다.
	 * @param InWeights 각 항목의 가중치 배열
	 * @return 선택된 인덱스 (-1이면 배열이 비어있거나 모든 가중치가 0)
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static int32 WeightedRandomIndex(const TArray<float>& InWeights);
};
