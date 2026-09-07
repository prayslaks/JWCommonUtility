---
name: unreal-code-refine
description: Unreal C++ 파일을 역할별 cpp 분할, 독립 타입 헤더 추출, CVar·로그 소유권 정리, include 축소로 리팩터링한다. 큰 C++ 파일의 구조 정리에 사용하며 프로젝트 폴더나 에이전트 설치 정리에는 사용하지 않는다.
---

<!-- Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential. -->

# unreal-code-refine

> 부분 갱신 일자: 2026-09-09 — 구조 검사기 공용화와 코드 검토 후보 연결.

Unreal C++의 동작을 유지하면서 파일 구조를 정리한다. 클래스 접두사, 모듈, 엔진 설치 경로, 저작권, 분할 사례는 호스트 프로젝트의 규약에서 읽는다. 구현 정본은 [Tools/scan_structure.py](../../../Tools/scan_structure.py)이며 이 폴더의 [scan_structure.py](scan_structure.py)는 호환 진입점이다. 플러그인 전체 구조를 유지하고 설치기가 만든 링크로 사용한다. 링크로 발견했다면 SkillDir과 이 문서의 상대 링크는 심볼릭 링크/junction을 해석한 플러그인 원본 디렉터리 기준으로 읽는다.

## 작업 기준

- [공용 지침](../../../Docs/CommonGuidance.md)을 읽고 호스트 .uproject의 루트를 확인한다. `Config/JWCommonUtilityTools.json`이 있으면 `agent.instructions`와 현재 작업에 관련된 `agent.guidance_files`를 함께 읽는다. 설정이 없으면 공용 지침을 기본으로 사용한다.
- 호스트의 기존 `AGENTS.md` 또는 `CLAUDE.md`가 있으면 관련 지침도 읽는다. 스킬 사용을 위해 이 파일을 새로 만들거나 덮어쓸 필요는 없다.
- 사용자 지정 파일과 짝 헤더/cpp가 기본 범위다. 후보 탐색 요청이면 진단 결과만 보고한다.
- 구조 변경에 로직 수정, 이름 변경, 최적화, 버그 수정을 섞지 않는다. 발견한 문제는 별도 항목으로 보고한다.
- 자동 진단은 정규식과 중괄호 개수에 기반한 후보 목록이다. 문자열·매크로·템플릿·다중 행 선언을 완전히 파싱하지 못한다. 코드를 읽고 판단한다.

## 진단과 계획

아래 `<SkillDir>`은 이 SKILL.md가 있는 디렉터리, `<HostRoot>`는 대상 프로젝트 루트다. 입력 상대 경로는 `--root` 기준이며 생략하면 현재 작업 디렉터리 기준이다.

```powershell
python "<SkillDir>/scan_structure.py" --root "<HostRoot>" Source
python "<SkillDir>/scan_structure.py" --root "<HostRoot>" --inventory Source
```

도구는 stderr에 `[JWCU context]`와 `[JWCU guidance]`로 정책·문서·프로젝트 지침을 안내한다. 인벤토리 stdout은 그대로 유지된다. `--no-context`로 안내 출력만 생략할 수 있으며, 이 옵션이 프로젝트 정책을 해제하지는 않는다. [설치와 설정 계약](../../../Docs/AgentSupport.md)을 참고한다.

프로젝트 excluded_paths는 인벤토리와 구조 진단 모두에 적용된다. 전방 선언 후보는 [check_code.py](../../../Tools/check_code.py)의 분석 로직과 공유한다. Guard Clause 로그 검토를 요청받았거나 변경에 관련된 진단 경로를 조사할 때는 같은 도구의 --checks guard-logs를 사용한다. 후보의 조건·반환 위치를 출발점으로 실제 코드와 호출자를 읽고, 모든 early return에 로그를 추가하지 않는다. [후보 탐색의 사용법과 한계](../../../Docs/CodeReview.md)를 따른다.

파일별 cpp 줄 수·멤버 함수 수, 구조체 크기·프로퍼티·메서드, 역할 버킷, CVar·로그·namespace 위치와 include 후보를 확인한다. 기본 분할 임계치는 cpp 800줄 또는 멤버 함수 60개다. 무거운 구조체 후보는 40줄, UPROPERTY 6개, 메서드 보유 중 하나이며 UCLASS와 동거할 때 추출을 검토한다. 임계치는 스크립트 상단 상수다.

실행 전에 새 파일별 함수 목록과 예상 크기, 분할 축의 이유를 정리한다. 사용자에게 이미 승인된 범위와 분할 방향이 있으면 진행한다. 동작 변경이 필요하거나 범위를 결정할 핵심 정보가 없을 때만 확인한다.

## 분할 판단

| 유형 | 분할 축 |
| --- | --- |
| Processor·Subsystem | 실제 Tick 파이프라인 단계 |
| Pawn·Actor·Component | 이동·카메라·장비 등 상태와 책임을 공유하는 기능 영역 |
| 여러 소유자가 쓰는 CVar·로그 | 한 클래스 이름이 아닌 공유 도메인 |

Core 버킷의 단어 빈도는 겹치는 개념을 별개 축으로 세기도 한다. 함수 이름만으로 축을 확정하지 않는다. 조각 하나가 원본 대부분을 차지하거나 지나치게 많은 조각이 필요하면 축을 재검토한다. 상태 자체를 나눠야 한다면 파일 분할을 넘어서는 클래스 설계 변경이다.

## 실행 순서와 불변 조건

1. **공유 헬퍼와 상태를 확인한다.** Unity 빌드에서 파일 로컬 이름이 충돌하는지 확인한다. 호스트가 named namespace + inline 헤더 패턴을 쓰면 그 규약을 따른다. 공유할 헬퍼는 클래스 전용이면 Private 내부 헤더, 도메인 공용이면 적절한 Public 헤더에 둔다. static 변수는 이동·복제하면 저장소와 수명이 바뀔 수 있으므로 기계적으로 복사하지 않는다.
2. **CVar와 로그의 정의 소유자를 고정한다.** 공유 선언과 단일 정의를 구분한다. `DEFINE_LOG_CATEGORY_STATIC`을 여러 조각에 복제하지 않는다. 호스트가 이미 inline CVar 패턴을 사용하는 경우 기존 방식에 맞춘다.
3. **독립 USTRUCT를 Types 헤더로 추출한다.** 원본 클래스 선언은 하나의 헤더에 유지한다. 값으로 쓰는 구조체는 새 타입 헤더를 include해야 한다. 이미 `*Types.h`인 파일은 구조체가 많다는 이유만으로 나누지 않는다. 타입 참조와 include 사용처를 함께 갱신한다.
4. **cpp를 함수 단위로 옮긴다.** 함수에 붙은 의미 있는 주석·단계 표식도 같이 이동한다. 생성자·Tick·공개 진입점 등 골격은 원본 cpp에 남긴다. 새 조각 이름은 `Owner_Role.cpp` 형태로 호스트 규약을 따른다.
5. **include를 정리한다.** 각 cpp 조각의 첫 include는 원본 클래스 헤더다. 조각마다 필요한 include를 판단하고, 원본 목록 전체를 복사하지 않는다. 헤더에서 포인터·참조로만 쓰는 타입은 전방 선언 후보지만 값 멤버·기저 클래스·컨테이너·인라인 본문 사용을 직접 확인한다.

새 타입 헤더의 `.generated.h`는 마지막 include다. 클래스 헤더의 region을 사용하는 프로젝트에서는 region과 cpp 조각의 대응을 유지한다. 기존 모듈 내부 cpp 분할에 불필요한 Build.cs 변경을 넣지 않는다. 모듈 경계를 넘는 타입 이동은 별도 의존성 검토가 필요하다.

## 검증과 마무리

- 작업 전후 동일한 **범위 전체**의 `--inventory` 출력을 비교한다. 새 cpp 조각도 반드시 포함한다. 이 출력은 함수 이름·줄 수만 비교하므로 동일 길이의 본문 수정, 자유 함수, 멤버 상태 변경은 증명하지 못한다.
- Git diff의 이동 코드·include·전처리 조건과 공유 상태를 검토해 동작 변경이 없는지 확인한다.
- 호스트의 .uproject, Target.cs, 엔진 설정과 빌드 지침으로 검증 명령을 정한다. 엔진 버전·절대 설치 경로·Game/Editor 타깃을 고정하지 않는다. 변경이 Editor 모듈에 걸리면 Game 빌드만으로 검증했다고 하지 않는다.
- Live Coding 잠금으로 실패하면 호스트 지침에 맞춰 처리한다. 구조체·리플렉션 변경에는 UHT 검증도 필요하다.
- 새 파일에는 호스트의 저작권과 주석 규약을 적용한다. 함께 배포된 `unreal-comment-maker`가 있으면 해당 스킬과 검사기를 사용하고, 없으면 호스트 규약으로 직접 점검한다.
- 파일 수와 분할 역할, 검증 결과, 별도 발견 문제를 보고한다. 주석 검사기의 종료 코드 0은 동작 동등성의 증명이 아니다.
