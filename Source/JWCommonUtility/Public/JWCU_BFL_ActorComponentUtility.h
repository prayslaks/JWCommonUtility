// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWCommonUtilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWCU_BFL_ActorComponentUtility.generated.h"

JWCOMMONUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWCU_BFL_ActorComponentUtility, Log, All);

/**
 * 액터/컴포넌트 관리 유틸리티 블루프린트 함수 라이브러리.
 * 안전한 컴포넌트 획득, 태그 기반 필터링, 자식 일괄 제어 등을 지원한다.
 */
UCLASS()
class JWCOMMONUTILITY_API UJWCU_BFL_ActorComponentUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * [ Blueprint Function Library ] \n Get Component Safe \n GetComponentByClass + 유효성 검사를 한 번에 수행한다.
	 * @param InActor 대상 액터
	 * @param InComponentClass 찾을 컴포넌트 클래스
	 * @param bOutSuccess 컴포넌트를 찾았는지 여부
	 * @return 찾은 컴포넌트 (실패 시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(DeterminesOutputType="InComponentClass", ComponentClass="/Script/Engine.ActorComponent"))
	static UActorComponent* GetComponentSafe(AActor* InActor, TSubclassOf<UActorComponent> InComponentClass, bool& bOutSuccess);

	/**
	 * [ Blueprint Function Library ] \n Filter Actors By Tag \n 액터 배열에서 특정 태그를 가진 액터만 필터링한다.
	 * @param InActors 필터링할 액터 배열
	 * @param InTag 찾을 태그 이름
	 * @param OutResult 필터링 결과 (Found / NotFound)
	 * @return 태그를 가진 액터 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(ExpandEnumAsExecs="OutResult"))
	static TArray<AActor*> FilterActorsByTag(const TArray<AActor*>& InActors, FName InTag, EJWCU_ActorFilterResult& OutResult);

	/**
	 * [ Blueprint Function Library ] \n Find Nearest Actor With Tag \n 액터 배열에서 특정 태그를 가진 가장 가까운 액터를 찾는다.
	 * @param InActors 검색할 액터 배열
	 * @param InTag 찾을 태그 이름
	 * @param InOrigin 기준 위치
	 * @param OutResult 검색 결과 (Found / NotFound)
	 * @param OutDistance 가장 가까운 액터와의 거리
	 * @return 가장 가까운 액터 (없으면 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(ExpandEnumAsExecs="OutResult"))
	static AActor* FindNearestActorWithTag(const TArray<AActor*>& InActors, FName InTag, FVector InOrigin, EJWCU_ActorFilterResult& OutResult, float& OutDistance);

	/**
	 * [ Blueprint Function Library ] \n Batch Child Component Action \n 액터의 모든 컴포넌트에 일괄 액션을 수행한다.
	 * @param InActor 대상 액터
	 * @param InAction 수행할 액션 (가시성, 활성/비활성)
	 * @param bIncludeChildActors 자식 액터의 컴포넌트도 포함할지 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static void BatchChildComponentAction(AActor* InActor, EJWCU_ChildComponentAction InAction, bool bIncludeChildActors = false);

	/**
	 * [ Blueprint Function Library ] \n Set Actor Visibility Recursive \n 액터와 자식 액터의 가시성을 재귀적으로 설정한다.
	 * @param InActor 대상 액터
	 * @param bNewVisibility 새 가시성 값
	 * @param bPropagateToChildren 자식 액터에도 전파할지 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library")
	static void SetActorVisibilityRecursive(AActor* InActor, bool bNewVisibility, bool bPropagateToChildren = true);

	/**
	 * [ Blueprint Function Library ] \n Get Actors Of Class In Radius \n 특정 클래스의 액터 중 반경 내에 있는 액터를 검색한다.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InActorClass 검색할 액터 클래스
	 * @param InOrigin 기준 위치
	 * @param InRadius 검색 반경
	 * @return 반경 내에 있는 액터 배열
	 */
	UFUNCTION(BlueprintCallable, Category="JWCU Blueprint Function Library", meta=(WorldContext="WorldContextObject", DeterminesOutputType="InActorClass"))
	static TArray<AActor*> GetActorsOfClassInRadius(const UObject* WorldContextObject, TSubclassOf<AActor> InActorClass, FVector InOrigin, float InRadius);
};
