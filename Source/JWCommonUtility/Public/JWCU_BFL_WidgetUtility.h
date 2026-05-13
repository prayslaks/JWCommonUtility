// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_WidgetUtility.generated.h"

class UWidget;
class UPanelWidget;

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_WidgetUtility, Log, All);

/**
 * UMG 위젯 관련 유틸리티 블루프린트 함수 라이브러리.
 * 가시성, 투명도, 활성화 제어 등을 간편하게 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_WidgetUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Set Widget Visible Or Collapsed \n 위젯의 가시성을 Visible 또는 Collapsed로 설정한다.
	 * @param InWidget 대상 위젯
	 * @param bVisible true=Visible, false=Collapsed
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static void SetWidgetVisibleOrCollapsed(UWidget* InWidget, bool bVisible);

	/**
	 * [ Blueprint Function Library ] \n Set Widget Visible Or Hidden \n 위젯의 가시성을 Visible 또는 Hidden으로 설정한다. (레이아웃 공간 유지)
	 * @param InWidget 대상 위젯
	 * @param bVisible true=Visible, false=Hidden
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static void SetWidgetVisibleOrHidden(UWidget* InWidget, bool bVisible);

	/**
	 * [ Blueprint Function Library ] \n Is Widget Visible \n 위젯이 현재 보이는 상태인지 확인한다.
	 * @param InWidget 확인할 위젯
	 * @return Visible이면 true, 그 외(Hidden/Collapsed 등)이면 false
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWCU Blueprint Function Library")
	static bool IsWidgetVisible(UWidget* InWidget);

	/**
	 * [ Blueprint Function Library ] \n Set Widget Opacity With Hit Test \n 위젯 투명도를 설정하고, 완전 투명 시 히트 테스트를 비활성화할 수 있다.
	 * @param InWidget 대상 위젯
	 * @param InOpacity 투명도 (0.0 = 완전 투명, 1.0 = 완전 불투명)
	 * @param bDisableHitTestWhenInvisible 투명도가 0일 때 히트 테스트 비활성화 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static void SetWidgetOpacityWithHitTest(UWidget* InWidget, float InOpacity, bool bDisableHitTestWhenInvisible = true);

	/**
	 * [ Blueprint Function Library ] \n Set Widget Enabled Recursive \n 위젯과 자식 위젯의 활성화 상태를 재귀적으로 설정한다.
	 * @param InWidget 대상 위젯
	 * @param bEnabled 활성화 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static void SetWidgetEnabledRecursive(UWidget* InWidget, bool bEnabled);
};
