<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# JWCommonUtility

> 부분 갱신 일자: 2026-09-30 — JWNU가 공급자 키 관리·내부 암호화를 독립 소유하도록 이관. 기존 키 API·자동 이관 제거.

> 부분 갱신 일자: 2026-09-29 — BP 그래프 포스트잇 메모와 사용 문서 추가.

> 부분 갱신 일자: 2026-09-28 — AI Provider Settings·관리 화면을 JWCU로 이관하고 GameInstance별 BP 서브시스템을 추가.

> 부분 갱신 일자: 2026-09-27 — OS 암호화와 개인 API 키 저장 기능 추가.

Unreal Engine 5 범용 유틸리티 플러그인. Runtime Blueprint 함수 라이브러리·디버그 매크로와 함께, Unreal C++ 프로젝트용 Python 개발 도구와 에이전트 스킬 원본을 보관한다.

- **엔진**: `EngineVersion` 을 고정하지 않고 호스트 프로젝트의 엔진을 따른다(UE 5.6–5.7에서 사용).
- **플랫폼**: Win64
- **의존 플러그인**: ModelViewViewModel

범용 OS 암호화는 [Security.md](Docs/Security.md), 공급자 키 관리는 [JWNU AIProviders.md](../JWNetworkUtility/Docs/AIProviders.md)를 본다.

BP 그래프의 독립적인 메모는 [Blueprint 포스트잇](Docs/StickyNotes.md)을 본다. 크기·색상·본문 편집, 접기·잠금과 기본 코멘트 변환을 제공한다.

## 구성

| 경로 | 내용 |
| --- | --- |
| `Source/JWCommonUtility` | DateTime·String·Actor/Component·Math·Color·Widget·Debug·Collection·PlatformCrypto BFL, `JWCU_*` 디버그 매크로 |
| `Source/JWCommonUtilityEditor` | BP 그래프 포스트잇 |
| `Tools/` | 주석·구조·코드 검사, 저작권 헤더 정리, 에이전트 스킬 설치기 (Python 3.10+, 표준 라이브러리만 사용) |
| `Agent/Skills/` | Unreal 코드 정리·주석·문서 작성용 에이전트 스킬 원본 |
| `Docs/` | 도구·스킬 사용 문서 |

## 설치

호스트 프로젝트의 `Plugins/` 아래에 서브모듈로 추가한다.

```bash
git submodule add https://github.com/prayslaks/JWCommonUtility.git Plugins/JWCommonUtility
```

도구와 스킬 사용법은 [Docs/Tooling.md](Docs/Tooling.md), 에이전트 연결은 [Docs/AgentSupport.md](Docs/AgentSupport.md) 를 본다.

## 테스트

```bash
python -m unittest discover -s Tools/Tests
```

## 라이선스

[MIT](LICENSE)
