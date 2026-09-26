// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCU_BFL_DebugUtility.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogJWCU_BFL_DebugUtility);

void UJWCU_BFL_DebugUtility::DrawDebugShapeAtLocation(const UObject* WorldContextObject, EJWCU_DebugShape InShape, FVector InLocation, FVector InSize, FRotator InRotation, FLinearColor InColor, float InDuration, float InThickness)
{
	if (!WorldContextObject)
	{
		return;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	const FColor DrawColor = InColor.ToFColor(true);
	const bool bPersistent = (InDuration <= 0.0f);

	switch (InShape)
	{
	case EJWCU_DebugShape::Sphere:
		DrawDebugSphere(World, InLocation, InSize.X, 16, DrawColor, bPersistent, InDuration, 0, InThickness);
		break;

	case EJWCU_DebugShape::Box:
		DrawDebugBox(World, InLocation, InSize, InRotation.Quaternion(), DrawColor, bPersistent, InDuration, 0, InThickness);
		break;

	case EJWCU_DebugShape::Line:
		// InSize를 끝점으로 사용
		DrawDebugLine(World, InLocation, InSize, DrawColor, bPersistent, InDuration, 0, InThickness);
		break;

	case EJWCU_DebugShape::Arrow:
		// InSize를 끝점으로 사용
		DrawDebugDirectionalArrow(World, InLocation, InSize, 20.0f, DrawColor, bPersistent, InDuration, 0, InThickness);
		break;

	case EJWCU_DebugShape::Point:
		DrawDebugPoint(World, InLocation, InSize.X, DrawColor, bPersistent, InDuration, 0);
		break;

	case EJWCU_DebugShape::Capsule:
		// InSize.X = 반지름, InSize.Z = 반높이
		DrawDebugCapsule(World, InLocation, InSize.Z, InSize.X, InRotation.Quaternion(), DrawColor, bPersistent, InDuration, 0, InThickness);
		break;
	}
}

void UJWCU_BFL_DebugUtility::PrintKeyValueToScreen(const FString& InKey, const FString& InValue, FColor InColor, float InDuration, int32 InScreenId)
{
	if (!GEngine)
	{
		return;
	}

	const FString Message = FString::Printf(TEXT("%s: %s"), *InKey, *InValue);
	GEngine->AddOnScreenDebugMessage(InScreenId, InDuration, InColor, Message);
}

void UJWCU_BFL_DebugUtility::PrintVectorToScreen(const FString& InLabel, FVector InVector, int32 InDecimalPlaces, FColor InColor, float InDuration, int32 InScreenId)
{
	if (!GEngine)
	{
		return;
	}

	const FString Message = FString::Printf(TEXT("%s: (X=%.*f, Y=%.*f, Z=%.*f)"),
		*InLabel,
		InDecimalPlaces, InVector.X,
		InDecimalPlaces, InVector.Y,
		InDecimalPlaces, InVector.Z);

	GEngine->AddOnScreenDebugMessage(InScreenId, InDuration, InColor, Message);
}

void UJWCU_BFL_DebugUtility::PrintRotatorToScreen(const FString& InLabel, FRotator InRotator, int32 InDecimalPlaces, FColor InColor, float InDuration, int32 InScreenId)
{
	if (!GEngine)
	{
		return;
	}

	const FString Message = FString::Printf(TEXT("%s: (Pitch=%.*f, Yaw=%.*f, Roll=%.*f)"),
		*InLabel,
		InDecimalPlaces, InRotator.Pitch,
		InDecimalPlaces, InRotator.Yaw,
		InDecimalPlaces, InRotator.Roll);

	GEngine->AddOnScreenDebugMessage(InScreenId, InDuration, InColor, Message);
}

void UJWCU_BFL_DebugUtility::DrawDebugTextAtLocation(const UObject* WorldContextObject, const FString& InText, FVector InWorldLocation, FColor InColor, float InDuration, float InFontScale)
{
	if (!WorldContextObject)
	{
		return;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	DrawDebugString(World, InWorldLocation, InText, nullptr, InColor, InDuration, false, InFontScale);
}
