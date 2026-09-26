---
name: unreal-doc-writer
description: Unreal 프로젝트의 개별 시스템 개발 문서를 한글로 생성·갱신하고 현재 코드·Config·에셋 근거와의 불일치를 점검한다. 런타임·에디터 동작, 데이터 구조, 사용법, 확장·검증·유지보수 절차를 문서화할 때 사용한다. 프로젝트 전체 관계도와 온보딩 개요는 unreal-overview-writer가 담당한다.
---

<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->


# Unreal 시스템 문서 작성기

이 스킬은 Unreal 프로젝트에서 구현된 시스템 또는 계획된 시스템을 프로젝트 폴더 안의 읽을 수 있는 개발 문서로 정리하고, 기존 시스템 문서가 현재 구현과 어긋나지 않도록 유지할 때 사용한다.

> 기본 원칙: 사용자가 다른 언어를 명시하지 않으면 산출 개발 문서는 한글로 작성한다. 코드 식별자, 파일 경로, 명령어, config key, JSON key, Unreal reflection macro, API 이름은 원문을 유지한다.

---

## 호스트 설정과 공용 원본

- 이 스킬과 참조 템플릿의 정본은 JWCommonUtility 플러그인이다. 링크로 발견했다면 이 문서의 상대 경로는 심볼릭 링크/junction을 해석한 원본 스킬 디렉터리 기준으로 읽는다.
- [공용 지침](../../../Docs/CommonGuidance.md)을 읽고 작업 대상 .uproject 루트의 `Config/JWCommonUtilityTools.json`이 있으면 읽는다. `agent.instructions`와 현재 작업에 관련된 `agent.guidance_files`에서 문서 위치·정본·갱신 규약을 찾는다. JSON이 없으면 기존 프로젝트 문서 구조와 아래 기본값을 사용한다.
- 기존 AGENTS.md/CLAUDE.md가 있으면 그 규약도 따른다. 이 스킬을 연결하기 위해 진입점 파일을 생성하거나 덮어쓰지 않는다.
- 생성·갱신할 문서는 대상 시스템을 소유하는 프로젝트 또는 플러그인에 둔다. 공용 스킬·템플릿 안에 호스트의 클래스 이름·엔진 절대 경로·아키텍처를 기록하지 않는다.

## 핵심 작성 흐름

1. 문서화 대상 결정:
   - 시스템 이름, 독자, 사용 목적을 파악한다. 예: 온보딩, 인수인계, 아키텍처 참조, 사용 가이드, 유지보수 가이드, 의사결정 기록.
   - 사용자가 저장 위치를 지정하면 그 위치를 사용한다.
   - 지정이 없으면 `Docs`, `docs`, `Documentation`, `docs/dev`, `Source/*/Docs` 같은 기존 문서 root를 찾는다.
   - 문서 규칙이 없으면 프로젝트 root 아래 `Docs/Systems/<SystemName>.md`를 기본값으로 사용한다.

2. 생성, 갱신, 감사 중 무엇인지 결정:
   - 문서가 없으면 source context에서 새 문서를 만든다.
   - 문서가 있으면 편집 전에 drift audit을 먼저 수행한다.
   - 사용자가 문서가 stale인지 묻는 경우, 명시적이거나 강하게 암시된 경우가 아니면 먼저 findings를 보고한다.

3. source context 수집:
   - 관련 코드, config, asset, data file, test, build file, 최근 대화 맥락을 조사한다.
   - 추론보다 실제 file reference를 우선한다. 기존 문서의 주장도 현재 소스와 대조한다.
   - .uasset 파일명만으로 Blueprint graph, parent class, replication 설정을 단정하지 않는다. 에디터나 추출 자료에서 확인하지 못한 내용은 추론 또는 확인 불가로 표시한다.
   - 계획된 시스템은 구현된 동작과 구분하고 아직 존재하지 않는 API·경로를 실제 구현처럼 쓰지 않는다.
   - 큰 시스템은 먼저 entry point를 맵핑하고, 동작 설명에 필요한 경로만 따라간다.

4. 기존 문서를 갱신할 때 drift audit 수행:
   - 문서가 언급하는 implementation anchor를 추출한다. 예: file path, class name, function, struct, reflected property, enum value, JSON key, config key, command, menu path, asset path, validation rule.
   - 참조된 파일이 아직 존재하는지 확인한다. 없으면 stale로 표시하기 전에 이동/이름 변경 후보를 검색한다.
   - 문서화된 public API, schema, editor path, runtime flow, validation rule, extension point를 현재 source와 비교한다.
   - 새로 생겼지만 문서에 빠진 source file, type, field, loader, validator, asset, workflow step이 있는지 확인한다.
   - 가능하면 `git status`, `git diff`, `git log -- <source paths> <doc path>`로 최근 변경에 집중한다. git이 막히면 직접 재스캔한다.
   - mismatch는 `stale`, `missing`, `renamed/moved`, `ambiguous`, `still accurate` 중 하나로 분류한다.
   - 유용한 기존 설명은 보존한다. 전체 재작성보다 stale section patch를 우선한다.

5. 문서 형태 선택:
   - 표준 시스템 문서 구조가 필요하면 `references/system-doc-template.md`를 사용한다.
   - 가치가 없는 섹션은 생략한다.
   - editor workflow, runtime behavior, data schema, networking, tooling, validation concern이 중요하면 프로젝트별 섹션을 추가한다.

6. 미래 개발자를 위해 작성:
   - 파일 이름보다 개념과 목적을 먼저 설명한다.
   - mental model을 설명한다. 무엇이 무엇을 소유하는지, 무엇이 어디로 흐르는지, 무엇을 안전하게 바꿀 수 있는지 드러낸다.
   - 사용 절차는 구체적이고 재현 가능하게 쓴다.
   - 중요한 implementation anchor의 file path를 포함한다.
   - “사용 방법”과 “동작 원리”를 분리한다.
   - invariant, failure mode, extension point를 명시한다.
   - 산문보다 다이어그램이나 표가 이해를 줄이면 Mermaid와 표를 사용한다.

7. 저장 및 검증:
   - 선택한 프로젝트 로컬 docs 영역에 Markdown 문서를 생성하거나 갱신한다.
   - 기존 문서를 편집할 때는 유용한 내용을 보존하고 stale한 부분을 갱신한다.
   - link와 file path가 실제 프로젝트 파일을 가리키는지 가능한 한 확인한다.
   - refresh 작업이면 발견한 주요 drift와 수정 내용을 요약한다.
   - 문서 저장 위치와 다룬 시스템 표면을 요약한다.

---

## 유지보수 흐름

코드나 수동 변경 후 기존 시스템 문서를 갱신, 감사, 검증, 복구, refresh, stale check할 때 이 흐름을 사용한다.

1. 문서와 source surface 찾기:
   - 사용자가 문서를 지정했다면 그 문서를 우선한다.
   - source folder만 지정했다면 문서 root에서 해당 시스템을 언급하는 문서를 찾는다.
   - 후보 문서가 여러 개이면 canonical page를 식별할 만큼만 조사한다. 잘못 고르면 문제가 될 때만 사용자에게 묻는다.

2. 압축된 source map 만들기:
   - 관련 source file을 public API, private implementation, editor/tooling, data/config, test, asset으로 나눠 본다.
   - 현재 entry point, owned data type, serialized schema field, validation rule, runtime/editor workflow를 식별한다.
   - Unreal 프로젝트에서는 `UCLASS`, `USTRUCT`, `UENUM`, `UFUNCTION`, `UPROPERTY`, module boundary, editor-only class, asset path, Blueprint-facing API에 특히 주의한다.

3. 압축된 document map 만들기:
   - heading과 각 section의 claim을 기록한다.
   - 문서가 언급하는 path, symbol, command, menu path, schema field, invariant를 기록한다.
   - mojibake, 부분 손상, 지나치게 모호한 설명, 누락된 유지보수 정보가 있는지 확인한다.

4. 비교하고 결정:
   - source가 명확하면 factual drift를 직접 갱신한다.
   - 저장소에서 해결할 수 없는 불확실성만 문서에 불확실하다고 표시한다.
   - maintainer에게 도움이 되지 않는 오래된 구현 세부사항은 제거한다.
   - 새 동작이 사용, debug, extension 방식에 영향을 줄 때만 새 section을 추가한다.
   - 문서를 raw inventory로 만들지 않는다. 읽히는 구조를 유지한다.

5. 결과 검증:
   - 변경한 file path와 중요 symbol을 다시 확인한다.
   - 편집한 문서에 내부 모순이 없는지 다시 읽는다.
   - 가능하면 문서 변경과 관련된 가벼운 validation command를 실행한다.
   - 최종 응답에서는 도움이 될 때 `drift found`, `doc updates`, `verification`을 분리한다.

---

## 문서 작성 기준

- 예측 가능한 Markdown heading 계층을 사용한다.
- 짧은 문단과 집중된 목록을 사용한다.
- schema, command, state, menu path, API list에는 표를 사용한다.
- 시스템에 의미 있는 flow, lifecycle, ownership 관계가 있으면 Mermaid 다이어그램을 사용한다.
- command name, menu path, JSON key, class name, file path는 정확히 쓴다.
- 불확실하거나 추론한 정보는 명시적으로 표시한다.
- maintenance update에서는 오래된 설명보다 현재 source truth를 우선한다. 다만 migration이나 compatibility 이해에 도움이 되는 historical note는 보존한다.
- marketing language, 모호한 요약, 긴 code dump를 피한다.
- 모든 구현 세부사항을 문서화하지 않는다. maintainer가 reasoning, use, debug, extension을 할 때 필요한 것을 문서화한다.

---

## 문서 어긋남 신호

다음 신호가 보이면 더 깊은 갱신 pass를 수행한다.

- 참조된 path가 더 이상 존재하지 않거나, 새 인접 source file이 문서에 빠져 있다.
- public type, reflected property, JSON/config field, enum value, Blueprint-facing API가 문서와 다르다.
- runtime flow, editor workflow, menu path, save location, load order, validation severity, failure handling이 바뀌었다.
- test, sample data, build module, plugin dependency, asset location이 바뀌었다.
- 문서에 mojibake, 깨진 Markdown table, stale TODO, 모순된 claim이 있다.
- 최근 git history에서 source 변경이 문서의 마지막 의미 있는 갱신보다 새롭다.

---

## 저장 위치 정책

위치가 지정되지 않았을 때:

1. 기존 프로젝트 문서 규칙을 재사용한다.
2. 흩어진 note보다 system-level docs를 선호한다.
3. 안정적인 파일 이름을 사용한다.
   - `Docs/Systems/<SystemName>.md`
   - `Documentation/Systems/<SystemName>.md`
   - 기존 프로젝트의 동등한 위치

문서 root 후보가 여러 개이고 잘못 고르면 혼란이 클 때만 사용자에게 묻는다.

---

## 템플릿 로드 기준

새 문서를 작성하거나, 큰 재작성을 하거나, 사용자가 특히 읽기 좋고 완결된 구조를 원할 때 `references/system-doc-template.md`를 읽는다. 아주 작은 갱신에는 전체 템플릿을 로드하지 말고 위의 workflow를 사용한다.
