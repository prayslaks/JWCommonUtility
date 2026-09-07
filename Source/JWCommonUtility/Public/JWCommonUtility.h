// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"

class FJWCommonUtilityModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

// 디버깅 로그 카테고리 선언
JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(JWCULog, Log, All);
JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(JWCULogWall, Log, All);

/**
 * 현재 함수와 라인 정보를 문자열로 변환하는 매크로.
 */
#define JWCU_CALL_INFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

/**
 * UE_LOG에 호출 위치를 포함하도록 포장하는 매크로.
 * @param cat 로그 카테고리
 * @param ver 로그 상세
 * @param fmt 언리얼 TEXT 문자열 포매팅, 가변 인자로 입력 가능
 */
#define JWCU_PRINT_LOG(cat, ver, fmt, ...) \
	do \
	{ \
		const FString __JWCULogMessage__ = FString::Printf(fmt, ##__VA_ARGS__); \
		const FString __JWCUFullMessage__ = FString::Printf(TEXT("%s : %s"), *JWCU_CALL_INFO, *__JWCULogMessage__); \
		UE_LOG(cat, ver, TEXT("%s"), *__JWCUFullMessage__); \
	} while (0)

#pragma region ScreenDebugger

/**
 * 플러그인 내부 온스크린 디버그 메시지 활성화 콘솔 변수.
 * 콘솔에서 JWCU.DebugScreen 1/0 으로 제어.
 */
extern JWCOMMONUTILITY_API TAutoConsoleVariable<bool> CVarJWCU_DebugScreen;

/**
 * 콘솔 변수로 제어 가능한 온스크린 디버그 메시지 매크로.
 * @param Key 메시지 키 (-1이면 매번 새 줄)
 * @param Duration 표시 시간 (초)
 * @param Color FColor 색상
 * @param fmt TEXT() 포맷 문자열 + 가변 인자
 */
#define JWCU_SCREEN_DEBUG(Key, Duration, Color, fmt, ...) \
	do \
	{ \
		if (CVarJWCU_DebugScreen.GetValueOnGameThread() && GEngine) \
		{ \
			const FString __JWCUScreenMsg__ = FString::Printf(fmt, ##__VA_ARGS__); \
			GEngine->AddOnScreenDebugMessage(Key, Duration, Color, __JWCUScreenMsg__); \
		} \
	} while (0)

#pragma endregion ScreenDebugger

#pragma region ScopeWallLogger

// 범위 로그 벽의 총 길이를 설정
#define JWCU_LOG_WALL_WIDTH 60

struct JWCOMMONUTILITY_API FJWCUScopeWallLogger
{
	FString FunctionName;
	explicit FJWCUScopeWallLogger(const char* InFuncName) : FunctionName(InFuncName)
	{
		const int32 TextLen = FunctionName.Len() + 6;
		const int32 SideLen = FMath::Max(0, (JWCU_LOG_WALL_WIDTH - TextLen) / 2);
		const FString Padding = FString::ChrN(SideLen, '=');
		UE_LOG(JWCULogWall, Warning, TEXT("%s < %s > %s"), *Padding, *FunctionName, *Padding);
		UE_LOG(JWCULogWall, Warning, TEXT(""));
	}
	~FJWCUScopeWallLogger()
	{
		// 함수가 끝날 때(Scope를 벗어날 때) 자동 호출
		UE_LOG(JWCULogWall, Warning, TEXT(""));
		const FString Padding = FString::ChrN(JWCU_LOG_WALL_WIDTH, '=');
		UE_LOG(JWCULogWall, Warning, TEXT("%s"), *Padding);
	}
};

// 매크로 정의
#define JWCU_SCOPE_WALL() FJWCUScopeWallLogger JWCUWallLogger(__FUNCTION__);

#pragma endregion ScopeWallLogger

#pragma region DeveloperWarning

/**
 * 넘겨받은 변수의 이름을 문자열로 반환하는 매크로.
 * @param var 이름을 문자열로 반환받고 싶은 변수
 */
#define JWCU_VAR_NAME_TEXT(var) TEXT(#var)

#if UE_BUILD_SHIPPING

/**
 * 온스크린 경고 출력 내부 매크로. Shipping 빌드에서는 아무것도 하지 않는다.
 */
#define JWCU_INTERNAL_SCREEN_WARN(Duration, Color, Message) do { } while (0)

#else

/**
 * 온스크린 경고 출력 내부 매크로. GEngine 이 없는 실행 경로(커맨드릿, 초기화 이전 등)에서는 건너뛴다.
 */
#define JWCU_INTERNAL_SCREEN_WARN(Duration, Color, Message) \
	do \
	{ \
		if (GEngine) \
		{ \
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message); \
		} \
	} while (0)

#endif

/**
 * 개발자 경고를 로그와 온스크린 메시지에 함께 남기는 내부 매크로.
 */
#define JWCU_INTERNAL_DEVELOPER_WARN(Message) \
	do \
	{ \
		const FString __JWCUWarnMessage__ = (Message); \
		UE_LOG(JWCULog, Warning, TEXT("%s"), *__JWCUWarnMessage__); \
		JWCU_INTERNAL_SCREEN_WARN(60.0f, FColor::Red, __JWCUWarnMessage__); \
	} while (0)

/**
 * GEngine->AddOnScreenDebugMessage 를 호출 위치와 함께 출력하는 매크로.
 * @param tme 메시지 출력 유지 시간
 * @param clr 메시지 출력 시 색상
 * @param fmt 언리얼 TEXT 문자열 포매팅, 가변 인자로 입력 가능
 */
#define JWCU_SCREEN_MESSAGE(tme, clr, fmt, ...) \
	do \
	{ \
		const FString __JWCULogMessage__ = FString::Printf(fmt, ##__VA_ARGS__); \
		const FString __JWCUFullMessage__ = FString::Printf(TEXT("[%s] %s"), *JWCU_CALL_INFO, *__JWCULogMessage__); \
		JWCU_INTERNAL_SCREEN_WARN(tme, clr, __JWCUFullMessage__); \
	} while (0)

/**
 * 넘겨받은 포인터 변수가 널포인터인지 확인하고, 널포인터라면 로그와 온스크린 메시지로 경고하는 매크로.
 * @param var 널포인터 확인이 필요한 변수
 */
#define JWCU_CHECK_NULLPTR(var) \
	JWCU_INTERNAL_DEVELOPER_WARN(FString::Printf(TEXT("%s : Warning! %s is nullptr!"), *JWCU_CALL_INFO, JWCU_VAR_NAME_TEXT(var)))

/**
 * 구현이 필요한 함수에 표시해 두는 매크로. 호출되면 로그와 온스크린 메시지로 경고한다.
 */
#define JWCU_WARN_NO_IMPLEMENT() \
	JWCU_INTERNAL_DEVELOPER_WARN(FString::Printf(TEXT("%s : Warning! Have to implement this function!"), *JWCU_CALL_INFO))

/**
 * 구현이 필요한 함수가 문자열을 반환해야 할 때 쓰는 자리 표시 문자열 매크로.
 */
#define JWCU_WARN_NO_IMPLEMENT_STRING() TEXT("Have to implement this function!")

/**
 * 호출되어서는 안 되는 함수에 표시해 두는 매크로. 호출되면 로그와 온스크린 메시지로 경고한다.
 */
#define JWCU_WARN_SHOULD_NO_CALL() \
	JWCU_INTERNAL_DEVELOPER_WARN(FString::Printf(TEXT("%s : Warning! Should not call this function!"), *JWCU_CALL_INFO))

#pragma endregion DeveloperWarning
