---
name: unreal-comment-maker
description: Unreal C++ 헤더와 cpp의 설명 주석, 저작권 헤더, include 순서를 점검하고 호스트 프로젝트 규약에 맞게 정리한다. 주석 작성·정리·감사 요청에 사용한다.
---

<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# unreal-comment-maker

> 부분 갱신 일자: 2026-09-09 — 간이·정밀 주석 검사 통합과 누락 전용 모드 추가.

호스트 프로젝트의 C++ 주석을 일관되게 정리한다. .uproject 루트의 `Config/JWCommonUtilityTools.json`을 먼저 읽고 `agent.instructions`, 작업에 관련된 `agent.guidance_files`, [공용 지침](../../../Docs/CommonGuidance.md)을 적용한다. 기존 `AGENTS.md` 또는 `CLAUDE.md`가 있으면 해당 규약도 읽으며 새로 만들거나 덮어쓰지 않는다. 클래스 이름이나 경로로 기준 파일을 가정하지 않고 실제 대상 코드·사용처를 읽는다.

검사기의 정본은 [Tools/check_comments.py](../../../Tools/check_comments.py)이며 이 폴더의 [check_comments.py](check_comments.py)는 같은 실행기로 전달하는 호환 진입점이다. Python 3.10 이상과 표준 라이브러리만 필요하며 플러그인 공용 Tools/jwcu_context.py를 사용한다. 플러그인 구조를 유지하고 설치기가 만든 링크로 사용한다. 링크로 발견했다면 SkillDir과 이 문서의 상대 링크는 심볼릭 링크/junction을 해석한 플러그인 원본 디렉터리 기준으로 읽는다.

## 검사기 실행

공용 검사기의 --missing-only는 reflected 선언의 주석 누락만 검사한다. 기존 Tools/detect_missing_comments.py도 같은 모드를 사용한다. UENUM을 포함하며 //와 블록 주석을 인정한다. 제외 경로는 다른 Python 소스 도구와 같은 호스트 정책을 적용한다.

`<SkillDir>`은 이 SKILL.md가 있는 디렉터리, `<HostRoot>`는 호스트 프로젝트 루트다. 입력과 명시한 `--config`의 상대 경로는 `--root` 기준이며, `--root`를 생략하면 현재 작업 디렉터리 기준이다. 기본 정책은 이 루트에서 상위 .uproject를 찾아 해당 호스트의 Config를 읽는다.

```powershell
python "<SkillDir>/check_comments.py" --root "<HostRoot>" Source
python "<SkillDir>/check_comments.py" --root "<HostRoot>" --summary --limit 20 Source
python "<SkillDir>/check_comments.py" --root "<HostRoot>" --errors-only Source
```

입력 경로를 생략하면 루트의 Source와 Plugins를 검사한다. 생성물·ThirdParty 디렉터리는 순회에서 제외한다. 종료 코드는 정상 0, ERROR 발견 1, 잘못된 입력·설정·읽기 실패 2다.

stderr의 `[JWCU context]`와 `[JWCU guidance]`는 적용 정책·관련 문서·프로젝트 지침을 안내한다. `--no-context`는 안내 출력만 생략하며 실제 주석 정책은 계속 적용한다.

## 프로젝트 설정

루트의 `Config/JWCommonUtilityTools.json`을 자동으로 읽는다. `--config`로 다른 설정 파일을 지정할 수도 있다. 명시한 파일이 없거나 설정이 잘못되면 종료 코드 2로 중단한다.

```json
{
  "schema_version": 1,
  "license_header": "// Copyright (c) 2026 Example Studio. All rights reserved.",
  "excluded_paths": [
    "Source/Example/Private/Example.cpp",
    "Source/Example/Public/Example.h"
  ]
}
```

- `license_header`: 검사 대상 C++ 파일 첫 줄의 기대 문구. 생략하거나 null이면 LICENSE 검사만 생략하고 이를 출력한다. 도구 소스의 저작권자를 대상 프로젝트에 강제하지 않는다.
- `excluded_paths`: 루트 기준, 대소문자를 구분하는 슬래시 경로 패턴. `*`, `?`를 지원하며 Python fnmatch의 `*`는 슬래시도 포함한다. 모듈 부트스트랩 제외는 프로젝트가 직접 지정한다. 명시적으로 입력한 파일에도 적용한다.
- `schema_version`: 현재 1. 선택적 `_copyright`는 설정 파일 자체의 저작권 메타데이터다.
- `agent.instructions`: 프로젝트 작업 지침 문자열 배열. `agent.guidance_files`: 호스트 내부의 기존 지침 파일 상대 경로 배열. 둘 다 생략하면 빈 배열이다.
- `_installation`: 설치기가 관리하는 연결 소유 기록이다. 주석 검사 정책으로 사용하지 않는다. 자세한 계약은 [AgentSupport.md](../../../Docs/AgentSupport.md)에 있다.

설정 파일이 없어도 일반 검사는 실행된다. 파일 예외·저작권 외의 세부 문체와 프로젝트 기준 예제는 호스트 문서에서 읽는다. 검사기는 아래 공용 스타일을 기준으로 진단하므로 프로젝트 규약과 다른 WARN은 맹목적으로 수정하지 않는다.

## 진단 코드

| 코드 | 심각도 | 의미 |
| --- | --- | --- |
| LICENSE | ERROR | 설정한 첫 줄 저작권 문구와 다름 |
| PRAGMA | ERROR | pragma once가 없거나 include보다 늦음 |
| TYPE_DOC | ERROR | UCLASS/USTRUCT/UENUM/UINTERFACE 위 설명 없음 |
| MEMBER_DOC | ERROR | UPROPERTY/UFUNCTION 위 설명 없음 |
| ORDER | WARN | 전방 선언 뒤에 include가 있음 |
| SELF_INCLUDE | WARN | 대응 헤더가 있는데 cpp 첫 include가 다름 |
| CPP_DOC | WARN | cpp 함수 정의 위에 문서 주석이 있음 |
| VERBOSE | WARN | 설명 블록이 5줄 이상 |
| DOXYGEN | WARN | @param/@return 등의 나열 |

정규식 기반 검사이며 전처리 분기·매크로·C++ 구문 전체를 이해하지 않는다. 의미 정확성, 실제 한 줄 문체, generated.h의 마지막 include 조건은 별도로 확인한다. 역할별로 분할한 cpp처럼 같은 이름의 헤더가 없으면 SELF_INCLUDE를 생략한다.

## 작업 흐름

1. 사용자가 지정한 범위에서 진단한다. 파일이 크면 헤더의 기능 그룹 단위로 나눈다.
2. 타입과 멤버의 실제 역할·사용처를 읽고 설명한다. 이름만으로 의미를 추측하지 않는다.
3. 주석을 수정한다. 리팩터링·이름 변경·로직 수정은 섞지 않는다. include 위치 수정은 의존성과 프로젝트 규약을 확인한 경우에만 한다.
4. 같은 범위에 검사기를 다시 실행한다. 규약에 따른 예외와 의미가 불분명해 남긴 항목을 보고한다.

## 기본 작성 스타일

호스트의 명시적 규약이 우선이다. 별도 규약이 없으면 다음을 출발점으로 사용한다.

- 헤더는 저작권(정책이 있으면) → pragma once → include → 전방 선언 → 타입 본문 순서다. generated.h는 마지막 include다.
- 타입 설명은 맡은 역할을 한 문장으로, reflected 멤버 설명은 한 줄 `/** … */`로 쓴다. 순수 virtual override와 비리플렉션 헬퍼에 일괄 주석을 추가하지 않는다.
- 함수 선언의 설명을 cpp 정의 위에서 반복하지 않는다. cpp 내부 주석은 계산의 이유, 상수의 근거, 순서 제약을 설명한다.
- 대부분 1~2줄이면 충분하다. 복잡한 네트워크·수명·엔진 제약은 필요한 만큼 설명한다.
- 인자 이름을 되풀이하는 @param 나열, 코드 평가, 요청하지 않은 TODO를 추가하지 않는다.
- 한국어/영어 문체, 문장 종결, region 사용, 기준 파일은 호스트 규약에 맞춘다.
