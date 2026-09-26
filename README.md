<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# JWCommonUtility

Unreal Engine 5 범용 유틸리티 플러그인. Runtime Blueprint 함수 라이브러리·디버그 매크로와 함께, Unreal C++ 프로젝트용 Python 개발 도구와 에이전트 스킬 원본을 보관한다.

- **엔진**: `EngineVersion` 을 고정하지 않고 호스트 프로젝트의 엔진을 따른다(UE 5.6–5.7에서 사용).
- **플랫폼**: Win64
- **의존 플러그인**: ModelViewViewModel

## 구성

| 경로 | 내용 |
| --- | --- |
| `Source/JWCommonUtility` | DateTime·String·Actor/Component·Math·Color·Widget·Debug·Collection BFL, `JWCU_*` 디버그 매크로 |
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
