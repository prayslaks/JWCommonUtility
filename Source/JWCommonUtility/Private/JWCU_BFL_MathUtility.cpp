// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWCU_BFL_MathUtility.h"
#include "Curves/CurveFloat.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_MathUtility);

bool UJWCU_BFL_MathUtility::RollPercentChance(float InPercent)
{
	if (InPercent <= 0.0f)
	{
		return false;
	}
	if (InPercent >= 100.0f)
	{
		return true;
	}

	return FMath::FRandRange(0.0f, 100.0f) < InPercent;
}

float UJWCU_BFL_MathUtility::CurveMapRangeClamped(float InValue, float InRangeMin, float InRangeMax, float OutRangeMin, float OutRangeMax, UCurveFloat* InOptionalCurve)
{
	// 입력값을 0~1 범위로 정규화 (클램프)
	float Alpha = 0.0f;
	if (!FMath::IsNearlyEqual(InRangeMin, InRangeMax))
	{
		Alpha = FMath::Clamp((InValue - InRangeMin) / (InRangeMax - InRangeMin), 0.0f, 1.0f);
	}

	// 커브가 있으면 커브를 통해 비선형 보간
	if (InOptionalCurve)
	{
		Alpha = InOptionalCurve->GetFloatValue(Alpha);
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	}

	// 출력 범위로 매핑
	return FMath::Lerp(OutRangeMin, OutRangeMax, Alpha);
}

float UJWCU_BFL_MathUtility::GetHorizontalDistance(FVector InA, FVector InB)
{
	const float DX = InA.X - InB.X;
	const float DY = InA.Y - InB.Y;
	return FMath::Sqrt(DX * DX + DY * DY);
}

FRotator UJWCU_BFL_MathUtility::GetLookAtRotation(FVector InOrigin, FVector InTarget, EJWCU_LookAtAxis InAxis)
{
	const FVector Direction = InTarget - InOrigin;
	FRotator LookAtRotation = Direction.Rotation();

	switch (InAxis)
	{
	case EJWCU_LookAtAxis::FullRotation:
		// Pitch + Yaw 모두 사용
		break;

	case EJWCU_LookAtAxis::YawOnly:
		LookAtRotation.Pitch = 0.0f;
		LookAtRotation.Roll = 0.0f;
		break;

	case EJWCU_LookAtAxis::PitchOnly:
		LookAtRotation.Yaw = 0.0f;
		LookAtRotation.Roll = 0.0f;
		break;
	}

	return LookAtRotation;
}

float UJWCU_BFL_MathUtility::NormalizeAngle180(float InAngle)
{
	// FRotator::NormalizeAxis와 동일한 결과를 반환
	float Angle = FMath::Fmod(InAngle, 360.0f);

	if (Angle > 180.0f)
	{
		Angle -= 360.0f;
	}
	else if (Angle < -180.0f)
	{
		Angle += 360.0f;
	}

	return Angle;
}

int32 UJWCU_BFL_MathUtility::WeightedRandomIndex(const TArray<float>& InWeights)
{
	if (InWeights.Num() == 0)
	{
		return -1;
	}

	// 전체 가중치 합산
	float TotalWeight = 0.0f;
	for (const float Weight : InWeights)
	{
		TotalWeight += FMath::Max(0.0f, Weight);
	}

	if (TotalWeight <= 0.0f)
	{
		return -1;
	}

	// 랜덤 값으로 인덱스 선택
	float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
	float Accumulator = 0.0f;

	for (int32 i = 0; i < InWeights.Num(); ++i)
	{
		Accumulator += FMath::Max(0.0f, InWeights[i]);
		if (RandomValue <= Accumulator)
		{
			return i;
		}
	}

	// 부동소수점 오차로 인한 폴백
	return InWeights.Num() - 1;
}
