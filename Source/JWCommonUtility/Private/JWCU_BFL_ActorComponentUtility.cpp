// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_BFL_ActorComponentUtility.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_ActorComponentUtility);

UActorComponent* UJWCU_BFL_ActorComponentUtility::GetComponentSafe(AActor* InActor, TSubclassOf<UActorComponent> InComponentClass, bool& bOutSuccess)
{
	bOutSuccess = false;

	if (!IsValid(InActor) || !InComponentClass)
	{
		return nullptr;
	}

	UActorComponent* FoundComponent = InActor->GetComponentByClass(InComponentClass);

	if (IsValid(FoundComponent))
	{
		bOutSuccess = true;
		return FoundComponent;
	}

	return nullptr;
}

TArray<AActor*> UJWCU_BFL_ActorComponentUtility::FilterActorsByTag(const TArray<AActor*>& InActors, FName InTag, EJWCU_ActorFilterResult& OutResult)
{
	TArray<AActor*> FilteredActors;

	for (AActor* Actor : InActors)
	{
		if (IsValid(Actor) && Actor->ActorHasTag(InTag))
		{
			FilteredActors.Add(Actor);
		}
	}

	OutResult = FilteredActors.Num() > 0 ? EJWCU_ActorFilterResult::Found : EJWCU_ActorFilterResult::NotFound;
	return FilteredActors;
}

AActor* UJWCU_BFL_ActorComponentUtility::FindNearestActorWithTag(const TArray<AActor*>& InActors, FName InTag, FVector InOrigin, EJWCU_ActorFilterResult& OutResult, float& OutDistance)
{
	AActor* NearestActor = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	for (AActor* Actor : InActors)
	{
		if (!IsValid(Actor) || !Actor->ActorHasTag(InTag))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(InOrigin, Actor->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestActor = Actor;
		}
	}

	if (NearestActor)
	{
		OutResult = EJWCU_ActorFilterResult::Found;
		OutDistance = FMath::Sqrt(NearestDistSq);
	}
	else
	{
		OutResult = EJWCU_ActorFilterResult::NotFound;
		OutDistance = 0.0f;
	}

	return NearestActor;
}

void UJWCU_BFL_ActorComponentUtility::BatchChildComponentAction(AActor* InActor, EJWCU_ChildComponentAction InAction, bool bIncludeChildActors)
{
	if (!IsValid(InActor))
	{
		return;
	}

	// 대상 액터 목록 구성
	TArray<AActor*> TargetActors;
	TargetActors.Add(InActor);

	if (bIncludeChildActors)
	{
		InActor->GetAllChildActors(TargetActors, true);
	}

	// 각 액터의 모든 컴포넌트에 액션 수행
	for (AActor* TargetActor : TargetActors)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}

		TArray<UActorComponent*> Components;
		TargetActor->GetComponents(Components);

		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component))
			{
				continue;
			}

			switch (InAction)
			{
			case EJWCU_ChildComponentAction::SetVisibility_Visible:
				if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
				{
					SceneComp->SetVisibility(true, true);
				}
				break;

			case EJWCU_ChildComponentAction::SetVisibility_Hidden:
				if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
				{
					SceneComp->SetVisibility(false, true);
				}
				break;

			case EJWCU_ChildComponentAction::Activate:
				Component->Activate(true);
				break;

			case EJWCU_ChildComponentAction::Deactivate:
				Component->Deactivate();
				break;
			}
		}
	}
}

void UJWCU_BFL_ActorComponentUtility::SetActorVisibilityRecursive(AActor* InActor, bool bNewVisibility, bool bPropagateToChildren)
{
	if (!IsValid(InActor))
	{
		return;
	}

	// 루트 컴포넌트의 가시성 설정
	if (USceneComponent* RootComp = InActor->GetRootComponent())
	{
		RootComp->SetVisibility(bNewVisibility, true);
	}

	// 자식 액터에도 전파
	if (bPropagateToChildren)
	{
		TArray<AActor*> ChildActors;
		InActor->GetAllChildActors(ChildActors, true);

		for (AActor* ChildActor : ChildActors)
		{
			if (IsValid(ChildActor))
			{
				if (USceneComponent* ChildRootComp = ChildActor->GetRootComponent())
				{
					ChildRootComp->SetVisibility(bNewVisibility, true);
				}
			}
		}
	}
}

TArray<AActor*> UJWCU_BFL_ActorComponentUtility::GetActorsOfClassInRadius(const UObject* WorldContextObject, TSubclassOf<AActor> InActorClass, FVector InOrigin, float InRadius)
{
	TArray<AActor*> Result;

	if (!WorldContextObject || !InActorClass)
	{
		return Result;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return Result;
	}

	const float RadiusSq = InRadius * InRadius;

	for (TActorIterator<AActor> It(World, InActorClass); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor))
		{
			const float DistSq = FVector::DistSquared(InOrigin, Actor->GetActorLocation());
			if (DistSq <= RadiusSq)
			{
				Result.Add(Actor);
			}
		}
	}

	return Result;
}
