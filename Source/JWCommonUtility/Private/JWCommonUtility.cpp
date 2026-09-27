// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWCommonUtility.h"
#include "JWCU_BFL_APIKeyStore.h"

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY(JWCULog);
DEFINE_LOG_CATEGORY(JWCULogWall);

// 온스크린 디버그 메시지 활성화 콘솔 변수 정의
TAutoConsoleVariable<bool> CVarJWCU_DebugScreen(
	TEXT("JWCU.DebugScreen"),
	false,
	TEXT("JWCommonUtility 온스크린 디버그 메시지 활성화/비활성화. 1=활성, 0=비활성"),
	ECVF_Default
);

void FJWCommonUtilityModule::StartupModule()
{
}

void FJWCommonUtilityModule::ShutdownModule()
{
	UJWCU_BFL_APIKeyStore::ClearAllMemoryKeys();
}

IMPLEMENT_MODULE(FJWCommonUtilityModule, JWCommonUtility)
