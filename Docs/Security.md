<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# OS 데이터 암호화

> 부분 갱신 일자: 2026-09-30 — 공급자 키 관리·저장소·설정 UI를 JWNU로 이관. JWCU는 범용 암호화 API만 유지.

## 범위

`UJWCU_BFL_PlatformCrypto`는 현재 Windows 사용자 계정에 결합된 DPAPI 데이터 보호를 제공한다. 별도 암호키를 코드나 설정에 넣지 않는다. `CRYPTPROTECT_UI_FORBIDDEN`을 사용하며 `CRYPTPROTECT_LOCAL_MACHINE`은 사용하지 않는다. Windows 외 플랫폼에서는 false를 반환하며 평문 대체는 없다.

## C++ / Blueprint 사용

Blueprint의 **JWCU → Security**에서 `ProtectData` / `UnprotectData`를 호출한다. 바이트 배열과 동일한 Context를 전달하며 실패 시 출력은 비운다. 입출력에 같은 배열을 전달하지 않는다. Context는 비밀키가 아니며 용도 구분 값이다. 평문 1MiB·암호문 2MiB·Context 1,024 TCHAR로 제한한다.

C++의 `ClearSensitiveBytes` / `ClearSensitiveString`은 전달받은 버퍼를 덮어쓰고 해제한다. 다른 문자열 복사본이나 메모리 덤프 전체 삭제까지 보장하지 않는다. 같은 사용자 권한의 프로세스 사이 격리를 제공하지 않는다.

## 공급자 키 관리 이관

[Deprecated 2026-09-30] JWCU의 APIKeyStore·APIKeyVault BFL, AIProviderSettings·AIProviderSubsystem과 키 관리 UI. 호환 클래스·redirect는 남기지 않는다.

공급자 키는 이제 [JWNU AI 공급자 키 관리](../../JWNetworkUtility/Docs/AIProviders.md)를 사용한다. JWNU는 자체 비공개 암호화를 가지며 JWCU를 참조하지 않는다. 저장 경로와 Context가 달라 기존 키는 새 설정 화면에서 재등록해야 한다. 기존 파일은 자동 삭제·이관하지 않는다.

## 검증과 구현

`JWCommonUtility.Security.PlatformCrypto`는 이진 왕복·Context 불일치·변조 거절·실패 시 출력 초기화를 검사한다. 공급자 키 검증은 `JWNetworkUtility.AI`로 이동했다. 2026-09-30 Editor/Game 빌드 및 범용 암호화 회귀 통과.

- [PlatformCrypto 헤더](../Source/JWCommonUtility/Public/JWCU_BFL_PlatformCrypto.h)
- [구현](../Source/JWCommonUtility/Private/JWCU_BFL_PlatformCrypto.cpp)
- [자동 테스트](../Source/JWCommonUtility/Private/JWCU_SecurityTests.cpp)
