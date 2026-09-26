// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_BFL_DateTimeUtility.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_DateTimeUtility);

FString UJWCU_BFL_DateTimeUtility::GetMonthAbbreviation(int32 InMonth)
{
	static const TCHAR* Months[] =
	{
		TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"),
		TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"),
		TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec")
	};

	if (InMonth >= 1 && InMonth <= 12)
	{
		return Months[InMonth - 1];
	}
	return TEXT("Unknown");
}

FString UJWCU_BFL_DateTimeUtility::GetWeekdayName(EDayOfWeek InDayOfWeek)
{
	switch (InDayOfWeek)
	{
	case EDayOfWeek::Monday:    return TEXT("Monday");
	case EDayOfWeek::Tuesday:   return TEXT("Tuesday");
	case EDayOfWeek::Wednesday: return TEXT("Wednesday");
	case EDayOfWeek::Thursday:  return TEXT("Thursday");
	case EDayOfWeek::Friday:    return TEXT("Friday");
	case EDayOfWeek::Saturday:  return TEXT("Saturday");
	case EDayOfWeek::Sunday:    return TEXT("Sunday");
	default:                    return TEXT("Unknown");
	}
}

void UJWCU_BFL_DateTimeUtility::ConvertTo12Hour(int32 InHour24, int32& OutHour12, FString& OutAmPm)
{
	if (InHour24 == 0)
	{
		OutHour12 = 12;
		OutAmPm = TEXT("AM");
	}
	else if (InHour24 < 12)
	{
		OutHour12 = InHour24;
		OutAmPm = TEXT("AM");
	}
	else if (InHour24 == 12)
	{
		OutHour12 = 12;
		OutAmPm = TEXT("PM");
	}
	else
	{
		OutHour12 = InHour24 - 12;
		OutAmPm = TEXT("PM");
	}
}

FText UJWCU_BFL_DateTimeUtility::FormatDate(const FDateTime& InDateTime, EJWCU_DateFormat InFormat)
{
	const int32 Year = InDateTime.GetYear();
	const int32 Month = InDateTime.GetMonth();
	const int32 Day = InDateTime.GetDay();

	FString Result;

	switch (InFormat)
	{
	case EJWCU_DateFormat::YYYY_MM_DD:
		Result = FString::Printf(TEXT("%04d-%02d-%02d"), Year, Month, Day);
		break;

	case EJWCU_DateFormat::DD_MM_YYYY:
		Result = FString::Printf(TEXT("%02d/%02d/%04d"), Day, Month, Year);
		break;

	case EJWCU_DateFormat::MM_DD_YYYY:
		Result = FString::Printf(TEXT("%02d/%02d/%04d"), Month, Day, Year);
		break;

	case EJWCU_DateFormat::YYYY_MM_DD_Dot:
		Result = FString::Printf(TEXT("%04d.%02d.%02d"), Year, Month, Day);
		break;

	case EJWCU_DateFormat::DD_Mon_YYYY:
		Result = FString::Printf(TEXT("%02d %s %04d"), Day, *GetMonthAbbreviation(Month), Year);
		break;

	case EJWCU_DateFormat::Mon_DD_YYYY:
		Result = FString::Printf(TEXT("%s %02d, %04d"), *GetMonthAbbreviation(Month), Day, Year);
		break;

	case EJWCU_DateFormat::Full_Weekday:
		Result = FString::Printf(TEXT("%s, %s %02d %04d"),
			*GetWeekdayName(InDateTime.GetDayOfWeek()),
			*GetMonthAbbreviation(Month), Day, Year);
		break;
	}

	return FText::FromString(Result);
}

FText UJWCU_BFL_DateTimeUtility::FormatTime(const FDateTime& InDateTime, EJWCU_TimeFormat InFormat)
{
	const int32 Hour = InDateTime.GetHour();
	const int32 Minute = InDateTime.GetMinute();
	const int32 Second = InDateTime.GetSecond();

	FString Result;

	switch (InFormat)
	{
	case EJWCU_TimeFormat::HH_MM_24:
		Result = FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
		break;

	case EJWCU_TimeFormat::HH_MM_SS_24:
		Result = FString::Printf(TEXT("%02d:%02d:%02d"), Hour, Minute, Second);
		break;

	case EJWCU_TimeFormat::HH_MM_12:
		{
			int32 Hour12;
			FString AmPm;
			ConvertTo12Hour(Hour, Hour12, AmPm);
			Result = FString::Printf(TEXT("%d:%02d %s"), Hour12, Minute, *AmPm);
		}
		break;

	case EJWCU_TimeFormat::HH_MM_SS_12:
		{
			int32 Hour12;
			FString AmPm;
			ConvertTo12Hour(Hour, Hour12, AmPm);
			Result = FString::Printf(TEXT("%d:%02d:%02d %s"), Hour12, Minute, Second, *AmPm);
		}
		break;
	}

	return FText::FromString(Result);
}

FText UJWCU_BFL_DateTimeUtility::FormatDateTime(const FDateTime& InDateTime, EJWCU_DateTimeFormat InFormat)
{
	const int32 Year = InDateTime.GetYear();
	const int32 Month = InDateTime.GetMonth();
	const int32 Day = InDateTime.GetDay();
	const int32 Hour = InDateTime.GetHour();
	const int32 Minute = InDateTime.GetMinute();
	const int32 Second = InDateTime.GetSecond();

	FString Result;

	switch (InFormat)
	{
	case EJWCU_DateTimeFormat::YYYY_MM_DD_HH_MM:
		Result = FString::Printf(TEXT("%04d-%02d-%02d %02d:%02d"), Year, Month, Day, Hour, Minute);
		break;

	case EJWCU_DateTimeFormat::YYYY_MM_DD_HH_MM_SS:
		Result = FString::Printf(TEXT("%04d-%02d-%02d %02d:%02d:%02d"), Year, Month, Day, Hour, Minute, Second);
		break;

	case EJWCU_DateTimeFormat::MM_DD_YYYY_12h:
		{
			int32 Hour12;
			FString AmPm;
			ConvertTo12Hour(Hour, Hour12, AmPm);
			Result = FString::Printf(TEXT("%02d/%02d/%04d %d:%02d %s"), Month, Day, Year, Hour12, Minute, *AmPm);
		}
		break;

	case EJWCU_DateTimeFormat::Full_Weekday_12h:
		{
			int32 Hour12;
			FString AmPm;
			ConvertTo12Hour(Hour, Hour12, AmPm);
			Result = FString::Printf(TEXT("%s, %s %02d %04d %d:%02d %s"),
				*GetWeekdayName(InDateTime.GetDayOfWeek()),
				*GetMonthAbbreviation(Month), Day, Year, Hour12, Minute, *AmPm);
		}
		break;

	case EJWCU_DateTimeFormat::ISO8601:
		Result = FString::Printf(TEXT("%04d-%02d-%02dT%02d:%02d:%02dZ"), Year, Month, Day, Hour, Minute, Second);
		break;
	}

	return FText::FromString(Result);
}

FJWCU_RelativeTimeResult UJWCU_BFL_DateTimeUtility::GetRelativeTime(const FDateTime& InDateTime, EJWCU_RelativeTimeGranularity InGranularity)
{
	FJWCU_RelativeTimeResult Result;

	const FDateTime Now = FDateTime::UtcNow();
	const FTimespan Delta = Now - InDateTime;
	const double TotalSec = Delta.GetTotalSeconds();

	Result.TotalSeconds = static_cast<float>(TotalSec);
	Result.bIsPast = TotalSec > 0.0;

	const double AbsSeconds = FMath::Abs(TotalSec);
	const FString Suffix = Result.bIsPast ? TEXT("ago") : TEXT("from now");

	FString TimeText;

	switch (InGranularity)
	{
	case EJWCU_RelativeTimeGranularity::ForceSeconds:
		TimeText = FString::Printf(TEXT("%.0f seconds %s"), AbsSeconds, *Suffix);
		break;

	case EJWCU_RelativeTimeGranularity::ForceMinutes:
		TimeText = FString::Printf(TEXT("%.1f minutes %s"), AbsSeconds / 60.0, *Suffix);
		break;

	case EJWCU_RelativeTimeGranularity::ForceHours:
		TimeText = FString::Printf(TEXT("%.1f hours %s"), AbsSeconds / 3600.0, *Suffix);
		break;

	case EJWCU_RelativeTimeGranularity::ForceDays:
		TimeText = FString::Printf(TEXT("%.1f days %s"), AbsSeconds / 86400.0, *Suffix);
		break;

	case EJWCU_RelativeTimeGranularity::Automatic:
	default:
		if (AbsSeconds < 60.0)
		{
			TimeText = FString::Printf(TEXT("%.0f seconds %s"), AbsSeconds, *Suffix);
		}
		else if (AbsSeconds < 3600.0)
		{
			const int32 Minutes = FMath::FloorToInt32(AbsSeconds / 60.0);
			TimeText = FString::Printf(TEXT("%d %s %s"), Minutes, Minutes == 1 ? TEXT("minute") : TEXT("minutes"), *Suffix);
		}
		else if (AbsSeconds < 86400.0)
		{
			const int32 Hours = FMath::FloorToInt32(AbsSeconds / 3600.0);
			TimeText = FString::Printf(TEXT("%d %s %s"), Hours, Hours == 1 ? TEXT("hour") : TEXT("hours"), *Suffix);
		}
		else
		{
			const int32 Days = FMath::FloorToInt32(AbsSeconds / 86400.0);
			TimeText = FString::Printf(TEXT("%d %s %s"), Days, Days == 1 ? TEXT("day") : TEXT("days"), *Suffix);
		}
		break;
	}

	Result.DisplayText = FText::FromString(TimeText);
	return Result;
}

FDateTime UJWCU_BFL_DateTimeUtility::GetNowUTC()
{
	return FDateTime::UtcNow();
}

FDateTime UJWCU_BFL_DateTimeUtility::GetNowLocal()
{
	return FDateTime::Now();
}

FDateTime UJWCU_BFL_DateTimeUtility::UnixTimestampToDateTime(int64 InUnixTimestamp)
{
	return FDateTime::FromUnixTimestamp(InUnixTimestamp);
}

int64 UJWCU_BFL_DateTimeUtility::DateTimeToUnixTimestamp(const FDateTime& InDateTime)
{
	return InDateTime.ToUnixTimestamp();
}
