---
name: unreal-overview-writer
description: Unreal Engine 프로젝트의 전체 구조를 Mermaid 중심으로 설명하는 한글 개요 문서를 생성하거나 갱신한다. Codex가 Unreal 프로젝트의 큰 그림, C++ 클래스, Blueprint 에셋, DataAsset, Subsystem, Interface, 런타임 흐름, 네트워크/Authority 경계, 온보딩용 `Docs/SystemOverview.md` 같은 상위 개발 문서를 작성해야 할 때 사용한다.
---

<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->


# Unreal 프로젝트 개요 작성기

이 스킬은 Unreal Engine 프로젝트의 상위 구조를 Mermaid 다이어그램 중심으로 설명하는 개발 문서를 작성할 때 사용한다. Blueprint, C++, 에셋, DataAsset, 네트워크 경계가 함께 얽힌 프로젝트에서 “전체 그림을 먼저 이해하는 문서”가 필요할 때 적합하다.

> 기본 원칙: 사용자가 다른 언어를 명시하지 않으면 산출 문서는 한글로 작성한다. Unreal 타입명, 파일 경로, API 이름, Blueprint 에셋명, 네트워크 role 이름은 원문을 유지하고, 설명 문장과 표 머리글은 한글로 쓴다.

특정 하위 시스템 하나를 깊게 설명하거나, 스키마/검증 규칙/에디터 도구/유지보수 절차를 상세히 문서화하는 작업에는 [unreal-doc-writer](../unreal-doc-writer/SKILL.md)를 사용한다. 이 스킬은 그런 세부 문서로 들어가기 전에 읽는 상위 개요와 관계도를 만든다.

---

## 호스트 설정과 공용 원본

- 이 스킬과 참조 템플릿의 정본은 JWCommonUtility 플러그인이다. 링크로 발견했다면 이 문서의 상대 경로는 심볼릭 링크/junction을 해석한 원본 스킬 디렉터리 기준으로 읽는다.
- [공용 지침](../../../Docs/CommonGuidance.md)을 읽고 작업 대상 .uproject 루트의 `Config/JWCommonUtilityTools.json`이 있으면 읽는다. `agent.instructions`와 현재 작업에 관련된 `agent.guidance_files`에서 문서 위치·정본·갱신 규약을 찾는다. JSON이 없으면 기존 프로젝트 문서 구조와 아래 기본값을 사용한다.
- 기존 AGENTS.md/CLAUDE.md가 있으면 그 규약도 따른다. 이 스킬을 연결하기 위해 진입점 파일을 생성하거나 덮어쓰지 않는다.
- 생성·갱신할 문서는 대상 시스템을 소유하는 프로젝트 또는 플러그인에 둔다. 공용 스킬·템플릿 안에 호스트의 클래스 이름·엔진 절대 경로·아키텍처를 기록하지 않는다.

## 핵심 작성 흐름

1. 개요 대상 결정:
   - 사용자가 프로젝트 영역을 지정하면 해당 영역과 인접 시스템을 개요화한다.
   - 프로젝트 전체 개요를 요청하면 기본 산출물은 `Docs/SystemOverview.md`로 둔다.
   - 이미 개요 문서가 있으면 먼저 drift audit을 수행한 뒤 갱신한다.

2. 압축된 source map 작성:
   - `Docs`, `Source`, `Content`, config 파일, build/module 파일을 조사한다.
   - C++ 런타임 클래스, interface, component, subsystem, controller, pawn, game mode, replicated field, RPC, 핵심 data type을 찾는다.
   - Blueprint와 에셋 계층은 파일명, 경로, 발견 가능한 parent class, 기존 문서/코드 참조를 기준으로 식별한다.
   - DataAsset, DataTable, Curve, Map, Widget, gameplay asset처럼 런타임 구조에 영향을 주는 에셋을 찾는다.
   - 추측하기 전에 `rg`, `rg --files`, Unreal reflection macro, 기존 문서를 우선 사용한다.

3. 사실과 추론 분리:
   - 현재 C++ 코드와 config, 직접 확인한 에셋 정보를 우선한다. 기존 문서는 탐색의 출발점이며 현재 구현과 대조한다.
   - `.uasset` 파일명은 에셋 존재와 이름의 근거일 뿐, Blueprint graph 내부 동작의 증거로 취급하지 않는다.
   - Blueprint 내부를 확인할 수 없으면 “에셋 이름/경로로부터 추정” 또는 “소스만으로는 내부 graph 확인 불가”라고 표시한다.
   - graph node, event 순서, replicated 설정, asset reference를 확인 없이 단정하지 않는다.

4. 문서 작성 전에 다이어그램 분할:
   - 가장 먼저 작은 전체 지도 다이어그램을 만든다.
   - 그 다음 런타임 흐름, 블루프린트/C++ 경계, 네트워크/권한 경계, 주요 하위 시스템 단면을 별도 다이어그램으로 나눈다.
   - 모든 객체를 한 Mermaid에 넣지 않는다. 중요한 노드가 7개 안팎을 넘으면 다이어그램을 쪼갠다.
   - 각 다이어그램은 “누가 무엇을 소유하는가”, “누가 누구를 호출하는가”, “무엇이 네트워크를 건너는가”, “어떤 상태가 바뀌는가” 중 하나의 질문에 답해야 한다.

5. 개요 문서 작성:
   - 새 개요 문서를 만들거나 크게 다시 쓸 때 `references/overview-doc-template.md`를 읽는다.
   - Mermaid를 작성하거나 검토하기 전에 `references/mermaid-unreal-patterns.md`를 읽는다.
   - 다이어그램 주변에는 해당 단면이 왜 중요한지 짧게 설명한다.
   - 하위 시스템 세부사항은 반복해서 복붙하지 말고 기존 상세 문서로 연결한다.
   - 구현 anchor는 필요한 만큼만 넣고, 파일 목록 자체가 문서의 중심이 되지 않게 한다.

6. 검증:
   - 문서에 적은 class, asset path, doc path, symbol이 실제로 존재하는지 다시 확인한다.
   - Mermaid 문법이 그럴듯하고 다이어그램이 읽을 수 있을 만큼 작은지 확인한다.
   - Blueprint-only claim은 source나 기존 문서 근거가 없으면 추론으로 표시되어 있는지 확인한다.
   - 기존 개요를 갱신했다면 발견한 drift와 수정 내용을 요약한다.

---

## Unreal 계층 매핑 체크리스트

> 이 표는 전체 개요를 쓰기 전에 빠뜨리기 쉬운 계층을 확인하는 용도다.

| 계층 | 찾아야 할 것 | 대표 근거 |
| --- | --- | --- |
| Player/control | `APlayerController`, pawn possession, input component, input mapping context | `Source`, `Config`, Blueprint 에셋명 |
| Crew/cockpit | pawn-to-interface handle, seat assignment, command forwarding | `UINTERFACE`, `TScriptInterface`, replicated cockpit field |
| Gameplay actor | 중심 actor/pawn, child actor, actor component, tick/lifecycle | `UCLASS`, constructor subobject, `BeginPlay`, `Tick` |
| Data/assets | DataAsset, DataTable, Curve, Blueprint subclass, Widget | `Content/**/*.uasset`, spec, developer settings |
| Network | authority-only path, RPC, replicated property, prediction/smoothing | `Server`, `Client`, `NetMulticast`, `DOREPLIFETIME`, role check |
| Subsystems | World/GameInstance subsystem, manager, processor, registry | `Subsystem`, registration call, settings path |

---

## 다이어그램 분할 전략

Mermaid는 장식이 아니라 읽기 경로로 사용한다.

1. 전체 지도: 주요 런타임 역할과 시스템 그룹만 보여준다.
2. 런타임 순서도: 사용자/player 계층에서 gameplay system으로 들어가는 대표 command flow를 보여준다.
3. 블루프린트/C++ 경계: 어떤 Blueprint 에셋이 어떤 C++ 타입 위에 있으며, source-only 관찰의 한계가 어디인지 보여준다.
4. 네트워크/권한 흐름: 서버 소유 결정, client prediction, replication, callback을 보여준다.
5. 하위 시스템 단면: 동작을 안전하게 바꾸기 위해 꼭 이해해야 하는 시스템만 확장해서 보여준다.

Mermaid 문법과 Unreal 전용 패턴은 `references/mermaid-unreal-patterns.md`를 따른다.

---

## 저장 위치 정책

사용자가 위치를 지정하지 않았을 때:

1. 저장소의 기존 문서 root를 재사용한다. 보통 `Docs`를 우선한다.
2. 프로젝트 전체 개요는 `Docs/SystemOverview.md`를 선호한다.
3. 기능 영역 개요는 `Docs/<FeatureName>Overview.md` 또는 저장소의 기존 명명 규칙을 따른다.
4. 문서 root 후보가 여러 개이고 잘못 고르면 혼란이 클 때만 사용자에게 묻는다.

---

## 참조 문서 로드 기준

- 개요 문서를 새로 만들거나 다시 쓸 때 `references/overview-doc-template.md`를 읽는다.
- Mermaid 다이어그램을 작성하거나 품질을 검토할 때 `references/mermaid-unreal-patterns.md`를 읽는다.
