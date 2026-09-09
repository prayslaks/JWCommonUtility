<!-- Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential. -->

# JWCommonUtility 도구와 에이전트 스킬

> 부분 갱신 일자: 2026-09-10 — 도구 선택과 공통 순서를 담은 unreal-tool-runbook 스킬을 배포 목록에 추가.

> 부분 갱신 일자: 2026-09-10 — 설치 기록을 추적 정책에서 분리해 Config/JWCommonUtilityTools.local.json 으로 이관. 일반 설치·제거는 프로젝트 정책 JSON을 더 이상 쓰지 않는다.

> 부분 갱신 일자: 2026-09-09 — Runtime 모듈에 범용 개발자 경고 매크로(`JWCU_CHECK_NULLPTR`, `JWCU_WARN_NO_IMPLEMENT`, `JWCU_WARN_SHOULD_NO_CALL`, `JWCU_SCREEN_MESSAGE`, `JWCU_VAR_NAME_TEXT`)를 추가. 호스트는 자기 접두사 별칭 헤더로 감싸 쓴다.

> 부분 갱신 일자: 2026-09-09 — Redirect 도구군·PowerShell 의존성·전용 Editor Commandlet을 폐기. 개발 도구는 Python 진입점만 제공.

> 부분 갱신 일자: 2026-09-09 — 주석 검사 통합, 구조 검사기 공용화, Python 도구의 제외 정책 통일과 코드 검토 후보 탐색 추가. [Deprecated 2026-09-09] 당시 유지하던 Redirect PowerShell 도구는 이후 폐기.

> 부분 갱신 일자: 2026-09-08 — unreal-doc-writer와 unreal-overview-writer를 참조 템플릿·메타데이터와 함께 배포.

> 부분 갱신 일자: 2026-09-08 — Python 설치·제거, 링크 기반 에이전트 연결, JSON 정책과 도구 출력의 안내 경로를 구현.

> 부분 갱신 일자: 2026-09-07 — 공용 도구·문서·스킬을 플러그인에 모으고 호스트 프로젝트 설정을 분리.

## 목적과 배포 단위

이 플러그인은 Unreal Runtime 모듈과 함께 개발 도구, 사용 문서, 에이전트 스킬 원본을 보관한다. 호스트 게임의 소스·설정·개인 에이전트 설정을 공용 원본에 복사하지 않는다.

```text
JWCommonUtility/
├─ Source/
├─ Resources/
├─ Tools/
│  ├─ install_agent_support.py
│  ├─ jwcu_context.py
│  ├─ check_comments.py
│  ├─ scan_structure.py
│  ├─ check_code.py
│  ├─ cpp_review.py
│  ├─ update_copyright.py
│  └─ detect_missing_comments.py
├─ Agent/Skills/
│  ├─ unreal-code-refine/       SKILL.md + scan_structure.py 호환 진입점
│  ├─ unreal-comment-maker/     SKILL.md + check_comments.py 호환 진입점
│  ├─ unreal-doc-writer/        SKILL.md + references + agents
│  ├─ unreal-overview-writer/   SKILL.md + references + agents
│  └─ unreal-tool-runbook/      SKILL.md
└─ Docs/
   ├─ Tooling.md
   ├─ AgentSupport.md
   ├─ CommonGuidance.md
   └─ CodeReview.md
```

Python 도구는 Python 3.10 이상과 표준 라이브러리만 사용한다. 스킬 실행기는 플러그인의 Tools/jwcu_context.py를 함께 사용하므로 플러그인 전체를 배포 단위로 유지한다. 별도 게임 코드나 사용자 홈 스킬을 요구하지 않는다.

배포할 때 이 소스·문서·Agent·Tools와 uplugin을 함께 보존한다. Binaries/Intermediate는 호스트 엔진에 맞춰 다시 생성한다. Unreal 패키징 산출물에 Agent/Tools/Docs가 포함되는 별도 배포 자동화는 아직 구성하지 않았다.

## Runtime 디버그 매크로

`Source/JWCommonUtility/Public/JWCommonUtility.h` 는 호스트 프로젝트가 접두사만 바꿔 쓸 수 있는 범용 디버그 매크로를 제공한다.

| 매크로 | 동작 |
| --- | --- |
| `JWCU_CALL_INFO` | `함수명(줄번호)` 문자열 |
| `JWCU_VAR_NAME_TEXT(var)` | 변수 이름을 `TEXT` 리터럴로 |
| `JWCU_PRINT_LOG(cat, ver, fmt, ...)` | 호출 위치를 붙인 `UE_LOG` |
| `JWCU_SCREEN_DEBUG(Key, Duration, Color, fmt, ...)` | `JWCU.DebugScreen` 콘솔 변수로 켜는 온스크린 출력 |
| `JWCU_SCREEN_MESSAGE(tme, clr, fmt, ...)` | 호출 위치를 붙인 온스크린 출력. CVar 게이트 없음 |
| `JWCU_CHECK_NULLPTR(var)` | 널이면 `JWCULog` 경고 + 온스크린 경고 |
| `JWCU_WARN_NO_IMPLEMENT()` / `_STRING()` | 미구현 함수 표시 |
| `JWCU_WARN_SHOULD_NO_CALL()` | 호출되면 안 되는 함수 표시 |
| `JWCU_SCOPE_WALL()` | 범위 진입·이탈 로그 벽 |

공통 계약:

- 화면 출력은 `GEngine` 이 유효할 때만 수행하고, `UE_BUILD_SHIPPING` 에서는 전처리 단계에서 제거된다. 경고 매크로의 `UE_LOG` 는 남는다.
- 문장 매크로는 모두 `do { } while (0)` 이므로 `if`/`else` 한 줄에 그대로 쓸 수 있다.
- 호스트는 자기 접두사 별칭 헤더 한 장으로 감싸는 방식을 권장한다. 예: ProjectZK 는 `Source/ProjectZK/_Core/Public/ProjectZK.h` 에서 `#define ZK_CHECK_NULLPTR(var) JWCU_CHECK_NULLPTR(var)` 형태로 이어 붙인다.
- 별칭 헤더가 호스트 모듈의 **공개** 헤더라면 `JWCommonUtility` 를 `PublicDependencyModuleNames` 에 넣는다. Private 의존이면 그 모듈을 의존하는 다른 모듈이 include 경로를 얻지 못해 빌드가 깨진다.

## 경로 계약

- Python 읽기 도구의 --root 기본값은 현재 작업 디렉터리다. 입력 경로와 명시한 --config 상대 경로는 --root 기준이다. 기본 정책은 --root에서 상위 .uproject를 찾아 해당 호스트의 Config/JWCommonUtilityTools.json을 읽는다. 스크립트 설치 위치에서 검사 대상을 추측하지 않는다.
- update_copyright.py는 대상 혼동을 막기 위해 --root와 --old, --new를 명시해야 한다.
- 검사기의 호스트별 저작권·제외 파일은 호스트 Config/JWCommonUtilityTools.json에 둔다. 플러그인 안에 특정 호스트의 설정 파일을 넣지 않는다.

다음 예는 플러그인을 호스트의 Plugins/JWCommonUtility에 놓고 호스트 루트에서 실행하는 경우다. 폴더 배치가 다르면 스크립트 경로를 실제 위치로 바꾼다.

## 공용 도구

| 도구 | 목적 | 기본 쓰기 동작 |
| --- | --- | --- |
| [check_comments.py](../Tools/check_comments.py) | 주석·헤더 스타일 감사 정본. 스킬 폴더의 동명 스크립트도 이 실행기로 전달 | 읽기 전용 |
| [install_agent_support.py](../Tools/install_agent_support.py) | 설치·제거·상태 확인 | 링크와 프로젝트 JSON 관리. --dry-run은 무변경 |
| [update_copyright.py](../Tools/update_copyright.py) | 저작권자 치환·선택적 코드 헤더 정규화 | 미리보기. --apply일 때만 변경 |
| [detect_missing_comments.py](../Tools/detect_missing_comments.py) | check_comments.py의 누락 전용 모드로 전달하는 호환 명령 | 읽기 전용 |
| [scan_structure.py](../Tools/scan_structure.py) | 구조 진단·함수 인벤토리·전방 선언 후보의 공용 진입점 | 읽기 전용 |
| [check_code.py](../Tools/check_code.py) | Guard Clause 로그·전방 선언 검토 시작점 탐색. 텍스트/JSON 출력 | 읽기 전용, 후보 수에 관계없이 정상 0 |

Redirect 생성·감사·제거 도구와 전용 Editor 모듈은 폐기했다. 실제 프로젝트의 Redirect 데이터와 에셋은 이 도구 제거 작업의 대상이 아니다. 새 코드 검사의 사용법·정책·한계는 [CodeReview.md](CodeReview.md)에 있다.

### 저작권 정리

```powershell
python Plugins/JWCommonUtility/Tools/update_copyright.py --root . --old OldOwner --new NewOwner
python Plugins/JWCommonUtility/Tools/update_copyright.py --root . --old OldOwner --new NewOwner --normalize --add-missing
```

미리보기를 확인한 명령에 --apply를 추가하면 쓴다. rename은 대상 텍스트 전체의 저작권자 문자열을 바꾸고, normalize는 대상 코드의 기존 저작권 헤더를 공용 proprietary 형식으로 통일한다. **normalize는 --old와 다른 소유자 문구도 바꿀 수 있으므로 소유한 코드 범위로 --root를 제한한다.** Epic 헤더는 --replace-epic 없이 정규화하지 않는다. --add-missing-under는 헤더 삽입 범위만 제한하며 rename/기존 헤더 정규화의 범위를 제한하지 않는다.

--year가 없으면 기존 연도를 유지하고, 연도를 찾지 못하거나 새 헤더를 넣으면 현재 연도를 쓴다. BOM과 줄바꿈을 보존한다. --root가 없거나 잘못되면 종료 코드 2로 실패한다.

### 주석 누락 간이 검사

```powershell
python Plugins/JWCommonUtility/Tools/detect_missing_comments.py Source
python Plugins/JWCommonUtility/Tools/check_comments.py --missing-only Source
```

두 명령은 같은 reflected 선언 주석 판정을 사용하며 누락 모드는 .h만 검사한다. UCLASS/USTRUCT/UENUM/UINTERFACE/UFUNCTION/UPROPERTY 위의 // 또는 블록 주석을 인정하고, 빈 줄과 기존 정밀 검사의 일부 전처리문을 건너뛴다. 누락 모드는 저작권·pragma·주석 길이 등의 스타일 검사를 생략하며 프로젝트 제외 경로는 적용한다. 정상 0, 누락 1, 입력·읽기 실패 2다.

기존 간이 명령과 -v/--verbose는 유지하되 출력은 정밀 검사기의 파일:줄·규칙 코드 형식으로 통일했다. 이전 간이 검사와 달리 UENUM·// 주석·JSON 제외 경로를 적용하고 위쪽 6줄 제한을 제거했다. 인자를 생략한 간이 명령의 기본 경로는 Source이며, 공용 명령은 Source/Plugins다. 스킬의 check_comments.py도 공용 실행기로 전달한다.

## 스킬 원본과 호스트 정책

- [unreal-code-refine](../Agent/Skills/unreal-code-refine/SKILL.md): 역할별 cpp 분할, 타입 추출, 공유 상태·CVar·로그·include 정리.
- [unreal-comment-maker](../Agent/Skills/unreal-comment-maker/SKILL.md): 주석·헤더 스타일 점검과 설명 작성.
- [unreal-doc-writer](../Agent/Skills/unreal-doc-writer/SKILL.md): 개별 Unreal 시스템의 문서 생성·갱신·구현과의 불일치 감사. 런타임·에디터·데이터·사용법·검증 절차를 설명.
- [unreal-overview-writer](../Agent/Skills/unreal-overview-writer/SKILL.md): 프로젝트 전체 구조·시스템 관계·네트워크 경계와 온보딩 개요. 상세 문서는 unreal-doc-writer로 연결.
- [unreal-tool-runbook](../Agent/Skills/unreal-tool-runbook/SKILL.md): 어떤 도구를 어떤 순서로 쓸지 고르는 진입점. 라우팅 표, 공통 순서, 쓰기 경계, 자주 틀리는 지점만 두고 상세 절차는 각 스킬로 보낸다.

```powershell
python Plugins/JWCommonUtility/Agent/Skills/unreal-code-refine/scan_structure.py Source
python Plugins/JWCommonUtility/Agent/Skills/unreal-comment-maker/check_comments.py --summary --limit 20 Source
```

구조 검사기의 구현 정본은 Tools/scan_structure.py이며 스킬 폴더에는 호환 진입점만 둔다. include 후보는 check_code.py와 cpp_review.py를 공유한다. 타입 접두사·헤더 이름 기반 추정에서 인라인 멤버 사용 등의 알려진 오탐을 걸러 주며, 실제 전방 선언 가능 여부를 확정하지 않는다. 인벤토리는 함수 이름·줄 수 비교 보조 수단이며 본문 동등성을 증명하지 않는다.

Python 주석·구조·코드 탐색·저작권 도구는 jwcu_context.py의 공통 excluded_paths 판정을 사용한다. --root가 프로젝트 하위 경로여도 기본 정책은 호스트 Config에서 읽고, 제외 패턴은 호스트 루트 기준으로 적용한다. 저작권자와 정규화 방식은 계속 명시한 CLI 옵션이 결정하며 license_header는 주석 감사 전용이다.

주석 검사기는 호스트 Config/JWCommonUtilityTools.json을 선택적으로 읽는다. 설정이 없으면 LICENSE만 생략하고 이를 출력한다. 네 스킬 모두 이 JSON의 agent.instructions와 작업에 관련된 agent.guidance_files를 읽는다. 기존 AGENTS.md/CLAUDE.md가 있으면 함께 따르지만 설치기가 이 파일을 생성하거나 덮어쓰지는 않는다. 스키마와 예시는 [AgentSupport.md](AgentSupport.md)에 있다.

문서 스킬은 언리얼 전용 실행 도구가 없어도 프로젝트 유지보수에 필요한 공용 작업 절차이므로 기본 배포에 포함한다. 프로젝트별 문서 정본·갱신 규약은 JSON에서 찾으며, 실제 프로젝트 문서와 게임 고유 예시는 해당 호스트가 소유한다. 사용자 홈의 system-doc-writer/system-overview-writer를 외부 의존성으로 호출하지 않는다.

## 설치와 도구 출력 안내

프로젝트 루트에서 아래 명령을 실행한다. 기본 설치는 .agents/skills와 .claude/skills의 개별 스킬, Tools/JWCommonUtility, Docs/JWCommonUtility를 플러그인 원본에 연결하고 Config/JWCommonUtilityTools.json에 설정과 소유 기록을 둔다. --agents none은 도구·문서만 연결한다.

```text
python Plugins/JWCommonUtility/Tools/install_agent_support.py install --dry-run
python Plugins/JWCommonUtility/Tools/install_agent_support.py install
python Plugins/JWCommonUtility/Tools/install_agent_support.py status
python Plugins/JWCommonUtility/Tools/install_agent_support.py uninstall
```

기존 파일 충돌 시 중단하고, 제거는 소유 기록과 현재 링크를 대조한 뒤 링크만 삭제한다. 소유 기록은 추적하지 않는 Config/JWCommonUtilityTools.local.json 에 두므로 설치·제거가 프로젝트 정책 JSON을 쓰지 않는다. 정책 JSON은 기본 보존한다. 재설치·원본 이동·제거 옵션과 운영체제별 링크 방식은 [AgentSupport.md](AgentSupport.md)를 읽는다.

도구와 검사기는 stderr에 [JWCU context] JSON과 [JWCU guidance] 안내를 출력한다. 여기에는 프로젝트 설정 경로, 공용 규약 경로, 프로젝트 지침이 들어간다. 검사 결과와 인벤토리 stdout 형식은 유지한다. --no-context로 안내만 숨길 수 있으며 설정 검증은 계속 수행한다. 스킬 없이 도구를 호출한 에이전트도 이 경로로 [공용 규약](CommonGuidance.md)과 프로젝트 정책을 발견한다.

## 검증

도구 변경은 다른 이름의 임시 호스트에서도 경로·접두사·설정 없는 실행을 확인한다. 파일을 쓰는 도구는 임시 UTF-8 Config/소스에 대해 미리보기 무변경, 적용, 재실행, BOM·줄바꿈 보존을 확인한다. 실제 게임 Config·에셋을 검증용으로 수정하지 않는다.

Python 스크립트만 수정했다면 Unreal 전체 빌드는 요구하지 않는다. Unreal 모듈이나 플러그인 모듈 구성을 수정했을 때는 호스트의 해당 타깃 빌드 검증을 추가한다.

자동 CLI 통합 검증은 `python Plugins/JWCommonUtility/Tools/Tests/test_tooling.py`로 실행한다. 표준 라이브러리로 다른 이름의 임시 호스트를 만들고 플러그인 복사본에서 검사·미리보기·적용을 검증한 뒤 임시 파일을 정리한다.

설치·제거의 충돌 방지, 롤백, 링크 백엔드와 도구 안내는 `python Plugins/JWCommonUtility/Tools/Tests/test_installer.py`로 검증한다.
