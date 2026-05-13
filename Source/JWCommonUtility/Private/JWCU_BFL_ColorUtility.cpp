// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWCU_BFL_ColorUtility.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_ColorUtility);

FLinearColor UJWCU_BFL_ColorUtility::BlendColors(FLinearColor InColorA, FLinearColor InColorB, float InAlpha, EJWCU_ColorBlendMode InBlendMode)
{
	const float ClampedAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);

	switch (InBlendMode)
	{
	case EJWCU_ColorBlendMode::Lerp:
		return FLinearColor::LerpUsingHSV(InColorA, InColorB, ClampedAlpha);

	case EJWCU_ColorBlendMode::Additive:
		{
			FLinearColor Result;
			Result.R = FMath::Clamp(InColorA.R + InColorB.R * ClampedAlpha, 0.0f, 1.0f);
			Result.G = FMath::Clamp(InColorA.G + InColorB.G * ClampedAlpha, 0.0f, 1.0f);
			Result.B = FMath::Clamp(InColorA.B + InColorB.B * ClampedAlpha, 0.0f, 1.0f);
			Result.A = FMath::Clamp(InColorA.A + InColorB.A * ClampedAlpha, 0.0f, 1.0f);
			return Result;
		}

	case EJWCU_ColorBlendMode::Multiply:
		{
			FLinearColor Blended;
			Blended.R = InColorA.R * InColorB.R;
			Blended.G = InColorA.G * InColorB.G;
			Blended.B = InColorA.B * InColorB.B;
			Blended.A = InColorA.A * InColorB.A;
			return FMath::Lerp(InColorA, Blended, ClampedAlpha);
		}

	case EJWCU_ColorBlendMode::Screen:
		{
			FLinearColor Blended;
			Blended.R = 1.0f - (1.0f - InColorA.R) * (1.0f - InColorB.R);
			Blended.G = 1.0f - (1.0f - InColorA.G) * (1.0f - InColorB.G);
			Blended.B = 1.0f - (1.0f - InColorA.B) * (1.0f - InColorB.B);
			Blended.A = 1.0f - (1.0f - InColorA.A) * (1.0f - InColorB.A);
			return FMath::Lerp(InColorA, Blended, ClampedAlpha);
		}
	}

	return InColorA;
}

float UJWCU_BFL_ColorUtility::GetColorComponent(FLinearColor InColor, EJWCU_ColorComponent InComponent)
{
	switch (InComponent)
	{
	case EJWCU_ColorComponent::Red:        return InColor.R;
	case EJWCU_ColorComponent::Green:      return InColor.G;
	case EJWCU_ColorComponent::Blue:       return InColor.B;
	case EJWCU_ColorComponent::Alpha:      return InColor.A;
	case EJWCU_ColorComponent::Hue:
		{
			FLinearColor HSV = InColor.LinearRGBToHSV();
			return HSV.R; // H는 HSV의 R 채널에 저장됨
		}
	case EJWCU_ColorComponent::Saturation:
		{
			FLinearColor HSV = InColor.LinearRGBToHSV();
			return HSV.G; // S는 HSV의 G 채널에 저장됨
		}
	case EJWCU_ColorComponent::Value:
		{
			FLinearColor HSV = InColor.LinearRGBToHSV();
			return HSV.B; // V는 HSV의 B 채널에 저장됨
		}
	}

	return 0.0f;
}

FLinearColor UJWCU_BFL_ColorUtility::SetColorComponent(FLinearColor InColor, EJWCU_ColorComponent InComponent, float InValue)
{
	FLinearColor Result = InColor;

	switch (InComponent)
	{
	case EJWCU_ColorComponent::Red:
		Result.R = InValue;
		break;
	case EJWCU_ColorComponent::Green:
		Result.G = InValue;
		break;
	case EJWCU_ColorComponent::Blue:
		Result.B = InValue;
		break;
	case EJWCU_ColorComponent::Alpha:
		Result.A = InValue;
		break;
	case EJWCU_ColorComponent::Hue:
		{
			FLinearColor HSV = InColor.LinearRGBToHSV();
			HSV.R = InValue;
			Result = HSV.HSVToLinearRGB();
			Result.A = InColor.A;
		}
		break;
	case EJWCU_ColorComponent::Saturation:
		{
			FLinearColor HSV = InColor.LinearRGBToHSV();
			HSV.G = InValue;
			Result = HSV.HSVToLinearRGB();
			Result.A = InColor.A;
		}
		break;
	case EJWCU_ColorComponent::Value:
		{
			FLinearColor HSV = InColor.LinearRGBToHSV();
			HSV.B = InValue;
			Result = HSV.HSVToLinearRGB();
			Result.A = InColor.A;
		}
		break;
	}

	return Result;
}

FLinearColor UJWCU_BFL_ColorUtility::LightenColor(FLinearColor InColor, float InFactor)
{
	const float ClampedFactor = FMath::Clamp(InFactor, 0.0f, 1.0f);
	FLinearColor Result;
	Result.R = FMath::Lerp(InColor.R, 1.0f, ClampedFactor);
	Result.G = FMath::Lerp(InColor.G, 1.0f, ClampedFactor);
	Result.B = FMath::Lerp(InColor.B, 1.0f, ClampedFactor);
	Result.A = InColor.A;
	return Result;
}

FLinearColor UJWCU_BFL_ColorUtility::DarkenColor(FLinearColor InColor, float InFactor)
{
	const float ClampedFactor = FMath::Clamp(InFactor, 0.0f, 1.0f);
	FLinearColor Result;
	Result.R = FMath::Lerp(InColor.R, 0.0f, ClampedFactor);
	Result.G = FMath::Lerp(InColor.G, 0.0f, ClampedFactor);
	Result.B = FMath::Lerp(InColor.B, 0.0f, ClampedFactor);
	Result.A = InColor.A;
	return Result;
}

bool UJWCU_BFL_ColorUtility::HexToLinearColor(const FString& InHex, FLinearColor& OutColor)
{
	FString CleanHex = InHex;

	// '#' 접두사 제거
	if (CleanHex.StartsWith(TEXT("#")))
	{
		CleanHex = CleanHex.Mid(1);
	}

	// FColor::FromHex는 RRGGBB 또는 RRGGBBAA 형식을 지원
	const FColor ParsedColor = FColor::FromHex(CleanHex);

	// 유효성 검증: 길이가 6(RGB) 또는 8(RGBA)이어야 함
	if (CleanHex.Len() != 6 && CleanHex.Len() != 8)
	{
		OutColor = FLinearColor::Black;
		return false;
	}

	OutColor = FLinearColor(ParsedColor);
	return true;
}

FString UJWCU_BFL_ColorUtility::LinearColorToHex(FLinearColor InColor, bool bIncludeAlpha)
{
	const FColor SRGBColor = InColor.ToFColor(true);

	if (bIncludeAlpha)
	{
		return FString::Printf(TEXT("#%02X%02X%02X%02X"), SRGBColor.R, SRGBColor.G, SRGBColor.B, SRGBColor.A);
	}

	return FString::Printf(TEXT("#%02X%02X%02X"), SRGBColor.R, SRGBColor.G, SRGBColor.B);
}

FLinearColor UJWCU_BFL_ColorUtility::DesaturateColor(FLinearColor InColor, float InDesaturation)
{
	const float ClampedDesat = FMath::Clamp(InDesaturation, 0.0f, 1.0f);

	// 휘도(Luminance) 기반 회색 계산
	const float Luminance = InColor.GetLuminance();
	const FLinearColor GreyColor(Luminance, Luminance, Luminance, InColor.A);

	return FMath::Lerp(InColor, GreyColor, ClampedDesat);
}
