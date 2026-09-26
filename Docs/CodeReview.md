<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# C++ 검토 후보 탐색

> 부분 갱신 일자: 2026-09-10 — 같은 헤더가 주는 심볼 중 하나라도 정의를 요구하면 그 include 를 전방 선언 후보에서 제외. cpp_review.py 가 단독 실행 진입점이 아님을 명시.

> 부분 갱신 일자: 2026-09-09 — Guard Clause 로그와 전방 선언 후보를 공용 Python 분석기로 제공.

## 목적과 사용 순서

[check_code.py](../Tools/check_code.py)는 에이전트가 실제 코드를 읽기 시작할 위치와 근거를 제공한다. 후보가 있다는 사실은 버그·규약 위반의 확정이 아니며, 후보가 없다는 사실도 완전한 검증을 뜻하지 않는다. 소스·Config·에셋을 수정하는 옵션은 없다.

프로젝트 전체 목록보다 작업 중인 시스템 경로부터 좁혀 실행한다. 입력과 --config 상대 경로는 --root 기준이며 기본 설정은 상위 .uproject 호스트의 Config/JWCommonUtilityTools.json이다.

```sh
python Plugins/JWCommonUtility/Tools/check_code.py --checks guard-logs --limit 20 Source
python Plugins/JWCommonUtility/Tools/check_code.py --checks forward-declarations --json Source
python Plugins/JWCommonUtility/Tools/check_code.py --all-guards --json --limit 0 Source
```

- --checks: all(기본), guard-logs, forward-declarations.
- --limit: 출력 후보 수, 기본 100, 0이면 전체. 보고서에 전체 개수와 잘림 여부를 기록한다.
- --json: stdout에 candidate_only, files, total_candidates, truncated, findings, read_errors를 포함한 JSON을 출력한다.
- --no-context: stderr의 공용 지침 안내만 숨긴다. 설정 검증과 검사 정책은 그대로 적용한다.
- 정상 종료는 후보 수와 관계없이 0, 입력·설정·읽기 실패는 2다. ERROR 기반 주석 감사와 종료 코드 의미가 다르다.

텍스트와 JSON 모두 파일·줄·규칙·근거를 제공한다. 에이전트는 후보의 함수·호출자·인접 분기를 읽고, 실제 수정이 필요할 때 기존 프로젝트 규약에 맞게 판단한다. 검사 결과만으로 로그나 전방 선언을 자동 삽입하지 않는다.

## Guard Clause 로그 후보

GUARD_LOG_REVIEW는 if 분기에서 직접 return하는 경로 중, 반환 앞에 무조건 호출되는 등록 로그를 찾지 못한 곳이다. 기본 탐색은 부정 조건·nullptr 비교 또는 false/nullptr 반환을 사용한다. 이는 실패의 증명이 아니라 탐색 우선순위다. 단순 HasAuthority/IsLocallyControlled/IsLocalController 조건은 기본 제외한다. --all-guards는 긍정 조건·정상 권한 분기도 포함한다.

| 관측 | 처리 |
| --- | --- |
| 같은 분기의 직접 UE_LOG/UE_LOGFMT 호출 뒤 return | 후보에서 제외 |
| 프로젝트에 등록한 로그 함수·매크로 호출 | 같은 방식으로 인식 |
| 다른 분기·함수 진입 시점의 로그 | 해당 guard의 로그로 인정하지 않음 |
| UE_CLOG, ensure/check 계열, 중첩 조건 아래 로그 | nearby_diagnostics에 표시하고 검토 후보 유지 |
| 주석·일반 문자열·raw 문자열·매크로 정의 안의 if/return | 탐색에서 제외 |
| 로그를 남기는 헬퍼, 호출자 진단, 화면 메시지 | 자동 추적하지 않음. 에이전트가 확인 |

priority의 failure-review는 false/nullptr 반환, guard-review는 그 밖의 부정 조건, normal-path-review는 명시적으로 확장한 단순 권한 분기를 뜻한다. 후보에는 condition과 return_line을 함께 기록한다.

프로젝트 JSON의 선택적 설정:

```json
{
  "schema_version": 1,
  "excluded_paths": ["Source/MyModule/Generated/*"],
  "guard_logs": {
    "log_functions": ["MY_PROJECT_LOG", "Diagnostics::LogFailure"],
    "excluded_conditions": ["!bOptionalFeatureEnabled"]
  }
}
```

log_functions는 기본 UE_LOG/UE_LOGFMT에 추가되는 이름 목록이다. 정확한 C++ 식별자 또는 namespace로 한정한 이름을 사용한다. 조건부로만 로그를 남기는 매크로를 무조건 로그로 등록하지 않는다. excluded_conditions는 공백을 제거한 조건 전체가 일치할 때 제외하며 부분 문자열·정규식·실행 코드가 아니다. 판단 근거는 프로젝트 지침 문서에 남긴다.

## 전방 선언 후보

FORWARD_DECL_REVIEW는 include 파일명과 Unreal 타입 접두사로 관련 타입을 추정한 결과다. 포인터·참조와 TObjectPtr/TWeakObjectPtr/TSoftObjectPtr/TSubclassOf 사용을 후보로 수집한다. 값 사용·상속·직접적인 타입 정적 멤버 접근·감지한 변수의 인라인 멤버 호출·delete는 보수적으로 제외한다.

후보의 declaration은 제안이며 실제 선언 종류와 namespace를 읽어 확정한다. F 접두사에는 struct, 나머지에는 class를 추정하므로 실제 선언과 다를 수 있다. 해당 헤더가 함께 제공하는 다른 타입·매크로, 템플릿 인스턴스화, 간접 호출과 UHT 요구 사항도 확인해야 한다. include를 제거하는 변경을 수행했다면 cpp의 직접 include를 확인하고 해당 타깃의 컴파일/UHT로 검증한다.

같은 헤더가 제공하는 심볼 중 하나라도 완전한 정의를 요구하면(기반 클래스, 값 멤버, 인라인 사용 등) 그 include 는 어차피 남아야 하므로 후보로 올리지 않는다. 예를 들어 Components/ActorComponent.h 를 include 하고 UActorComponent 를 상속하는 컴포넌트 헤더는, 같은 헤더의 FActorComponentTickFunction 이 포인터로만 쓰여도 후보가 아니다.

[scan_structure.py](../Tools/scan_structure.py)의 include 후보도 같은 [cpp_review.py](../Tools/cpp_review.py)를 사용한다. 서로 다른 전방 선언 판정 로직을 유지하지 않는다. cpp_review.py 는 check_code.py 와 scan_structure.py 가 공유하는 분석 모듈이며 단독 실행 진입점이 아니다.

## 공통 설정과 한계

주석·구조·코드 탐색·저작권 도구는 모두 프로젝트 excluded_paths를 적용한다. 패턴은 호스트 루트 기준이며 대소문자를 구분하고 슬래시로 정규화한다. 생성물·ThirdParty 디렉터리와 symlink/junction 별칭을 순회하지 않는다. 모든 소스가 정책으로 제외되면 파일 수 0으로 정상 종료한다.

로그·전방 선언 탐색은 Python 표준 라이브러리만 사용한다. 전처리 조건의 활성 상태, 매크로 확장, 모든 C++ 구문, 완전한 제어 흐름·호출 그래프·타입 해석을 제공하지 않는다. 중첩 return만 있는 복잡한 분기나 사용자 래퍼 타입을 놓칠 수 있다. 예외·단락 평가 등으로 로그가 실제 실행되지 않는 상황까지 보증하지 않는다. 텍스트에 있는 로그 호출은 빌드의 로그 활성화 설정이나 실행 빈도의 적절함을 증명하지 않는다.

검증은 `python -B Plugins/JWCommonUtility/Tools/Tests/test_code_review.py`로 수행한다. 판정 후보·오탐 방지, CLI 호환, 제외 경로 공유, JSON 출력과 소스 무변경을 확인한다.
