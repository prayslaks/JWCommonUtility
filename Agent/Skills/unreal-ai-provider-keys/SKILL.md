---
name: unreal-ai-provider-keys
description: JWNetworkUtility의 AI 공급자 카탈로그, 프로젝트 설정 키 관리 UI, OS 암호화 저장소와 GameInstanceSubsystem 기반 BP 키 조회를 연결·수정·검토한다. STT·TTS·LLM 등에서 공통 인증 저장을 사용할 때 적용하며 전사 프로토콜·게임 어댑터 구현은 다루지 않는다.
---

<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# AI 공급자 키 관리

[공용 지침](../../../Docs/CommonGuidance.md)과 호스트 지침을 읽는다. 링크로 발견했다면 상대 경로는 플러그인 원본 기준이다. 키 관리 구현은 JWNU가 소유하고 이 스킬은 JWCU 공용 도구 묶음에 배포된다. 저장·선택 계약의 정본은 [AIProviders.md](../../../../JWNetworkUtility/Docs/AIProviders.md)다. 실제 키 변경이나 외부 API 시험은 사용자가 요청한 범위에서 수행하며, 스킬 적용 자체를 별도 작업 승인으로 해석하지 않는다.

## 공급자 등록과 책임

- `UJWNU_AIProviderSettings::GetProviderCatalog()`에 안정적인 문자열 ID를 등록한다. 같은 공급자의 STT·TTS·LLM은 키 공급자 ID를 공유할 수 있다. 모델 이름이나 표시 이름 때문에 저장 ID를 바꾸지 않는다.
- 카탈로그는 코드가 소유하며 사용자가 편집하는 Registry를 추가하지 않는다. 설정 UI와 GIS가 같은 목록을 검증한다. 지원 종료는 `bDeprecated`로 처리해 기존 키 목록 조회·삭제를 보존한다.
- 등록은 키 관리 지원을 뜻한다. 해당 공급자의 모델·네트워크 API 구현이나 인증 성공을 보장하지 않는다.
- JWNU는 키 저장·선택·UI를 소유한다. 암호화·저장소는 AI 모듈 Private의 native 구현으로 유지한다. 공급자별 인증 header·endpoint 검증은 호출자가 담당한다. JWNU 키 관리 모듈에 게임 타입이나 전사 프로토콜 의존성을 추가하지 않는다.

## 저장과 사용 경로

1. 설정 UI는 `Project Settings → JWNetworkUtility → AI Provider Settings`다. 마스킹 입력 후 명시적 저장을 사용하며 비밀을 Config 속성에 넣지 않는다. 첫 키는 자동 선택되고 이후 키는 사용 키로 선택한다.
2. BP/런타임은 현재 GameInstance의 `UJWNU_AIProviderSubsystem`을 사용한다. `ListKeys`는 비밀 없는 목록, `GetApiKey`는 실제 키다. GameInstance가 없는 에디터 네이티브 호출만 설정 저장소를 직접 사용할 수 있다.
3. `SetRuntimeApiKey`는 해당 GameInstance에만 적용된다. 빈 override는 저장 키 조회도 차단한다. `ClearRuntimeApiKey`는 저장 키 사용으로 복귀한다. 다른 PIE 인스턴스의 임시 키를 바꾸지 않는다.
4. 영구 목록·선택은 프로젝트 단위로 공유된다. AppId는 프로젝트 이름이며 이름 변경은 저장 네임스페이스 변경이다. `FJWNU_APIKeyVault → FJWNU_APIKeyStore → FJWNU_KeyCrypto`의 비공개 저장 경로를 사용한다. 실패 시 평문으로 대체하지 않는다.
5. 관리용 만료일·키 존재 여부를 원격 유효성·권한·quota 확인으로 표현하지 않는다. 사용 키 삭제는 선택을 비우며 다른 키를 자동 선택하거나 원격 키를 폐기하지 않는다.

공급자 호출자는 공식 endpoint를 검증한 뒤 키를 조회·주입한다. 임의 endpoint나 relay로 저장 키를 자동 전달하지 않는다. 키 원문을 로그·채팅·명령 인자·에셋·공유 ini에 넣지 않으며 불필요한 임시 버퍼는 모듈 내부에서 정리한다. 정리 편의를 위해 JWNU 암호화·저장소를 공개하지 않는다.

## 변경 시 읽을 코드

- [AIProviderSettings](../../../../JWNetworkUtility/Source/JWNetworkUtilityAI/Public/JWNU_AIProviderSettings.h): 고정 카탈로그·프로젝트 저장 경계.
- [AIProviderSubsystem](../../../../JWNetworkUtility/Source/JWNetworkUtilityAI/Public/JWNU_AIProviderSubsystem.h): BP·GameInstance별 override와 저장 키 선택.
- [APIKeyVault](../../../../JWNetworkUtility/Source/JWNetworkUtilityAI/Private/JWNU_APIKeyVault.h): 다중 키·메타데이터·선택·비공개 암호화 저장.
- [관리 화면](../../../../JWNetworkUtility/Source/JWNetworkUtilityEditor/Private/JWNU_AIProviderKeysCustomization.cpp): 키 입력·마스킹·명시적 저장.

## 검증과 전달

변경 범위에 따라 가짜 키와 고유 AppId로 카탈로그 노출·미등록 ID 거절·여러 키 선택·선택 키 삭제·GI별 override 격리·저장 실패 시 기존 데이터 보존을 검증한다. 기존 `JWNetworkUtility.AI` 자동 검사와 에디터 UI 변경에 맞는 Editor 빌드를 활용한다. 실제 사용자 키를 자동 검사 데이터로 쓰지 않는다.

기존 JWCU API·암호문·평문 ini의 자동 이관이나 호환 계층은 제공하지 않는다. 실제 API 시험을 하지 않았다면 저장소 검증과 구분해 보고한다. 공용 계약이 바뀌면 AIProviders.md를 갱신하고 호스트의 구체적인 연결 절차는 호스트 문서에 남긴다.
