// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWCU_BFL_CollectionUtility.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_CollectionUtility);

// Fisher-Yates 셔플 내부 헬퍼 템플릿
namespace JWCU_Internal
{
	template<typename T>
	TArray<T> ShuffleArray(const TArray<T>& InArray)
	{
		TArray<T> Result = InArray;
		const int32 Num = Result.Num();

		for (int32 i = Num - 1; i > 0; --i)
		{
			const int32 j = FMath::RandRange(0, i);
			Result.Swap(i, j);
		}

		return Result;
	}
}

TArray<FString> UJWCU_BFL_CollectionUtility::ShuffleStringArray(const TArray<FString>& InArray)
{
	return JWCU_Internal::ShuffleArray(InArray);
}

TArray<int32> UJWCU_BFL_CollectionUtility::ShuffleIntArray(const TArray<int32>& InArray)
{
	return JWCU_Internal::ShuffleArray(InArray);
}

TArray<AActor*> UJWCU_BFL_CollectionUtility::ShuffleActorArray(const TArray<AActor*>& InArray)
{
	return JWCU_Internal::ShuffleArray(InArray);
}

TArray<FString> UJWCU_BFL_CollectionUtility::PickRandomStrings(const TArray<FString>& InArray, int32 InCount)
{
	if (InArray.Num() == 0 || InCount <= 0)
	{
		return TArray<FString>();
	}

	// 요청 수가 배열 크기 이상이면 셔플해서 전체 반환
	if (InCount >= InArray.Num())
	{
		return JWCU_Internal::ShuffleArray(InArray);
	}

	// 셔플 후 앞에서 N개 추출
	TArray<FString> Shuffled = JWCU_Internal::ShuffleArray(InArray);
	TArray<FString> Result;
	Result.Reserve(InCount);

	for (int32 i = 0; i < InCount; ++i)
	{
		Result.Add(Shuffled[i]);
	}

	return Result;
}

TArray<FString> UJWCU_BFL_CollectionUtility::RemoveDuplicateStrings(const TArray<FString>& InArray)
{
	TArray<FString> Result;
	TSet<FString> Seen;

	for (const FString& Str : InArray)
	{
		if (!Seen.Contains(Str))
		{
			Seen.Add(Str);
			Result.Add(Str);
		}
	}

	return Result;
}

TArray<AActor*> UJWCU_BFL_CollectionUtility::RemoveNullActors(const TArray<AActor*>& InArray)
{
	TArray<AActor*> Result;

	for (AActor* Actor : InArray)
	{
		if (IsValid(Actor))
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

TArray<float> UJWCU_BFL_CollectionUtility::SortFloatArray(const TArray<float>& InArray, EJWCU_SortOrder InOrder)
{
	TArray<float> Result = InArray;

	if (InOrder == EJWCU_SortOrder::Ascending)
	{
		Result.Sort([](float A, float B) { return A < B; });
	}
	else
	{
		Result.Sort([](float A, float B) { return A > B; });
	}

	return Result;
}

TArray<int32> UJWCU_BFL_CollectionUtility::SortIntArray(const TArray<int32>& InArray, EJWCU_SortOrder InOrder)
{
	TArray<int32> Result = InArray;

	if (InOrder == EJWCU_SortOrder::Ascending)
	{
		Result.Sort([](int32 A, int32 B) { return A < B; });
	}
	else
	{
		Result.Sort([](int32 A, int32 B) { return A > B; });
	}

	return Result;
}
