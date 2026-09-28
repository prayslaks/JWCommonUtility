<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# Blueprint 포스트잇

> 부분 갱신 일자: 2026-09-29 — 사용자 제공 SVG 네 개를 플러그인에 포함하고 에디터 스타일로 등록.

> 부분 갱신 일자: 2026-09-29 — 잠금·접기 버튼을 정사각형 아이콘으로 압축하고 본문 입력창의 메모 배경색을 보존.

> 부분 갱신 일자: 2026-09-29 — 독립 포스트잇, 본문 편집, 접기·잠금·프리셋, 기본 코멘트 변환 추가.

BP 그래프에 제목과 여러 줄 본문을 가진 직사각형 메모를 배치한다. 메모는 BP 에셋에 함께 저장되며 그래프의 이동·확대 배율을 따른다. 메모를 움직여도 주변 노드를 데려가지 않는다. 실행 핀이나 런타임 로직은 없다.

## 사용 방법

1. 플러그인을 빌드한 에디터에서 BP 그래프의 빈 공간을 우클릭한다.
2. `Add Sticky Note` 또는 `포스트잇`, `메모`를 검색한다. 경로는 `JWCommonUtility → Notes → Add Sticky Note`다. 선택한 노드를 감싸지 않고 커서 위치에 생성한다.
3. 제목은 선택 후 F2로 바꾸고, 본문은 더블클릭하여 입력한다. Enter로 줄바꿈하며, Ctrl+Enter·Esc 또는 다른 곳으로 포커스를 옮기면 편집을 마친다. Esc도 현재 입력을 보존한다.
4. 제목 영역을 끌어 이동한다. 테두리·모서리를 끌어 크기를 변경한다. 크기는 220×120부터 1600×1600까지이며 긴 본문은 스크롤한다.
5. 메모를 선택하고 Details의 `Sticky Note`에서 배경색·글자색·글자 크기를 변경한다. 본문도 Details에서 편집할 수 있다.
6. BP를 저장한다. 복사·붙여넣기·복제·삭제와 에디터 Undo/Redo를 사용한다.

| 조작 | 동작 |
| --- | --- |
| 제목의 안쪽 / 바깥쪽 화살표 아이콘 | 펼친 상태에서는 접기, 접힌 상태에서는 펼치기를 표시한다. 본문·펼친 크기를 보존한다. |
| 제목의 열린 / 닫힌 자물쇠 아이콘 | 현재 잠금 상태를 표시하며 클릭하면 전환한다. 위치·크기만 잠그고 제목·본문·색상은 계속 편집할 수 있다. |
| 우클릭 `Style: Note / TODO / Warning / Bug` | 노랑·파랑·주황·분홍 프리셋. 기본 용도 제목만 바꾸고 사용자가 작성한 제목과 본문은 보존한다. |
| 우클릭 `Convert to Standard Comment` | 제목·본문·위치·펼친 크기·배경색·글자 크기를 엔진 기본 코멘트로 옮긴다. 접기·잠금·별도 글자색은 해제된다. Undo로 되돌릴 수 있다. |

읽기 전용 그래프에서는 수정할 수 없다. 최초 범위는 K2 스키마 기반 BP 그래프이며 Material·Niagara 등 다른 그래프는 생성·붙여넣기를 허용하지 않는다. 체크리스트, 프로젝트 전체 메모 검색, 노드 링크, Markdown과 이미지 첨부는 포함하지 않는다.

## 저장과 모듈 경계

모든 구현은 `JWCommonUtilityEditor`에 있다. Runtime 모듈에 GraphEditor나 UnrealEd 의존성을 추가하지 않는다. 데이터 타입은 `UJWCU_StickyNote : UEdGraphNode_Comment`이며, 엔진의 비실행 코멘트 경로를 따른다.

| 저장 값 | 역할 |
| --- | --- |
| `NodeComment` | 제목. 기본 그래프 이름 변경 기능과 연결한다. |
| `Body` | 여러 줄 본문. 편집 중에도 실제 노드에 갱신하여 포커스가 남은 채 저장해도 보존한다. |
| `NodePosX/Y`, `NodeWidth/Height` | 그래프 좌표와 펼친 크기. 접힘은 저장 크기를 덮어쓰지 않는다. |
| `BackgroundColor`, `TextColor`, `TextSize` | 표시 스타일. |
| `bCollapsed`, `bLocked` | 접힘, 위치·크기 잠금. |
| `NodeGuid` | 그래프의 노드 식별자. |

기본 코멘트의 그룹 이동과 확대 시 말풍선 설정을 가져오지 않는다. 시각 위젯이 `SGraphNodeResizable`에서 직접 파생되어 코멘트의 영역 수집·그룹 이동 UI를 사용하지 않는다. Details 변경과 Undo는 그래프에 갱신을 알리고, 본문 편집은 포커스 단위 트랜잭션으로 묶는다.

플러그인을 끄거나 제거하기 전에 **모든 관련 BP의 포스트잇을 기본 코멘트로 변환하고 저장**해야 한다. 포스트잇이 남은 에셋은 해당 플러그인 클래스에 의존한다. 변환은 메모별로 제공하며 프로젝트 전체 자동 변환은 없다.

## 구현 위치와 확장

경로 기준은 `Source/JWCommonUtilityEditor/Private/StickyNotes/`다.

| 파일 | 책임 |
| --- | --- |
| `JWCU_StickyNote.h/.cpp` | 저장 데이터, K2 그래프 허용, 프리셋·접기·잠금·변환 트랜잭션과 우클릭 메뉴 |
| `SJWCU_StickyNote.h/.cpp` | 크기 조절, 본문 편집, 줄바꿈·스크롤, 위치 잠금과 Slate 표시 |
| `JWCU_StickyNoteStyle.h/.cpp` | `Resources/StickyNotes`의 사용자 제공 SVG를 16×16 벡터 브러시로 등록·해제. 22×22 버튼과 메모 글자색 tint 사용 |
| `JWCU_StickyNoteActions.h/.cpp` | BP 검색용 K2 액션 등록·템플릿 |
| `JWCU_StickyNoteSpawner.h/.cpp` | 검색 템플릿과 실제 생성 타입 분리 |
| `JWCU_StickyNoteTests.cpp` | 그래프·직렬화·위젯 자동 테스트 |

UE 5.7 액션 DB는 `UK2Node::GetMenuActions`를 수집하고, 등록 클래스와 스포너의 `NodeClass`가 같아야 한다. 따라서 검색 템플릿은 `UJWCU_StickyNoteActions`를 사용한다. 실제 `Invoke`는 `UJWCU_StickyNote`를 배치하며 검색용 K2 타입을 BP에 저장하지 않는다. 다른 타입을 스포너에 직접 등록하면 엔진 ensure가 발생한다. 이 경로를 바꾸면 컨텍스트 검색 필터와 실제 생성 타입을 함께 검증한다.

`CreateVisualWidget`가 전용 Slate 위젯을 반환하므로 글로벌 노드 팩토리나 엔진 소스 패치가 필요하지 않다. 새로운 프리셋은 노드의 enum·ApplyPreset·메뉴 라벨을 함께 수정한다. 저장 필드를 개명할 때는 기존 BP와 붙여넣기 텍스트의 호환성을 고려한다.

## 검증

Editor 타깃 빌드 후 Session Frontend에서 `JWCommonUtility.Editor.StickyNotes`를 실행한다.

2026-09-29 검증: 호스트 UE 5.7.4의 `ProjectZKEditor Win64 Development` 빌드 성공. `-NullRHI` 명령행에서 아래 3종 모두 통과했다. 저장 검증은 아래 설명한 프로세스 한정 `bValidateOnSave=False` 옵션으로 호스트 Localization validator와 분리했다.

- `Graph`: 검색 등록·컨텍스트 필터·커서 위치 생성, 비실행 핀, 다른 스키마 차단, 잠금 Undo/Redo, BP 간에 쓰이는 복사 포맷, 기본 코멘트 변환 Undo/Redo, BP 컴파일.
- `SaveReload`: Saved 아래 임시 패키지에 저장하고 새로운 패키지 인스턴스로 읽어 한글·여러 줄·스타일·좌표·크기·접힘·잠금 보존 및 재컴파일 확인.
- `Widget`: 실제 Slate 위젯의 접힘/펼침 크기, 이동 독립성·잠금·읽기 전용, 더블클릭 편집과 즉시 본문 저장.

테스트 산출물은 `Saved/Automation/JWCUStickyNotes` 아래에 남는다. 테스트에서는 사용자 BP 에셋을 수정하지 않는다. 호스트의 Localization validator가 임시 테스트 에셋 저장에서 ensure를 내는 경우, 명령행 테스트 프로세스에만 `-ini:Editor:[/Script/DataValidation.DataValidationSettings]:bValidateOnSave=False`를 지정해 분리할 수 있다. 프로젝트 설정 파일은 변경하지 않는다.

배포 전 수동 확인: 실제 Windows 한글 IME 조합, 다양한 DPI와 그래프 배율에서 테두리 드래그·스크롤·선택, 입력 중 Delete/Ctrl+Z, PIE의 읽기 전용 전환. 전체 게임 패키징과 화면 렌더링 검증은 자동 테스트와 별도로 수행한다.
