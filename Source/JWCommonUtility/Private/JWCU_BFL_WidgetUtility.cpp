// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_BFL_WidgetUtility.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_WidgetUtility);

void UJWCU_BFL_WidgetUtility::SetWidgetVisibleOrCollapsed(UWidget* InWidget, bool bVisible)
{
	if (!InWidget)
	{
		return;
	}

	InWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UJWCU_BFL_WidgetUtility::SetWidgetVisibleOrHidden(UWidget* InWidget, bool bVisible)
{
	if (!InWidget)
	{
		return;
	}

	InWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

bool UJWCU_BFL_WidgetUtility::IsWidgetVisible(UWidget* InWidget)
{
	if (!InWidget)
	{
		return false;
	}

	return InWidget->GetVisibility() == ESlateVisibility::Visible
		|| InWidget->GetVisibility() == ESlateVisibility::HitTestInvisible
		|| InWidget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible;
}

void UJWCU_BFL_WidgetUtility::SetWidgetOpacityWithHitTest(UWidget* InWidget, float InOpacity, bool bDisableHitTestWhenInvisible)
{
	if (!InWidget)
	{
		return;
	}

	const float ClampedOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
	InWidget->SetRenderOpacity(ClampedOpacity);

	if (bDisableHitTestWhenInvisible && ClampedOpacity <= 0.0f)
	{
		InWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (bDisableHitTestWhenInvisible && ClampedOpacity > 0.0f)
	{
		// 투명도가 0이 아니면 가시 상태로 복원
		if (InWidget->GetVisibility() == ESlateVisibility::HitTestInvisible)
		{
			InWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UJWCU_BFL_WidgetUtility::SetWidgetEnabledRecursive(UWidget* InWidget, bool bEnabled)
{
	if (!InWidget)
	{
		return;
	}

	InWidget->SetIsEnabled(bEnabled);

	// PanelWidget인 경우 자식 위젯에도 재귀 적용
	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(InWidget))
	{
		const int32 ChildCount = PanelWidget->GetChildrenCount();
		for (int32 i = 0; i < ChildCount; ++i)
		{
			UWidget* ChildWidget = PanelWidget->GetChildAt(i);
			if (ChildWidget)
			{
				SetWidgetEnabledRecursive(ChildWidget, bEnabled);
			}
		}
	}
}
