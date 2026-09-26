// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_BFL_StringUtility.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_StringUtility);

FString UJWCU_BFL_StringUtility::AbbreviateNumber(float InValue, EJWCU_NumberAbbreviationThreshold InThreshold, int32 InDecimalPlaces)
{
	const float AbsValue = FMath::Abs(InValue);
	const FString SignPrefix = InValue < 0.0f ? TEXT("-") : TEXT("");

	// 단위 임계값과 접미사 정의
	struct FAbbreviationUnit
	{
		float Threshold;
		float Divisor;
		const TCHAR* Suffix;
	};

	// Adaptive 모드: 자동으로 최적 단위 선택
	if (InThreshold == EJWCU_NumberAbbreviationThreshold::Adaptive)
	{
		static const FAbbreviationUnit AdaptiveUnits[] =
		{
			{ 1000000000.0f, 1000000000.0f, TEXT("B") },
			{ 1000000.0f,    1000000.0f,    TEXT("M") },
			{ 1000.0f,       1000.0f,       TEXT("K") },
		};

		for (const FAbbreviationUnit& Unit : AdaptiveUnits)
		{
			if (AbsValue >= Unit.Threshold)
			{
				const float Divided = AbsValue / Unit.Divisor;
				return FString::Printf(TEXT("%s%.*f%s"), *SignPrefix, InDecimalPlaces, Divided, Unit.Suffix);
			}
		}

		// 1000 미만인 경우 그대로 반환
		return FString::Printf(TEXT("%s%.*f"), *SignPrefix, InDecimalPlaces, AbsValue);
	}

	// 특정 임계값 모드
	float ThresholdValue = 0.0f;
	switch (InThreshold)
	{
	case EJWCU_NumberAbbreviationThreshold::Thousands:
		ThresholdValue = 1000.0f;
		break;
	case EJWCU_NumberAbbreviationThreshold::TenThousands:
		ThresholdValue = 10000.0f;
		break;
	case EJWCU_NumberAbbreviationThreshold::Millions:
		ThresholdValue = 1000000.0f;
		break;
	default:
		ThresholdValue = 1000.0f;
		break;
	}

	// 임계값 미만이면 그대로 반환
	if (AbsValue < ThresholdValue)
	{
		return FString::Printf(TEXT("%s%.*f"), *SignPrefix, InDecimalPlaces, AbsValue);
	}

	// 임계값 이상이면 최적 단위로 변환
	static const FAbbreviationUnit Units[] =
	{
		{ 1000000000.0f, 1000000000.0f, TEXT("B") },
		{ 1000000.0f,    1000000.0f,    TEXT("M") },
		{ 1000.0f,       1000.0f,       TEXT("K") },
	};

	for (const FAbbreviationUnit& Unit : Units)
	{
		if (AbsValue >= Unit.Threshold)
		{
			const float Divided = AbsValue / Unit.Divisor;
			return FString::Printf(TEXT("%s%.*f%s"), *SignPrefix, InDecimalPlaces, Divided, Unit.Suffix);
		}
	}

	return FString::Printf(TEXT("%s%.*f"), *SignPrefix, InDecimalPlaces, AbsValue);
}

FString UJWCU_BFL_StringUtility::FormatIntWithCommas(int32 InValue)
{
	// 음수 처리
	const bool bNegative = InValue < 0;
	// int32 최소값 오버플로 방지
	int64 AbsValue = bNegative ? -static_cast<int64>(InValue) : static_cast<int64>(InValue);

	FString Result = FString::FromInt(AbsValue);

	// 오른쪽에서 3자리마다 콤마 삽입
	int32 InsertPosition = Result.Len() - 3;
	while (InsertPosition > 0)
	{
		Result.InsertAt(InsertPosition, TEXT(","));
		InsertPosition -= 3;
	}

	if (bNegative)
	{
		Result = TEXT("-") + Result;
	}

	return Result;
}

FString UJWCU_BFL_StringUtility::FormatFloatWithCommas(float InValue, int32 InDecimalPlaces)
{
	// 음수 처리
	const bool bNegative = InValue < 0.0f;
	const float AbsValue = FMath::Abs(InValue);

	// 소수점 이하 포맷
	FString FullString = FString::Printf(TEXT("%.*f"), InDecimalPlaces, AbsValue);

	// 정수 부분과 소수 부분 분리
	FString IntegerPart;
	FString DecimalPart;

	if (FullString.Contains(TEXT(".")))
	{
		int32 DotIndex = INDEX_NONE;
		FullString.FindChar('.', DotIndex);
		IntegerPart = FullString.Left(DotIndex);
		DecimalPart = FullString.Mid(DotIndex); // "." 포함
	}
	else
	{
		IntegerPart = FullString;
	}

	// 정수 부분에 콤마 삽입
	int32 InsertPosition = IntegerPart.Len() - 3;
	while (InsertPosition > 0)
	{
		IntegerPart.InsertAt(InsertPosition, TEXT(","));
		InsertPosition -= 3;
	}

	FString Result = IntegerPart + DecimalPart;

	if (bNegative)
	{
		Result = TEXT("-") + Result;
	}

	return Result;
}

FString UJWCU_BFL_StringUtility::WrapInRichTextStyle(const FString& InContent, const FString& InStyleName)
{
	return FString::Printf(TEXT("<%s>%s</>"), *InStyleName, *InContent);
}

FString UJWCU_BFL_StringUtility::WrapEachLineInRichTextStyle(const FString& InContent, const FString& InStyleName)
{
	if (InContent.IsEmpty())
	{
		return FString::Printf(TEXT("<%s></>"), *InStyleName);
	}

	FString Result;
	FString CurrentLine;

	auto AppendWrappedLine = [&Result, &CurrentLine, &InStyleName]()
	{
		Result += FString::Printf(TEXT("<%s>%s</>"), *InStyleName, *CurrentLine);
		CurrentLine.Reset();
	};

	for (int32 Index = 0; Index < InContent.Len(); ++Index)
	{
		const TCHAR Character = InContent[Index];
		if (Character == TEXT('\r') || Character == TEXT('\n'))
		{
			AppendWrappedLine();

			if (Character == TEXT('\r') && InContent.IsValidIndex(Index + 1) && InContent[Index + 1] == TEXT('\n'))
			{
				Result += TEXT("\r\n");
				++Index;
			}
			else
			{
				Result.AppendChar(Character);
			}
		}
		else
		{
			CurrentLine.AppendChar(Character);
		}
	}

	AppendWrappedLine();
	return Result;
}

FString UJWCU_BFL_StringUtility::ReplaceTextSlots(const FString& InTemplate, const TMap<FString, FString>& InReplacements)
{
	FString Result = InTemplate;

	for (const auto& Pair : InReplacements)
	{
		const FString Placeholder = FString::Printf(TEXT("{%s}"), *Pair.Key);
		Result = Result.Replace(*Placeholder, *Pair.Value);
	}

	return Result;
}

FString UJWCU_BFL_StringUtility::TruncateString(const FString& InString, int32 InMaxLength, const FString& InSuffix)
{
	if (InMaxLength <= 0)
	{
		return InSuffix;
	}

	if (InString.Len() <= InMaxLength)
	{
		return InString;
	}

	return InString.Left(InMaxLength) + InSuffix;
}

bool UJWCU_BFL_StringUtility::IsNullOrWhitespace(const FString& InString)
{
	if (InString.IsEmpty())
	{
		return true;
	}

	for (int32 i = 0; i < InString.Len(); ++i)
	{
		if (!FChar::IsWhitespace(InString[i]))
		{
			return false;
		}
	}

	return true;
}
