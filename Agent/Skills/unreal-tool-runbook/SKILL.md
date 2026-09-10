---
name: unreal-tool-runbook
description: JWCommonUtility 공용 도구와 스킬의 진입점. 어떤 도구를 어떤 순서로 쓸지 고를 때, 코드 검토·주석·저작권·구조·문서 작업의 시작점을 찾을 때, 설치 상태를 확인할 때 사용한다. 개별 작업의 상세 절차는 각 전용 스킬이 담당하므로 여기서는 라우팅과 공통 순서만 다룬다.
---

<!-- Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential. -->

# JWCommonUtility 도구 운영 순서

> 부분 갱신 일자: 2026-09-10 — 도구 라우팅·공통 순서·자주 틀리는 지점을 모은 진입점으로 신설.

> 부분 갱신 일자: 2026-09-10 — owned_paths 소유 범위와 --include-external 을 반영.

이 문서는 **무엇을 할지 정해졌을 때 어떤 도구로 시작할지**를 고르는 용도다. 개별 작업의 판단 기준은 각 전용 스킬과 [CodeReview.md](../../../Docs/CodeReview.md), [Tooling.md](../../../Docs/Tooling.md)에 있다. 링크로 발견했다면 이 문서의 상대 경로는 심볼릭 링크/junction을 해석한 플러그인 원본 디렉터리 기준으로 읽는다.

## 시작 전 확인

1. [공용 지침](../../../Docs/CommonGuidance.md)을 읽고 호스트 .uproject 루트를 확인한다. `Config/JWCommonUtilityTools.json`이 있으면 `agent.instructions`와 관련 `agent.guidance_files`를 함께 읽는다.
2. 호스트에 `AGENTS.md` 또는 `CLAUDE.md`가 있으면 그 규약도 읽는다. 스킬을 쓰려고 이 파일을 만들거나 덮어쓰지 않는다.
3. 도구가 링크로 보이지 않으면 설치 상태를 본다. 설치는 링크와 상태 파일만 만들고 에이전트 지침 파일은 건드리지 않는다.

```sh
python Plugins/JWCommonUtility/Tools/install_agent_support.py status
python Plugins/JWCommonUtility/Tools/install_agent_support.py install --dry-run
```

## 무엇을 하려는가 → 어디서 시작하는가

| 하려는 일 | 시작점 | 비고 |
| --- | --- | --- |
| 주석·설명이 빠졌는지 보고 채우기 | `unreal-comment-maker` 스킬, `check_comments.py` | ERROR 기반 종료 코드 |
| 저작권 헤더가 규약과 다른 파일 정리 | `update_copyright.py` | 유일한 쓰기 도구. 아래 함정 참고 |
| 큰 파일을 역할별로 나누기, 타입 헤더 추출 | `unreal-code-refine` 스킬, `scan_structure.py` | 동작 변경을 섞지 않는다 |
| Guard Clause 로그·전방 선언 검토 후보 찾기 | `check_code.py` | 후보는 확정이 아니다 |
| 개별 시스템 개발 문서 작성·갱신 | `unreal-doc-writer` 스킬 | 근거는 코드·Config·에셋 |
| 프로젝트 전체 구조 개요 작성 | `unreal-overview-writer` 스킬 | Mermaid 중심 |
| 도구·스킬 연결 설치·제거·상태 | `install_agent_support.py` | 링크와 상태 파일만 다룬다 |

## 공통 순서

검사 → 읽고 판단 → 적용 → 재검사 순서를 지킨다. 검사 결과만으로 코드를 고치지 않는다.

1. **좁게 검사한다.** 프로젝트 전체보다 작업 중인 시스템 경로부터 준다. 기본 대상은 `Source`, `Plugins`이며 `Plugins`는 잊기 쉬우니 한 번은 포함해 본다.
2. **후보를 읽는다.** 모든 검사기는 정규식·중괄호 개수 기반 추정이다. 해당 함수와 호출자를 읽고 판단한다.
3. **적용은 미리보기부터.** 쓰기 도구는 기본이 dry-run이다. 결과를 확인한 뒤에만 `--apply`를 붙인다.
4. **재검사로 닫는다.** 같은 명령을 다시 돌려 후보가 0이 되는지 본다.
5. **필요할 때만 빌드한다.** Python·문서만 바뀌었으면 Unreal 빌드는 요구하지 않는다. 모듈 구성이나 C++가 바뀌었으면 호스트 타깃을 빌드한다.

## 쓰기 경계

| 도구 | 기본 동작 |
| --- | --- |
| `check_comments.py`, `check_code.py`, `scan_structure.py` | 읽기 전용 |
| `update_copyright.py` | 미리보기. `--apply`일 때만 파일을 고친다 |
| `install_agent_support.py` | 링크와 설치 상태 파일을 만든다. `--dry-run`은 무변경 |

`cpp_review.py`는 실행 진입점이 아니라 `check_code.py`와 `scan_structure.py`가 함께 쓰는 분석 모듈이다.

## 자주 틀리는 지점

- **Epic 헤더는 `--old`/`--new`로 바뀌지 않는다.** 치환 정규식이 `Copyright (c) YYYY <이름>` 형태만 잡는다. `--normalize --replace-epic`으로 정규화한다.
- **헤더가 아예 없는 파일은 정규화 대상이 아니다.** `--add-missing`과 `--add-missing-under`로 삽입 범위를 명시해야 들어간다.
- **`excluded_paths`에 걸린 파일은 조용히 빠진다.** 검사 결과의 "검사 파일 N개"가 넘긴 파일 수와 맞는지 본다. 제외 사유가 사라졌으면 정책에서 지운다.
- **`owned_paths` 밖의 코드도 조용히 빠진다.** 외부 플러그인은 규약이 달라 기본적으로 모든 도구가 건너뛴다. 그 코드를 직접 고쳐야 할 때만 `--include-external` 을 붙인다.
- **종료 코드 의미가 도구마다 다르다.** 주석 감사는 ERROR가 있으면 실패로, `check_code.py`는 후보 수와 관계없이 0으로 끝난다. 입력·설정 오류만 2다.
- **로그를 남기는 매크로를 새로 만들면 `guard_logs.log_functions`에 등록한다.** 등록하지 않으면 그 매크로로 진단하는 guard clause가 계속 후보로 잡힌다. 화면 출력만 하는 매크로는 등록하지 않는다.
- **`--config`에 준 정책의 `agent.guidance_files`는 실제로 있어야 한다.** 임시 정책으로 검사할 때 이 항목을 비우지 않으면 입력 오류로 끝난다.

## 도구 자체를 고쳤을 때

플러그인의 Python을 고쳤으면 아래 3종을 모두 돌린다. 임시 호스트를 만들어 검증하므로 실제 프로젝트 파일을 건드리지 않는다.

```sh
python Plugins/JWCommonUtility/Tools/Tests/test_tooling.py
python Plugins/JWCommonUtility/Tools/Tests/test_installer.py
python Plugins/JWCommonUtility/Tools/Tests/test_code_review.py
```

새 스킬 폴더를 추가했으면 `install`을 다시 돌려 링크를 만들고 `status`로 확인한다.
