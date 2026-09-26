<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# 프로젝트 에이전트 지원 설치·제거

> 부분 갱신 일자: 2026-09-10 — owned_paths 로 검사·정리 대상 범위를 프로젝트 소유 코드로 한정. 외부 코드는 --include-external 로만 다룬다.

> 부분 갱신 일자: 2026-09-10 — 설치 기록을 추적 정책에서 분리해 Config/JWCommonUtilityTools.local.json 으로 이관. 일반 설치·제거는 프로젝트 정책 JSON을 더 이상 쓰지 않는다.

> 부분 갱신 일자: 2026-09-09 — Redirect PowerShell 도구 폐기에 따라 Python 전용 사용법으로 정리.

> 부분 갱신 일자: 2026-09-09 — Python 소스 도구 공통 제외 정책과 guard_logs 설정 문서화.

> 부분 갱신 일자: 2026-09-08 — Python 설치기, 소유 링크 기록, JSON 정책, 도구 지침 출력을 구현.

## 배포와 소유권

JWCommonUtility를 Git으로 clone하거나 복사한 뒤 [install_agent_support.py](../Tools/install_agent_support.py)를 실행한다. 플러그인은 스킬·검사기·공용 지침·사용 문서 원본을 소유하고, 프로젝트에는 원본을 가리키는 링크와 Config/JWCommonUtilityTools.json을 만든다.

설치기는 AGENTS.md, CLAUDE.md, 개인 홈 설정, .uproject, Git 설정을 수정하지 않는다. 에이전트나 Python을 다운로드·설치하지 않는다. 루트 지침 파일이 없는 새 프로젝트에서도 스킬과 도구는 JSON 위치를 공용 절차로 알고 있다.

| 프로젝트 연결 | 플러그인 원본 |
| --- | --- |
| .agents/skills/unreal-code-refine | Agent/Skills/unreal-code-refine |
| .agents/skills/unreal-comment-maker | Agent/Skills/unreal-comment-maker |
| .agents/skills/unreal-doc-writer | Agent/Skills/unreal-doc-writer |
| .agents/skills/unreal-overview-writer | Agent/Skills/unreal-overview-writer |
| .claude/skills/unreal-code-refine | Agent/Skills/unreal-code-refine |
| .claude/skills/unreal-comment-maker | Agent/Skills/unreal-comment-maker |
| .claude/skills/unreal-doc-writer | Agent/Skills/unreal-doc-writer |
| .claude/skills/unreal-overview-writer | Agent/Skills/unreal-overview-writer |
| Tools/JWCommonUtility | Tools |
| Docs/JWCommonUtility | Docs: 공용 지침과 사용 문서 |

스킬 디렉터리는 설치 시 검색하므로 새 스킬을 추가한 후 재실행하면 링크가 추가된다. 검사기는 원본의 Tools/jwcu_context.py를 함께 사용한다. 스킬 폴더만 별도로 복사하는 대신 플러그인 배포 구조를 유지한다.

문서 스킬 두 개는 참조 템플릿과 agents/openai.yaml을 함께 포함한다. unreal-doc-writer는 개별 시스템 문서의 작성·갱신·감사, unreal-overview-writer는 전체 관계와 온보딩 개요를 담당한다. 둘 다 프로젝트 JSON을 읽어 문서 정본과 갱신 규약을 찾는다. 플러그인 배포본은 이 경로가 정본이며, 사용자 홈의 기존 system-doc-writer/system-overview-writer는 설치·제거 대상이 아니다. unreal-project-runner는 현재 제공하지 않는다.

Codex의 .agents/skills 및 디렉터리 심볼릭 링크 지원은 [OpenAI 문서](https://learn.chatgpt.com/docs/build-skills#where-codex-loads-local-skills), Claude Code의 .claude/skills 및 링크 지원은 [Claude Code 문서](https://code.claude.com/docs/en/skills#where-skills-live)를 기준으로 한다. 이 설치기는 로컬 파일 검색을 연결하며 에이전트의 동작이나 보안 설정을 우회하지 않는다.

## 실행

Python 3.10 이상, 표준 라이브러리만 필요하다. macOS에서는 환경에 따라 python 대신 python3를 사용한다. 아래 명령은 호스트 루트에서 실행한다.

```sh
python Plugins/JWCommonUtility/Tools/install_agent_support.py install --dry-run
python Plugins/JWCommonUtility/Tools/install_agent_support.py install
python Plugins/JWCommonUtility/Tools/install_agent_support.py status
python Plugins/JWCommonUtility/Tools/install_agent_support.py uninstall --dry-run
python Plugins/JWCommonUtility/Tools/install_agent_support.py uninstall
```

자동 탐색은 현재 디렉터리와 플러그인 위치의 상위 .uproject를 확인한다. 두 위치가 서로 다른 프로젝트를 가리키거나 여러 .uproject가 있으면 추측하지 않는다. --project로 .uproject 파일 또는 정확히 하나의 .uproject가 있는 디렉터리를 지정한다.

```sh
python /path/to/JWCommonUtility/Tools/install_agent_support.py install --project /path/to/MyGame/MyGame.uproject
python Plugins/JWCommonUtility/Tools/install_agent_support.py install --agents codex
python Plugins/JWCommonUtility/Tools/install_agent_support.py install --agents none
```

기본 에이전트는 codex와 claude다. --agents none은 Tools/Docs 링크와 JSON만 제공한다. 재설치는 추가 방식이다. 이미 설치된 다른 에이전트 링크를 제거하지 않으며 구성 범위를 줄이려면 uninstall 후 원하는 옵션으로 install한다. --dry-run은 파일·디렉터리·잠금 파일도 만들지 않는다.

## 링크와 플랫폼

기본 --link-mode auto는 상대 디렉터리 심볼릭 링크를 만든다. 다른 Windows 볼륨 사이에서는 절대 링크를 사용한다. Windows에서 심볼릭 링크 권한 부족(오류 1314)이면 CPython의 _winapi.CreateJunction으로 디렉터리 junction을 만든다. 쉘이나 관리자 권한 요청을 설치기가 자동 실행하지 않는다.

--link-mode symlink 또는 Windows 전용 --link-mode junction으로 방식을 고정할 수 있다. junction 기능이 없는 Python 배포판이라면 Windows Developer Mode의 심볼릭 링크 지원을 사용한다. [Python의 Windows 심볼릭 링크 요구 사항](https://docs.python.org/3/library/os.html#os.symlink)을 참고한다.

상대 심볼릭 링크는 프로젝트와 플러그인을 같이 옮길 때 유리하다. junction은 절대 대상을 가지므로 프로젝트 이동 후 install을 재실행해 기록된 링크를 현재 원본으로 다시 연결한다. 외부 clone 자체의 위치를 바꾸었다면 기존 연결을 uninstall한 뒤 새 위치에서 install한다. 원본 위치가 사라져도 같은 버전의 설치기를 다른 위치에서 호출해 기록된 깨진 링크를 제거할 수 있다.

Git clone/checkout이 심볼릭 링크를 일반 텍스트 파일로 만든 경우 이를 자동 덮어쓰지 않는다. 생성 연결은 로컬 설치 산출물로 취급하고 저장소의 추적 정책을 정한다. 설치기는 .gitignore를 자동 편집하지 않는다.

설치 기록도 같은 로컬 산출물이므로 Config/JWCommonUtilityTools.local.json 에 따로 둔다. 링크와 기록이 같은 생애주기를 갖게 되어, 클론한 사람은 둘 다 없는 상태에서 install 한 번으로 시작한다. 호스트는 이 파일과 생성 링크를 함께 추적 제외한다.

```gitignore
/Config/JWCommonUtilityTools.local.json
/Tools/JWCommonUtility
/Docs/JWCommonUtility
```

Python 설치기·검사기는 Windows/macOS용 경로·링크 API를 사용한다. Unreal C++ 플러그인은 여전히 Win64 allow-list다. 설치기의 macOS 대응을 Unreal 모듈의 macOS 빌드 검증으로 해석하지 않는다.

## JSON 설정

기존 schema_version 1의 license_header/excluded_paths는 그대로 지원한다. 설치기가 생성하는 최소 기본값은 다음과 같다.

```json
{
  "schema_version": 1,
  "license_header": null,
  "excluded_paths": [],
  "agent": {
    "instructions": [],
    "guidance_files": []
  }
}
```

| 항목 | 의미 |
| --- | --- |
| schema_version | 현재 1. 알 수 없는 버전·키와 중복 JSON 키는 Python 도구에서 오류 |
| license_header | 주석 검사기의 C++ 첫 줄 기대 문구. null/생략이면 LICENSE만 생략 |
| owned_paths | 프로젝트가 소유한 경로 패턴 배열. 생략·빈 배열이면 모든 파일이 대상. 디렉터리 이름만 적으면 그 하위 전체가 범위 |
| excluded_paths | Python 주석·구조·코드 탐색·저작권 도구의 공통 제외 경로 패턴. 호스트 루트 기준, 슬래시 사용 |

owned_paths 는 소유 경계를, excluded_paths 는 소유 경로 안에서의 개별 예외를 정한다. 남이 만든 플러그인은 규약이 다르므로 owned_paths 밖에 두면 주석·구조·코드 탐색·저작권 도구가 모두 건너뛴다. 특히 update_copyright 는 남의 저작권 표기를 우리 것으로 덮어쓰지 않는다.

그 코드를 직접 손봐야 하는 경우에만 각 도구에 --include-external 을 붙여 범위를 넓힌다. 기본값은 항상 제외다.
| guard_logs | 코드 탐색의 로그 함수 추가·조건 제외 설정. [CodeReview.md](CodeReview.md) 참조 |
| agent.instructions | 작업에 필요한 프로젝트 지침 문자열 배열 |
| agent.guidance_files | 프로젝트 루트 내부의 기존 지침 파일 경로 배열. 없으면 빈 배열 |
| _copyright | 선택적 설정 파일 저작권 메타데이터 |
| _installation | [Deprecated 2026-09-10] 분리 이전의 설치 기록. 남아 있으면 다음 install·uninstall이 상태 파일로 옮기고 정책에서 제거한다 |

모듈·엔진 경로는 .uproject와 호스트 구조에서 찾는다. 설치 시 프로젝트 저작권자나 예외 파일을 임의로 채우지 않는다. 기존 정책 JSON이 있으면 설치·제거가 그 파일을 쓰지 않는다. 설치기가 정책 JSON을 건드리는 경우는 새로 만들 때와, 분리 이전 _installation 기록을 지울 때뿐이다.

## 설치 상태 파일

설치 기록은 Config/JWCommonUtilityTools.local.json 에 둔다. 설치기 전용 문서이며 사람이 편집하지 않는다.

기록에는 원본 위치, 링크별 상대 배치 경로·실제 링크 대상·종류, 설치기가 만든 디렉터리, 정책 JSON 최초 생성 여부·기본 정책 해시를 둔다. 이 기록이 있어야 설치기가 자기 링크와 사용자 파일을 구분하고, 자기가 만든 디렉터리만 정리하며, 실패한 작업을 되돌릴 수 있다. 기록을 손으로 지우면 다음 install이 남은 링크를 미등록 대상으로 보고 멈춘다.

상태 파일이 없고 정책에 _installation 이 남아 있으면 그 기록을 읽어 동작하고, install·uninstall 성공 시 상태 파일로 옮긴 뒤 정책에서 지운다. 이때만 정책 JSON이 다시 직렬화되므로 배열 줄바꿈 같은 서식이 한 번 바뀔 수 있다. status는 이 상태를 stderr 한 줄로 알린다.

## 도구만 실행할 때의 지침 전달

Python 검사기·저작권 도구는 stderr에 다음을 출력한다.

- [JWCU context]: 프로젝트 루트, 설정 파일, 공용 지침, 프로젝트 지침 문서, agent.instructions를 담은 한 줄 JSON.
- [JWCU guidance]: 정책과 관련 문서를 읽고 결과를 해석하도록 안내하는 짧은 공용 절차.

프로젝트 문자열의 개행·제어 문자는 JSON으로 이스케이프하여 진단 출력처럼 위장되지 않게 한다. 자유 지침이나 문서 내용을 코드로 실행하지 않는다. 이 안내는 읽는 에이전트에 문맥을 제공하며, 에이전트가 이를 채택한다는 보장은 아니다.

--no-context로 안내 출력만 끌 수 있다. 설정 검증과 주석 검사 정책은 계속 적용된다. stdout의 인벤토리·진단 데이터와 종료 코드는 지침 출력 때문에 달라지지 않는다.

```sh
python Tools/JWCommonUtility/jwcu_context.py --root .
python Tools/JWCommonUtility/detect_missing_comments.py --root . Source
python Plugins/JWCommonUtility/Agent/Skills/unreal-code-refine/scan_structure.py --inventory --no-context Source
```

## 제거와 실패 처리

- 대상과 유형이 설치 기록에 일치하는 링크만 제거한다. 실제 폴더·파일, 다른 대상의 링크, 미등록 링크는 덮어쓰거나 가져오지 않는다.
- 링크의 상위 경로나 Config가 symlink/junction이면 쓰기를 거부한다. 원본 디렉터리를 재귀 삭제하지 않는다.
- Config/.JWCommonUtilityTools.lock을 배타 생성해 동시 설치·제거를 막는다. 정상 종료 시 지우며 남은 잠금을 자동으로 빼앗지 않는다.
- 일반적인 실행 실패는 이번에 만든 링크를 되돌리고, 제거 실패는 이미 제거한 링크를 복원한다. JSON은 같은 디렉터리의 임시 파일에 기록한 뒤 원자 교체한다.
- 설치 기록과 실제 연결이 다르면 변경 전에 멈춘다. 상태를 확인하고 사용자 변경을 보존한 채 충돌을 해결한다.
- uninstall은 프로젝트 JSON을 건드리지 않고 상태 파일만 지운다. 설치기가 만든 빈 디렉터리만 rmdir로 정리하며 사용자 파일이 있으면 남긴다.
- --remove-config는 설치기가 새로 만든 JSON이 아직 기본 정책 그대로인 경우만 삭제한다. 기존 파일이거나 사용자 설정이 바뀌었으면 거부한다.

```sh
python Plugins/JWCommonUtility/Tools/install_agent_support.py uninstall --remove-config
```

전원 차단·강제 종료까지 원자적인 다중 파일 트랜잭션은 아니다. 잠금이나 미등록 연결이 남으면 실행 중인 설치기가 없는지 확인하고 status와 실제 대상을 검토한다. 상태 파일을 지워 강제 재설치하거나 실제 폴더를 지우는 복구는 하지 않는다.

## 검증

```sh
python -B Plugins/JWCommonUtility/Tools/Tests/test_installer.py
python -B Plugins/JWCommonUtility/Tools/Tests/test_tooling.py
```

임시 호스트에서 설치·재설치·제거·실패 복구, 기존 규약/정책 보존, 대상 변경·상위 링크·손상 기록 거부, 외부 clone·프로젝트 이동, 상대 심볼릭 링크와 Windows junction, 도구 stdout 보존을 검증한다. macOS에서도 같은 테스트를 실행할 수 있다. Windows 전용 검증은 다른 OS에서 skip된다.
