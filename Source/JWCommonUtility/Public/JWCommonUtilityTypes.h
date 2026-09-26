// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.generated.h"


// ==================== DateTime 열거형 ====================


/**
 * 날짜 출력 포맷 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_DateFormat : uint8
{
	YYYY_MM_DD			UMETA(DisplayName = "YYYY-MM-DD"),				// 2026-04-09
	DD_MM_YYYY			UMETA(DisplayName = "DD/MM/YYYY"),				// 09/04/2026
	MM_DD_YYYY			UMETA(DisplayName = "MM/DD/YYYY"),				// 04/09/2026
	YYYY_MM_DD_Dot		UMETA(DisplayName = "YYYY.MM.DD"),				// 2026.04.09
	DD_Mon_YYYY			UMETA(DisplayName = "DD Mon YYYY"),				// 09 Apr 2026
	Mon_DD_YYYY			UMETA(DisplayName = "Mon DD, YYYY"),			// Apr 09, 2026
	Full_Weekday		UMETA(DisplayName = "Weekday, Mon DD YYYY"),	// Wednesday, Apr 09 2026
};

/**
 * 시간 출력 포맷 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_TimeFormat : uint8
{
	HH_MM_24			UMETA(DisplayName = "HH:MM (24h)"),			// 14:30
	HH_MM_SS_24			UMETA(DisplayName = "HH:MM:SS (24h)"),		// 14:30:05
	HH_MM_12			UMETA(DisplayName = "hh:MM AM/PM"),			// 2:30 PM
	HH_MM_SS_12			UMETA(DisplayName = "hh:MM:SS AM/PM"),		// 2:30:05 PM
};

/**
 * 날짜+시간 결합 출력 포맷 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_DateTimeFormat : uint8
{
	YYYY_MM_DD_HH_MM		UMETA(DisplayName = "YYYY-MM-DD HH:MM"),
	YYYY_MM_DD_HH_MM_SS	UMETA(DisplayName = "YYYY-MM-DD HH:MM:SS"),
	MM_DD_YYYY_12h			UMETA(DisplayName = "MM/DD/YYYY hh:MM AM/PM"),
	Full_Weekday_12h		UMETA(DisplayName = "Weekday, Mon DD YYYY hh:MM AM/PM"),
	ISO8601					UMETA(DisplayName = "ISO 8601"),			// 2026-04-09T14:30:05Z
};

/**
 * 상대 시간 표시 시 단위 세분화 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_RelativeTimeGranularity : uint8
{
	Automatic			UMETA(DisplayName = "Automatic"),				// 자동으로 최적 단위 선택
	ForceSeconds		UMETA(DisplayName = "Force Seconds"),
	ForceMinutes		UMETA(DisplayName = "Force Minutes"),
	ForceHours			UMETA(DisplayName = "Force Hours"),
	ForceDays			UMETA(DisplayName = "Force Days"),
};


// ==================== String 열거형 ====================


/**
 * 숫자 단위 요약 임계값 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_NumberAbbreviationThreshold : uint8
{
	Thousands			UMETA(DisplayName = "K (1,000+)"),				// 1,000부터 K로 변환
	TenThousands		UMETA(DisplayName = "K (10,000+)"),				// 10,000부터 K로 변환
	Millions			UMETA(DisplayName = "M (1,000,000+)"),			// 1,000,000부터 M으로 변환
	Adaptive			UMETA(DisplayName = "Adaptive (auto)"),			// 자동: K(1K~), M(1M~), B(1B~)
};


// ==================== Actor / Component 열거형 ====================


/**
 * 자식 컴포넌트 일괄 제어 액션 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_ChildComponentAction : uint8
{
	SetVisibility_Visible	UMETA(DisplayName = "Set Visible"),
	SetVisibility_Hidden	UMETA(DisplayName = "Set Hidden"),
	Activate				UMETA(DisplayName = "Activate"),
	Deactivate				UMETA(DisplayName = "Deactivate"),
};

/**
 * 액터 필터링 결과 열거형. ExpandEnumAsExecs용.
 */
UENUM(BlueprintType)
enum class EJWCU_ActorFilterResult : uint8
{
	Found				UMETA(DisplayName = "Found"),
	NotFound			UMETA(DisplayName = "Not Found"),
};


// ==================== Math 열거형 ====================


/**
 * LookAt 회전 축 제어 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_LookAtAxis : uint8
{
	FullRotation		UMETA(DisplayName = "Full Rotation (Pitch+Yaw)"),
	YawOnly				UMETA(DisplayName = "Yaw Only (Ignore Pitch)"),
	PitchOnly			UMETA(DisplayName = "Pitch Only (Ignore Yaw)"),
};


// ==================== Color 열거형 ====================


/**
 * 색상 블렌딩 모드 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_ColorBlendMode : uint8
{
	Lerp				UMETA(DisplayName = "Linear Interpolation"),
	Additive			UMETA(DisplayName = "Additive"),
	Multiply			UMETA(DisplayName = "Multiply"),
	Screen				UMETA(DisplayName = "Screen"),
};

/**
 * 색상 컴포넌트 선택 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_ColorComponent : uint8
{
	Red					UMETA(DisplayName = "Red"),
	Green				UMETA(DisplayName = "Green"),
	Blue				UMETA(DisplayName = "Blue"),
	Alpha				UMETA(DisplayName = "Alpha"),
	Hue					UMETA(DisplayName = "Hue"),
	Saturation			UMETA(DisplayName = "Saturation"),
	Value				UMETA(DisplayName = "Value / Brightness"),
};


// ==================== Debug 열거형 ====================


/**
 * 디버그 도형 타입 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_DebugShape : uint8
{
	Sphere				UMETA(DisplayName = "Sphere"),
	Box					UMETA(DisplayName = "Box"),
	Line				UMETA(DisplayName = "Line"),
	Arrow				UMETA(DisplayName = "Arrow"),
	Point				UMETA(DisplayName = "Point"),
	Capsule				UMETA(DisplayName = "Capsule"),
};


// ==================== Collection 열거형 ====================


/**
 * 정렬 순서 열거형.
 */
UENUM(BlueprintType)
enum class EJWCU_SortOrder : uint8
{
	Ascending			UMETA(DisplayName = "Ascending"),
	Descending			UMETA(DisplayName = "Descending"),
};


// ==================== 구조체 ====================


/**
 * 상대 시간 결과 구조체.
 */
USTRUCT(BlueprintType)
struct JWCOMMONUTILITY_API FJWCU_RelativeTimeResult
{
	GENERATED_BODY()

	/**
	 * 표시용 텍스트. 예: "3분 전", "2시간 후"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWCommonUtility")
	FText DisplayText;

	/**
	 * 기준 시간과의 차이 (초 단위). 부호 포함.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWCommonUtility")
	float TotalSeconds;

	/**
	 * 입력 시간이 과거인지 여부.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWCommonUtility")
	bool bIsPast;

	/**
	 * 기본 생성자.
	 */
	FJWCU_RelativeTimeResult()
	{
		DisplayText = FText::GetEmpty();
		TotalSeconds = 0.0f;
		bIsPast = false;
	}
};
